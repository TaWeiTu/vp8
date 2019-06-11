#ifndef FILTER_H_
#define FILTER_H_

#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"
#include "utils.h"

namespace vp8 {
namespace internal {

int16_t minus128(int16_t x);
int16_t plus128(int16_t x);

class LoopFilter {
 public:
  LoopFilter() = default;
  LoopFilter(int16_t p3, int16_t p2, int16_t p1, int16_t p0, int16_t q0,
             int16_t q1, int16_t q2, int16_t q3);

  bool IsFilterNormal(int16_t, const int16_t) const;
  bool IsFilterSimple(int16_t) const;
  bool IsHighVariance(int16_t) const;
  int16_t Adjust(bool);
  void SubBlockFilter(int16_t, int16_t, int16_t);
  void MacroBlockFilter(int16_t, int16_t, int16_t);

  void SimpleFilter(int16_t);

  void Horizontal(const SubBlock &, const SubBlock &, size_t);
  void FillHorizontal(SubBlock &, SubBlock &, size_t) const;
  void Vertical(const SubBlock &, const SubBlock &, size_t);
  void FillVertical(SubBlock &, SubBlock &, size_t) const;

 private:
  int16_t p3_, p2_, p1_, p0_;
  int16_t q0_, q1_, q2_, q3_;
};

template <size_t C>
void PlaneFilterNormal(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<InterContext>> &interc,
                       const std::vector<std::vector<IntraContext>> &intrac,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &nonzero,
                       Plane<C> &frame);

void PlaneFilterSimple(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &skip_lf,
                       Plane<4> &frame);

}  // namespace internal

void FrameFilter(const FrameHeader &header, bool is_key_frame,
                 const std::vector<std::vector<uint8_t>> &lf,
                 const std::vector<std::vector<uint8_t>> &skip_lf,
                 Frame &frame);

}  // namespace vp8

#endif  // FILTER_H_
