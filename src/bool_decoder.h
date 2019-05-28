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
  std::unique_ptr<std::ifstream> fs_;

  uint8_t ReadByte();
};

}  // namespace vp8

#endif  // BOOL_DECODER_H_
