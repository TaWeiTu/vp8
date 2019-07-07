#include "dct_test.h"
#include "yuv_test.h"

#include <iostream>

int main() {
  std::cout << "[Info] Start unit testing." << std::endl;
  vp8_test::TestDct();
  vp8_test::TestWht();
  // vp8_test::TestYuv();
  std::cout << "[Info] All unit tests completed." << std::endl;
}
