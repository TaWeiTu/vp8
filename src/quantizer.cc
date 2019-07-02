#include "quantizer.h"

namespace vp8 {
namespace internal {

void Quantize(std::array<int16_t, 16>& coefficients, int16_t DCfact,
              int16_t ACfact) {
  coefficients.at(0) /= DCfact;
  for (size_t i = 1; i < 16; i++) {
    coefficients.at(i) /= ACfact;
  }
}
void Dequantize(std::array<int16_t, 16>& coefficients, int16_t DCfact,
                int16_t ACfact) {
  coefficients.at(0) *= DCfact;
  for (size_t i = 1; i < 16; i++) {
    coefficients.at(i) *= ACfact;
  }
}

}  // namespace internal

using namespace internal;

static const int kClampLb = 0;
static const int kClampUb = 127;

void QuantizeY(std::array<int16_t, 16>& coefficients, int16_t qp,
               const QuantIndices& quantizer_header) {
  size_t idx = size_t(
      std::clamp(qp + quantizer_header.y_dc_delta_q, kClampLb, kClampUb));
  int16_t DCfact =
      std::clamp(int16_t(kDClookup.at(idx)), int16_t(8), int16_t(132));
  int16_t ACfact =
      std::clamp(int16_t(kAClookup.at(size_t(qp))), int16_t(8), int16_t(132));
  Quantize(coefficients, DCfact, ACfact);
}

void QuantizeUV(std::array<int16_t, 16>& coefficients, int16_t qp,
                const QuantIndices& quantizer_header) {
  size_t idx_dc = size_t(
      std::clamp(qp + quantizer_header.uv_dc_delta_q, kClampLb, kClampUb));
  size_t idx_ac = size_t(
      std::clamp(qp + quantizer_header.uv_ac_delta_q, kClampLb, kClampUb));
  int16_t DCfact =
      std::clamp(int16_t(kDClookup.at(idx_dc)), int16_t(8), int16_t(132));
  int16_t ACfact =
      std::clamp(int16_t(kAClookup.at(idx_ac)), int16_t(8), int16_t(132));
  Quantize(coefficients, DCfact, ACfact);
}

void QuantizeY2(std::array<int16_t, 16>& coefficients, int16_t qp,
                const QuantIndices& quantizer_header) {
  size_t idx_dc = size_t(
      std::clamp(quantizer_header.y2_dc_delta_q + qp, kClampLb, kClampUb));
  size_t idx_ac = size_t(
      std::clamp(quantizer_header.y2_ac_delta_q + qp, kClampLb, kClampUb));
  int16_t DCfact =
      std::clamp(int16_t(kDClookup.at(idx_dc) * 2), int16_t(8), int16_t(132));
  int16_t ACfact = std::clamp(int16_t(kAClookup.at(idx_ac) * 155 / 100),
                              int16_t(8), int16_t(132));
  Quantize(coefficients, DCfact, ACfact);
}

void DequantizeY(std::array<int16_t, 16>& coefficients, int16_t qp,
                 const QuantIndices& quantizer_header) {
  size_t idx = size_t(
      std::clamp(quantizer_header.y_dc_delta_q + qp, kClampLb, kClampUb));
  int16_t DCfact = int16_t(kDClookup.at(idx));
  int16_t ACfact = int16_t(kAClookup.at(size_t(qp)));
  Dequantize(coefficients, DCfact, ACfact);
}

void DequantizeUV(std::array<int16_t, 16>& coefficients, int16_t qp,
                  const QuantIndices& quantizer_header) {
  size_t idx_dc = size_t(
      std::clamp(quantizer_header.uv_dc_delta_q + qp, kClampLb, kClampUb));
  size_t idx_ac = size_t(
      std::clamp(quantizer_header.uv_ac_delta_q + qp, kClampLb, kClampUb));
  int16_t DCfact = int16_t(kDClookup.at(idx_dc));
  if (DCfact > 132) DCfact = 132;
  int16_t ACfact = int16_t(kAClookup.at(idx_ac));
  Dequantize(coefficients, DCfact, ACfact);
}

void DequantizeY2(std::array<int16_t, 16>& coefficients, int16_t qp,
                  const QuantIndices& quantizer_header) {
  size_t idx_dc = size_t(
      std::clamp(quantizer_header.y2_dc_delta_q + qp, kClampLb, kClampUb));
  size_t idx_ac = size_t(
      std::clamp(quantizer_header.y2_ac_delta_q + qp, kClampLb, kClampUb));
  int16_t DCfact = int16_t(kDClookup.at(idx_dc) * 2);
  int16_t ACfact = int16_t(kAClookup.at(idx_ac) * 155 / 100);
  if (ACfact < 8) ACfact = 8;
  Dequantize(coefficients, DCfact, ACfact);
}

}  // namespace vp8
