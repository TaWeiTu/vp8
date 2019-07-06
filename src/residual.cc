#include "residual.h"

namespace vp8 {

ResidualData QuantizeResidualValue(const ResidualValue &rv, int16_t qp,
                                   const QuantIndices &quant, bool has_y2) {
  ResidualData rd{};
  if (has_y2) {
    rd.has_y2 = true;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rd.dct_coeff.at(0).at(i << 2 | j) = rv.y2.at(i).at(j);
    }
    QuantizeY2(rd.dct_coeff.at(0), qp, quant);
  }
  for (size_t p = 1; p <= 16; ++p) {
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rd.dct_coeff.at(p).at(i << 2 | j) = rv.y.at(p - 1).at(i).at(j);
    }
    QuantizeY(rd.dct_coeff.at(p), qp, quant);
  }
  for (size_t p = 17; p <= 20; ++p) {
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rd.dct_coeff.at(p).at(i << 2 | j) = rv.u.at(p - 17).at(i).at(j);
    }
    QuantizeUV(rd.dct_coeff.at(p), qp, quant);
  }
  for (size_t p = 21; p <= 24; ++p) {
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        rd.dct_coeff.at(p).at(i << 2 | j) = rv.v.at(p - 21).at(i).at(j);
    }
    QuantizeUV(rd.dct_coeff.at(p), qp, quant);
  }
  return rd;
}

ResidualValue DequantizeResidualData(ResidualData &rd, int16_t qp,
                                     const QuantIndices &quant) {
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

void TransformResidual(ResidualValue &rv, bool has_y2) {
  for (size_t p = 0; p < 16; ++p) DCT(rv.y.at(p));
  for (size_t p = 0; p < 4; ++p) DCT(rv.u.at(p));
  for (size_t p = 0; p < 4; ++p) DCT(rv.v.at(p));

  if (has_y2) {
    for (size_t r = 0; r < 4; ++r) {
      for (size_t c = 0; c < 4; ++c)
        rv.y2.at(r).at(c) = rv.y.at(r << 2 | c).at(0).at(0);
    }
    WHT(rv.y2);
  }
}

void InverseTransformResidual(ResidualValue &rv, bool has_y2) {
  if (has_y2) IWHT(rv.y2);
  for (size_t p = 0; p < 16; ++p) {
    if (has_y2) rv.y.at(p).at(0).at(0) = rv.y2.at(p >> 2).at(p & 3);
    IDCT(rv.y.at(p));
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
          mb.at(r).at(c).at(i).at(j) = Clamp255(int16_t(
              mb.at(r).at(c).at(i).at(j) + residual.at(r * C + c).at(i).at(j)));
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
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j)
      sub.at(i).at(j) =
          Clamp255(int16_t(sub.at(i).at(j) + residual.at(i).at(j)));
  }
}

template <size_t C>
std::array<std::array<std::array<int16_t, 4>, 4>, C * C> ComputeMBResidual(
    const MacroBlock<C> &mb, const MacroBlock<C> &target) {
  std::array<std::array<std::array<int16_t, 4>, 4>, C * C> res{};
  for (size_t r = 0; r < C; ++r) {
    for (size_t c = 0; c < C; ++c) {
      for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
          int16_t ori = mb.at(r).at(c).at(i).at(j);
          int16_t tar = target.at(r).at(c).at(i).at(j);
          res.at(r * C + c).at(i).at(j) = tar - ori;
        }
      }
    }
  }
  return res;
}

template std::array<std::array<std::array<int16_t, 4>, 4>, 16>
ComputeMBResidual(const MacroBlock<4> &mb, const MacroBlock<4> &target);

template std::array<std::array<std::array<int16_t, 4>, 4>, 4> ComputeMBResidual(
    const MacroBlock<2> &mb, const MacroBlock<2> &target);

std::array<std::array<int16_t, 4>, 4> ComputeSBResidual(
    const SubBlock &sub, const SubBlock &target) {
  std::array<std::array<int16_t, 4>, 4> res{};
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int16_t ori = sub.at(i).at(j);
      int16_t tar = target.at(i).at(j);
      res.at(i).at(j) = tar - ori;
    }
  }
  return res;
}

}  // namespace vp8
