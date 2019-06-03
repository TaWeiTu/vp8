#include "dct.h"

namespace vp8 {

void DCT(std::array<std::array<int16_t, 4>, 4> &subblock) {
  for (size_t i = 0; i < 4; i++) {
    int a = (int(subblock.at(i).at(0)) + int(subblock.at(i).at(3))) << 3;
    int b = (int(subblock.at(i).at(1)) + int(subblock.at(i).at(2))) << 3;
    int c = (int(subblock.at(i).at(1)) - int(subblock.at(i).at(2))) << 3;
    int d = (int(subblock.at(i).at(0)) - int(subblock.at(i).at(3))) << 3;

    subblock.at(i).at(0) = int16_t(a + b);
    subblock.at(i).at(2) = int16_t(a - b);
    subblock.at(i).at(1) = int16_t((c * 2217 + d * 5352 + 14500) >> 12);
    subblock.at(i).at(3) = int16_t((d * 2217 - c * 5352 + 7500) >> 12);
  }

  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(0).at(i)) + int(subblock.at(3).at(i));
    int b = int(subblock.at(1).at(i)) + int(subblock.at(2).at(i));
    int c = int(subblock.at(1).at(i)) - int(subblock.at(2).at(i));
    int d = int(subblock.at(0).at(i)) - int(subblock.at(3).at(i));

    subblock.at(0).at(i) = int16_t((a + b + 7) >> 4);
    subblock.at(2).at(i) = int16_t((a - b + 7) >> 4);
    subblock.at(1).at(i) =
        int16_t(((c * 2217 + d * 5352 + 12000) >> 16) + (d != 0 ? 1 : 0));
    subblock.at(3).at(i) = int16_t((d * 2217 - c * 5352 + 51000) >> 16);
  }
}

void WHT(std::array<std::array<int16_t, 4>, 4> &subblock) {
  for (size_t i = 0; i < 4; i++) {
    int a = (int(subblock.at(i).at(0)) + int(subblock.at(i).at(2))) << 2;
    int d = (int(subblock.at(i).at(1)) + int(subblock.at(i).at(3))) << 2;
    int c = (int(subblock.at(i).at(1)) - int(subblock.at(i).at(3))) << 2;
    int b = (int(subblock.at(i).at(0)) - int(subblock.at(i).at(2))) << 2;
    subblock.at(i).at(0) = int16_t(a + d + (a != 0 ? 1 : 0));
    subblock.at(i).at(1) = int16_t(b + c);
    subblock.at(i).at(2) = int16_t(b - c);
    subblock.at(i).at(3) = int16_t(a - d);
  }

  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(0).at(i)) + int(subblock.at(2).at(i));
    int d = int(subblock.at(1).at(i)) + int(subblock.at(3).at(i));
    int c = int(subblock.at(1).at(i)) - int(subblock.at(3).at(i));
    int b = int(subblock.at(0).at(i)) - int(subblock.at(2).at(i));

    int a2 = a + d;
    int b2 = b + c;
    int c2 = b - c;
    int d2 = a - d;

    a2 += a2 < 0 ? 1 : 0;
    b2 += b2 < 0 ? 1 : 0;
    c2 += c2 < 0 ? 1 : 0;
    d2 += d2 < 0 ? 1 : 0;

    subblock.at(0).at(i) = int16_t((a2 + 3) >> 3);
    subblock.at(1).at(i) = int16_t((b2 + 3) >> 3);
    subblock.at(2).at(i) = int16_t((c2 + 3) >> 3);
    subblock.at(3).at(i) = int16_t((d2 + 3) >> 3);
  }
}


void IDCT(std::array<std::array<int16_t, 4>, 4> &subblock) {
  static const int cospi8_sqrt2_minus1 = 20091;
  static const int sinpi8_sqrt2 = 35468;
  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(0).at(i)) + int(subblock.at(2).at(i));
    int b = int(subblock.at(0).at(i)) - int(subblock.at(2).at(i));

    int tmp1 = (int(subblock.at(1).at(i)) * sinpi8_sqrt2) >> 16;
    int tmp2 = int(subblock.at(3).at(i)) +
               ((int(subblock.at(3).at(i)) * cospi8_sqrt2_minus1) >> 16);
    int c = tmp1 - tmp2;
    tmp1 = int(subblock.at(1).at(i)) +
           ((int(subblock.at(1).at(i)) * cospi8_sqrt2_minus1) >> 16);
    tmp2 = (int(subblock.at(3).at(i)) * sinpi8_sqrt2) >> 16;
    int d = tmp1 + tmp2;

    subblock.at(0).at(i) = int16_t(a + d);
    subblock.at(3).at(i) = int16_t(a - d);
    subblock.at(1).at(i) = int16_t(b + c);
    subblock.at(2).at(i) = int16_t(b - c);
  }

  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(i).at(0)) + int(subblock.at(i).at(2));
    int b = int(subblock.at(i).at(0)) - int(subblock.at(i).at(2));

    int tmp1 = (int(subblock.at(i).at(1)) * sinpi8_sqrt2) >> 16;
    int tmp2 = int(subblock.at(i).at(3)) +
               ((int(subblock.at(i).at(3)) * cospi8_sqrt2_minus1) >> 16);
    int c = tmp1 - tmp2;
    tmp1 = int(subblock.at(i).at(1)) +
           ((int(subblock.at(i).at(1)) * cospi8_sqrt2_minus1) >> 16);
    tmp2 = (int(subblock.at(i).at(3)) * sinpi8_sqrt2) >> 16;
    int d = tmp1 + tmp2;

    subblock.at(i).at(0) = int16_t((a + d + 4) >> 3);
    subblock.at(i).at(3) = int16_t((a - d + 4) >> 3);
    subblock.at(i).at(1) = int16_t((b + c + 4) >> 3);
    subblock.at(i).at(2) = int16_t((b - c + 4) >> 3);
  }
}

void IWHT(std::array<std::array<int16_t, 4>, 4> &subblock) {
  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(0).at(i)) + int(subblock.at(3).at(i));
    int b = int(subblock.at(1).at(i)) + int(subblock.at(2).at(i));
    int c = int(subblock.at(1).at(i)) - int(subblock.at(2).at(i));
    int d = int(subblock.at(0).at(i)) - int(subblock.at(3).at(i));

    subblock.at(0).at(i) = int16_t(a + b);
    subblock.at(1).at(i) = int16_t(c + d);
    subblock.at(2).at(i) = int16_t(a - b);
    subblock.at(3).at(i) = int16_t(d - c);
  }

  for (size_t i = 0; i < 4; i++) {
    int a = int(subblock.at(i).at(0)) + int(subblock.at(i).at(3));
    int b = int(subblock.at(i).at(1)) + int(subblock.at(i).at(2));
    int c = int(subblock.at(i).at(1)) - int(subblock.at(i).at(2));
    int d = int(subblock.at(i).at(0)) - int(subblock.at(i).at(3));

    subblock.at(i).at(0) = int16_t((a + b + 3) >> 3);
    subblock.at(i).at(1) = int16_t((c + d + 3) >> 3);
    subblock.at(i).at(2) = int16_t((a - b + 3) >> 3);
    subblock.at(i).at(3) = int16_t((d - c + 3) >> 3);
  }
}

}  // namespace vp8
