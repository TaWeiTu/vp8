#include "filter.h"

namespace vp8 {
namespace internal {

int16_t minus128(int16_t x) { return int16_t(Clamp128(x - int16_t(128))); }

int16_t plus128(int16_t x) { return int16_t(Clamp255(x + int16_t(128))); }

LoopFilter::LoopFilter(int16_t p3, int16_t p2, int16_t p1, int16_t p0,
                       int16_t q0, int16_t q1, int16_t q2, int16_t q3)
    : p3_(p3), p2_(p2), p1_(p1), p0_(p0), q0_(q0), q1_(q1), q2_(q2), q3_(q3) {}

bool LoopFilter::IsFilter(int16_t interior, int16_t edge) const {
  return ((abs(p0_ - q0_) << 2) + (abs(p1_ - q1_) >> 2)) <= edge &&
         abs(p3_ - p2_) <= interior && abs(p2_ - p1_) <= interior &&
         abs(p1_ - p0_) <= interior && abs(p0_ - q0_) <= interior &&
         abs(q0_ - q1_) <= interior && abs(q1_ - q2_) <= interior &&
         abs(q2_ - q3_) <= interior;
}

bool LoopFilter::IsHighVariance(int16_t threshold) const {
  return abs(p1_ - p0_) > threshold || abs(q1_ - q0_) > threshold;
}

int16_t LoopFilter::Adjust(bool use_outer_taps) {
  int16_t P1 = minus128(p1_);
  int16_t P0 = minus128(p0_);
  int16_t Q0 = minus128(q0_);
  int16_t Q1 = minus128(q1_);

  int16_t a = int16_t(
      Clamp128((use_outer_taps ? Clamp128(P1 - Q1) : 0) + 3 * (Q0 - P0)));
  int16_t b = int16_t(Clamp128(a + int16_t(3))) >> 3;
  a = int16_t(Clamp128(a + int16_t(4))) >> 3;

  q0_ = plus128(Q0 - a);
  p0_ = plus128(P0 + b);

  return a;
}

void LoopFilter::SubBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                int16_t edge_limit) {
  if (!IsFilter(interior_limit, edge_limit)) return;
  int16_t P1 = minus128(p1_);
  int16_t Q1 = minus128(q1_);

  if (IsHighVariance(hev_threshold)) {
    int16_t a = (Adjust(false) + 1) >> 1;
    q1_ = plus128(Q1 - a);
    p1_ = plus128(P1 + a);
  }
}

void LoopFilter::MacroBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                  int16_t edge_limit) {
  if (!IsFilter(interior_limit, edge_limit)) return;
  int16_t P2 = minus128(p2_);
  int16_t P1 = minus128(p1_);
  int16_t P0 = minus128(p0_);
  int16_t Q0 = minus128(q0_);
  int16_t Q1 = minus128(q1_);
  int16_t Q2 = minus128(q2_);

  if (!IsHighVariance(hev_threshold)) {
    int16_t w = int16_t(Clamp128(Clamp128(P1 - Q1) + 3 * (Q0 - P0)));

    int16_t a = (int16_t(27) * w + int16_t(63)) >> 7;
    q0_ = plus128(Q0 - a);
    p0_ = plus128(P0 + a);

    a = (int16_t(18) * w + int16_t(63)) >> 7;
    q1_ = plus128(Q1 - a);
    p1_ = plus128(P1 + a);

    a = (9 * w + 63) >> 7;
    q2_ = plus128(Q2 - a);
    p2_ = plus128(P2 + a);
  } else {
    Adjust(true);
  }
}

void LoopFilter::Horizontal(const SubBlock &lsb, const SubBlock &rsb,
                            size_t idx) {
  p3_ = lsb.at(idx).at(0);
  p2_ = lsb.at(idx).at(1);
  p1_ = lsb.at(idx).at(2);
  p0_ = lsb.at(idx).at(3);
  q0_ = rsb.at(idx).at(0);
  q1_ = rsb.at(idx).at(1);
  q2_ = rsb.at(idx).at(2);
  q3_ = rsb.at(idx).at(3);
}

