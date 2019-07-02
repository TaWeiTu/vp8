#include "bool_encoder.h"

BoolEncoder::~BoolEncoder() {
  int8_t c = bit_count_;
  uint32_t v = bottom_;

  if (v & (1 << (32 - c))) {
    // TODO: add one to output
  }
  v <<= c & 7;
  c >>= 3;
  while (--c >= 0) v <<= 8;
  c = 4;
  while (--c >= 0) {
    // TODO: write byte to output
    v <<= 8;
  }
}

void BoolEncoder::Bool(uint8_t prob, uint8_t val) {
  uint32_t split = 1 + (((range_ - 1) * prob) >> 8);
  if (val) {
    bottom_ += split;
    range_ -= split;
  } else {
    range_ = split;
  }

  while (range_ < 128) {
    range_ <<= 1;
    if (bottom_ & (1 << 31)) {
      // TODO: add one to output
    }
    bottom_ <<= 1;
    if (--bit_count_ == 0) {
      // TODO: write byte to output
      bottom_ &= (1 << 24) - 1;
      bit_count_ = 8;
    }
  }
}
