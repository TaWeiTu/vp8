#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>

namespace vp8 {

int16_t clamp255(int16_t);

int16_t clamp255(int16_t x) {
  static const int16_t lo = 0;
  static const int16_t hi = 255;
  return std::clamp(x, lo, hi);
}

}  // namespace vp8

#endif  // UTILS_H_
