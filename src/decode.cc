#include "frame.h"
#include "intra_predict.h"
#include "inter_predict.h"

void Reconstruct(const FrameHeader &header, Frame &frame) {
  auto &mh = header.macroblock_header;
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      if (mh[r][c].is_inter_mb) InterPredict(header, r, c, frame);
      else InterPredict(header, r, c, frame);

      // TODO: Dequantize the DCT coefficients and perform inverse DCT to get the residuals. 
    }
  }
}
