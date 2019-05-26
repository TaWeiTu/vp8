#include "bool_decoder.h"

Decoder::Decoder(const std::string &filename) {
  fs_.open(filename, std::ios::binary);
  value_ = (uint32_t)ReadByte_() << 8 | ReadByte_();
  range_ = 255;
  bit_count_ = 0;
}

uint8_t Decoder::ReadByte_() {
  uint8_t res;
  fs_ >> res;
  return res;
}

uint8_t Decoder::Bool(uint8_t prob) {
  uint32_t split = 1 + (((range_ - 1) * prob) >> 8);
  uint8_t res = 0;
  if (value_ >= (split << 8)) {
    res = 1;
    range_ -= split;
    value_ -= (split << 8);
  } else {
    res = 0;
    range_ = split;
  }

  while (range_ < 128) {
    value_ <<= 1;
    range_ <<= 1;
    if (++bit_count_ == 8) {
      bit_count_ = 0;
      value_ |= ReadByte_();
    }
  }
  return res;
}

uint16_t Decoder::Lit(size_t n) {
  uint16_t res = 0;
  for (size_t i = 0; i < n; ++i) res = (res << 1) | Bool(128);
  return res;
}

int16_t Decoder::SignedLit(size_t n) {
  if (!n) return 0;

  int16_t res = 0;
  uint8_t sign = Bool(128);
  for (size_t i = 0; i + 1 < n; ++i) res = (res << 1) | Bool(128);

  if (sign) res = -res;
  return res;
}

uint8_t Decoder::Prob8() { return static_cast<uint8_t>(Lit(8)); }

uint8_t Decoder::Prob7() {
  uint8_t res = static_cast<uint8_t>(Lit(7));
  return res ? res << 1 : 1;
}

int16_t Tree(const std::vector<uint8_t> &prob,
             const std::vector<int16_t> &tree) {
  int16_t res = 0;
  while (true) {
    res = tree[res + Bool(prob[res])];
    if (res < 0) break;
  }
  return res;
}
