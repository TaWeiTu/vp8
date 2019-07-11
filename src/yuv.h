#ifndef YUV_H_
#define YUV_H_

#include <fstream>
#include <memory>
#include <string>

#include "frame.h"
#include "utils.h"

namespace vp8 {

enum IOMode { READ, WRITE };

static const size_t kYuvBufsize = 1048576;

template <IOMode Mode>
class YUV {
 public:
  YUV() = default;
  ~YUV();
  explicit YUV(const char *filename);

  void WriteFrame(const std::shared_ptr<Frame> &frame);
  Frame ReadFrame(size_t height, size_t width);

 private:
  void WriteByte(uint8_t byte);

  std::ifstream ifs_;
  std::ofstream ofs_;
  uint8_t buf_[kYuvBufsize];
  size_t ptr_;
};

}  // namespace vp8

#endif  // YUV_H_
