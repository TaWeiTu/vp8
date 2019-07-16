#ifndef ENCODE_FRAME_H_
#define ENCODE_FRAME_H_

#include <climits>
#include <utility>

#include "inter_predict.h"
#include "intra_predict.h"

namespace vp8 {
namespace internal {

// The prediction error is calculated as the sum of squared error of each pair
// of pixels on the same position.
template <size_t C>
uint32_t GetPredictionError(const MacroBlock<C> &target,
                            const MacroBlock<C> &predict);

uint32_t GetPredicionError(const SubBlock &target, const SubBlock &predict);

MacroBlockMode PickIntraModeChroma(size_t r, size_t c,
                                   const MacroBlock<2> &u_target,
                                   const MacroBlock<2> &v_target,
                                   Plane<2> &u_predict, Plane<2> &v_predict);

// Select prediction mode for subblock and return a pair consisting of the
// selected mode and the corresponding cost.
std::pair<SubBlockMode, uint32_t> PickIntraSubBlockModeSB(
    const std::array<int16_t, 8> &above, const std::array<int16_t, 4> &left,
    int16_t p, const SubBlock &target, SubBlock &predict);

// For each of the 16 subblocks, find the best prediction mode and store them in
// sub_mode. Return the total cost.
uint32_t PickIntraSubBlockModeMB(size_t r, size_t c,
                                 const MacroBlock<4> &target, Plane<4> &predict,
                                 std::array<SubBlockMode, 16> &sub_mode);

// Select the best intra mode for luma macroblocks. If B_PRED is the best, fill
// sub_mode with the prediction modes of each subblock.
MacroBlockMode PickIntraModeLuma(size_t r, size_t c,
                                 const MacroBlock<4> &target, Plane<4> &predict,
                                 std::array<SubBlockMode, 16> &sub_mode);

}  // namespace internal

}  // namespace vp8

#endif  // ENCODE_FRAME_H_
