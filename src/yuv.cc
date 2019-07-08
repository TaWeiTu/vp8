#include "yuv.h"

namespace vp8 {

template <>
YUV<WRITE>::YUV(const char *filename) {
  ofs_.open(filename, std::ios::binary);
  ensure(!ofs_.fail(), "[Error] YUV::YUV: Fail to open file.");
}

template <>
YUV<READ>::YUV(const char *filename) {
  ifs_.open(filename, std::ios::binary);
  ensure(!ifs_.fail(), "[Error] YUV::YUV: Fail to open file.");
}

template <>
void YUV<WRITE>::WriteFrame(const std::shared_ptr<Frame> &frame) {
  for (size_t r = 0; r < frame->vsize; ++r) {
    for (size_t c = 0; c < frame->hsize; ++c) {
      uint8_t y =
          uint8_t(frame->Y.at(r >> 4).at(c >> 4).GetPixel(r & 15, c & 15));
      ofs_.write(reinterpret_cast<char *>(&y), sizeof(y));
    }
  }
  for (size_t r = 0; r < frame->vsize; r += 2) {
    for (size_t c = 0; c < frame->hsize; c += 2) {
      uint8_t u = uint8_t(
          frame->U.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
      ofs_.write(reinterpret_cast<char *>(&u), sizeof(u));
    }
  }
  for (size_t r = 0; r < frame->vsize; r += 2) {
    for (size_t c = 0; c < frame->hsize; c += 2) {
      uint8_t v = uint8_t(
          frame->V.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
      ofs_.write(reinterpret_cast<char *>(&v), sizeof(v));
    }
  }
}

template <>
Frame YUV<READ>::ReadFrame(size_t height, size_t width) {
  Frame res(height, width);
  for (size_t r = 0; r < height; ++r) {
    for (size_t c = 0; c < width; ++c) {
      uint8_t y;
      ifs_.read(reinterpret_cast<char *>(&y), sizeof(y));
      res.Y.at(r >> 4).at(c >> 4).SetPixel(r & 15, c & 15, y);
    }
  }
  for (size_t r = 0; r < height; r += 2) {
    for (size_t c = 0; c < width; c += 2) {
      uint8_t u;
      ifs_.read(reinterpret_cast<char *>(&u), sizeof(u));
      res.U.at(r >> 4).at(c >> 4).SetPixel((r >> 1) & 7, (c >> 1) & 7, u);
    }
  }
  for (size_t r = 0; r < height; r += 2) {
    for (size_t c = 0; c < width; c += 2) {
      uint8_t v;
      ifs_.read(reinterpret_cast<char *>(&v), sizeof(v));
      res.V.at(r >> 4).at(c >> 4).SetPixel((r >> 1) & 7, (c >> 1) & 7, v);
    }
  }
  return res;
}

template <>
YUV<WRITE>::~YUV() {
  if (ofs_.is_open()) ofs_.close();
}

template <>
YUV<READ>::~YUV() {
  if (ifs_.is_open()) ifs_.close();
}

}  // namespace vp8
