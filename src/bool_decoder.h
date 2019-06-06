#ifndef BOOL_DECODER_H_
#define BOOL_DECODER_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "utils.h"

namespace vp8 {

class BoolDecoder {
 public:
  BoolDecoder() = default;
  explicit BoolDecoder(std::unique_ptr<std::istream>);

  // Decode a 1-bit boolean value.
  uint8_t Bool(uint8_t prob);
  // Decode an unsigned n-bit literal.
  uint16_t Lit(size_t n);
  uint8_t LitU8(size_t n) { return uint8_t(Lit(n)); }
  // Decode a signed n-bit literal.
  int16_t SignedLit(size_t n);
  // Decode a 8-bit probability (being an alias of Lit(8)).
  uint8_t Prob8();
  // Decode a 7-bit probability p and return p ? p << 1 : 1.
  uint8_t Prob7();
  // Decode tokens from the tree
  template <class P, class T>
  uint16_t Tree(const P &prob, const T &tree) {
#ifdef DEBUG
      std::cerr << "prob.size() = " << prob.size() << std::endl;
      std::cerr << "tree.size() = " << tree.size() << std::endl;
#endif
    int16_t res = 0;
    while (true) {
#ifdef DEBUG
      std::cerr << "before res = " << res << std::endl;
#endif
      res = tree.at(size_t(res + Bool(prob.at(size_t(res >> 1)))));
#ifdef DEBUG
      std::cerr << "after res = " << res << std::endl;
#endif
      if (res <= 0) break;
    }
#ifdef DEBUG
      std::cerr << "return res = " << res << std::endl;
#endif
    return uint16_t(-res);
  }

  void Init();

  // Read an unsigned n-bit integer (uncoded) presented in little-endian format.
  uint32_t Raw(size_t n);

 private:
  uint32_t value_;
  uint32_t range_;
  uint8_t bit_count_;
  std::unique_ptr<std::istream> fs_;

  uint8_t ReadByte();
};

}  // namespace vp8

#endif  // BOOL_DECODER_H_
