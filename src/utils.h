#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>

namespace vp8 {

template <typename T>
T Clamp255(T);

template <typename T>
T Clamp(T val, T from, T to) {
	return val < from ? from : (val > to ? to : val);
}

template <typename T>
T Clamp255(T x) {
  return Clamp(x, T(0), T(255));
}
}  // namespace vp8

#endif  // UTILS_H_
