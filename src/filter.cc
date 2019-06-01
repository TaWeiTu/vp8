#include "filter.h"
#include "utils.h"

namespace vp8 {
struct LoopFilter {
  int16_t p3, p2, p1, p0;
  int16_t q0, q1, q2, q3;

  bool IsFilter(int interior, int edge) {
    return ((abs(p0 - q0) << 2) + (abs(p1 - q1) >> 2)) <= edge &&
           abs(p3 - p2) <= interior && abs(p2 - p1) <= interior &&
           abs(p1 - p0) <= interior && abs(p0 - q0) <= interior &&
           abs(q0 - q1) <= interior && abs(q1 - q2) <= interior &&
           abs(q2 - q3) <= interior;
  }

  bool IsHighVariance(int16_t threshold) {
    return abs(p1 - p0) > threshold || abs(q1 - q0) > threshold;
  }

  int16_t Adjust(bool use_outer_taps) {
    int16_t p1 = minus128(this->p1);
    int16_t p0 = minus128(this->p0);
    int16_t q0 = minus128(this->q0);
    int16_t q1 = minus128(this->q1);

    int16_t a =
        Clamp128((use_outer_taps ? Clamp128(p1 - q1) : 0) + 3 * (q0 - p0));
    int16_t b = Clamp128(a + 3) >> 3;
    a = Clamp128(a + 4) >> 3;

    this->q0 = plus128(q0 - a);
    this->p0 = plus128(p0 + b);

    return a;
  }

  void SubBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                      int16_t edge_limit) {
    if (!IsFilter(interior_limit, edge_limit)) return;
    int16_t p3 = minus128(this->p3);
    int16_t p2 = minus128(this->p2);
    int16_t p1 = minus128(this->p1);
    int16_t p0 = minus128(this->p0);
    int16_t q0 = minus128(this->q0);
    int16_t q1 = minus128(this->q1);
    int16_t q2 = minus128(this->q2);
    int16_t q3 = minus128(this->q3);

    if (IsHighVariance(hev_threshold)) {
      int16_t a = (Adjust(false) + 1) >> 1;
      this->q1 = plus128(q1 - a);
      this->p1 = plus128(p1 + a);
    }
  }

  void MacroBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                        int16_t edge_limit) {
    if (!IsFilter(interior, edge_limit)) return;
    int16_t p3 = minus128(this->p3);
    int16_t p2 = minus128(this->p2);
    int16_t p1 = minus128(this->p1);
    int16_t p0 = minus128(this->p0);
    int16_t q0 = minus128(this->q0);
    int16_t q1 = minus128(this->q1);
    int16_t q2 = minus128(this->q2);
    int16_t q3 = minus128(this->q3);

    if (!IsHighVariance(hev_threshold)) {
      int16_t w = Clamp128(Clamp128(p1 - q1) + 3 * (q0 - p0));

      int16_t a = (27 * w + 63) >> 7;
      this->q0 = plus128(q0 - a);
      this->p0 = plus128(p0 + a);

      a = (18 * w + 63) >> 7;
      this->q1 = plus128(q1 - a);
      this->p1 = plus128(p1 + a);

      a = (9 * w + 63) >> 7;
      this->q2 = plus128(q2 - a);
      this->p2 = plus128(p2 + a);
    } else
      Adjust(true);
  }
};
}  // namespace vp8
