#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "intra_predict.h"
#include "inter_predict.h"

void Reconstruct(const FrameHeader &header, BitStreamParser &ps, Frame &frame) {
  std::vector<std::vector<MacroBlockHeader>> mbheaders(vblock, std::vector<MacroBlockHeader>(hblock));
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockHeader mh = ps.ReadMacroBlockHeader();
      if (mh.is_inter_mb) InterPredict(header, r, c, mbheaders, mh, frame);
      else IntraPredict(header, r, c, mh, frame);
      mbheaders[r][c] = mh;

      // TODO: Dequantize the DCT coefficients and perform inverse DCT to get the residuals. 
    }
  }
}
