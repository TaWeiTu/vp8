#include <cstdint>
#include "quantizer.h"
#include "utils.h"
#include <vector> 
namespace vp8 {
  QuantizerHeader quantizer_header;
  void QuantizeY(std::vector<int16_t> & coefficients, uint8_t qp) {
    int16_t DCfact = Clamp(kDClookup[ + quantizer_header.y_dc_delta_q], int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(kAClookup[qp], int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void QuantizeUV(std::vector<int16_t> & coefficients, int qp) {
    int16_t DCfact = Clamp(kDClookup[qp + quantizer_header.uv_dc_delta_q], int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(kAClookup[qp + quantizer_header.uv_ac_delta_q], int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void QuantizeY2(std::vector<int16_t> & coefficients, uint8_t qp) {
    int16_t DCfact = Clamp(int16_t(kDClookup[qp + quantizer_header.y2_dc_delta_q] * 2), int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(int16_t(kAClookup[qp + quantizer_header.y2_ac_delta_q] * 155 / 100), int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void Quantize(std::vector<int16_t> & coefficients, int16_t DCfact,
                int16_t ACfact) {
    coefficients[0] /= DCfact;
    for (size_t i = 1; i < 16; i++) {
      coefficients[i] /= ACfact;
    }
  }
  void DequantizeY(std::vector<int16_t> & coefficients, uint8_t qp) {
    int16_t DCfact = Clamp(kDClookup[qp + quantizer_header.y_dc_delta_q], int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(kAClookup[qp], int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void DequantizeUV(std::vector<int16_t> & coefficients, uint8_t qp) {
    int16_t DCfact = Clamp(kDClookup[qp + quantizer_header.uv_dc_delta_q], int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(kAClookup[qp + quantizer_header.uv_ac_delta_q], int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void DequantizeY2(std::vector<int16_t> & coefficients, uint8_t qp) {
    int16_t DCfact = Clamp(int16_t(kDClookup[qp + quantizer_header.y2_dc_delta_q] * 2), int16_t(8), int16_t(132));
    int16_t ACfact = Clamp(int16_t(kAClookup[qp + quantizer_header.y2_ac_delta_q] * 155 / 100), int16_t(8), int16_t(132));
    Quantize(coefficients, DCfact, ACfact);
  }
  void Dequantize(std::vector<int16_t> & coefficients, int16_t DCfact, int16_t ACfact) {
    coefficients[0] *= DCfact;
    for (size_t i = 1; i < 16; i++) {
      coefficients[i] *= ACfact;
    }
  }
}  // namespace bp8
