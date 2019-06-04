#ifndef RECONSTRUCT_H_
#define RECONSTRUCT_H_

#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"

namespace vp8 {
namespace {

void Predict(const FrameTag &tag, const std::array<Frame, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             BitstreamParser &ps, Frame &frame);

}  // namespace

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<Frame, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 Frame &frame);

}  // namespace vp8

#endif
