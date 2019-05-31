#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>

namespace vp8 {

template <typename T>
T Clamp255(T);

template <typename T>
T Clamp255(T x) {
  return std::clamp(x, T(0), T(255));
}

}  // namespace vp8

#endif  // UTILS_H_
