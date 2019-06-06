#ifndef YUV_H_
#define YUV_H_

#include <fstream>
#include <string>

#include "frame.h"
#include "utils.h"

namespace vp8 {

class YUV {
 public:
  YUV() = default;
  ~YUV();
  explicit YUV(const std::string &filename);
  explicit YUV(const char *filename);
  void WriteFrame(const Frame &frame);

 private:
  std::ofstream fs_;
};

}  // namespace vp8

#endif  // YUV_H_
