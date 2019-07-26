#include "quantizer.h"

namespace vp8 {

void Quantize(std::array<int16_t, 16>& coefficients, const QuantFactor& qf) {
  coefficients.at(0) /= qf.first;
  for (size_t i = 1; i < 16; i++) coefficients.at(i) /= qf.second;
}

void Dequantize(std::array<int16_t, 16>& coefficients, const QuantFactor& dqf) {
  coefficients.at(0) *= dqf.first;
  for (size_t i = 1; i < 16; i++) coefficients.at(i) *= dqf.second;
}

void BuildQuantFactorsY2(
    const QuantIndices& quant,
    std::array<QuantFactor, kMaxQuantIndex>& y2dqf) noexcept {
  for (int16_t i = 0; i < kMaxQuantIndex; ++i) {
    size_t idx_dc = size_t(std::clamp(quant.y2_dc_delta_q + i, 0, 127));
    size_t idx_ac = size_t(std::clamp(quant.y2_ac_delta_q + i, 0, 127));
    int16_t DCfact = int16_t(kDClookup.at(idx_dc) * 2);
    int16_t ACfact = int16_t((kAClookup.at(idx_ac) * 101581) >> 16);
    if (ACfact < 8) ACfact = 8;

    y2dqf.at(size_t(i)) = std::make_pair(DCfact, ACfact);
  }
}

void BuildQuantFactorsY(
    const QuantIndices& quant,
    std::array<QuantFactor, kMaxQuantIndex>& ydqf) noexcept {
  for (int16_t i = 0; i < kMaxQuantIndex; ++i) {
    size_t idx = size_t(std::clamp(quant.y_dc_delta_q + i, 0, 127));
    int16_t DCfact = int16_t(kDClookup.at(idx));
    int16_t ACfact = int16_t(kAClookup.at(size_t(i)));

    ydqf.at(size_t(i)) = std::make_pair(DCfact, ACfact);
  }
}

void BuildQuantFactorsUV(
    const QuantIndices& quant,
    std::array<QuantFactor, kMaxQuantIndex>& uvdqf) noexcept {
  for (int16_t i = 0; i < kMaxQuantIndex; ++i) {
    size_t idx_dc = size_t(std::clamp(quant.uv_dc_delta_q + i, 0, 127));
    size_t idx_ac = size_t(std::clamp(quant.uv_ac_delta_q + i, 0, 127));
    int16_t DCfact = int16_t(kDClookup.at(idx_dc));
    if (DCfact > 132) DCfact = 132;
    int16_t ACfact = int16_t(kAClookup.at(idx_ac));

    uvdqf.at(size_t(i)) = std::make_pair(DCfact, ACfact);
  }
}

}  // namespace vp8
