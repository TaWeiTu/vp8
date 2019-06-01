#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <iostream>
#include <string>

namespace vp8 {

template <typename T>
T Clamp255(T);

void ensure(bool, const std::string & = "");

template <typename T>
T Clamp255(T x) {
  return std::clamp(x, T(0), T(255));
}

void ensure(bool cond, const std::string &message) {
  if (__builtin_expect(cond, true)) return;
  if (!message.empty()) std::cerr << message << std::endl;
  exit(1);
}

}  // namespace vp8

#endif  // UTILS_H_
