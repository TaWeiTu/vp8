#include "filter.h"
#include <vector>
#include "utils.h"

namespace vp8 {

bool LoopFilter::IsFilter(int16_t interior, int16_t edge) {
  return ((abs(p0 - q0) << 2) + (abs(p1 - q1) >> 2)) <= edge &&
         abs(p3 - p2) <= interior && abs(p2 - p1) <= interior &&
         abs(p1 - p0) <= interior && abs(p0 - q0) <= interior &&
         abs(q0 - q1) <= interior && abs(q1 - q2) <= interior &&
         abs(q2 - q3) <= interior;
}

bool LoopFilter::IsHighVariance(int16_t threshold) {
  return abs(p1 - p0) > threshold || abs(q1 - q0) > threshold;
}

int16_t LoopFilter::Adjust(bool use_outer_taps) {
  int16_t P1 = minus128(p1);
  int16_t P0 = minus128(p0);
  int16_t Q0 = minus128(q0);
  int16_t Q1 = minus128(q1);

  int16_t a = int16_t(
      Clamp128((use_outer_taps ? Clamp128(P1 - Q1) : 0) + 3 * (Q0 - P0)));
  int16_t b = int16_t(Clamp128(a + int16_t(3))) >> 3;
  a = int16_t(Clamp128(a + int16_t(4))) >> 3;

  q0 = plus128(Q0 - a);
  p0 = plus128(P0 + b);

  return a;
}

void LoopFilter::SubBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                int16_t edge_limit) {
  if (!IsFilter(interior_limit, edge_limit)) return;
  int16_t P1 = minus128(p1);
  int16_t Q1 = minus128(q1);

  if (IsHighVariance(hev_threshold)) {
    int16_t a = (Adjust(false) + 1) >> 1;
    q1 = plus128(Q1 - a);
    p1 = plus128(P1 + a);
  }
}

void LoopFilter::MacroBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                  int16_t edge_limit) {
  if (!IsFilter(interior_limit, edge_limit)) return;
  int16_t P2 = minus128(p2);
  int16_t P1 = minus128(p1);
  int16_t P0 = minus128(p0);
  int16_t Q0 = minus128(q0);
  int16_t Q1 = minus128(q1);
  int16_t Q2 = minus128(q2);

  if (!IsHighVariance(hev_threshold)) {
    int16_t w = int16_t(Clamp128(Clamp128(P1 - Q1) + 3 * (Q0 - P0)));

    int16_t a = (int16_t(27) * w + int16_t(63)) >> 7;
    q0 = plus128(Q0 - a);
    p0 = plus128(P0 + a);

    a = (int16_t(18) * w + int16_t(63)) >> 7;
    q1 = plus128(Q1 - a);
    p1 = plus128(P1 + a);

    a = (9 * w + 63) >> 7;
    q2 = plus128(Q2 - a);
    p2 = plus128(P2 + a);
  } else
    Adjust(true);
}

void LoopFilter::Horizontal(SubBlock &lsb, SubBlock &rsb, size_t idx) {
  this->p3 = lsb[idx][0];
  this->p2 = lsb[idx][1];
  this->p1 = lsb[idx][2];
  this->p0 = lsb[idx][3];
  this->q0 = rsb[idx][0];
  this->q1 = rsb[idx][1];
  this->q2 = rsb[idx][2];
  this->q3 = rsb[idx][3];
}

void LoopFilter::FillHorizontal(SubBlock &lsb, SubBlock &rsb, size_t idx) {
  lsb[idx][0] = this->p3;
  lsb[idx][1] = this->p2;
  lsb[idx][2] = this->p1;
  lsb[idx][3] = this->p0;
  rsb[idx][0] = this->q0;
  rsb[idx][1] = this->q1;
  rsb[idx][2] = this->q2;
  rsb[idx][3] = this->q3;
}

void LoopFilter::Vertical(SubBlock &usb, SubBlock &dsb, size_t idx) {
  this->p3 = usb[0][idx];
  this->p2 = usb[1][idx];
  this->p1 = usb[2][idx];
  this->p0 = usb[3][idx];
  this->q0 = dsb[0][idx];
  this->q1 = dsb[1][idx];
  this->q2 = dsb[2][idx];
  this->q3 = dsb[3][idx];
}

