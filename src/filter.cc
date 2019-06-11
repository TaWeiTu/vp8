#include "filter.h"

namespace vp8 {
namespace internal {

int16_t minus128(int16_t x) { return int16_t(Clamp128(x - int16_t(128))); }

int16_t plus128(int16_t x) { return int16_t(Clamp255(x + int16_t(128))); }

LoopFilter::LoopFilter(int16_t p3, int16_t p2, int16_t p1, int16_t p0,
                       int16_t q0, int16_t q1, int16_t q2, int16_t q3)
    : p3_(p3), p2_(p2), p1_(p1), p0_(p0), q0_(q0), q1_(q1), q2_(q2), q3_(q3) {}

bool LoopFilter::IsFilterNormal(int16_t interior, int16_t edge) const {
  return ((abs(p0_ - q0_) << 1) + (abs(p1_ - q1_) >> 1)) <= edge &&
         abs(p3_ - p2_) <= interior && abs(p2_ - p1_) <= interior &&
         abs(p1_ - p0_) <= interior && abs(q0_ - q1_) <= interior &&
         abs(q1_ - q2_) <= interior && abs(q2_ - q3_) <= interior;
}

bool LoopFilter::IsFilterSimple(int16_t edge) const {
  return (abs(p0_ - q0_) << 1) + (abs(p1_ - q1_) >> 1) <= edge;
}

bool LoopFilter::IsHighVariance(int16_t threshold) const {
  return abs(p1_ - p0_) > threshold || abs(q1_ - q0_) > threshold;
}

int16_t LoopFilter::Adjust(bool use_outer_taps) {
  int16_t a = int16_t(
      Clamp128((use_outer_taps ? Clamp128(p1_ - q1_) : 0) + 3 * (q0_ - p0_)));

  int16_t f1 = ((a + 4 > 127) ? 127 : a + 4) >> 3;
  int16_t f2 = ((a + 3 > 127) ? 127 : a + 3) >> 3;

  p0_ = Clamp255(int16_t(p0_ + f2));
  q0_ = Clamp255(int16_t(q0_ - f1));

  if (!use_outer_taps) {
    a = (f1 + 1) >> 1;
    p1_ = Clamp255(int16_t(p1_ + a));
    q1_ = Clamp255(int16_t(q1_ - a));
  }

  return a;
}

void LoopFilter::SubBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                int16_t edge_limit) {
  if (!IsFilterNormal(interior_limit, edge_limit)) return;
  bool hv = IsHighVariance(hev_threshold);
  Adjust(hv);
}

void LoopFilter::MacroBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                                  int16_t edge_limit) {
  if (!IsFilterNormal(interior_limit, edge_limit)) return;

  if (!IsHighVariance(hev_threshold)) {
    int16_t w = int16_t(Clamp128(Clamp128(p1_ - q1_) + 3 * (q0_ - p0_)));

    int16_t a = (int16_t(27) * w + int16_t(63)) >> 7;
    q0_ = Clamp255(int16_t(q0_ - a));
    p0_ = Clamp255(int16_t(p0_ + a));

    a = (int16_t(18) * w + int16_t(63)) >> 7;
    q1_ = Clamp255(int16_t(q1_ - a));
    p1_ = Clamp255(int16_t(p1_ + a));

    a = (int16_t(9) * w + int16_t(63)) >> 7;
    q2_ = Clamp255(int16_t(q2_ - a));
    p2_ = Clamp255(int16_t(p2_ + a));
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

void LoopFilter::SimpleFilter(int16_t limit) {
  if (IsFilterSimple(limit)) Adjust(true);
}

template <size_t C>
void PlaneFilterNormal(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &skip_lf,
                       Plane<C> &frame) {
  uint8_t sharpness_level = header.sharpness_level;
  if (header.loop_filter_level == 0) return;
  LoopFilter filter;

  for (size_t r = 0; r < vblock; r++) {
    for (size_t c = 0; c < hblock; c++) {
      MacroBlock<C> &mb = frame.at(r).at(c);
      uint8_t loop_filter_level = lf.at(r).at(c);

      if (loop_filter_level == 0) continue;

      uint8_t interior_limit = loop_filter_level;
      if (sharpness_level) {
        interior_limit >>= ((sharpness_level > 4) ? 2 : 1);
        if (interior_limit > 9 - sharpness_level)
          interior_limit = 9 - sharpness_level;
      }
      if (interior_limit < 1) interior_limit = 1;

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
        else if (loop_filter_level >= 15)
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

      if (!skip_lf.at(r).at(c)) {
        for (size_t i = 1; i < C; i++) {
          for (size_t j = 0; j < C; j++) {
            SubBlock &rsb = mb.at(j).at(i);
            SubBlock &lsb = mb.at(j).at(i - 1);

            for (size_t a = 0; a < 4; a++) {
              filter.Horizontal(lsb, rsb, a);
              filter.SubBlockFilter(hev_threshold, interior_limit,
                                    edge_limit_sb);
              filter.FillHorizontal(lsb, rsb, a);
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

      if (!skip_lf.at(r).at(c)) {
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

void PlaneFilterSimple(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &skip_lf,
                       Plane<4> &frame) {
  uint8_t sharpness_level = header.sharpness_level;
  if (header.loop_filter_level == 0) return;
  LoopFilter filter;

  for (size_t r = 0; r < vblock; r++) {
    for (size_t c = 0; c < hblock; c++) {
      MacroBlock<4> &mb = frame.at(r).at(c);
      uint8_t loop_filter_level = lf.at(r).at(c);

      if (loop_filter_level == 0) continue;
      uint8_t interior_limit = loop_filter_level;
      if (sharpness_level) {
        interior_limit >>= ((sharpness_level > 4) ? 2 : 1);
        if (interior_limit > 9 - sharpness_level)
          interior_limit = 9 - sharpness_level;
      }
      if (interior_limit < 1) interior_limit = 1;

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
        else if (loop_filter_level >= 15)
          hev_threshold = 1;
      }

      int16_t edge_limit_mb =
          (int16_t(loop_filter_level + 2) * 2) + int16_t(interior_limit);
      int16_t edge_limit_sb =
          (int16_t(loop_filter_level) * 2) + int16_t(interior_limit);

      if (c > 0) {
        MacroBlock<4> &lmb = frame.at(r).at(c - 1);
        for (size_t i = 0; i < 4; i++) {
          SubBlock &rsb = mb.at(i).at(0);
          SubBlock &lsb = lmb.at(i).at(3);

          for (size_t j = 0; j < 4; j++) {
            filter.Horizontal(lsb, rsb, j);
            filter.SimpleFilter(edge_limit_mb);
            filter.FillHorizontal(lsb, rsb, j);
          }
        }
      }

      if (!skip_lf.at(r).at(c)) {
        for (size_t i = 1; i < 4; i++) {
          for (size_t j = 0; j < 4; j++) {
            SubBlock &rsb = mb.at(j).at(i);
            SubBlock &lsb = mb.at(j).at(i - 1);

            for (size_t a = 0; a < 4; a++) {
              filter.Horizontal(lsb, rsb, a);
              filter.SimpleFilter(edge_limit_sb);
              filter.FillHorizontal(lsb, rsb, a);
            }
          }
        }
      }

      if (r > 0) {
        MacroBlock<4> &umb = frame.at(r - 1).at(c);
        for (size_t i = 0; i < 4; i++) {
          SubBlock &dsb = mb.at(0).at(i);
          SubBlock &usb = umb.at(3).at(i);

          for (size_t j = 0; j < 4; j++) {
            filter.Vertical(usb, dsb, j);
            filter.SimpleFilter(edge_limit_mb);
            filter.FillVertical(usb, dsb, j);
          }
        }
      }

      if (!skip_lf.at(r).at(c)) {
        for (size_t i = 1; i < 4; i++) {
          for (size_t j = 0; j < 4; j++) {
            SubBlock &dsb = mb.at(i).at(j);
            SubBlock &usb = mb.at(i - 1).at(j);

            for (size_t a = 0; a < 4; a++) {
              filter.Vertical(usb, dsb, a);
              filter.SimpleFilter(edge_limit_sb);
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
                 const std::vector<std::vector<uint8_t>> &skip_lf,
                 Frame &frame) {
  size_t hblock = frame.hblock, vblock = frame.vblock;
  if (!header.filter_type) {
    PlaneFilterNormal(header, hblock, vblock, is_key_frame, lf,
                      skip_lf, frame.Y);
    PlaneFilterNormal(header, hblock, vblock, is_key_frame, lf,
                      skip_lf, frame.U);
    PlaneFilterNormal(header, hblock, vblock, is_key_frame, lf,
                      skip_lf, frame.V);
  } else {
    PlaneFilterSimple(header, hblock, vblock, is_key_frame, lf,
                      skip_lf, frame.Y);
  }
}

}  // namespace vp8
