#ifndef BOOL_DECODER_H_
#define BOOL_DECODER_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Decoder {
 public:
  Decoder() = default;
  Decoder(const std::string&);

  uint8_t Bool(uint8_t);
  uint16_t Lit(size_t n);
  int16_t SignedLit(size_t n);
  uint8_t Prob8();
  uint8_t Prob7();
  int16_t Tree(const std::vector<uint8_t>&, const std::vector<int16_t>&);

 private:
  uint32_t value_;
  uint32_t range_;
  uint8_t bit_count_;
  std::ifstream fs_;

  uint8_t ReadByte();
};

#endif  // BOOL_DECODER_H_
