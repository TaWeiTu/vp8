#include "dct_test.h"

#include <iostream>

int main() {
  std::cout << "[Info] Start testing." << std::endl;
  vp8_test::TestDct();
  std::cout << "[Info] Tests completed." << std::endl;
}
