#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

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

template <class T>
class IteratorArray {
 private:
  using IterT = typename T::iterator;
  using ObjT = typename std::iterator_traits<IterT>::value_type;
  IterT it_begin_, it_end_;

 public:
  IteratorArray(IterT _begin, IterT _end) : it_begin_(_begin), it_end_(_end) {}
  ObjT at(const size_t idx) const {
    if (idx >= std::distance(it_begin_, it_end_)) {
      throw std::out_of_range("Out of range");
    } else {
      return *(it_begin_ + idx);
    }
  }
  size_t size() const { return std::distance(it_begin_, it_end_); }
};

}  //namespace vp8

#endif  // UTILS_H_
