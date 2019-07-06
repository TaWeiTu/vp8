#include "bool_encoder.h"

namespace vp8 {

BoolEncoder::~BoolEncoder() {
  int8_t c = int8_t(bit_count_);
  uint32_t v = bottom_;

  if (v & (1u << (32 - c))) AddOne();
  v <<= c & 7;
  c >>= 3;
  while (--c >= 0) v <<= 8;
  c = 4;
  while (--c >= 0) {
    WriteByte(uint8_t(bottom_ >> 24));
    v <<= 8;
  }
}

void BoolEncoder::Bool(uint8_t val, uint8_t prob) {
  uint32_t split = 1 + (((range_ - 1) * prob) >> 8);
  if (val) {
    bottom_ += split;
    range_ -= split;
  } else {
    range_ = split;
  }

  while (range_ < 128) {
    range_ <<= 1;
    if (bottom_ & (1u << 31)) AddOne();
    bottom_ <<= 1;
    if (--bit_count_ == 0) {
      WriteByte(uint8_t(bottom_ >> 24));
      bottom_ &= (1 << 24) - 1;
      bit_count_ = 8;
    }
  }
}

void BoolEncoder::Lit(uint16_t val, uint8_t n) {
  for (int i = n - 1; i >= 0; --i) 
    Bool(val >> i & 1, 128);
}

void BoolEncoder::SignedLit(int16_t val, uint8_t n) {
  if (!n) return;

  uint8_t sgn = val >= 0 ? 0 : 1;
  Bool(sgn, 128);
  uint16_t absval = uint16_t(abs(val));

  for (int i = n - 2; i >= 0; --i) 
    Bool(absval >> i & 1, 128);
}

void BoolEncoder::Prob7(uint8_t prob) {
  if (prob == 1) Lit(0, 7);
  else Lit(prob >> 1, 7);
}

}  // namespace vp8