void LoopFilter::FillHorizontal(SubBlock &lsb, SubBlock &rsb,
                                size_t idx) const {
  lsb.at(idx).at(0) = p3_;
  lsb.at(idx).at(1) = p2_;
  lsb.at(idx).at(2) = p1_;
  lsb.at(idx).at(3) = p0_;
  rsb.at(idx).at(0) = q0_;
  rsb.at(idx).at(1) = q1_;
  rsb.at(idx).at(2) = q2_;
  rsb.at(idx).at(3) = q3_;
}

void LoopFilter::Vertical(const SubBlock &usb, const SubBlock &dsb,
                          size_t idx) {
  p3_ = usb.at(0).at(idx);
  p2_ = usb.at(1).at(idx);
  p1_ = usb.at(2).at(idx);
  p0_ = usb.at(3).at(idx);
  q0_ = dsb.at(0).at(idx);
  q1_ = dsb.at(1).at(idx);
  q2_ = dsb.at(2).at(idx);
  q3_ = dsb.at(3).at(idx);
}

void LoopFilter::FillVertical(SubBlock &usb, SubBlock &dsb, size_t idx) const {
  usb.at(0).at(idx) = p3_;
  usb.at(1).at(idx) = p2_;
  usb.at(2).at(idx) = p1_;
  usb.at(3).at(idx) = p0_;
  dsb.at(0).at(idx) = q0_;
  dsb.at(1).at(idx) = q1_;
  dsb.at(2).at(idx) = q2_;
  dsb.at(3).at(idx) = q3_;
}

template <size_t C>
void PlaneFilter(const FrameHeader &header, size_t hblock, size_t vblock,
                 bool is_key_frame,
                 const std::vector<std::vector<uint8_t>> &lf,
                 Plane<C> &frame) {
  uint8_t sharpness_level = header.sharpness_level;
  LoopFilter filter;
  for (size_t r = 0; r < vblock; r++) {
    for (size_t c = 0; c < hblock; c++) {
      MacroBlock<C> &mb = frame.at(r).at(c);
      uint8_t loop_filter_level = lf.at(r).at(c);
      // if (header.segmentation_enabled) {
        // uint8_t id = segment_id.at(r).at(c);
        // loop_filter_level =
            // header.segment_feature_mode == SEGMENT_MODE_ABSOLUTE
                // ? uint8_t(header.loop_filter_level_segment.at(id))
                // : uint8_t(header.loop_filter_level_segment.at(id) +
                          // loop_filter_level);
      // }
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
        MacroBlock<C> &lmb = frame.at(r).at(c - 1);
        for (size_t i = 0; i < C; i++) {
          SubBlock &rsb = mb.at(i).at(0);
          SubBlock &lsb = lmb.at(i).at(C - 1);

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
            SubBlock &rsb = mb.at(i).at(j);
            SubBlock &lsb = mb.at(i).at(j - 1);

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
        MacroBlock<C> &umb = frame.at(r - 1).at(c);
        for (size_t i = 0; i < C; i++) {
          SubBlock &dsb = mb.at(0).at(i);
          SubBlock &usb = umb.at(C - 1).at(i);

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
            SubBlock &dsb = mb.at(i).at(j);
            SubBlock &usb = mb.at(i - 1).at(j);

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

}  // namespace internal

using namespace internal;

void FrameFilter(const FrameHeader &header, bool is_key_frame,
                 const std::vector<std::vector<uint8_t>> &lf,
                 Frame &frame) {
  size_t hblock = frame.hblock, vblock = frame.vblock;
  PlaneFilter(header, hblock, vblock, is_key_frame, lf, frame.Y);
  PlaneFilter(header, hblock, vblock, is_key_frame, lf, frame.U);
  PlaneFilter(header, hblock, vblock, is_key_frame, lf, frame.V);
}

}  // namespace vp8
