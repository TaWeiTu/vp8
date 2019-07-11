#ifndef DCT_TEST_H_
#define DCT_TEST_H_

#include "../src/dct.h"

#include <array>
#include <cassert>
#include <iostream>
#include <random>

namespace vp8_test {

void TestDct();
void TestWht();

void TestDct() {
  static const size_t kTest = 100;
  static const int16_t kDiff = 2;
  static std::mt19937 kRng(7122);
  static std::uniform_int_distribution<int16_t> kDis(0, 255);

  std::cout << "[Test] DCT test started." << std::endl;
  for (size_t t = 0; t < kTest; ++t) {
    std::array<std::array<int16_t, 4>, 4> input;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) input[i][j] = kDis(kRng);
    }
    std::array<std::array<int16_t, 4>, 4> clone = input;
    vp8::DCT(input);
    vp8::IDCT(input);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        assert(abs(clone[i][j] - input[i][j]) <= kDiff);
    }
  }
  for (size_t t = 0; t < kTest; ++t) {
    std::array<std::array<int16_t, 4>, 4> input{};
    input[0][0] = kDis(kRng);
    std::array<std::array<int16_t, 4>, 4> clone = input;
    vp8::IDCT(input);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) {
        assert(input[i][j] == (clone[0][0] + 4) >> 3);
      }
    }
  }
  std::cout << "[Test] DCT test completed." << std::endl;
}

void TestWht() {
  static const size_t kTest = 100;
  static const int16_t kDiff = 2;
  static std::mt19937 kRng(1234);
  static std::uniform_int_distribution<int16_t> kDis(0, 255);

  std::cout << "[Test] WHT test started." << std::endl;
  for (size_t t = 0; t < kTest; ++t) {
    std::array<std::array<int16_t, 4>, 4> input;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) input[i][j] = kDis(kRng);
    }
    std::array<std::array<int16_t, 4>, 4> clone = input;
    vp8::WHT(input);
    vp8::IWHT(input);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        assert(abs(clone[i][j] - input[i][j]) <= kDiff);
    }
  }
  std::cout << "[Test] WHT test completed." << std::endl;
}

}

#endif  // DCT_TEST_H_
