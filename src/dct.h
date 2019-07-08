#ifndef DCT_H_
#define DCT_H_

#include <array>
#include <iostream>

#include "quantizer.h"

namespace vp8 {

void DCT(std::array<std::array<int16_t, 4>, 4> &subblock);

void IDCT(std::array<std::array<int16_t, 4>, 4> &subblock);

void WHT(std::array<std::array<int16_t, 4>, 4> &subblock);

void IWHT(std::array<std::array<int16_t, 4>, 4> &subblock);

}  // namespace vp8

#endif  // DCT_H_
