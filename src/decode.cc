#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"

namespace vp8 {

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<Frame, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 Frame &frame) {
  std::vector<std::vector<InterContext>> interc(
      frame.vblock, std::vector<InterContext>(frame.hblock));
  std::vector<std::vector<IntraContext>> intrac(
      frame.vblock << 2, std::vector<IntraContext>(frame.hblock << 2));

  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      if (pre.is_inter_mb)
        InterPredict(tag, r, c, refs, ref_frame_bias, pre.ref_frame, interc, ps,
                     frame);
      else
        IntraPredict(r, c, intrac, ps, frame);

      // TODO: Dequantize the DCT coefficients and perform inverse DCT to get
      // the residuals.
    }
  }
}

}

int main() {

}
