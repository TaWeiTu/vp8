#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <iostream>
#include <string>

namespace vp8 {

template <typename T>
T Clamp255(T);

template <typename T>
T Clamp128(T);

void ensure(bool, const std::string & = "");

}  // namespace vp8

#endif  // UTILS_H_
