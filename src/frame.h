#ifndef FRAME_H_
#define FRAME_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace vp8 {

enum Component { LUMA = 4, CHROMA = 2 };

struct MotionVector {
  MotionVector() : dr(0), dc(0) {}
  explicit MotionVector(int16_t dr, int16_t dc) : dr(dr), dc(dc) {}

  int16_t dr;
  int16_t dc;
};

class SubBlock {
 public:
  SubBlock() = default;
  std::array<int16_t, 4>& operator[](size_t);
  std::array<int16_t, 4> operator[](size_t) const;
  void FillWith(int16_t);
  void FillRow(const std::array<int16_t, 4>&);
  void FillCol(const std::array<int16_t, 4>&);

  std::array<int16_t, 4> GetRow(size_t) const;
  std::array<int16_t, 4> GetCol(size_t) const;

 private:
  std::array<std::array<int16_t, 4>, 4> pixels_;
};

template <size_t C>
class MacroBlock {
 public:
  MacroBlock() = default;
  std::array<SubBlock, C>& operator[](size_t);
  std::array<SubBlock, C> operator[](size_t) const;

  void FillWith(int16_t);
  void FillRow(const std::array<int16_t, C * 4>&);
  void FillCol(const std::array<int16_t, C * 4>&);

  std::array<int16_t, C * 4> GetRow(size_t) const;
  std::array<int16_t, C * 4> GetCol(size_t) const;

  int16_t GetPixel(size_t, size_t) const;
  void SetPixel(size_t, size_t, int16_t);

  MotionVector GetMotionVector() const;
  void SetMotionVector(size_t, size_t);
  void SetMotionVector(const MotionVector&);

 private:
  MotionVector mv_;
  std::array<std::array<SubBlock, C>, C> subs_;
};

struct Frame {
  Frame() = default;
  size_t hsize, vsize, hblock, vblock;
  std::vector<std::vector<MacroBlock<LUMA>>> YBlocks;
  std::vector<std::vector<MacroBlock<CHROMA>>> UBlocks;
  std::vector<std::vector<MacroBlock<CHROMA>>> VBlocks;
};

void SubBlock::FillWith(int16_t p) {
  std::fill(pixels_.begin(), pixels_.end(), std::array<int16_t, 4>{p, p, p, p});
}

std::array<int16_t, 4>& SubBlock::operator[](size_t i) { return pixels_[i]; }
std::array<int16_t, 4> SubBlock::operator[](size_t i) const {
  return pixels_[i];
}

void SubBlock::FillRow(const std::array<int16_t, 4>& row) {
  for (size_t i = 0; i < 4; ++i)
    std::copy(row.begin(), row.end(), pixels_[i].begin());
}

void SubBlock::FillCol(const std::array<int16_t, 4>& col) {
  for (size_t i = 0; i < 4; ++i)
    std::fill(pixels_[i].begin(), pixels_[i].end(), col[i]);
}

std::array<int16_t, 4> SubBlock::GetRow(size_t i) const { return pixels_[i]; }

std::array<int16_t, 4> SubBlock::GetCol(size_t i) const {
  return std::array<int16_t, 4>{pixels_[0][i], pixels_[1][i], pixels_[2][i],
                                pixels_[3][i]};
}

template <size_t C>
void MacroBlock<C>::FillWith(int16_t p) {
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j) subs_[i][j].FillWith(p);
  }
}

template <size_t C>
std::array<SubBlock, C>& MacroBlock<C>::operator[](size_t i) {
  return subs_[i];
}

template <size_t C>
std::array<SubBlock, C> MacroBlock<C>::operator[](size_t i) const {
  return subs_[i];
}

template <size_t C>
void MacroBlock<C>::FillRow(const std::array<int16_t, C * 4>& row) {
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j)
      subs_[i][j].FillRow(
          std::array<int16_t, 4>{row[j], row[j + 1], row[j + 2], row[j + 3]});
  }
}

template <size_t C>
void MacroBlock<C>::FillCol(const std::array<int16_t, C * 4>& col) {
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j)
      subs_[i][j].FillRow(
          std::array<int16_t, 4>{col[j], col[j + 1], col[j + 2], col[j + 3]});
  }
}

template <size_t C>
std::array<int16_t, C * 4> MacroBlock<C>::GetRow(size_t i) const {
  std::array<int16_t, C * 4> res;
  for (size_t c = 0; c < C; ++c) {
    std::array<int16_t, 4> row = subs_[i >> 2][c].GetRow(i & 3);
    std::copy(row.begin(), row.end(), res.begin() + (c << 2));
  }
  return res;
}

template <size_t C>
std::array<int16_t, C * 4> MacroBlock<C>::GetCol(size_t i) const {
  std::array<int16_t, C * 4> res;
  for (size_t c = 0; c < C; ++c) {
    std::array<int16_t, 4> col = subs_[c][i >> 2].GetCol(i & 3);
    std::copy(col.begin(), col.end(), res.begin() + (c << 2));
  }
  return res;
}

template <size_t C>
int16_t MacroBlock<C>::GetPixel(size_t r, size_t c) const {
  return subs_[r >> 2][c >> 2][r & 3][c & 3];
}

template <size_t C>
void MacroBlock<C>::SetPixel(size_t r, size_t c, int16_t v) {
  subs_[r >> 2][c >> 2][r & 3][c & 3] = v;
}

template <size_t C>
MotionVector MacroBlock<C>::GetMotionVector() const {
  return mv_;
}

template <size_t C>
void MacroBlock<C>::SetMotionVector(size_t dr, size_t dc) {
  mv_ = MotionVector(dr, dc);
}

template <size_t C>
void MacroBlock<C>::SetMotionVector(const MotionVector& v) {
  mv_ = v;
}

}  // namespace vp8

#endif  // FRAME_H_
