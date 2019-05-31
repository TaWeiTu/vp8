#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "frame.h"
// #include "frame_header.h"
#include "predict_mode.h"
#include "utils.h"

namespace vp8 {
namespace {

void VPredChroma(size_t, size_t, Plane<2> &);
void HPredChroma(size_t, size_t, Plane<2> &);
void DCPredChroma(size_t, size_t, Plane<2> &);
void TMPredChroma(size_t, size_t, Plane<2> &);

void VPredLuma(size_t, size_t, Plane<4> &);
void HPredLuma(size_t, size_t, Plane<4> &);
void DCPredLuma(size_t, size_t, Plane<4> &);
void TMPredLuma(size_t, size_t, Plane<4> &);

void BPredLuma(size_t, size_t,
               const std::array<std::array<SubBlockMode, 4>, 4> &, Plane<4> &);
void BPredSubBlock(const std::array<int16_t, 8> &,
                   const std::array<int16_t, 4> &, int16_t, SubBlockMode,
                   SubBlock &);
}  // namespace

void IntraPredict(const FrameHeader &, Frame &);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
