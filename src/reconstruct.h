#ifndef RECONSTRUCT_H_
#define RECONSTRUCT_H_

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

static std::array<QuantFactor, 128> y2dqf;
static std::array<QuantFactor, 128> ydqf;
static std::array<QuantFactor, 128> uvdqf;

void UpdateNonzero(const ResidualValue &rv, bool has_y2, size_t r, size_t c,
                   std::vector<uint8_t> &y2_row, std::vector<uint8_t> &y2_col,
                   std::vector<std::vector<uint8_t>> &y1_nonzero,
                   std::vector<std::vector<uint8_t>> &u_nonzero,
                   std::vector<std::vector<uint8_t>> &v_nonzero);

void UpdateDequantFactor(const QuantIndices &quant);

void Predict(const FrameHeader &header, const FrameTag &tag,
             const std::array<std::shared_ptr<Frame>, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             std::vector<std::vector<uint8_t>> &lf,
             std::vector<std::vector<uint8_t>> &skip_lf, BitstreamParser &ps,
             const std::shared_ptr<Frame> &frame);

}  // namespace internal

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<std::shared_ptr<Frame>, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 const std::shared_ptr<Frame> &frame);

}  // namespace vp8

#endif
