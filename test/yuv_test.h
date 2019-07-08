#ifndef YUV_TEST_H_
#define YUV_TEST_H_

#include "../src/yuv.h"

#include <iostream>

namespace vp8_test {

void TestYuv();

void TestYuv() {
  std::cout << "[Test] YUV test started." << std::endl;
  static std::mt19937 kRng(7122);
  static std::uniform_int_distribution<int16_t> kDis(0, 255);
  const size_t kH = 176, kW = 144, kF = 100;

  vp8::YUV<vp8::WRITE> yuv("test.yuv");
  for (size_t k = 0; k < kF; ++k) {
    std::shared_ptr<vp8::Frame> f = std::make_unique<vp8::Frame>(kH, kW);
    for (size_t r = 0; r < kH / 16; ++r) {
      for (size_t c = 0; c < kW / 16; ++c) {
        for (size_t i = 0; i < 16; ++i) {
          for (size_t j = 0; j < 16; ++j)
            f->Y.at(r).at(c).SetPixel(i, j, kDis(kRng));
        }
        for (size_t i = 0; i < 8; ++i) {
          for (size_t j = 0; j < 8; ++j)
            f->U.at(r).at(c).SetPixel(i, j, kDis(kRng));
        }
        for (size_t i = 0; i < 8; ++i) {
          for (size_t j = 0; j < 8; ++j)
            f->V.at(r).at(c).SetPixel(i, j, kDis(kRng));
        }
      }
    }
    yuv.WriteFrame(f);
  }
  std::cout << "[Test] YUV test completed." << std::endl;
}

}

#endif  // YUV_TEST_H_
