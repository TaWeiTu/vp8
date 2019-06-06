#ifndef RESIDUAL_H_
#define RESIDUAL_H_

#include "dct.h"
#include "quantizer.h"

namespace vp8 {

struct ResidualValue {
  std::array<std::array<int16_t, 4>, 4> y2;
  std::array<std::array<std::array<int16_t, 4>, 4>, 16> y;
  std::array<std::array<std::array<int16_t, 4>, 4>, 4> u;
  std::array<std::array<std::array<int16_t, 4>, 4>, 4> v;
};

ResidualValue DequantizeResidualData(ResidualData &rd, int16_t qp,
                                     const QuantIndices &quant);

void InverseTransformResidual(ResidualValue &rv, bool has_y2);

}  // namespace vp8

#endif  // RESIDUAL_H_
