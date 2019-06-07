#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace vp8 {

template <typename T>
inline T Clamp255(T x) {
  return std::clamp(x, T(0), T(255));
}

template <typename T>
inline T Clamp128(T x) {
  return std::clamp(x, T(-128), T(127));
}

inline void ensure(bool cond, const std::string &message) {
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
    if (ptrdiff_t(idx) >= std::distance(it_begin_, it_end_)) {
      throw std::out_of_range("Out of range");
    } else {
      return *(it_begin_ + idx);
    }
  }
  size_t size() const { return std::distance(it_begin_, it_end_); }
};

template <class T>
class SpanReader {
 private:
  const T *begin_, *end_, *cursor_;

 public:
  SpanReader(const T *begin, const T *end) : begin_(begin), end_(end), cursor_(begin) {}

  SpanReader() : SpanReader(nullptr, nullptr) {}

  uint8_t ReadByte() {
    if (begin_ >= end_) {
      throw std::out_of_range("Out of range");
    }
    return *cursor_++;
  }

  uint32_t ReadBytes(size_t n) {
    uint32_t res = 0;
    for (size_t i = 0; i < n; ++i) {
      uint8_t byte = ReadByte();
      res |= uint32_t(byte) << (i << 3);
    }
    return res;
  }

  SpanReader SubSpan(size_t offset, size_t count) {
    if (begin_ + offset + count >= end_) {
      throw std::out_of_range("Out of range");
    }
    return SpanReader(begin_ + offset, begin_ + offset + count);
  }

  SpanReader SubSpan(size_t offset) {
    if (begin_ + offset >= end_) {
      throw std::out_of_range("Out of range");
    }
    return SpanReader(begin_ + offset, end_);
  }
};

}  // namespace vp8

#endif  // UTILS_H_
