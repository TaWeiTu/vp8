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

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;
using ChromaBlock = std::vector<std::vector<MacroBlock<CHROMA>>>;

void VPredChroma(size_t, size_t, ChromaBlock &);
void HPredChroma(size_t, size_t, ChromaBlock &);
void DCPredChroma(size_t, size_t, ChromaBlock &);
void TMPredChroma(size_t, size_t, ChromaBlock &);

void VPredLuma(size_t, size_t, LumaBlock &);
void HPredLuma(size_t, size_t, LumaBlock &);
void DCPredLuma(size_t, size_t, LumaBlock &);
void TMPredLuma(size_t, size_t, LumaBlock &);

void BPredLuma(size_t, size_t,
               const std::array<std::array<SubBlockMode, 4>, 4> &,
               LumaBlock &);
void BPredSubBlock(const std::array<int16_t, 8> &,
                   const std::array<int16_t, 4> &, int16_t, SubBlockMode,
                   SubBlock &);
}  // namespace

void IntraPredict(const FrameHeader &, Frame &);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
