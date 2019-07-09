#include "reconstruct.h"

namespace vp8 {
namespace internal {

void UpdateNonzero(const ResidualValue &rv, bool has_y2, size_t r, size_t c,
                   std::vector<uint8_t> &y2_row, std::vector<uint8_t> &y2_col,
                   std::vector<std::vector<uint8_t>> &y1_nonzero,
                   std::vector<std::vector<uint8_t>> &u_nonzero,
                   std::vector<std::vector<uint8_t>> &v_nonzero) {
  if (has_y2) {
    // If the current coefficients contain Y2 block, then update the most recent
    // Y2 status.
    uint8_t nonzero = 0;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) nonzero += rv.y2.at(i).at(j) != 0;
    }
    y2_row.at(r) = uint8_t(nonzero > 0);
    y2_col.at(c) = uint8_t(nonzero > 0);
  }
  for (size_t p = 0; p < 16; ++p) {
    uint8_t nonzero = 0;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) nonzero += rv.y.at(p).at(i).at(j) != 0;
    }
    y1_nonzero.at(r << 2 | (p >> 2)).at(c << 2 | (p & 3)) =
        uint8_t(nonzero > 0);
  }
  for (size_t p = 0; p < 4; ++p) {
    uint8_t nonzero = 0;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) nonzero += rv.u.at(p).at(i).at(j) != 0;
    }
    u_nonzero.at(r << 1 | (p >> 1)).at(c << 1 | (p & 1)) = uint8_t(nonzero > 0);
  }
  for (size_t p = 0; p < 4; ++p) {
    uint8_t nonzero = 0;
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) nonzero += rv.v.at(p).at(i).at(j) != 0;
    }
    v_nonzero.at(r << 1 | (p >> 1)).at(c << 1 | (p & 1)) = uint8_t(nonzero > 0);
  }
}

void UpdateDequantFactor(const QuantIndices &quant) {
  static bool first_entry = true;
  static QuantIndices config{};

  if (first_entry) {
    first_entry = false;
    BuildQuantFactorsY2(quant, y2dqf);
    BuildQuantFactorsY(quant, ydqf);
    BuildQuantFactorsUV(quant, uvdqf);
    config = quant;
    return;
  }

  if (quant.y2_dc_delta_q != config.y2_dc_delta_q ||
      quant.y2_ac_delta_q != config.y2_ac_delta_q)
    BuildQuantFactorsY2(quant, y2dqf);

  if (quant.y_dc_delta_q != config.y_dc_delta_q)
    BuildQuantFactorsY(quant, ydqf);

  if (quant.uv_dc_delta_q != config.uv_dc_delta_q ||
      quant.uv_ac_delta_q != config.uv_ac_delta_q)
    BuildQuantFactorsUV(quant, uvdqf);

  config = quant;
}

