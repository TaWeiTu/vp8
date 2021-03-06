#ifndef DECODE_FRAME_H_
#define DECODE_FRAME_H_

#include <array>
#include <memory>
#include <vector>

#include "bitstream_parser.h"
#include "dct.h"
#include "filter.h"
#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"
#include "quantizer.h"
#include "residual.h"

namespace vp8 {
namespace internal {

static std::array<QuantFactor, kMaxQuantIndex> y2dqf;
static std::array<QuantFactor, kMaxQuantIndex> ydqf;
static std::array<QuantFactor, kMaxQuantIndex> uvdqf;

void UpdateNonzero(const ResidualValue &rv, bool has_y2, size_t r, size_t c,
                   std::vector<uint8_t> &y2_row, std::vector<uint8_t> &y2_col,
                   std::vector<std::vector<uint8_t>> &y1_nonzero,
                   std::vector<std::vector<uint8_t>> &u_nonzero,
                   std::vector<std::vector<uint8_t>> &v_nonzero) noexcept;

void UpdateDequantFactor(const QuantIndices &quant);

void Predict(const FrameHeader &header, const FrameTag &tag,
             const std::array<std::shared_ptr<Frame>, kNumRefFrames> &refs,
             const std::array<bool, kNumRefFrames> &ref_frame_bias,
             std::vector<std::vector<uint8_t>> &lf,
             std::vector<std::vector<uint8_t>> &skip_lf,
             const std::unique_ptr<BitstreamParser> &ps,
             const std::shared_ptr<Frame> &frame);

}  // namespace internal

void DecodeFrame(const FrameHeader &header, const FrameTag &tag,
                 const std::array<std::shared_ptr<Frame>, kNumRefFrames> &refs,
                 const std::array<bool, kNumRefFrames> &ref_frame_bias,
                 const std::unique_ptr<BitstreamParser> &ps,
                 const std::shared_ptr<Frame> &frame);

}  // namespace vp8

#endif
