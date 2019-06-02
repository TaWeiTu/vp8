#ifndef FILTER_H_
#define FILTER_H_

#include <vector>
#include "frame.h"
#include "utils.h"

namespace vp8 {
int16_t minus128(const int16_t);
int16_t plus128(const int16_t);
int16_t minus128(const int16_t x) { return int16_t(Clamp128(x - int16_t(128))); }
int16_t plus128(const int16_t x) { return int16_t(Clamp255(x + int16_t(128))); }
struct LoopFilter {
  int16_t p3, p2, p1, p0;
  int16_t q0, q1, q2, q3;
  LoopFilter(int16_t P3, int16_t P2, int16_t P1, int16_t P0, int16_t Q0,
             int16_t Q1, int16_t Q2, int16_t Q3) {
    p3 = P3;
    p2 = P2;
    p1 = P1;
    p0 = P0;
    q0 = Q0;
    q1 = Q1;
    q2 = Q2;
    q3 = Q3;
  }
  bool IsFilter(const int16_t, const int16_t);
  bool IsHighVariance(const int16_t);
  int16_t Adjust(const bool);
  void SubBlockFilter(const int16_t, const int16_t, const int16_t);
  void MacroBlockFilter(const int16_t, const int16_t, const int16_t);

  void Horizontal(SubBlock &, SubBlock &, size_t);
  void FillHorizontal(SubBlock &, SubBlock &, size_t);
  void Vertical(SubBlock &, SubBlock &, size_t);
  void FillVertical(SubBlock &, SubBlock &, size_t);
};
template <size_t C>
void FrameFilter(FrameHeader &, const uint8_t,
                 std::vector<std::vector<MacroBlock<C>>> &, const size_t, const size_t, const bool,
                 LoopFilter &);
}  // namespace vp8
#endif  // FILTER_H_
