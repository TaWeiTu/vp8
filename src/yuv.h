#ifndef YUV_H_
#define YUV_H_

#include <fstream>
#include <memory>
#include <string>
#include <type_traits>

#include "frame.h"
#include "utils.h"

namespace vp8 {

enum IOMode { READ, WRITE };

constexpr size_t kBufSize = 1048576;

template <IOMode Mode>
class YUV {
 public:
  YUV() = default;
  ~YUV();
  explicit YUV(const char *filename)
      : ptr_(Mode == WRITE ? 0 : kBufSize),
        buf_(std::make_unique<uint8_t[]>(kBufSize)) {
    fs_.open(filename, std::ios::binary);
    ensure(!fs_.fail(), "[Error] YUV::YUV: Fail to open file.");
  }

  void WriteFrame(const std::shared_ptr<Frame> &frame);
  Frame ReadFrame(size_t height, size_t width);

 private:
  template <IOMode M = Mode>
  typename std::enable_if<M == WRITE>::type WriteByte(uint8_t byte) {
    buf_[ptr_++] = byte;
    if (ptr_ == kBufSize) {
      fs_.write(reinterpret_cast<char *>(buf_.get()), kBufSize);
      ptr_ = 0;
    }
  }

  template <IOMode M = Mode>
  typename std::enable_if<M == READ, uint8_t>::type ReadByte() {
    if (ptr_ == kBufSize) {
      fs_.read(reinterpret_cast<char *>(buf_.get()), kBufSize);
      ptr_ = 0;
    }
    return buf_[ptr_++];
  }

  std::conditional_t<Mode == READ, std::ifstream, std::ofstream> fs_;
  size_t ptr_;
  std::unique_ptr<uint8_t[]> buf_;
};

}  // namespace vp8

#endif  // YUV_H_
