#ifndef FRAME_H_
#define FRAME_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "utils.h"

namespace vp8 {

enum Component { LUMA = 4, CHROMA = 2 };

struct MotionVector {
  MotionVector() : dr(0), dc(0) {}
  explicit MotionVector(int16_t dr_, int16_t dc_) : dr(dr_), dc(dc_) {}

  bool operator==(const MotionVector& rhs) const {
    return dr == rhs.dr && dc == rhs.dc;
  }
  bool operator!=(const MotionVector& rhs) const {
    return dr != rhs.dr || dc != rhs.dc;
  }

  MotionVector operator+(const MotionVector& rhs) const {
    return MotionVector(int16_t(dr + rhs.dr), int16_t(dc + rhs.dc));
  }

  int16_t dr;
  int16_t dc;
};

class SubBlock {
 public:
  SubBlock() = default;

  std::array<int16_t, 4>& operator[](size_t i) {
    ensure(i < 4, "[Error] SubBlock::operator[]: Index out of range.");
    return pixels_[i];
  }

  std::array<int16_t, 4> operator[](size_t i) const {
    ensure(i < 4, "[Error] SubBlock::operator[]: Index out of range.");
    return pixels_[i];
  }

  void FillWith(int16_t p) {
    std::fill(pixels_.begin(), pixels_.end(),
              std::array<int16_t, 4>{p, p, p, p});
  }

  void FillRow(const std::array<int16_t, 4>& row) {
    for (size_t i = 0; i < 4; ++i)
      std::copy(row.begin(), row.end(), pixels_[i].begin());
  }

  void FillCol(const std::array<int16_t, 4>& col) {
    for (size_t i = 0; i < 4; ++i)
      std::fill(pixels_[i].begin(), pixels_[i].end(), col[i]);
  }

  std::array<int16_t, 4> GetRow(size_t i) const {
    ensure(i < 4, "[Error] SubBlock::GetRow: Index out of range.");
    return pixels_[i];
  }

  std::array<int16_t, 4> GetCol(size_t i) const {
    ensure(i < 4, "[Error] SubBlock::GetCol: Index out of range.");
    return std::array<int16_t, 4>{pixels_[0][i], pixels_[1][i], pixels_[2][i],
                                  pixels_[3][i]};
  }

  MotionVector GetMotionVector() const { return mv_; }

  void SetMotionVector(int16_t dr, int16_t dc) { mv_ = MotionVector(dr, dc); }

  void SetMotionVector(const MotionVector& v) { mv_ = v; }

 private:
  std::array<std::array<int16_t, 4>, 4> pixels_;
  MotionVector mv_;
};

template <size_t C>
class MacroBlock {
 public:
  MacroBlock() : offset_(C == 4 ? 2 : 1), mask_((1 << offset_) - 1) {}

  std::array<SubBlock, C>& operator[](size_t i) {
    ensure(i < C, "[Error] MacroBlock<C>::operator[]: Index out of range.");
    return subs_[i];
  }

  std::array<SubBlock, C> operator[](size_t i) const {
    ensure(i < C, "[Error] MacroBlock<C>::operator[]: Index out of range.");
    return subs_[i];
  }

  void FillWith(int16_t p) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j) subs_[i][j].FillWith(p);
    }
  }

  void FillRow(const std::array<int16_t, C * 4>& row) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j)
        subs_[i][j].FillRow(
            std::array<int16_t, 4>{row[j], row[j + 1], row[j + 2], row[j + 3]});
    }
  }

  void FillCol(const std::array<int16_t, C * 4>& col) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j)
        subs_[i][j].FillRow(
            std::array<int16_t, 4>{col[j], col[j + 1], col[j + 2], col[j + 3]});
    }
  }

  std::array<int16_t, C * 4> GetRow(size_t i) const {
    std::array<int16_t, C * 4> res;
    for (size_t c = 0; c < C; ++c) {
      std::array<int16_t, 4> row = subs_[i >> 2][c].GetRow(i & 3);
      std::copy(row.begin(), row.end(), res.begin() + (c << 2));
    }
    return res;
  }

  std::array<int16_t, C * 4> GetCol(size_t i) const {
    std::array<int16_t, C * 4> res;
    for (size_t c = 0; c < C; ++c) {
      std::array<int16_t, 4> col = subs_[c][i >> 2].GetCol(i & 3);
      std::copy(col.begin(), col.end(), res.begin() + (c << 2));
    }
    return res;
  }

  int16_t GetPixel(size_t r, size_t c) const {
    ensure(r < (C << 2) && c < (C << 2),
           "[Error] MacroBlock<C>::GetPixel: Index out of range.");
    return subs_[r >> 2][c >> 2][r & 3][c & 3];
  }

  void SetPixel(size_t r, size_t c, int16_t v) {
    ensure(r < (C << 2) && c < (C << 2),
           "[Error] MacroBlock<C>::SetPixel: Index out of range.");
    subs_[r >> 2][c >> 2][r & 3][c & 3] = v;
  }

  MotionVector GetMotionVector() const { return mv_; }

  void SetMotionVector(int16_t dr, int16_t dc) { mv_ = MotionVector(dr, dc); }

  void SetMotionVector(const MotionVector& v) { mv_ = v; }

  MotionVector GetSubBlockMV(size_t r, size_t c) const {
    ensure(r < C && c < C,
           "[Error] MacroBlock<C>::GetSubBlockMV: Index out of range.");
    return subs_[r][c].GetMotionVector();
  }

  MotionVector GetSubBlockMV(size_t id) const {
    ensure(id < C * C,
           "[Error] MacroBlock<C>::GetSubBlockMV: Index out of range.");
    return subs_[id >> offset_][id & mask_].GetMotionVector();
  }

  // The motion vectors are the same for all subblocks.
  void SetSubBlockMVs(const MotionVector& v) {
    for (size_t i = 0; i < C; ++i) {
      for (size_t j = 0; j < C; ++j) subs_[i][j].SetMotionVector(v);
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
    ensure(r < blocks_.size() * 4 * C && c < blocks_[0].size() * 4 * C,
           "[Error] Plane<C>::GetPixel: Index out of range.");
    return blocks_[r >> offset_][c >> offset_].GetPixel(r & mask_, c & mask_);
  }
  std::vector<MacroBlock<C>>& operator[](size_t i) {
    ensure(i < blocks_.size(),
           "[Error] Plane<C>::operator[]: Index out of range.");
    return blocks_[i];
  }
  const std::vector<MacroBlock<C>>& operator[](size_t i) const {
    ensure(i < blocks_.size(),
           "[Error] Plane<C>::operator[]: Index out of range.");
    return blocks_[i];
  }

 private:
  size_t offset_, mask_;
  std::vector<std::vector<MacroBlock<C>>> blocks_;
};

struct Frame {
  Frame() = default;
  explicit Frame(size_t h, size_t w)
      : vsize(h), hsize(w), vblock((h + 15) >> 4), hblock((w + 15) >> 4) {
    Y = Plane<LUMA>(vblock, hblock);
    U = Plane<CHROMA>(vblock, hblock);
    V = Plane<CHROMA>(vblock, hblock);
  }
  size_t vsize, hsize, vblock, hblock;
  Plane<LUMA> Y;
  Plane<CHROMA> U, V;
};

}  // namespace vp8

#endif  // FRAME_H_
