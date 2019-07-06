#ifndef BOOL_ENCODER_H_
#define BOOL_ENCODER_H_

#include <algorithm>
#include <vector>

#include "utils.h"

namespace vp8 {

class BoolEncoder {
 public:
  BoolEncoder() = default;
  // TODO: Non-trivial constructor.
  ~BoolEncoder();

  // Encode a 1-bit boolean value.
  void Bool(uint8_t val, uint8_t prob);
  // Encode an unsigned n-bit literal.
  void Lit(uint16_t val, uint8_t n);
  void LitU8(uint8_t val, uint8_t n) { Lit(uint16_t(val), n); }
  // Encode a signed n-bit literal.
  void SignedLit(int16_t val, uint8_t n);
  // Encode a 8-bit probability (being an alias of Lit(8)).
  void Prob8(uint8_t prob) { LitU8(prob, 8); }
  // Encode a 7-bit probability.
  void Prob7(uint8_t prob);

  // Encode tokens from the tree.
  template <class ProbType, class TreeType>
  void Tree(uint16_t val, const ProbType &prob, const TreeType &tree) {
    auto it = std::find(tree.begin(), tree.end(), val);
    ensure(it != tree.end(), "[Error] BoolEncoder::Tree(): Token not found.");
    uint8_t pos = it - tree.begin();

    std::vector<uint8_t> buf;

    if (pos == 0) {
      Bool(0, prob.at(0));
      return;
    }

    while (true) {
      uint8_t res = pos >> 1;
      assert(pos - tree.at(res) == 0 || pos - tree.at(res) == 1);
      buf.push_back(pos - tree.at(res));
      pos = res;
      if (pos == 0) break;
    }

    int16_t res = 0;
    while (true) {
      uint8_t p = prob.at(res >> 1);
      ensure(!buf.empty(), "[Error] BoolEncoder::Tree(), Buffer is empty.");
      uint8_t diff = buf.back();
      buf.pop_back();
      Bool(diff, p);
      res = tree.at(res + diff);
      if (res <= 0) break;
    }

    ensure(buf.empty(),
           "[Error] BoolEncoder::Tree(), Buffer is not empty after encoding.");
  }

 private:
  uint32_t range_;
  uint32_t bottom_;
  uint8_t bit_count_;

  // TODO:
  void AddOne();
  void WriteByte(uint8_t byte);
};

}  // namespace vp8

#endif  // BOOL_ENCODER_H_
