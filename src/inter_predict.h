#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "predict_mode.h"
#include "utils.h"

namespace vp8 {
namespace {

static const MotionVector kZero = MotionVector(0, 0);

// Search for motion vectors in the left, above and upper-left macroblocks and
// return the best, nearest and near motion vectors.
MacroBlockMV SearchMVs(size_t, size_t, const FrameHeader &, const Plane<4> &,
                       MotionVector &, MotionVector &, MotionVector &);

// Make sure that the motion vector indeed point to a valid position.
void ClampMV(MotionVector &, int16_t, int16_t, int16_t, int16_t);

// Invert the motion vector the sign bias is different in the reference frames
// of two macroblocks.
MotionVector Invert(const MotionVector &, bool, bool);

// Decide the probability table of the current subblock based on the motion
// vectors of the left and above subblocks.
uint8_t SubBlockProb(const MotionVector &, const MotionVector &);

// The motion vectors of chroma subblocks are the average value of the motion
// vectors occupying the same position in the luma subblocks.
void ConfigureChromaMVs(const MacroBlock<4> &, bool, MacroBlock<2> &);

// In case of mode MV_SPLIT, set the motion vectors of each subblock
// independently.
void ConfigureSubBlockMVs(MVPartition, size_t, size_t, Plane<4> &);

// For each (luma or chroma) macroblocks, configure their motion vectors (if
// needed).
void ConfigureMVs(const FrameHeader &, size_t, size_t, bool, Frame &);

// Horizontal pixel interpolation, this should return a 9x4 temporary matrix for
// the vertical pixel interpolation later.
template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorSixtap(const Plane<C> &, size_t,
                                                size_t,
                                                const std::array<int16_t, 6> &);

// Vertical pixel interpolation.
void VerSixtap(const std::array<std::array<int16_t, 4>, 9> &, size_t, size_t,
               const std::array<int16_t, 6> &, SubBlock &);

// Sixtap pixel interpolation. First do the horizontal interpolation, then
// vertical.
template <size_t C>
void Sixtap(const Plane<C> &, size_t, size_t, uint8_t, uint8_t,
            const std::array<std::array<int16_t, 6>, 8> &, SubBlock &);

// For each of the macroblock in the current plane, predict the value of it.
template <size_t C>
void InterpBlock(const Plane<C> &,
                 const std::array<std::array<int16_t, 6>, 8> &, size_t, size_t,
                 MacroBlock<C> &);

}  // namespace

void InterPredict(const FrameHeader &, const FrameTag &, size_t, size_t,
                  Frame &);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
