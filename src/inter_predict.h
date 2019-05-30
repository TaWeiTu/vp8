#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "frame.h"
#include "predict_mode.h"

namespace vp8 {
namespace {

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;
using ChromaBlock = std::vector<std::vector<MacroBlock<CHROMA>>>;
static const MotionVector kZero = MotionVector(0, 0);

// Search for motion vectors in the left, above and upper-left macroblocks and
// return the best, nearest and near motion vectors.
MacroBlockMV SearchMVs(size_t, size_t, const LumaBlock &, MotionVector &,
                       MotionVector &, MotionVector &);

// Make sure that the motion vector indeed point to a valid position.
void ClampMV(MotionVector &);

// Invert the motion vector the sign bias is different in the reference frames
// of two macroblocks.
MotionVector Invert(const MotionVector &, bool, bool);

// Decide the probability table of the current subblock based on the motion
// vectors of the left and above subblocks.
uint8_t SubBlockProb(const MotionVector &, const MotionVector &);

// The motion vectors of chroma subblocks are the average value of the motion
// vectors occupying the same position in the luma subblocks.
MotionVector Average(size_t, size_t, const LumaBlock &);

// In case of mode MV_SPLIT, set the motion vectors of each subblock
// independently.
void ConfigureSubBlockMVs(MVPartition, LumaBlock &);

}  // namespace

void InterPredict(const FrameHeader &, Frame &);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
