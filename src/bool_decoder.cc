#include "bool_decoder.h"

namespace vp8 {

BoolDecoder::BoolDecoder(const std::string &filename) {
  fs_ = std::make_unique<std::ifstream>();
  fs_->open(filename, std::ios::binary);
  value_ = uint32_t(ReadByte()) << 8 | ReadByte();
  range_ = 255;
  bit_count_ = 0;
}

BoolDecoder::BoolDecoder(std::unique_ptr<std::ifstream> fs)
    : fs_(std::move(fs)) {
  value_ = uint32_t(ReadByte()) << 8 | ReadByte();
  range_ = 255;
  bit_count_ = 0;
}

uint8_t BoolDecoder::ReadByte() {
  uint8_t res;
  (*fs_) >> res;
  return res;
}

uint8_t BoolDecoder::Bool(uint8_t prob) {
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
      value_ |= ReadByte();
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

int16_t BoolDecoder::Tree(const std::vector<uint8_t> &prob,
                          const std::vector<int16_t> &tree) {
  int16_t res = 0;
  while (true) {
    res = tree[size_t(res + Bool(prob[size_t(res)]))];
    if (res < 0) break;
  }
  return res;
}

uint32_t BoolDecoder::ReadUncoded(size_t n) {
  uint32_t res = 0;
  for (size_t i = 0; i < n; ++i) res |= ReadByte() << (i << 3);
  return res;
}

}  // namespace vp8
