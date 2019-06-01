#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <iostream>
#include <string>

namespace vp8 {

template <typename T>
T Clamp(T, T, T);

template <typename T>
T Clamp255(T);

template <typename T>
T Clamp128(T);

void ensure(bool, const std::string & = "");

template <typename T>
T abs(T x) {
  return x < T(0) ? -x : x;
}

template <typename T>
T Clamp(T val, T from, T to) {
  return val < from ? from : (val > to ? to : val);
}

template <typename T>
T Clamp255(T x) {
  return Clamp(x, T(0), T(255));
}

template <typename T>
T Clamp128(T x) {
  return Clamp(x, T(-128), T(127));
}

void ensure(bool cond, const std::string &message) {
  if (__builtin_expect(cond, true)) return;
  if (!message.empty()) std::cerr << message << std::endl;
  exit(1);
}

}  // namespace vp8

#endif  // UTILS_H_
