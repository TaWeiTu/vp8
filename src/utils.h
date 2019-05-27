#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>

int16_t clamp255(int16_t);

int16_t clamp255(int16_t x) {
  static const int16_t lo = 0;
  static const int16_t hi = 255;
  return std::clamp(x, lo, hi);
}

#endif  // UTILS_H_
