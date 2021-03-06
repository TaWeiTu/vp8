#ifndef QUANTIZER_H_
#define QUANTIZER_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "bitstream_parser.h"
#include "utils.h"

namespace vp8 {

using QuantFactor = std::pair<int16_t, int16_t>;

constexpr std::array<int, kMaxQuantIndex> kDClookup = {
    4,   5,   6,   7,   8,   9,   10,  10,  11,  12,  13,  14,  15,  16,  17,
    17,  18,  19,  20,  20,  21,  21,  22,  22,  23,  23,  24,  25,  25,  26,
    27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  53,  54,
    55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
    70,  71,  72,  73,  74,  75,  76,  76,  77,  78,  79,  80,  81,  82,  83,
    84,  85,  86,  87,  88,  89,  91,  93,  95,  96,  98,  100, 101, 102, 104,
    106, 108, 110, 112, 114, 116, 118, 122, 124, 126, 128, 130, 132, 134, 136,
    138, 140, 143, 145, 148, 151, 154, 157};

constexpr std::array<int, kMaxQuantIndex> kAClookup = {
    4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  60,  62,  64,  66,  68,
    70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,
    100, 102, 104, 106, 108, 110, 112, 114, 116, 119, 122, 125, 128, 131, 134,
    137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 177, 181,
    185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 234, 239, 245,
    249, 254, 259, 264, 269, 274, 279, 284};

void Quantize(std::array<int16_t, 16>& coefficients, const QuantFactor& qf);

void Dequantize(std::array<int16_t, 16>& coefficients, const QuantFactor& dqf);

void BuildQuantFactorsY2(
    const QuantIndices& quant,
    std::array<QuantFactor, kMaxQuantIndex>& y2dqf) noexcept;

void BuildQuantFactorsY(const QuantIndices& quant,
                        std::array<QuantFactor, kMaxQuantIndex>& ydqf) noexcept;

void BuildQuantFactorsUV(
    const QuantIndices& quant,
    std::array<QuantFactor, kMaxQuantIndex>& uvdqf) noexcept;

}  // namespace vp8

#endif  // QUANTIZER_H_
