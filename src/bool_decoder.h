#ifndef BOOL_DECODER_H_
#define BOOL_DECODER_H_

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace vp8 {

class BoolDecoder {
 public:
  BoolDecoder() = default;
  explicit BoolDecoder(const std::string&);
  explicit BoolDecoder(std::unique_ptr<std::ifstream>);

  // Decode a 1-bit boolean value.
  uint8_t Bool(uint8_t);
  // Decode an unsigned n-bit literal.
  uint16_t Lit(size_t);
  // Decode a signed n-bit literal.
  int16_t SignedLit(size_t);
  // Decode a 8-bit probability (being an alias of Lit(8)).
  uint8_t Prob8();
  // Decode a 7-bit probability p and return p ? p << 1 : 1.
  uint8_t Prob7();
  // Decode tokens from the tree
  int16_t Tree(const std::vector<uint8_t>&, const std::vector<int16_t>&);

 private:
  uint32_t value_;
  uint32_t range_;
  uint8_t bit_count_;
  std::unique_ptr<std::ifstream> fs_;

  uint8_t ReadByte();
};

}  // namespace vp8

#endif  // BOOL_DECODER_H_
