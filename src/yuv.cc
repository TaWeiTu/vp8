#include "yuv.h"

namespace vp8 {

YUV::YUV(const std::string &filename) {
  fs_.open(filename, std::ios::binary);
  ensure(!fs_.fail(), "[Error] YUV::YUV: Fail to open file.");
}

YUV::YUV(const char *filename) {
  fs_.open(filename, std::ios::binary);
  ensure(!fs_.fail(), "[Error] YUV::YUV: Fail to open file.");
}

void YUV::WriteFrame(const Frame &frame) {
  // ffplay -video_size 176x144 -framerate 1 -pixel_format yuv420p output.yuv
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      for (size_t i = 0; i < 16; ++i) {
        for (size_t j = 0; j < 16; ++j) {
          uint8_t y = uint8_t(frame.Y.at(r).at(c).GetPixel(i, j));
          fs_.write(reinterpret_cast<char *>(&y), sizeof(y));
        }
      }
    }
  }
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
          uint8_t u = uint8_t(frame.U.at(r).at(c).GetPixel(i, j));
          fs_.write(reinterpret_cast<char *>(&u), sizeof(u));
        }
      }
    }
  }
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
          uint8_t v = uint8_t(frame.V.at(r).at(c).GetPixel(i, j));
          fs_.write(reinterpret_cast<char *>(&v), sizeof(v));
        }
      }
    }
  }
  if (fs_.is_open()) fs_.close();
  exit(1);
}

YUV::~YUV() {
  if (fs_.is_open()) fs_.close();
}

}  // namespace vp8
