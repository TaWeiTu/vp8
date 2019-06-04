#include "reconstruct.h"

namespace vp8 {
namespace {

void Predict(const FrameTag &tag, const std::array<Frame, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             BitstreamParser &ps, Frame &frame) {
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      if (pre.is_inter_mb)
        InterPredict(tag, r, c, refs, ref_frame_bias, pre.ref_frame, interc, ps,
                     frame);
      else
        IntraPredict(r, c, intrac, ps, frame);
    }
  }
}

void AddResidual(const FrameHeader &header,
                 const std::vector<std::vector<InterContext>> &interc,
                 const std::vector<std::vector<IntraContext>> &intrac,
                 BitstreamParser &ps, Frame &frame) {
  std::vector<bool> y2_row(frame.vblock);
  std::vector<bool> y2_col(frame.hblock);

  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      ResidualData rd = ps.ReadResidualData(first_coef);
      
    }
  }
}

}  // namespace

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<Frame, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 Frame &frame) {
  std::vector<std::vector<InterContext>> interc(
      frame.vblock, std::vector<InterContext>(frame.hblock));
  std::vector<std::vector<IntraContext>> intrac(
      frame.vblock << 2, std::vector<IntraContext>(frame.hblock << 2));
  Predict(tag, refs, ref_frame_bias, interc, intrac, ps, frame);
}

}  // namespace vp8
