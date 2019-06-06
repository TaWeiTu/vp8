#include "bitstream_parser.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "frame.h"
#include "utils.h"

namespace vp8 {

std::pair<ParserContext, std::unique_ptr<BoolDecoder>> BitstreamParser::DropStream() {
  // TODO: (Improvement) Check validity in other places?
  ensure(bool(bd_), "[Error] DropStream: bd_ already dropped.");
  return make_pair(context_, std::move(bd_));
}

std::pair<FrameTag, FrameHeader> BitstreamParser::ReadFrameTagHeader() {
  auto ft = ReadFrameTag();
  auto fh = ReadFrameHeader();
  return std::make_pair(ft, fh);
}

FrameTag BitstreamParser::ReadFrameTag() {
  uint32_t tag = bd_->Raw(3);
  frame_tag_.key_frame = !(tag & 0x1);
  frame_tag_.version = (tag >> 1) & 0x7;
  frame_tag_.show_frame = (tag >> 4) & 0x1;
  frame_tag_.first_part_size = (tag >> 5) & 0x7FFFF;
  ensure(!(frame_tag_.version >> 2),
         "[Error] ReadFrameTag: Experimental streams unsupported.");
  if (frame_tag_.key_frame) {
    uint32_t start_code = bd_->Raw(3);
    ensure(start_code == 0x2A019D,
           "[Error] ReadFrameTag: Incorrect start_code.");
    uint32_t horizontal_size_code = bd_->Raw(2);
    frame_tag_.width = horizontal_size_code & 0x3FFF;
    frame_tag_.horizontal_scale = uint16_t(horizontal_size_code >> 14);
    uint32_t vertical_size_code = bd_->Raw(2);
    frame_tag_.height = vertical_size_code & 0x3FFF;
    frame_tag_.vertical_scale = uint16_t(vertical_size_code >> 14);
		std::cerr << frame_tag_.width << ' ' << frame_tag_.height << std::endl;
  }
  bd_->Init();
  return frame_tag_;
}

FrameHeader BitstreamParser::ReadFrameHeader() {
  if (frame_tag_.key_frame) {
    context_ = ParserContext();
    frame_header_.color_space = bd_->LitU8(1);
    frame_header_.clamping_type = bd_->LitU8(1);
    ensure(!frame_header_.color_space && !frame_header_.clamping_type,
           "[Error] ReadFrameHeader: Unsupported color_space / clamping_type");
  }
  uint32_t macroblock_cnt = (uint32_t(frame_tag_.width + 15) / 16) *
                            (uint32_t(frame_tag_.height + 15) / 16);
  context_.mb_metadata.resize(macroblock_cnt);
  fill(context_.mb_metadata.begin(), context_.mb_metadata.end(), 0);
  frame_header_.segmentation_enabled = bd_->LitU8(1);
  if (frame_header_.segmentation_enabled) {
    UpdateSegmentation();
  }
  frame_header_.filter_type = bd_->LitU8(1);
  frame_header_.loop_filter_level = bd_->LitU8(6);
  frame_header_.sharpness_level = bd_->LitU8(3);
  MbLfAdjust();
  // TODO: (Improvement) Deal with multiple partitions
  frame_header_.nbr_of_dct_partitions = uint8_t(1 << bd_->LitU8(2));
  frame_header_.quant_indices = ReadQuantIndices();
  if (frame_tag_.key_frame) {
    frame_header_.refresh_entropy_probs = bd_->LitU8(1);
  } else {
    frame_header_.refresh_golden_frame = bd_->LitU8(1);
    frame_header_.refresh_alternate_frame = bd_->LitU8(1);
    if (!frame_header_.refresh_golden_frame) {
      frame_header_.copy_buffer_to_golden = bd_->LitU8(2);
    }
    if (!frame_header_.refresh_alternate_frame) {
      frame_header_.copy_buffer_to_alternate = bd_->LitU8(2);
    }
    frame_header_.sign_bias_golden = bd_->LitU8(1);
    frame_header_.sign_bias_alternate = bd_->LitU8(1);
    frame_header_.refresh_entropy_probs = bd_->LitU8(1);
    frame_header_.refresh_last = bd_->LitU8(1);
  }
  TokenProbUpdate();
  frame_header_.mb_no_skip_coeff = bd_->LitU8(1);
  if (frame_header_.mb_no_skip_coeff) {
    frame_header_.prob_skip_false = bd_->Prob8();
  }
  if (!frame_tag_.key_frame) {
    frame_header_.prob_intra = bd_->Prob8();
    frame_header_.prob_last = bd_->Prob8();
    frame_header_.prob_gf = bd_->Prob8();
    bool intra_16x16_prob_update_flag = bd_->LitU8(1);
    if (intra_16x16_prob_update_flag) {
      for (unsigned i = 0; i < kNumYModeProb; i++) {
        context_.intra_16x16_prob.at(i) = bd_->Prob8();
      }
    }
    bool intra_chroma_prob_update_flag = bd_->LitU8(1);
    if (intra_chroma_prob_update_flag) {
      for (unsigned i = 0; i < kNumUVModeProb; i++) {
        context_.intra_chroma_prob.at(i) = bd_->Prob8();
      }
    }
    MVProbUpdate();
  }
  return frame_header_;
}

void BitstreamParser::UpdateSegmentation() {
  frame_header_.update_mb_segmentation_map = bd_->LitU8(1);
  bool update_segment_feature_data = bd_->LitU8(1);
  if (update_segment_feature_data) {
    frame_header_.segment_feature_mode = SegmentMode(bd_->LitU8(1));
    for (unsigned i = 0; i < kMaxMacroBlockSegments; i++) {
      bool quantizer_update = bd_->LitU8(1);
      if (quantizer_update) {
        int16_t quantizer_update_value = bd_->Prob7();
        bool quantizer_update_sign = bd_->LitU8(1);
        frame_header_.quantizer_segment.at(i) = quantizer_update_sign
                                                    ? -quantizer_update_value
                                                    : quantizer_update_value;
      }
    }
    for (unsigned i = 0; i < kMaxMacroBlockSegments; i++) {
      bool loop_filter_update = bd_->LitU8(1);
      if (loop_filter_update) {
        int16_t lf_update_value = bd_->LitU8(6);
        bool lf_update_sign = bd_->LitU8(1);
        frame_header_.loop_filter_level_segment.at(i) =
            lf_update_sign ? -lf_update_value : lf_update_value;
      }
    }
  }
  if (frame_header_.update_mb_segmentation_map) {
    for (unsigned i = 0; i < kNumMacroBlockSegmentProb; i++) {
      bool segment_prob_update = bd_->LitU8(1);
      if (segment_prob_update) {
        context_.segment_prob.at(i) = bd_->Prob8();
      } else {
        context_.segment_prob.at(i) = UINT8_MAX;
      }
    }
  }
}

void BitstreamParser::MbLfAdjust() {
  bool loop_filter_adj_enable = bd_->LitU8(1);
  if (loop_filter_adj_enable) {
    bool mode_ref_lf_delta_update = bd_->LitU8(1);
    if (mode_ref_lf_delta_update) {
      for (unsigned i = 0; i < kNumRefFrames; i++) {
        bool ref_frame_delta_update_flag = bd_->LitU8(1);
        if (ref_frame_delta_update_flag) {
          int8_t delta_q = int8_t(bd_->LitU8(6));
          bool delta_sign = bd_->LitU8(1);
          context_.ref_frame_delta_lf.at(i) = delta_sign ? -delta_q : delta_q;
        }
      }
      for (unsigned i = 0; i < kNumLfPredictionDelta; i++) {
        bool mb_mode_delta_update_flag = bd_->LitU8(1);
        if (mb_mode_delta_update_flag) {
          int8_t delta_q = int8_t(bd_->LitU8(6));
          bool delta_sign = bd_->LitU8(1);
          context_.mb_mode_delta_lf.at(i) = delta_sign ? -delta_q : delta_q;
        }
      }
    }
  }
}

QuantIndices BitstreamParser::ReadQuantIndices() {
  QuantIndices result{};
  result.y_ac_qi = bd_->LitU8(7);
  bool y_dc_delta_present = bd_->LitU8(1);
  if (y_dc_delta_present) {
    result.y_dc_delta_q = bd_->LitU8(4);
    bool y_dc_delta_sign = bd_->LitU8(1);
    if (y_dc_delta_sign) {
      result.y_dc_delta_q = -result.y_dc_delta_q;
    }
  }
  bool y2_dc_delta_present = bd_->LitU8(1);
  if (y2_dc_delta_present) {
    result.y2_dc_delta_q = bd_->LitU8(4);
    bool y2_dc_delta_sign = bd_->LitU8(1);
    if (y2_dc_delta_sign) {
      result.y2_dc_delta_q = -result.y2_dc_delta_q;
    }
  }
  bool y2_ac_delta_present = bd_->LitU8(1);
  if (y2_ac_delta_present) {
    result.y2_ac_delta_q = bd_->LitU8(4);
    bool y2_ac_delta_sign = bd_->LitU8(1);
    if (y2_ac_delta_sign) {
      result.y2_ac_delta_q = -result.y2_ac_delta_q;
    }
  }
  bool uv_dc_delta_present = bd_->LitU8(1);
  if (uv_dc_delta_present) {
    result.uv_dc_delta_q = bd_->LitU8(4);
    bool uv_dc_delta_sign = bd_->LitU8(1);
    if (uv_dc_delta_sign) {
      result.uv_dc_delta_q = -result.uv_dc_delta_q;
    }
  }
  bool uv_ac_delta_present = bd_->LitU8(1);
  if (uv_ac_delta_present) {
    result.uv_ac_delta_q = bd_->LitU8(4);
    bool uv_ac_delta_sign = bd_->LitU8(1);
    if (uv_ac_delta_sign) {
      result.uv_ac_delta_q = -result.uv_ac_delta_q;
    }
  }
  return result;
}

void BitstreamParser::TokenProbUpdate() {
  if (!frame_header_.refresh_entropy_probs) {
    context_.coeff_prob = std::ref(context_.coeff_prob_persistent);
  } else {
    // TODO: (Improvement) Write a safe_copy with bounds checking
    std::copy(context_.coeff_prob_persistent.begin(),
              context_.coeff_prob_persistent.end(),
              context_.coeff_prob_temp.begin());
    context_.coeff_prob = std::ref(context_.coeff_prob_temp);
  }
  for (unsigned i = 0; i < kNumBlockType; i++) {
    for (unsigned j = 0; j < kNumCoeffBand; j++) {
      for (unsigned k = 0; k < kNumDctContextType; k++) {
        for (unsigned l = 0; l < kNumCoeffProb; l++) {
          bool coeff_prob_update_flag =
              bd_->Bool(kCoeffUpdateProbs.at(i).at(j).at(k).at(l));
          if (coeff_prob_update_flag) {
            context_.coeff_prob.get().at(i).at(j).at(k).at(l) = bd_->Prob8();
          }
        }
      }
    }
  }
}

void BitstreamParser::MVProbUpdate() {
  for (unsigned i = 0; i < kNumMVDimen; i++) {
    for (unsigned j = 0; j < kMVPCount; j++) {
      bool mv_prob_update_flag = bd_->Bool(kMVUpdateProbs.at(i).at(j));
      if (mv_prob_update_flag) {
        context_.mv_prob.at(i).at(j) = bd_->Prob7();
      }
    }
  }
}

MacroBlockPreHeader BitstreamParser::ReadMacroBlockPreHeader() {
  MacroBlockPreHeader result{};
  if (frame_header_.update_mb_segmentation_map) {
    result.segment_id =
        uint8_t(bd_->Tree(context_.segment_prob, kMbSegmentTree));
    context_.mb_metadata.at(macroblock_metadata_idx) |= result.segment_id << 2;
  }
  if (frame_header_.mb_no_skip_coeff) {
    result.mb_skip_coeff = bd_->Bool(frame_header_.prob_skip_false);
    context_.mb_metadata.at(macroblock_metadata_idx) |= result.mb_skip_coeff
                                                        << 1;
  }
  if (!frame_tag_.key_frame) {
    result.is_inter_mb = bd_->Bool(frame_header_.prob_intra);
  }
  result.ref_frame = CURRENT_FRAME;
  if (result.is_inter_mb) {
    bool mb_ref_frame_sel1 = bd_->Bool(frame_header_.prob_last);
    if (mb_ref_frame_sel1) {
      bool mb_ref_frame_sel2 = bd_->Bool(frame_header_.prob_gf);
      result.ref_frame = GOLDEN_FRAME + mb_ref_frame_sel2;
    } else {
      result.ref_frame = LAST_FRAME;
    }
    context_.mb_metadata.at(macroblock_metadata_idx) |= (result.ref_frame << 4);
  }
  return result;
}

SubBlockMVMode BitstreamParser::ReadSubBlockMVMode(uint8_t sub_mv_context) {
  return SubBlockMVMode(
      bd_->Tree(kSubMVRefProbs.at(sub_mv_context), kSubBlockMVTree));
}

MotionVector BitstreamParser::ReadSubBlockMV() {
  auto h = ReadMVComponent(false);
  auto w = ReadMVComponent(true);
  return MotionVector(h, w);
}

InterMBHeader BitstreamParser::ReadInterMBHeader(
    const std::array<uint8_t, 4> &cnt) {
  std::array<uint8_t, 4> mv_ref_probs{};
  mv_ref_probs[0] = kSubMVRefProbs[cnt[0]][0];
  mv_ref_probs[1] = kSubMVRefProbs[cnt[1]][1];
  mv_ref_probs[2] = kSubMVRefProbs[cnt[2]][2];
  mv_ref_probs[3] = kSubMVRefProbs[cnt[3]][3];
  InterMBHeader result{};
  result.mv_mode = MacroBlockMV(bd_->Tree(mv_ref_probs, kMVRefTree));
  if (result.mv_mode == MV_SPLIT) {
    result.mv_split_mode =
        MVPartition(bd_->Tree(kMVPartitionProbs, kMVPartitionTree));
    // The rest is up to the caller to call ReadSubBlockMode() &
    // ReadSubBlockMV().
  } else if (result.mv_mode == MV_NEW) {
    auto h = ReadMVComponent(false);
    auto w = ReadMVComponent(true);
    result.mv_new = MotionVector(h, w);
  }
  context_.mb_metadata.at(macroblock_metadata_idx) |=
      (result.mv_mode == MV_SPLIT);
  macroblock_metadata_idx++;
  return result;
}

SubBlockMode BitstreamParser::ReadSubBlockBModeKF(int above_bmode,
                                                  int left_bmode) {
  return SubBlockMode(bd_->Tree(
      kKeyFrameBModeProbs.at(size_t(above_bmode)).at(size_t(left_bmode)),
      kSubBlockModeTree));
}

SubBlockMode BitstreamParser::ReadSubBlockBModeNonKF() {
  return SubBlockMode(bd_->Tree(kBModeProb, kSubBlockModeTree));
}

MacroBlockMode BitstreamParser::ReadIntraMB_UVMode() {
  return MacroBlockMode(bd_->Tree(context_.intra_chroma_prob, kUVModeTree));
}

IntraMBHeader BitstreamParser::ReadIntraMBHeader() {
  IntraMBHeader result{};
  result.intra_y_mode =
      MacroBlockMode(bd_->Tree(context_.intra_16x16_prob, kYModeTree));
  // The rest is up to the caller to call ReadSubBlockBMode() &
  // ReadMB_UVMode().
  context_.mb_metadata.at(macroblock_metadata_idx) |=
      (result.intra_y_mode != B_PRED);
  context_.mb_metadata.at(macroblock_metadata_idx) |=
      (result.intra_y_mode << 6);
  macroblock_metadata_idx++;
  return result;
}

int16_t BitstreamParser::ReadMVComponent(bool kind) {
  auto p = context_.mv_prob.at(kind);
  int16_t a = 0;
  if (bd_->Bool(p.at(MVP_IS_SHORT))) {
    for (unsigned i = 0; i < 3; i++) {
      a += bd_->Bool(p.at(kMVPBits + i)) << i;
    }
    for (unsigned i = 9; i > 3; i--) {
      a += bd_->Bool(p.at(kMVPBits + i)) << i;
    }
    if ((a & 0xFFF0) == 0) {
      a += bd_->Bool(p.at(kMVPBits + 3)) << 3;
    }
  } else {
    a = int16_t(
        bd_->Tree(IteratorArray<decltype(p)>(p.begin() + MVP_SHORT, p.end()),
                  kSmallMVTree));
  }
  if (a) {
    bool sign = bd_->Bool(p.at(MVP_SIGN));
    if (sign) a = -a;
  }
  return a;
}

ResidualData BitstreamParser::ReadResidualData(
    const ResidualParam &residual_ctx) {
  ResidualData result{};
  auto macroblock_metadata = context_.mb_metadata.at(residual_macroblock_idx);
  auto first_coeff = (macroblock_metadata & 0x2) ? 1 : 0;
  residual_macroblock_idx++;
  if (macroblock_metadata & 0x2) {
    std::array<bool, 25> non_zero{};
    if (macroblock_metadata & 0x1) {
      auto &prob = context_.coeff_prob.get()
                       .at(1)
                       .at(kCoeffBands.at(0))
                       .at(residual_ctx.y2_nonzero);
      tie(result.dct_coeff.at(0), non_zero.at(0)) =
          ReadResidualBlock(first_coeff, prob);
    }
    unsigned block_type_y = first_coeff ? 0 : 3;
    for (unsigned i = 1; i <= 16; i++) {
      unsigned above_nonzero =
          (i <= 4) ? residual_ctx.y1_above.at(i - 1) : non_zero.at(i - 4);
      unsigned left_nonzero = ((i - 1) & 4)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.y1_left.at((i - 1) >> 2);
      auto &prob = context_.coeff_prob.get()
                       .at(block_type_y)
                       .at(kCoeffBands.at(i - 1))
                       .at(above_nonzero + left_nonzero);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(first_coeff, prob);
    }
    for (unsigned i = 17; i <= 20; i++) {
      unsigned above_nonzero =
          (i <= 18) ? residual_ctx.u_above.at(i - 17) : non_zero.at(i - 2);
      unsigned left_nonzero = ((i - 17) & 2)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.y1_left.at((i - 17) >> 1);
      auto &prob = context_.coeff_prob.get()
                       .at(2)
                       .at(kCoeffBands.at(i - 17))
                       .at(above_nonzero + left_nonzero);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(first_coeff, prob);
    }
    for (unsigned i = 21; i <= 24; i++) {
      unsigned above_nonzero =
          (i <= 22) ? residual_ctx.u_above.at(i - 21) : non_zero.at(i - 2);
      unsigned left_nonzero = ((i - 21) & 2)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.y1_left.at((i - 21) >> 1);
      auto &prob = context_.coeff_prob.get()
                       .at(2)
                       .at(kCoeffBands.at(i - 21))
                       .at(above_nonzero + left_nonzero);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(first_coeff, prob);
    }
  }
  result.segment_id = (macroblock_metadata >> 2) & 0x3;
  result.loop_filter_level = int8_t(frame_header_.loop_filter_level);
  result.loop_filter_level +=
      context_.ref_frame_delta_lf.at((macroblock_metadata >> 4) & 0x3);
  auto prediction_mode = (macroblock_metadata >> 6) & 0x3;
  if (((macroblock_metadata >> 4) & 0x3) == CURRENT_FRAME) {
    if (prediction_mode == B_PRED) {
      result.loop_filter_level += context_.mb_mode_delta_lf.at(0);
    }
  } else if (prediction_mode == MV_ZERO) {
    result.loop_filter_level += context_.mb_mode_delta_lf.at(1);
  } else if (prediction_mode == MV_SPLIT) {
    result.loop_filter_level += context_.mb_mode_delta_lf.at(3);
  } else {
    result.loop_filter_level += context_.mb_mode_delta_lf.at(2);
  }
  return result;
}

std::pair<std::array<int16_t, 16>, bool> BitstreamParser::ReadResidualBlock(
    int first_coeff, const std::array<Prob, kNumCoeffProb> &prob) {
  std::array<int16_t, 16> result{};
  bool non_zero = false;
  for (unsigned i = unsigned(first_coeff); i < 16; i++) {
    DctToken token = DctToken(bd_->Tree(prob, kCoeffTree));
    if (token == DCT_EOB) {
      break;
    }
    result.at(i) = kTokenToCoeff.at(token);
    if (result.at(i) != DCT_0) {
      non_zero = true;
    }
    if (token >= DCT_CAT1) {
      size_t idx = size_t(token - DCT_CAT1 + 1);
      uint16_t v = 0;
      for (unsigned j = 0; j < idx; j++) {
        v += v + bd_->Bool(kPcat.at(idx - 1).at(j));
      }
      result.at(i) += v;
    }
    bool sign = bd_->LitU8(1);
    if (sign) {
      result.at(i) = -result.at(i);
    }
  }
  return make_pair(result, non_zero);
}

}  // namespace vp8
