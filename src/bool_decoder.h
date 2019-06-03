#ifndef BOOL_DECODER_H_
#define BOOL_DECODER_H_

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace vp8 {

class BoolDecoder {
 public:
  BoolDecoder() = default;
  explicit BoolDecoder(const std::string &);
  explicit BoolDecoder(std::unique_ptr<std::ifstream>);

  // Decode a 1-bit boolean value.
  uint8_t Bool(uint8_t prob);
  // Decode an unsigned n-bit literal.
  uint16_t Lit(size_t n);
  // Decode a signed n-bit literal.
  int16_t SignedLit(size_t n);
  // Decode a 8-bit probability (being an alias of Lit(8)).
  uint8_t Prob8();
  // Decode a 7-bit probability p and return p ? p << 1 : 1.
  uint8_t Prob7();
  // Decode tokens from the tree
  template <class P, class T>
  int16_t Tree(const P &prob, const T &tree) {
    int16_t res = 0;
    while (true) {
      res = tree.at(res + Bool(prob.at(res)));
      if (res <= 0) break;
    }
    return -res;
  }

  // Read an unsigned n-bit integer (uncoded) presented in little-endian format.
  uint32_t Raw(size_t n);

 private:
  uint32_t value_;
  uint32_t range_;
  uint8_t bit_count_;
  std::unique_ptr<std::ifstream> fs_;

  uint8_t ReadByte();
};

}  // namespace vp8

#endif  // BOOL_DECODER_H_
