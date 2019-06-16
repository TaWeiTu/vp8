#include "bool_decoder.h"

#include <utility>

namespace vp8 {

void BoolDecoder::Init() {
  uint8_t byte1 = sp_.ReadByte();
  uint8_t byte2 = sp_.ReadByte();
  value_ = uint32_t(byte1) << 8 | uint32_t(byte2);
  range_ = 255;
  bit_count_ = 0;
}

uint8_t BoolDecoder::Bool(uint8_t prob) {
  if (!has_init_) {
    Init();
    has_init_ = true;
  }
  uint32_t split = 1 + (((range_ - 1) * prob) >> 8);
  uint8_t res = 0;
  if (value_ >= (split << 8)) {
    res = 1;
    assert(split <= range_);
    assert((split << 8) <= value_);
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
      uint8_t byte = sp_.ReadByte();
      value_ |= byte;
    }
  }
  return res;
}

uint16_t BoolDecoder::Lit(size_t n) {
  uint16_t res = 0;
  for (size_t i = 0; i < n; ++i) res = uint16_t(res << 1) | Bool(128);
  return res;
}

int16_t BoolDecoder::SignedLit(size_t n) {
  if (!n) return 0;

  int16_t res = 0;
  uint8_t sign = Bool(128);
  for (size_t i = 0; i + 1 < n; ++i) res = int16_t(res << 1) | Bool(128);

  if (sign) res = -res;
  return res;
}

uint8_t BoolDecoder::Prob8() { return uint8_t(Lit(8)); }

uint8_t BoolDecoder::Prob7() {
  uint8_t res = uint8_t(Lit(7));
  return res ? uint8_t(res << 1) : 1;
}

}  // namespace vp8
