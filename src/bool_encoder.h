#ifndef BOOL_ENCODER_H_
#define BOOL_ENCODER_H_

namespace vp8 {

class BoolEncoder {
 public:
  BoolEncoder() = default;
  ~BoolEncoder();

  void Bool(uint8_t prob, uint8_t val);

 private:
  uint32_t range_;
  uint32_t bottom_;
  uint8_t bit_count_;
};

}  // namespace vp8

#endif  // BOOL_ENCODER_H_
