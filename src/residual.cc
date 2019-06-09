#include "residual.h"

namespace vp8 {

#ifdef DEBUG
  
std::ostream &operator<<(std::ostream &s, const std::array<int16_t, 16> &coeff) {
  for (size_t i = 0; i < 16; ++i)
    s << coeff.at(i) << ' ';
  return s;
}

#endif

ResidualValue DequantizeResidualData(ResidualData &rd, int16_t qp,
                                     const QuantIndices &quant) {
// #ifdef DEBUG
  // std::cerr << "DequantizeResidualData" << std::endl;
// #endif
  ResidualValue rv;
  if (rd.has_y2) {
// #ifdef DEBUG
    // std::cerr << "has_y2: " << rd.dct_coeff.at(0) << std::endl;
// #endif
    DequantizeY2(rd.dct_coeff.at(0), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.y2.at(i).at(j) = rd.dct_coeff.at(0).at(i << 2 | j);
    }
  }
  for (size_t p = 1; p <= 16; ++p) {
// #ifdef DEBUG
    // std::cerr << "y: " << rd.dct_coeff.at(p) << std::endl;
// #endif
    DequantizeY(rd.dct_coeff.at(p), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.y.at(p - 1).at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
    }
  }
  for (size_t p = 17; p <= 20; ++p) {
// #ifdef DEBUG
    // std::cerr << "u: " << rd.dct_coeff.at(p) << std::endl;
// #endif
    DequantizeUV(rd.dct_coeff.at(p), qp, quant);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rv.u.at(p - 17).at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
    }
  }
  for (size_t p = 21; p <= 24; ++p) {
// #ifdef DEBUG
    // std::cerr << "v: " << rd.dct_coeff.at(p) << std::endl;
// #endif
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

template <size_t C>
void ApplyMBResidual(
    const std::array<std::array<std::array<int16_t, 4>, 4>, C * C> &residual,
    MacroBlock<C> &mb) {
  for (size_t r = 0; r < C; ++r) {
    for (size_t c = 0; c < C; ++c) {
      for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j)
          mb.at(r).at(c).at(i).at(j) = Clamp255(mb.at(r).at(c).at(i).at(j) + residual.at(r * C + c).at(i).at(j));
      }
    }
  }
}

template void ApplyMBResidual<4>(
    const std::array<std::array<std::array<int16_t, 4>, 4>, 16> &residual,
    MacroBlock<4> &mb);

template void ApplyMBResidual<2>(
    const std::array<std::array<std::array<int16_t, 4>, 4>, 4> &residual,
    MacroBlock<2> &mb);

void ApplySBResidual(const std::array<std::array<int16_t, 4>, 4> &residual,
                     SubBlock &sub) {
// #ifdef DEBUG
  // std::cerr << "ApplySBResidual: " << std::endl;
// #endif
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      sub.at(i).at(j) = Clamp255(sub.at(i).at(j) + residual.at(i).at(j));
// #ifdef DEBUG
      // std::cerr << residual.at(i).at(j) << ' ';
// #endif
    }
// #ifdef DEBUG
    // std::cerr << std::endl;
// #endif
  }
}

}  // namespace vp8
