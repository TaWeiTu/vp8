#include "reconstruct.h"

namespace vp8 {
namespace internal {

void UpdateNonzero(const ResidualValue &rv, bool has_y2, size_t r, size_t c,
                   std::vector<bool> &y2_row, std::vector<bool> &y2_col,
                   std::vector<std::vector<bool>> &y1_nonzero,
                   std::vector<std::vector<bool>> &u_nonzero,
                   std::vector<std::vector<bool>> &v_nonzero) {
  if (has_y2) {
    bool nonzero = false;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        nonzero |= rv.y2.at(i).at(j) != 0;
    }
    y2_row.at(r) = nonzero;
    y2_col.at(c) = nonzero;
  }
  for (size_t p = 0; p < 16; ++p) {
    bool nonzero = false;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        nonzero |= rv.y.at(p).at(i).at(j) != 0;
    }
    y1_nonzero.at(r << 2 | (p >> 2)).at(c << 2 | (p & 3)) = nonzero;
  }
  for (size_t p = 0; p < 4; ++p) {
    bool nonzero = false;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        nonzero |= rv.u.at(p).at(i).at(j) != 0;
    }
    u_nonzero.at(r << 1 | (p >> 1)).at(c << 1 | (p & 1)) = nonzero;
  }
  for (size_t p = 0; p < 4; ++p) {
    bool nonzero = false;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        nonzero |= rv.v.at(p).at(i).at(j) != 0;
    }
    v_nonzero.at(r << 1 | (p >> 1)).at(c << 1 | (p & 1)) = nonzero;
  }
}

void Predict(const FrameHeader &header, const FrameTag &tag,
             const std::array<Frame, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             std::vector<std::vector<uint8_t>> &seg_id, BitstreamParser &ps,
             Frame &frame) {
  std::vector<bool> y2_row(frame.vblock, false);
  std::vector<bool> y2_col(frame.hblock, false);
  std::vector<std::vector<bool>> y1_nonzero(
      frame.vblock << 2, std::vector<bool>(frame.hblock << 2, false));
  std::vector<std::vector<bool>> u_nonzero(
      frame.vblock << 1, std::vector<bool>(frame.hblock << 1, false));
  std::vector<std::vector<bool>> v_nonzero(
      frame.vblock << 1, std::vector<bool>(frame.hblock << 1, false));

#ifdef DEBUG
  std::cerr << "Start prediction" << std::endl;
#endif

  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      int16_t qp = header.quant_indices.y_ac_qi;
      if (header.segmentation_enabled)
        qp = header.segment_feature_mode == SEGMENT_MODE_ABSOLUTE
                 ? header.quantizer_segment[pre.segment_id]
                 : header.quantizer_segment[pre.segment_id] + qp;

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
      ResidualData rd = ps.ReadResidualData(ResidualParam(
          y2_nonzero, y1_above, y1_left, u_above, u_left, v_above, v_left));

      ResidualValue rv = DequantizeResidualData(rd, qp, header.quant_indices);
      UpdateNonzero(rv, rd.has_y2, r, c, y2_row, y2_col, y1_nonzero, u_nonzero,
                    v_nonzero);
      InverseTransformResidual(rv, rd.has_y2);
      
      if (pre.is_inter_mb)
        InterPredict(tag, r, c, refs, ref_frame_bias, pre.ref_frame, interc, ps,
                     frame);
      else
        IntraPredict(tag, r, c, rv, intrac, ps, frame);
    }
  }
#ifdef DEBUG
  std::cerr << "Done prediction" << std::endl;
#endif
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
  std::vector<std::vector<uint8_t>> seg_id(frame.vblock,
                                           std::vector<uint8_t>(frame.hblock));
  Predict(header, tag, refs, ref_frame_bias, interc, intrac, seg_id, ps, frame);
#ifdef DEBUG
  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j)
      std::cerr << frame.Y.at(0).at(0).GetPixel(i, j) << ' ';
    std::cerr << std::endl;
  }
#endif
#ifdef DEBUG
  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j)
      std::cerr << frame.Y.at(0).at(0).GetPixel(i, j) << ' ';
    std::cerr << std::endl;
  }
#endif
  FrameFilter(header, tag.key_frame, frame);
}

}  // namespace vp8
