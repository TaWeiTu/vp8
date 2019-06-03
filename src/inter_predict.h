#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {
namespace {

static const MotionVector kZero = MotionVector(0, 0);

static const std::array<std::array<int16_t, 6>, 8> kBicubicFilter = {
    {{0, 0, 128, 0, 0, 0},
     {0, -6, 123, 12, -1, 0},
     {2, -11, 108, 36, -8, 1},
     {0, -9, 93, 50, -6, 0},
     {3, -16, 77, 77, -16, 3},
     {0, -6, 50, 93, -9, 0},
     {1, -8, 36, 108, -11, 2},
     {0, -1, 12, 123, -6, 0}}};

static const std::array<std::array<int16_t, 6>, 8> kBilinearFilter = {
    {{0, 0, 128, 0, 0, 0},
     {0, 0, 112, 16, 0, 0},
     {0, 0, 96, 32, 0, 0},
     {0, 0, 80, 48, 0, 0},
     {0, 0, 64, 64, 0, 0},
     {0, 0, 48, 80, 0, 0},
     {0, 0, 32, 96, 0, 0},
     {0, 0, 16, 112, 0, 0}}};

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
