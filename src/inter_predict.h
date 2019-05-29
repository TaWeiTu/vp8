#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "frame.h"

namespace vp8 {
namespace {

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;

// Search for motion vectors in the left, above and upper-left macroblocks and
// return the best, nearest and near motion vectors.
std::array<MotionVector, 3> SearchMVs(size_t, size_t, const LumaBlock &);

// Invert the motion vector the sign bias is different in the reference frames
// of two macroblocks
MotionVector Invert(const MotionVector &, bool, bool);

// Decide the probability table of the current subblock based on the motion
// vectors of the left and above subblocks
uint8_t SubBlockProb(const MotionVecor &, const MotionVector &);

}  // namespace

void InterPredict(const FrameHeader &, Frame &);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
