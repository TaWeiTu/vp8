#ifndef ENCODE_FRAME_H_
#define ENCODE_FRAME_H_

#include <climits>
#include <utility>

#include "inter_predict.h"
#include "intra_predict.h"

namespace vp8 {
namespace internal {

// Compute the sum of squared-error between every pixels in the macroblocks.
template <size_t C>
uint32_t GetPredictionError(const MacroBlock<C> &target,
                            const MacroBlock<C> &predict);

uint32_t GetPredicionError(const SubBlock &target, const SubBlock &predict);

MacroBlockMode PickIntraModeChroma(size_t r, size_t c,
                                   const MacroBlock<2> &u_target,
                                   const MacroBlock<2> &v_target,
                                   Plane<2> &u_predict, Plane<2> &v_predict);

std::pair<SubBlockMode, uint32_t> PickIntraSubBlockMode(
    const std::array<int16_t, 8> &above, const std::array<int16_t, 4> &left,
    int16_t p, const SubBlock &target, SubBlock &predict);

MacroBlockMode PickIntraModeLuma(size_t r, size_t c);

}  // namespace internal

}  // namespace vp8

#endif  // ENCODE_FRAME_H_