void LoopFilter::FillVertical(SubBlock &usb, SubBlock &dsb, size_t idx) {
  usb[0][idx] = this->p3;
  usb[1][idx] = this->p2;
  usb[2][idx] = this->p1;
  usb[3][idx] = this->p0;
  dsb[0][idx] = this->q0;
  dsb[1][idx] = this->q1;
  dsb[2][idx] = this->q2;
  dsb[3][idx] = this->q3;
}
template <size_t C>
void FrameFilter(FrameHeader &header, uint8_t sharpness_level,
                 std::vector<std::vector<MacroBlock<C>>> frame, size_t hblock,
                 size_t vblock, bool is_key_frame, LoopFilter &filter) {
  for (size_t r = 0; r < vblock; r++) {
    for (size_t c = 0; c < hblock; c++) {
      MacroBlock<4> &mb = frame[r][c];
      uint8_t loop_filter_level = header.loop_filter_level;
      bool filter_type = header.filter_type;

      if (loop_filter_level == 0) continue;
      uint8_t interior_limit = loop_filter_level;
      if (sharpness_level) {
        interior_limit >>= ((sharpness_level > 4) ? 2 : 1);
        if (interior_limit > 9 - sharpness_level)
          interior_limit = 9 - sharpness_level;
      }
      if (!interior_limit) interior_limit = 1;

      uint8_t hev_threshold = 0;
      if (is_key_frame) {
        if (loop_filter_level >= 40)
          hev_threshold = 2;
        else if (loop_filter_level >= 15)
          hev_threshold = 1;
      } else {
        if (loop_filter_level >= 40)
          hev_threshold = 3;
        else if (loop_filter_level >= 20)
          hev_threshold = 2;
        else
          hev_threshold = 1;
      }

      int16_t edge_limit_mb =
          (int16_t(loop_filter_level + 2) * 2) + int16_t(interior_limit);
      int16_t edge_limit_sb =
          (int16_t(loop_filter_level) * 2) + int16_t(interior_limit);

      if (c > 0) {
        MacroBlock<4> &lmb = frame[r][c - 1];
        for (size_t i = 0; i < C; i++) {
          SubBlock &rsb = mb[i][0];
          SubBlock &lsb = lmb[i][3];

          for (size_t j = 0; j < 4; j++) {
            filter.Horizontal(lsb, rsb, j);
            filter.MacroBlockFilter(hev_threshold, interior_limit,
                                    edge_limit_mb);
            filter.FillHorizontal(lsb, rsb, j);
          }
        }
      }

      if (filter_type) {
        for (size_t i = 0; i < C; i++) {
          for (size_t j = 1; j < C; j++) {
            SubBlock &rsb = mb[i][j];
            SubBlock &lsb = mb[i][j - 1];

            for (size_t a = 0; a < 4; a++) {
              filter.Horizontal(lsb, rsb, a);
              filter.SubBlockFilter(hev_threshold, interior_limit,
                                    edge_limit_sb);
              filter.FillVertical(lsb, rsb, a);
            }
          }
        }
      }

      if (r > 0) {
        MacroBlock<4> &umb = frame[r - 1][c];
        for (size_t i = 0; i < C; i++) {
          SubBlock &dsb = mb[0][i];
          SubBlock &usb = umb[3][i];

          for (size_t j = 0; j < 4; j++) {
            filter.Vertical(usb, dsb, j);
            filter.MacroBlockFilter(hev_threshold, interior_limit,
                                    edge_limit_mb);
            filter.FillVertical(usb, dsb, j);
          }
        }
      }

      if (filter_type) {
        for (size_t i = 1; i < C; i++) {
          for (size_t j = 0; j < C; j++) {
            SubBlock &dsb = mb[i][j];
            SubBlock &usb = mb[i - 1][j];

            for (size_t a = 0; a < 4; a++) {
              filter.Vertical(usb, dsb, a);
              filter.SubBlockFilter(hev_threshold, interior_limit,
                                    edge_limit_sb);
              filter.FillVertical(usb, dsb, a);
            }
          }
        }
      }
    }
  }
}
}  // namespace vp8
