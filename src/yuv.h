#ifndef YUV_H_
#define YUV_H_

#include <fstream>
#include <string>

#include "frame.h"
#include "utils.h"

namespace vp8 {

enum IOMode { READ, WRITE };

template <IOMode Mode>
class YUV {
 public:
  YUV() = default;
  ~YUV();
  explicit YUV(const char *filename);

  void WriteFrame(const Frame &frame);
  Frame ReadFrame(size_t height, size_t width);

 private:
  std::ifstream ifs_;
  std::ofstream ofs_;
};

}  // namespace vp8

#endif  // YUV_H_
