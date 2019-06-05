#ifndef FILTER_H_
#define FILTER_H_

#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {
// namespace {

int16_t minus128(int16_t);
int16_t plus128(int16_t);

class LoopFilter {
 public:
  LoopFilter() = default;
  LoopFilter(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t,
             int16_t);

  bool IsFilter(int16_t, const int16_t) const;
  bool IsHighVariance(int16_t) const;
  int16_t Adjust(bool);
  void SubBlockFilter(int16_t, int16_t, int16_t);
  void MacroBlockFilter(int16_t, int16_t, int16_t);

  void Horizontal(const SubBlock &, const SubBlock &, size_t);
  void FillHorizontal(SubBlock &, SubBlock &, size_t) const;
  void Vertical(const SubBlock &, const SubBlock &, size_t);
  void FillVertical(SubBlock &, SubBlock &, size_t) const;

 private:
  int16_t p3_, p2_, p1_, p0_;
  int16_t q0_, q1_, q2_, q3_;
};

// }  // namespace

template <size_t C>
void FrameFilter(const FrameHeader &, size_t, size_t, bool, Plane<C> &);

}  // namespace vp8

#endif  // FILTER_H_