void Predict(const FrameHeader &header, const FrameTag &tag,
             const std::array<std::shared_ptr<Frame>, 4> &refs,
             const std::array<bool, 4> &ref_frame_bias,
             std::vector<std::vector<InterContext>> &interc,
             std::vector<std::vector<IntraContext>> &intrac,
             std::vector<std::vector<uint8_t>> &lf,
             std::vector<std::vector<uint8_t>> &skip_lf, BitstreamParser &ps,
             const std::shared_ptr<Frame> &frame) {
  std::vector<uint8_t> y2_row(frame->vblock, 0);
  std::vector<uint8_t> y2_col(frame->hblock, 0);
  std::vector<std::vector<uint8_t>> y1_nonzero(
      frame->vblock << 2, std::vector<uint8_t>(frame->hblock << 2, 0));
  std::vector<std::vector<uint8_t>> u_nonzero(
      frame->vblock << 1, std::vector<uint8_t>(frame->hblock << 1, 0));
  std::vector<std::vector<uint8_t>> v_nonzero(
      frame->vblock << 1, std::vector<uint8_t>(frame->hblock << 1, 0));

  UpdateDequantFactor(header.quant_indices);

  for (size_t r = 0; r < frame->vblock; ++r) {
    for (size_t c = 0; c < frame->hblock; ++c) {
      MacroBlockPreHeader pre = ps.ReadMacroBlockPreHeader();
      int16_t qp = header.quant_indices.y_ac_qi;
      if (header.segmentation_enabled) {
        qp = header.segment_feature_mode == SEGMENT_MODE_ABSOLUTE
                 ? header.quantizer_segment.at(pre.segment_id)
                 : header.quantizer_segment.at(pre.segment_id) + qp;
      }

      size_t dq = size_t(std::clamp(qp, int16_t(0), int16_t(127)));

      uint8_t y2_nonzero = y2_row.at(r) + y2_col.at(c);
      uint8_t y1_above = 0, y1_left = 0;
      uint8_t u_above = 0, u_left = 0, v_above = 0, v_left = 0;

      for (size_t i = 0; i < 4; ++i) {
        if (r > 0)
          y1_above |= y1_nonzero.at((r - 1) << 2 | 3).at(c << 2 | i) << i;
        if (c > 0)
          y1_left |= y1_nonzero.at(r << 2 | i).at((c - 1) << 2 | 3) << i;
      }
      for (size_t i = 0; i < 2; ++i) {
        if (r > 0) {
          u_above |= u_nonzero.at((r - 1) << 1 | 1).at(c << 1 | i) << i;
          v_above |= v_nonzero.at((r - 1) << 1 | 1).at(c << 1 | i) << i;
        }
        if (c > 0) {
          u_left |= u_nonzero.at(r << 1 | i).at((c - 1) << 1 | 1) << i;
          v_left |= v_nonzero.at(r << 1 | i).at((c - 1) << 1 | 1) << i;
        }
      }

      IntraMBHeader mh;

      if (pre.is_inter_mb) {
        InterPredict(tag, r, c, refs, ref_frame_bias, pre.ref_frame, interc,
                     skip_lf, ps, frame);
      } else {
        mh = tag.key_frame ? ps.ReadIntraMBHeaderKF()
                           : ps.ReadIntraMBHeaderNonKF();
      }

      ResidualData rd = ps.ReadResidualData(ResidualParam(
          y2_nonzero, y1_above, y1_left, u_above, u_left, v_above, v_left));

      if (!pre.mb_skip_coeff && !rd.is_zero) skip_lf.at(r).at(c) = 0;
      lf.at(r).at(c) = rd.loop_filter_level;
      ResidualValue rv =
          DequantizeResidualData(rd, y2dqf.at(dq), ydqf.at(dq), uvdqf.at(dq));
      UpdateNonzero(rv, rd.has_y2, r, c, y2_row, y2_col, y1_nonzero, u_nonzero,
                    v_nonzero);
      InverseTransformResidual(rv, rd.has_y2);

      if (pre.is_inter_mb) {
        ApplyMBResidual(rv.y, frame->Y.at(r).at(c));
        ApplyMBResidual(rv.u, frame->U.at(r).at(c));
        ApplyMBResidual(rv.v, frame->V.at(r).at(c));
      } else {
        IntraPredict(tag, r, c, rv, mh, intrac, skip_lf, ps, frame);
      }
    }
  }
}

}  // namespace internal

using namespace internal;

void Reconstruct(const FrameHeader &header, const FrameTag &tag,
                 const std::array<std::shared_ptr<Frame>, 4> &refs,
                 const std::array<bool, 4> &ref_frame_bias, BitstreamParser &ps,
                 const std::shared_ptr<Frame> &frame) {
  std::vector<std::vector<InterContext>> interc(
      frame->vblock, std::vector<InterContext>(frame->hblock));
  std::vector<std::vector<IntraContext>> intrac(
      frame->vblock << 2, std::vector<IntraContext>(frame->hblock << 2));
  std::vector<std::vector<uint8_t>> lf(frame->vblock,
                                       std::vector<uint8_t>(frame->hblock));
  std::vector<std::vector<uint8_t>> skip_lf(
      frame->vblock, std::vector<uint8_t>(frame->hblock, 1));

  Predict(header, tag, refs, ref_frame_bias, interc, intrac, lf, skip_lf, ps,
          frame);
  FrameFilter(header, tag.key_frame, lf, skip_lf, frame);
}

}  // namespace vp8
