#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {
namespace {

void VPredChroma(size_t r, size_t c, Plane<2> &mb);
void HPredChroma(size_t r, size_t c, Plane<2> &mb);
void DCPredChroma(size_t r, size_t c, Plane<2> &mb);
void TMPredChroma(size_t r, size_t c, Plane<2> &mb);

void VPredLuma(size_t r, size_t c, Plane<4> &mb);
void HPredLuma(size_t r, size_t c, Plane<4> &mb);
void DCPredLuma(size_t r, size_t c, Plane<4> &mb);
void TMPredLuma(size_t r, size_t c, Plane<4> &mb);

void BPredLuma(size_t r, size_t c,
               const std::array<SubBlockMode, 16> &pred, Plane<4> &mb);
void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p, SubBlockMode mode,
                   SubBlock &sub);
}  // namespace

void IntraPredict(const FrameHeader &header, size_t r, size_t c, const MacroBlockHeader &mh,
                  Frame &frame);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
