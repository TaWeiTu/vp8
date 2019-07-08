#ifndef FRAME_H_
#define FRAME_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "utils.h"

namespace vp8 {

struct MotionVector {
  MotionVector() : dr(0), dc(0) {}
  explicit MotionVector(int16_t dr_, int16_t dc_) : dr(dr_), dc(dc_) {}

  bool operator==(const MotionVector& rhs) const {
    return dr == rhs.dr && dc == rhs.dc;
  }

  bool operator!=(const MotionVector& rhs) const {
    return dr != rhs.dr || dc != rhs.dc;
  }

  explicit operator bool() const { return dr != 0 || dc != 0; }

  MotionVector operator+(const MotionVector& rhs) const {
    return MotionVector(int16_t(dr + rhs.dr), int16_t(dc + rhs.dc));
  }

  int16_t dr;
  int16_t dc;
};

class SubBlock {
 public:
  SubBlock() = default;

  std::array<int16_t, 4>& at(size_t i) { return pixels_.at(i); }

  std::array<int16_t, 4> at(size_t i) const { return pixels_.at(i); }

  void FillWith(int16_t p) {
    for (size_t i = 0; i < 4; ++i)
      std::fill(pixels_.at(i).begin(), pixels_.at(i).end(), p);
  }

  void FillRow(const std::array<int16_t, 4>& row) {
    for (size_t i = 0; i < 4; ++i)
      std::copy(row.begin(), row.end(), pixels_.at(i).begin());
  }

  template <class Iterator>
  void FillRow(Iterator iter) {
    for (size_t i = 0; i < 4; ++i) {
      auto it1 = pixels_.at(i).begin();
      auto it2 = iter;
      for (size_t j = 0; j < 4; ++j) *it1++ = *it2++;
    }
  }

  void FillCol(const std::array<int16_t, 4>& col) {
    for (size_t i = 0; i < 4; ++i)
      std::fill(pixels_.at(i).begin(), pixels_.at(i).end(), col.at(i));
  }

  template <class Iterator>
  void FillCol(Iterator iter) {
    for (size_t i = 0; i < 4; ++i)
      std::fill(pixels_.at(i).begin(), pixels_.at(i).end(), *iter++);
  }

  std::array<int16_t, 4> GetRow(size_t i) const { return pixels_.at(i); }

  std::array<int16_t, 4> GetCol(size_t i) const {
    return std::array<int16_t, 4>{pixels_.at(0).at(i), pixels_.at(1).at(i),
                                  pixels_.at(2).at(i), pixels_.at(3).at(i)};
  }

  MotionVector GetMotionVector() const { return mv_; }

  void SetMotionVector(int16_t dr, int16_t dc) { mv_ = MotionVector(dr, dc); }

  void SetMotionVector(const MotionVector& v) { mv_ = v; }

 private:
  std::array<std::array<int16_t, 4>, 4> pixels_{};
  MotionVector mv_;
};

template <size_t C>
class MacroBlock {
 public:
  MacroBlock() : offset_(C == 4 ? 2 : 1), mask_((1 << offset_) - 1) {}

  std::array<SubBlock, C>& at(size_t i) { return subs_.at(i); }

  std::array<SubBlock, C> at(size_t i) const { return subs_.at(i); }

