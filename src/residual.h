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

ResidualData QuantizeResidualValue(const ResidualValue &rv, int16_t qp,
                                   const QuantIndices &quant, bool has_y2);

void TransformResidual(ResidualValue &rv, bool has_y2);

// Perform IWHT on Y2 component (if any) and replace the first entry of each Y
// component with the corresponding Y2 component. Then perform IDCT on both luma
// and chroma components.
void InverseTransformResidual(ResidualValue &rv, bool has_y2);

// Apply residuals to each subblocks in the macroblock and clamp each pixel to
// range [0, 255].
template <size_t C>
void ApplyMBResidual(
    const std::array<std::array<std::array<int16_t, 4>, 4>, C * C> &residual,
    MacroBlock<C> &mb);

// Apply residuals to the subblock and clamp each pixel to range [0, 255].
void ApplySBResidual(const std::array<std::array<int16_t, 4>, 4> &residual,
                     SubBlock &sub);

template <size_t C>
std::array<std::array<std::array<int16_t, 4>, 4>, C * C> ComputeMBResidual(
    const MacroBlock<C> &mb, const MacroBlock<C> &target);

std::array<std::array<int16_t, 4>, 4> ComputeSBResidual(const SubBlock &sub,
                                                        const SubBlock &target);

}  // namespace vp8

#endif  // RESIDUAL_H_
