#include "yuv.h"

namespace vp8 {

YUV::YUV(const std::string &filename) {
  fs_.open(filename, std::ios::binary);
  ensure(!fs_.fail(), ".at(Error) YUV::YUV: Fail to open file.");
}

YUV::YUV(const char *filename) {
  fs_.open(filename, std::ios::binary);
  ensure(!fs_.fail(), ".at(Error) YUV::YUV: Fail to open file.");
}

void YUV::WriteFrame(const Frame &frame) {
#ifdef DEBUG
  std::cerr << "vblock = " << frame.vblock << " hblock = " << frame.hblock << std::endl;
#endif
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      for (size_t i = 0; i < 16; ++i) {
        for (size_t j = 0; j < 16; j += 2) {
          int16_t y1 = frame.Y.at(r).at(c).GetPixel(i, j);
          int16_t y2 = frame.Y.at(r).at(c).GetPixel(i, j + 1);
          int16_t u = frame.U.at(r).at(c).GetPixel(i >> 1, j >> 1);
          int16_t v = frame.V.at(r).at(c).GetPixel(i >> 1, j >> 1);
          ensure(0 <= y1 && y1 <= 255,
                 ".at(Error) The Y value of the frame is not in range "
                 ".at(0, 255).");
          ensure(0 <= y2 && y2 <= 255,
                 ".at(Error) The Y value of the frame is not in range "
                 ".at(0, 255).");
          ensure(0 <= u && u <= 255,
                 ".at(Error) The U value of the frame is not in range "
                 ".at(0, 255).");
          ensure(0 <= v && v <= 255,
                 ".at(Error) The V value of the frame is not in range "
                 ".at(0, 255).");

          uint8_t sy1 = uint8_t(y1);
          uint8_t sy2 = uint8_t(y2);
          uint8_t su = uint8_t(u);
          uint8_t sv = uint8_t(v);

          fs_.write(reinterpret_cast<char *>(&sy1), sizeof(sy1));
          fs_.write(reinterpret_cast<char *>(&su), sizeof(su));
          fs_.write(reinterpret_cast<char *>(&sy2), sizeof(sy2));
          fs_.write(reinterpret_cast<char *>(&sv), sizeof(sv));
        }
      }
    }
  }
}

YUV::~YUV() {
  if (fs_.is_open()) fs_.close();
}

}  // namespace vp8
