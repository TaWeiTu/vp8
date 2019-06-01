#include "quantizer.h"
#include "utils.h"

namepspace vp8 {
	QuantizerHeader quantizer_header;
	void QuantizeY(std::vector<int16_t> &coefficients, uint8_t qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + y_dc_delta_q], 8, 132);
		uint8_t ACfact = Clamp(kAClookup[qp], 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
	void QuantizeUV(std::vector<int16_t> &coefficients, int qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + uv_dc_delta_q], 8, 132);
		uint8_t ACfact = Clamp(kAClookup[qp + uv_ac_delta_q], 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
	void QuantizeY2(std::vector<int16_t> &coefficients, uint8_t qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + y2_dc_delta_q] * 2, 8, 132);
		uint8_t ACfact = Clamp(uint8_t(int16_t(kAClookup[qp + y2_ac_delta_q]) * 155 / 100), 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
 	void Quantize(std::vector<int16_t> &coefficients, uint8_t DCfact, uint8_t ACfact) {
		coefficients[0] /= int16_t(DCfact);
		for(int i=1;i<16;i++) {
			coefficients[i] /= int16_t(ACfact);
		}
	}
	void DequantizeY(std::vector<int16_t> &coefficients, uint8_t qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + y_dc_delta_q], 8, 132);
		uint8_t ACfact = Clamp(kAClookup[qp], 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
	void DequantizeUV(std::vector<int16_t> &coefficients, uint8_t qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + uv_dc_delta_q], 8, 132);
		uint8_t ACfact = Clamp(kAClookup[qp + uv_ac_delta_q], 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
	void DequantizeY2(std::vector<int16_t> &coefficients, uint8_t qp) {
		uint8_t DCfact = Clamp(kDClookup[qp + y2_dc_delta_q] * 2, 8, 132);
		uint8_t ACfact = Clamp(uint8_t(int16_t(kAClookup[qp + y2_ac_delta_q]) * 155 / 100), 8, 132);
		Quantize(coefficients, DCfact, ACfact);
	}
 	void Dequantize(std::vector<int16_t> &coefficients, uint8_t DCfact, uint8_t ACfact) {
		coefficients[0] *= int16_t(DCfact);
		for(int i=1;i<16;i++) {
			coefficients[i] *= int16_t(ACfact);
		}
	}
} // namespace bp8
