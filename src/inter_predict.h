#ifndef INTER_PREDICT_H_
#define INTER_PREDICT_H_

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "residual.h"
#include "utils.h"

namespace vp8 {

struct InterContext {
  bool is_inter_mb;
  MacroBlockMV mv_mode;
  uint8_t ref_frame;

  InterContext() : is_inter_mb(false), mv_mode(MV_ZERO), ref_frame(0) {}

  explicit InterContext(MacroBlockMV mv_mode_, uint8_t ref_frame_)
      : is_inter_mb(true), mv_mode(mv_mode_), ref_frame(ref_frame_) {}

  explicit InterContext(bool is_inter_mb_, MacroBlockMV mv_mode_,
                        uint8_t ref_frame_)
      : is_inter_mb(is_inter_mb_), mv_mode(mv_mode_), ref_frame(ref_frame_) {}
};

namespace internal {

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
InterMBHeader SearchMVs(size_t r, size_t c, const Plane<4> &mb,
                        const std::array<bool, 4> &ref_frame_bias,
                        uint8_t ref_frame,
                        const std::vector<std::vector<InterContext>> &context,
                        BitstreamParser &ps, MotionVector &best,
                        MotionVector &nearest, MotionVector &near);

// Make sure that the motion vector indeed points to a valid position.
void ClampMV2(int16_t left, int16_t right, int16_t top, int16_t bottom,
              MotionVector &mb);
void ClampMV(int16_t left, int16_t right, int16_t top, int16_t bottom,
             MotionVector &mb);

// Invert the motion vector the sign bias is different in the reference frames
// of two macroblocks.
MotionVector Invert(const MotionVector &mv, uint8_t ref_frame1,
                    uint8_t ref_frame2,
                    const std::array<bool, 4> &ref_frame_bias);

// Decide the probability table of the current subblock based on the motion
// vectors of the left and above subblocks.
uint8_t SubBlockContext(const MotionVector &left, const MotionVector &above);

// The motion vectors of chroma subblocks are the average value of the motion
// vectors occupying the same position in the luma subblocks.
void ConfigureChromaMVs(const MacroBlock<4> &luma, size_t vblock, size_t hblock,
                        bool trim, MacroBlock<2> &U, MacroBlock<2> &V);

// Pre-built table for fast partition of the macroblocks.
constexpr std::array<uint8_t, 4> kNumPartition = {2, 2, 4, 16};

constexpr std::array<uint64_t, 4> kHead = {128, 32, 43040,
                                           18364758544493064720};

constexpr std::array<std::array<int8_t, 16>, 4> kNext = {
    {{1, 2, 3, 4, 5, 6, 7, -1, 9, 10, 11, 12, 13, 14, 15, -1},
     {1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, -1, 15, -1},
     {1, 4, 3, 6, 5, -1, 7, -1, 9, 12, 11, 14, 13, -1, 15, -1},
     {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}};

// In case of mode MV_SPLIT, set the motion vectors of each subblock
// independently.
void ConfigureSubBlockMVs(const InterMBHeader &hd, size_t r, size_t c,
                          MotionVector best, BitstreamParser &ps, Plane<4> &mb);

// For each (luma or chroma) macroblocks, configure their motion vectors (if
// needed).
void ConfigureMVs(size_t r, size_t c, bool trim,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame);

// Horizontal pixel interpolation, this should return a 9x4 temporary matrix for
// the vertical pixel interpolation later.
template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorizontalSixtap(
    const Plane<C> &refer, int32_t r, int32_t c,
    const std::array<int16_t, 6> &filter);

// Vertical pixel interpolation.
void VerticalSixtap(const std::array<std::array<int16_t, 4>, 9> &refer,
                    const std::array<int16_t, 6> &filter, SubBlock &sub);

// Sixtap pixel interpolation. First do the horizontal interpolation, then
// vertical.
template <size_t C>
void Sixtap(const Plane<C> &refer, int32_t r, int32_t c, uint8_t mr, uint8_t mc,
            const std::array<std::array<int16_t, 6>, 8> &filter, SubBlock &sub);

// For each of the macroblock in the current plane, predict the value of it.
template <size_t C>
void InterpBlock(const Plane<C> &refer,
                 const std::array<std::array<int16_t, 6>, 8> &filter, size_t r,
                 size_t c, MacroBlock<C> &mb);

}  // namespace internal

void InterPredict(const FrameTag &tag, size_t r, size_t c,
                  const std::array<Frame, 4> &refs,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame);

}  // namespace vp8

#endif  // INTER_PREDICT_H_