  void FillWith(int16_t p) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j) subs_.at(i).at(j).FillWith(p);
    }
  }

  void FillRow(const std::array<int16_t, C * 4>& row) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j)
        subs_.at(i).at(j).FillRow(row.begin() + (j << 2));
    }
  }

  void FillCol(const std::array<int16_t, C * 4>& col) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j)
        subs_.at(i).at(j).FillCol(col.begin() + (i << 2));
    }
  }

  std::array<int16_t, C * 4> GetRow(size_t i) const {
    std::array<int16_t, C * 4> res{};
    for (size_t c = 0; c < C; ++c) {
      std::array<int16_t, 4> row = subs_.at(i >> 2).at(c).GetRow(i & 3);
      std::copy(row.begin(), row.end(), res.begin() + (c << 2));
    }
    return res;
  }

  std::array<int16_t, C * 4> GetCol(size_t i) const {
    std::array<int16_t, C * 4> res{};
    for (size_t c = 0; c < C; ++c) {
      std::array<int16_t, 4> col = subs_.at(c).at(i >> 2).GetCol(i & 3);
      std::copy(col.begin(), col.end(), res.begin() + (c << 2));
    }
    return res;
  }

  int16_t GetPixel(size_t r, size_t c) const {
    return subs_.at(r >> 2).at(c >> 2).at(r & 3).at(c & 3);
  }

  void SetPixel(size_t r, size_t c, int16_t v) {
    subs_.at(r >> 2).at(c >> 2).at(r & 3).at(c & 3) = v;
  }

  void IncrementPixel(size_t r, size_t c, int16_t v) {
    subs_.at(r >> 2).at(c >> 2).at(r & 3).at(c & 3) += v;
  }

  MotionVector GetMotionVector() const { return mv_; }

  void SetMotionVector(int16_t dr, int16_t dc) { mv_ = MotionVector(dr, dc); }

  void SetMotionVector(const MotionVector& v) { mv_ = v; }

  MotionVector GetSubBlockMV(size_t r, size_t c) const {
    return subs_.at(r).at(c).GetMotionVector();
  }

  MotionVector GetSubBlockMV(size_t id) const {
    return subs_.at(id >> offset_).at(id & mask_).GetMotionVector();
  }

  // The motion vectors are the same for all subblocks.
  void SetSubBlockMVs(const MotionVector& v) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j) subs_.at(i).at(j).SetMotionVector(v);
    }
  }

 private:
  MotionVector mv_;
  size_t offset_, mask_;
  std::array<std::array<SubBlock, C>, C> subs_;
};

template <size_t C>
class Plane {
 public:
  Plane() : offset_(C == 4 ? 4 : 3), mask_((1 << offset_) - 1) {}

  explicit Plane(size_t h, size_t w)
      : offset_(C == 4 ? 4 : 3), mask_((1 << offset_) - 1) {
    blocks_.resize(h, std::vector<MacroBlock<C>>(w));
  }

  int16_t GetPixel(size_t r, size_t c) const {
    return blocks_.at(r >> offset_)
        .at(c >> offset_)
        .GetPixel(r & mask_, c & mask_);
  }

  std::vector<MacroBlock<C>>& at(size_t i) { return blocks_.at(i); }

  const std::vector<MacroBlock<C>>& at(size_t i) const { return blocks_.at(i); }

  size_t vblock() const { return blocks_.size(); }
  size_t hblock() const { return blocks_.at(0).size(); }

  size_t vsize() const { return blocks_.size() * C * 4; }
  size_t hsize() const { return blocks_.at(0).size() * C * 4; }

 private:
  size_t offset_, mask_;
  std::vector<std::vector<MacroBlock<C>>> blocks_;
};

struct Frame {
  Frame() : vsize(0), hsize(0), vblock(0), hblock(0) {}
  explicit Frame(size_t h, size_t w)
      : vsize(h), hsize(w), vblock((h + 15) >> 4), hblock((w + 15) >> 4) {
    Y = Plane<4>(vblock, hblock);
    U = Plane<2>(vblock, hblock);
    V = Plane<2>(vblock, hblock);
  }

  void resize(size_t h, size_t w) {
    vsize = h;
    hsize = w;
    vblock = (h + 15) >> 4;
    hblock = (w + 15) >> 4;
    Y = Plane<4>(vblock, hblock);
    U = Plane<2>(vblock, hblock);
    V = Plane<2>(vblock, hblock);
  }

  size_t vsize, hsize, vblock, hblock;
  Plane<4> Y;
  Plane<2> U, V;
};

}  // namespace vp8

#endif  // FRAME_H_
