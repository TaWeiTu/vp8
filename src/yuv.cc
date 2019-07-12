#include "yuv.h"

namespace vp8 {

template <>
void YUV<WRITE>::WriteFrame(const std::shared_ptr<Frame> &frame) {
  for (size_t r = 0; r < frame->vsize; ++r) {
    for (size_t c = 0; c < frame->hsize; ++c) {
      uint8_t y =
          uint8_t(frame->Y.at(r >> 4).at(c >> 4).GetPixel(r & 15, c & 15));
      WriteByte(y);
    }
  }
  for (size_t r = 0; r < frame->vsize; r += 2) {
    for (size_t c = 0; c < frame->hsize; c += 2) {
      uint8_t u = uint8_t(
          frame->U.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
      WriteByte(u);
    }
  }
  for (size_t r = 0; r < frame->vsize; r += 2) {
    for (size_t c = 0; c < frame->hsize; c += 2) {
      uint8_t v = uint8_t(
          frame->V.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
      WriteByte(v);
    }
  }
}

template <>
Frame YUV<READ>::ReadFrame(size_t height, size_t width) {
  Frame res(height, width);
  for (size_t r = 0; r < height; ++r) {
    for (size_t c = 0; c < width; ++c) {
      uint8_t y = ReadByte();
      res.Y.at(r >> 4).at(c >> 4).SetPixel(r & 15, c & 15, y);
    }
  }
  for (size_t r = 0; r < height; r += 2) {
    for (size_t c = 0; c < width; c += 2) {
      uint8_t u = ReadByte();
      res.U.at(r >> 4).at(c >> 4).SetPixel((r >> 1) & 7, (c >> 1) & 7, u);
    }
  }
  for (size_t r = 0; r < height; r += 2) {
    for (size_t c = 0; c < width; c += 2) {
      uint8_t v = ReadByte();
      res.V.at(r >> 4).at(c >> 4).SetPixel((r >> 1) & 7, (c >> 1) & 7, v);
    }
  }
  return res;
}

template <>
YUV<WRITE>::~YUV() {
  if (ptr_ > 0) {
    fs_.write(reinterpret_cast<char *>(buf_.get()), long(ptr_));
  }
  if (fs_.is_open()) fs_.close();
}

template <>
YUV<READ>::~YUV() {
  if (fs_.is_open()) fs_.close();
}

}  // namespace vp8
