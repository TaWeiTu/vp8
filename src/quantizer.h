#ifndef QUANTIZER_H_
#define QUANTIZER_H_

#include <cstdint>
#include <vector>

namespace vp8 {

struct QuantizerHeader {
  size_t q_index;
  uint8_t delta_update;
  uint8_t y_dc_delta_q;
  uint8_t y2_dc_delta_q;
  uint8_t y2_ac_delta_q;
  uint8_t uv_dc_delta_q;
  uint8_t uv_ac_delta_q;
};

const std::vector<int16_t> kDClookup = {
    4,   5,   6,   7,   8,   9,   10,  10,  11,  12,  13,  14,  15,  16,  17,
    17,  18,  19,  20,  20,  21,  21,  22,  22,  23,  23,  24,  25,  25,  26,
    27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  53,  54,
    55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
    70,  71,  72,  73,  74,  75,  76,  76,  77,  78,  79,  80,  81,  82,  83,
    84,  85,  86,  87,  88,  89,  91,  93,  95,  96,  98,  100, 101, 102, 104,
    106, 108, 110, 112, 114, 116, 118, 122, 124, 126, 128, 130, 132, 134, 136,
    138, 140, 143, 145, 148, 151, 154, 157};

const std::vector<int16_t> kAClookup = {
    4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  60,  62,  64,  66,  68,
    70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,
    100, 102, 104, 106, 108, 110, 112, 114, 116, 119, 122, 125, 128, 131, 134,
    137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 177, 181,
    185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 234, 239, 245,
    249, 254, 259, 264, 269, 274, 279, 284};

void QuantizeY(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void QuantizeUV(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void QuantizeY2(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void Quantize(std::vector<int16_t> &, int16_t, int16_t);
void DequantizeY(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void DequantizeUV(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void DequantizeY2(std::vector<int16_t> &, uint8_t, const QuantizerHeader &);
void Dequantize(std::vector<int16_t> &, int16_t, int16_t);

}  // namespace vp8

#endif  // QUANTIZER_H_