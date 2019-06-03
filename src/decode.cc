#include <array>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<Frame, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 Frame &frame) {
  std::vector<std::vector<InterContext>> interc(
      vblock, std::vector<InterContextr>(hblock));
  std;:vector<std::vector<IntraContext>> intrac(vblock << 2, std::vector<IntraContext>(hblock << 2));
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      if (pre.is_inter_mb)
        InterPredict(r, c, tag, refs, ref_frame_bias, pre.ref_frame, interc, ps,
                     frame);
      else
        IntraPredict(r, c, intrac, ps, frame);

      // TODO: Dequantize the DCT coefficients and perform inverse DCT to get
      // the residuals.
    }
  }
}
