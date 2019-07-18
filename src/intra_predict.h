#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "bitstream_parser.h"
#include "context.h"
#include "frame.h"
#include "residual.h"
#include "utils.h"

namespace vp8 {

constexpr uint16_t kAllVPred =
  (B_VE_PRED << 12) | (B_VE_PRED << 8) | (B_VE_PRED << 4) | B_VE_PRED;
constexpr uint16_t kAllHPred =
  (B_HE_PRED << 12) | (B_HE_PRED << 8) | (B_HE_PRED << 4) | B_HE_PRED;
constexpr uint16_t kAllDCPred =
  (B_DC_PRED << 12) | (B_DC_PRED << 8) | (B_DC_PRED << 4) | B_DC_PRED;
constexpr uint16_t kAllTMPred =
  (B_TM_PRED << 12) | (B_TM_PRED << 8) | (B_TM_PRED << 4) | B_TM_PRED;

namespace internal {

constexpr int16_t kUpperPixel = 127;
constexpr int16_t kUpperLeftPixel = 128;
constexpr int16_t kLeftPixel = 129;

void VPredChroma(size_t r, size_t c, Plane<2> &mb);
void HPredChroma(size_t r, size_t c, Plane<2> &mb);
void DCPredChroma(size_t r, size_t c, Plane<2> &mb);
void TMPredChroma(size_t r, size_t c, Plane<2> &mb);

void VPredLuma(size_t r, size_t c, Plane<4> &mb);
void HPredLuma(size_t r, size_t c, Plane<4> &mb);
void DCPredLuma(size_t r, size_t c, Plane<4> &mb);
void TMPredLuma(size_t r, size_t c, Plane<4> &mb);

std::array<Context, 2> BPredLuma(size_t r, size_t c, bool is_key_frame,
                                 const ResidualValue &rv,
                                 const std::array<Context, 2> &context,
                                 const std::unique_ptr<BitstreamParser> &ps,
                                 Plane<4> &mb);

void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p,
                   SubBlockMode mode, SubBlock &sub);
}  // namespace internal

std::array<Context, 2> IntraPredict(const FrameTag &tag, size_t r, size_t c,
                                    const ResidualValue &rv,
                                    const IntraMBHeader &mh,
                                    const std::array<Context, 2> &context,
                                    std::vector<std::vector<uint8_t>> &skip_lf,
                                    const std::unique_ptr<BitstreamParser> &ps,
                                    const std::shared_ptr<Frame> &frame);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
