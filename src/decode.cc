#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "intra_predict.h"
#include "inter_predict.h"

void Reconstruct(const FrameHeader &header, BitstreamParser &ps, Frame &frame) {
  std::vector<std::vector<BlockContext>> context(vblock, std::vector<BlockContextr>(hblock));
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      if (pre.is_inter_mb) InterPredict(header, r, c, pre, context, ps, frame);
      else IntraPredict(header, r, c, pre, ps, frame);
      mbheaders[r][c] = mh;

      // TODO: Dequantize the DCT coefficients and perform inverse DCT to get the residuals. 
    }
  }
}
