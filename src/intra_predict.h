#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include "frame.h"

namespace {

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;
using ChromaBlock = std::vector<std::vector<MacroBlock<CHROMA>>>;

void VPred(size_t, size_t, Frame &);
void HPred(size_t, size_t, Frame &);
void DCPred(size_t, size_t, Frame &);
void TMPred(size_t, size_t, Frame &);

void VPredChroma(size_t, size_t, ChromaBlock &);
void HPredChroma(size_t, size_t, ChromaBlock &);
void DCPredChroma(size_t, size_t, ChromaBlock &);
void TMPredChroma(size_t, size_t, ChromaBlock &);

void VPredLuma(size_t, size_t, LumaBlock &);
void HPredLuma(size_t, size_t, LumaBlock &);
void DCPredLuma(size_t, size_t, LumaBlock &);
void TMPredLuma(size_t, size_t, LumaBlock &);

}  // namespace

void IntraPredict(const FrameHeader &, Frame &);

#endif  // INTRA_PREDICT_H_
