#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "residual.h"
#include "utils.h"

namespace vp8 {

struct IntraContext {
  bool is_intra_mb;
  bool is_b_pred;
  SubBlockMode mode;

  IntraContext() : is_intra_mb(false) {}

  explicit IntraContext(bool is_b_pred_, SubBlockMode mode_)
      : is_intra_mb(true), is_b_pred(is_b_pred_), mode(mode_) {}
};

namespace internal {

void VPredChroma(size_t r, size_t c, Plane<2> &mb);
void HPredChroma(size_t r, size_t c, Plane<2> &mb);
void DCPredChroma(size_t r, size_t c, Plane<2> &mb);
void TMPredChroma(size_t r, size_t c, Plane<2> &mb);

void VPredLuma(size_t r, size_t c, Plane<4> &mb);
void HPredLuma(size_t r, size_t c, Plane<4> &mb);
void DCPredLuma(size_t r, size_t c, Plane<4> &mb);
void TMPredLuma(size_t r, size_t c, Plane<4> &mb);

void BPredLuma(size_t r, size_t c, bool is_key_frame, const ResidualValue &rv,
               std::vector<std::vector<IntraContext>> &context,
               BitstreamParser &ps, Plane<4> &mb);

void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p,
                   SubBlockMode mode, SubBlock &sub);
}  // namespace internal

void IntraPredict(const FrameTag &tag, size_t r, size_t c,
                  const ResidualValue &rv, const IntraMBHeader &mh,
                  std::vector<std::vector<IntraContext>> &context,
                  BitstreamParser &ps, Frame &frame);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
