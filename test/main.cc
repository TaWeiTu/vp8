#include "dct_test.h"

#include <iostream>

int main() {
  std::cout << "[Info] Start testing." << std::endl;
  vp8_test::TestDct();
  vp8_test::TestWht();
  std::cout << "[Info] Tests completed." << std::endl;
}
