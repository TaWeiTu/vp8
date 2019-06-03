#include <algorithm>

#include "utils.h"

namespace vp8 {

template <typename T>
T Clamp255(T x) {
  return std::clamp(x, T(0), T(255));
}

template <typename T>
T Clamp128(T x) {
  return std::clamp(x, T(-128), T(127));
}

void ensure(bool cond, const std::string &message) {
  if (__builtin_expect(cond, true)) return;
  if (!message.empty()) std::cerr << message << std::endl;
  exit(1);
}

}  // namespace vp8