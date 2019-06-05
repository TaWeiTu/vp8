#include "reconstruct.h"

namespace vp8 {
namespace internal {

void Predict(const FrameTag &tag, const std::array<Frame, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             BitstreamParser &ps, Frame &frame) {
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      if (pre.is_inter_mb)
        InterPredict(tag, r, c, refs, ref_frame_bias, pre.ref_frame, interc, ps,
                     frame);
      else
        IntraPredict(r, c, intrac, ps, frame);
    }
  }
}

void AddResidual(const FrameHeader &header,
                 const std::vector<std::vector<InterContext>> &interc,
                 const std::vector<std::vector<IntraContext>> &intrac,
                 BitstreamParser &ps, Frame &frame) {
  uint8_t qp = header.quant_indices.y_ac_qi;
  // if (header.segmentation_enabled) qp = 0;

  std::vector<bool> y2_row(frame.vblock, false);
  std::vector<bool> y2_col(frame.hblock, false);
  std::vector<std::vector<bool>> y1_nonzero(
      frame.vblock << 2, std::vector<bool>(frame.hblock << 2, false));
  std::vector<std::vector<bool>> u_nonzero(
      frame.vblock << 1, std::vector<bool>(frame.hblock << 1, false));
  std::vector<std::vector<bool>> v_nonzero(
      frame.vblock << 1, std::vector<bool>(frame.hblock << 1, false));

  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      uint8_t y2_nonzero = uint8_t(y2_row[r]) + uint8_t(y2_col[c]);
      std::array<bool, 4> y1_above, y1_left;
      for (size_t i = 0; i < 4; ++i) {
        if (r > 0) y1_above.at(i) = y1_nonzero.at((r - 1) << 2).at(c << 2 | i);
        if (c > 0) y1_left.at(i) = y1_nonzero.at(r << 2 | i).at(c << 2);
      }
      std::array<bool, 2> u_above, u_left, v_above, v_left;
      for (size_t i = 0; i < 2; ++i) {
        if (r > 0) {
          u_above.at(i) = u_nonzero.at((r - 1) << 1).at(c << 1 | i);
          v_above.at(i) = v_nonzero.at((r - 1) << 1).at(c << 1 | i);
        }
        if (c > 0) {
          u_above.at(i) = u_nonzero.at(r << 1 | i).at((c - 1) << 1);
          v_above.at(i) = v_nonzero.at(r << 1 | i).at((c - 1) << 1);
        }
      }

      // ResidualData rd = ps.ReadResidualData(ResidualParam(
      // y2_nonzero, y1_above, y1_left, u_above, u_left, v_above, v_left));
      ResidualData rd;

      bool has_y2 =
          (interc.at(r).at(c).is_inter_mb &&
           interc.at(r).at(c).mv_mode != MV_SPLIT) ||
          (intrac.at(r).at(c).is_intra_mb && !intrac.at(r).at(c).is_b_pred);

      std::array<std::array<int16_t, 4>, 4> y2;
      if (has_y2) {
        DequantizeY2(rd.dct_coeff.at(0), qp, header.quant_indices);
        bool nonzero = false;
        for (size_t i = 0; i < 4; ++i) {
          for (size_t j = 0; j < 4; ++j) {
            y2.at(i).at(j) = rd.dct_coeff.at(0).at(i << 2 | j);
            nonzero &= y2.at(i).at(j) != 0;
          }
        }
        y2_row.at(r) = nonzero;
        y2_col.at(c) = nonzero;
        IWHT(y2);
      }
      for (size_t p = 1; p <= 16; ++p) {
        DequantizeY(rd.dct_coeff.at(p), qp, header.quant_indices);
        std::array<std::array<int16_t, 4>, 4> y;
        bool nonzero = false;
        for (size_t i = 0; i < 4; ++i) {
          for (size_t j = 0; j < 4; ++j) {
            y.at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
            nonzero |= y.at(i).at(j) != 0;
          }
        }
        y1_nonzero.at(r << 2 | ((p - 1) >> 2)).at(c << 2 | ((p - 1) & 3)) =
            nonzero;
        IDCT(y);
        if (has_y2) y.at(0).at(0) = y2.at((p - 1) >> 2).at((p - 1) & 3);
        for (size_t i = 0; i < 4; ++i) {
          for (size_t j = 0; j < 4; ++j)
            frame.Y.at(r).at(c).SetPixel(r << 4 | ((p - 1) & 12) | i,
                                         c << 4 | ((p - 1) & 3) << 2 | j,
                                         y.at(i).at(j));
        }
      }
      for (size_t p = 17; p <= 24; ++p) {
        DequantizeUV(rd.dct_coeff.at(p), qp, header.quant_indices);
        std::array<std::array<int16_t, 4>, 4> uv;
        bool nonzero = false;
        for (size_t i = 0; i < 4; ++i) {
          for (size_t j = 0; j < 4; ++j) {
            uv.at(i).at(j) = rd.dct_coeff.at(p).at(i << 2 | j);
            nonzero |= uv.at(i).at(j) != 0;
          }
        }
        if (p <= 20) {
          u_nonzero.at(r << 1 | ((p - 17) >> 1)).at(c << 1 | ((p - 17) & 1)) =
              nonzero;
          for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j)
              frame.U.at(r).at(c).SetPixel(r << 2 | ((p - 17) & 2) | i,
                                           c << 2 | ((p - 17) & 1 << 1) | j,
                                           uv.at(i).at(j));
          }
        } else {
          v_nonzero.at(r << 1 | ((p - 21) >> 1)).at(c << 1 | ((p - 21) & 1)) =
              nonzero;
          for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j)
              frame.V.at(r).at(c).SetPixel(r << 2 | ((p - 21) & 2) | i,
                                           c << 2 | ((p - 21) & 1 << 1) | j,
                                           uv.at(i).at(j));
          }
        }
        IDCT(uv);
      }
    }
  }
}

}  // namespace internal

using namespace internal;

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<Frame, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 Frame &frame) {

  std::vector<std::vector<InterContext>> interc(
      frame.vblock, std::vector<InterContext>(frame.hblock));
  std::vector<std::vector<IntraContext>> intrac(
      frame.vblock << 2, std::vector<IntraContext>(frame.hblock << 2));
  Predict(tag, refs, ref_frame_bias, interc, intrac, ps, frame);
  AddResidual(header, interc, intrac, ps, frame);
  FrameFilter(header, tag.key_frame, frame);
}

}  // namespace vp8
