#ifndef FILTER_H_
#define FILTER_H_

#include "utils.h"

namespace vp8 {
int16_t minus128(int16_t x) { return Clamp128(x - 128); }
int16_t plus128(int16_t x) { return Clamp255(x + 128); }
}  // namespace vp8
#endif  // FILTER_H_
