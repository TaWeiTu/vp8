#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include "frame.h"
#include "utils.h"
// #include "frame_header.h"

#include <algorithm>
#include <array>
#include <vector>

namespace {

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;
using ChromaBlock = std::vector<std::vector<MacroBlock<CHROMA>>>;
using PredictionMode = int32_t;

void VPredChroma(size_t, size_t, ChromaBlock &);
void HPredChroma(size_t, size_t, ChromaBlock &);
void DCPredChroma(size_t, size_t, ChromaBlock &);
void TMPredChroma(size_t, size_t, ChromaBlock &);

void VPredLuma(size_t, size_t, LumaBlock &);
void HPredLuma(size_t, size_t, LumaBlock &);
void DCPredLuma(size_t, size_t, LumaBlock &);
void TMPredLuma(size_t, size_t, LumaBlock &);

void BPredLuma(size_t, size_t, const std::array<std::array<PredictionMode, 4>, 4> &,
               LumaBlock &);
void BPredSubBlock(const std::array<int16_t, 8> &,
                   const std::array<int16_t, 4> &, int16_t, PredictionMode,
                   SubBlock &);
}  // namespace

// void IntraPredict(const FrameHeader &, Frame &);

#endif  // INTRA_PREDICT_H_
