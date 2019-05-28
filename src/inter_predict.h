#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "frame.h"

namespace vp8 {
namespace {

using LumaBlock = std::vector<std::vector<MacroBlock<LUMA>>>;

std::array<MotionVector, 3> SearchMVs(size_t, size_t, const LumaBlock &);
MvRef ReadMode(const std::array<uint8_t, 4> &);

}  // namespace

void InterPredict(const FrameHeader &, Frame &);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
