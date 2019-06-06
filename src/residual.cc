#include "residual.h"

namespace vp8 {

ResidualValue DequantizeResidualData(ResidualData& rd, int16_t qp,
                                     const QuantIndices& quant) {
  ResidualValue rv;
  if (rd.has_y2) {
    DequantizeY2(rd.dct_coeff.at(0), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.y2.at(i).at(j) = rd.dct_coeff.at(0).at(i << 2 | j);
    }
  }
  for (size_t p = 1; p <= 16; ++p) {
    DequantizeY(rd.dct_coeff.at(p), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.y.at(p - 1).at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
    }
  }
  for (size_t p = 17; p <= 20; ++p) {
    DequantizeUV(rd.dct_coeff.at(p), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.u.at(p - 17).at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
    }
  }
  for (size_t p = 21; p <= 24; ++p) {
    DequantizeUV(rd.dct_coeff.at(p), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.v.at(p - 21).at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
    }
  }
  return rv;
}

void InverseTransformResidual(ResidualValue &rv, bool has_y2) {
  if (has_y2) IWHT(rv.y2);
  for (size_t p = 0; p < 16; ++p) {
    IDCT(rv.y.at(p));
    if (has_y2) rv.y.at(p).at(0).at(0) = rv.y2.at(p >> 2).at(p & 3);
  }
  for (size_t p = 0; p < 4; ++p) IDCT(rv.u.at(p));
  for (size_t p = 0; p < 4; ++p) IDCT(rv.v.at(p));
}

}  // namespace vp8
