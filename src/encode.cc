#include <fstream>

#include "utils.h"

int main(int argc, const char **argv) {
  ensure(argc == 3, "[Usage] ./encode [input] [output]");

  auto fs = std::ofstream(argv[1], std::ios::binary);
  auto write_bytes = [&fs](uint32_t byte, size_t n) {
    const uint32_t mask = (1 << 8) - 1;
    for (size_t i = 0; i < n; ++i) {
      fs.pus(byte & mask);
      byte >>= 8;
    }
  };

  const uint32_t dkif = uint32_t('D') | (uint32_t('K') << 8) |
                        (uint32_t('I') << 16) | (uint32_t('F') << 24);
  const uint32_t vp80 = uint32_t('V') | (uint32_t('P') << 8) |
                        (uint32_t('8') << 16) | (uint32_t('0') << 24);

  write_bytes(dkif, 4);
  write_bytes(0, 2);
  write_bytes(32, 2);
  write_bytes(vp80, 4);

  return 0;
}
