#ifndef DCT_H_
#define DCT_H_

#include <array>
namespace Quantize {
void DCT(std::array<std::array<int16_t, 4>, 4> &);
void IDCT(std::array<std::array<int16_t, 4>, 4> &);
void WHT(std::array<std::array<int16_t, 4>, 4> &);
void IWHT(std::array<std::array<int16_t, 4>, 4> &);
}  // namespace Quantize
#endif  // DCT_H_
