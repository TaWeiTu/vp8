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
MacroBlockMV SearchMVs(size_t r, size_t c, const FrameHeader &header,
                       const Plane<4> &mb, MacroBlockHeader &mh,
                       MotionVector &best, MotionVector &nearest,
                       MotionVector &near);

// Make sure that the motion vector indeed point to a valid position.
void ClampMV(MotionVector &mb, int16_t left, int16_t right, int16_t up,
             int16_t down);

// Invert the motion vector the sign bias is different in the reference frames
// of two macroblocks.
MotionVector Invert(const MotionVector &, bool, bool);

// Decide the probability table of the current subblock based on the motion
// vectors of the left and above subblocks.
uint8_t SubBlockProb(const MotionVector &left, const MotionVector &above);

// The motion vectors of chroma subblocks are the average value of the motion
// vectors occupying the same position in the luma subblocks.
void ConfigureChromaMVs(const MacroBlock<4> &luma, bool trim,
                        MacroBlock<2> &chroma);

// In case of mode MV_SPLIT, set the motion vectors of each subblock
// independently.
void ConfigureSubBlockMVs(MVPartition p, size_t r, size_t c,
                          MacroBlockHeader &mh, Plane<4> &mb);

// For each (luma or chroma) macroblocks, configure their motion vectors (if
// needed).
void ConfigureMVs(const FrameHeader &header, size_t r, size_t c, bool trim,
                  MacroBlockHeader &mh, Frame &frame);

// Horizontal pixel interpolation, this should return a 9x4 temporary matrix for
// the vertical pixel interpolation later.
template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorSixtap(
    const Plane<C> &refer, size_t r, size_t c,
    const std::array<int16_t, 6> &filter);

// Vertical pixel interpolation.
void VerSixtap(const std::array<std::array<int16_t, 4>, 9> &refer, size_t r,
               size_t c, const std::array<int16_t, 6> &filter, SubBlock &sub);

// Sixtap pixel interpolation. First do the horizontal interpolation, then
// vertical.
template <size_t C>
void Sixtap(const Plane<C> &refer, size_t r, size_t c, uint8_t mr, uint8_t mc,
            const std::array<std::array<int16_t, 6>, 8> &filter, SubBlock &sub);

// For each of the macroblock in the current plane, predict the value of it.
template <size_t C>
void InterpBlock(const Plane<C> &refer,
                 const std::array<std::array<int16_t, 6>, 8> &filter, size_t r,
                 size_t c, MacroBlock<C> &mb);

}  // namespace

void InterPredict(const FrameHeader &header, const FrameTag &tag, size_t r,
                  size_t c, Frame &frame);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
