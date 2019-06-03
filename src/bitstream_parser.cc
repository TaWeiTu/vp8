#include <cassert>

#include "bitstream_parser.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {

std::unique_ptr<BoolDecoder> BitstreamParser::DropStream() {
  // TODO: Check validity?
  return std::move(bd_);
}

std::pair<FrameTag, FrameHeader> BitstreamParser::ReadFrameTagHeader() {
  auto ft = ReadFrameTag();
  auto fh = ReadFrameHeader();
  return std::make_pair(ft, fh);
}

FrameTag BitstreamParser::ReadFrameTag() {
  uint32_t tag = bd_->Raw(3);
  frame_tag_.key_frame = tag & 0x1;
  frame_tag_.version = (tag >> 1) & 0x7;
  frame_tag_.show_frame = (tag >> 4) & 0x1;
  frame_tag_.first_part_size = (tag >> 5) & 0x7FFFF;
  if (frame_tag_.key_frame) {
    uint32_t start_code = bd_->Raw(3);
    assert(start_code == 0x9D012A);
    uint32_t horizontal_size_code = bd_->Raw(2);
    frame_tag_.width = horizontal_size_code & 0x3FFF;
    frame_tag_.horizontal_scale = horizontal_size_code >> 14;
    uint32_t vertical_size_code = bd_->Raw(2);
    frame_tag_.height = vertical_size_code & 0x3FFF;
    frame_tag_.vertical_scale = vertical_size_code >> 14;
  }
  context_.mb_metadata.resize(((frame_tag_.width + 3) / 4) *
                              ((frame_tag_.height + 3) / 4));
  fill(context_.mb_metadata.begin(), context_.mb_metadata.end(), 0);
  return frame_tag_;
}

FrameHeader BitstreamParser::ReadFrameHeader() {
  FrameHeader frame_header{};
  if (frame_tag_.key_frame) {
    // TODO: Set context defaults
    context_ = ParserContext{};
    frame_header.color_space = bd_->Lit(1);
    frame_header.clamping_type = bd_->Lit(1);
  }
  frame_header.segmentation_enabled = bd_->Lit(1);
  if (frame_header.segmentation_enabled) {
    UpdateSegmentation();
  }
  frame_header.filter_type = bd_->Lit(1);
  frame_header.loop_filter_level = bd_->Lit(6);
  frame_header.sharpness_level = bd_->Lit(3);
  MbLfAdjust();
  frame_header.log2_nbr_of_dct_partitions = bd_->Lit(2);
  frame_header.quant_indices = ReadQuantIndices();
  if (frame_tag_.key_frame) {
    frame_header.refresh_entropy_probs = bd_->Lit(1);
  } else {
    frame_header.refresh_golden_frame = bd_->Lit(1);
    frame_header.refresh_alternate_frame = bd_->Lit(1);
    if (!frame_header.refresh_golden_frame) {
      frame_header.copy_buffer_to_golden = bd_->Lit(2);
    }
    if (!frame_header.refresh_alternate_frame) {
      frame_header.copy_buffer_to_alternate = bd_->Lit(2);
    }
    frame_header.sign_bias_golden = bd_->Lit(1);
    frame_header.sign_bias_alternate = bd_->Lit(1);
    frame_header.refresh_entropy_probs = bd_->Lit(1);
    frame_header.refresh_last = bd_->Lit(1);
  }
  TokenProbUpdate();
  frame_header.mb_no_skip_coeff = bd_->Lit(1);
  if (frame_header.mb_no_skip_coeff) {
    frame_header.prob_skip_false = bd_->Lit(1);
  }
  if (!frame_tag_.key_frame) {
    frame_header.prob_intra = bd_->Lit(8);
    frame_header.prob_last = bd_->Lit(8);
    frame_header.prob_gf = bd_->Lit(8);
    bool intra_16x16_prob_update_flag = bd_->Lit(1);
    if (intra_16x16_prob_update_flag) {
      for (int i = 0; i < 4; i++) {
        context_.intra_16x16_prob.at(i) = bd_->Lit(8);
      }
    }
    bool intra_chroma_prob_update_flag = bd_->Lit(1);
    if (intra_chroma_prob_update_flag) {
      for (int i = 0; i < 3; i++) {
        context_.intra_chroma_prob.at(i) = bd_->Lit(8);
      }
    }
    MvProbUpdate();
  }
  return frame_header;
}

void BitstreamParser::UpdateSegmentation() {
  frame_header_.update_mb_segmentation_map = bd_->Lit(1);
  bool update_segment_feature_data = bd_->Lit(1);
  if (update_segment_feature_data) {
    bool segment_feature_mode = bd_->Lit(1);
    for (int i = 0; i < 4; i++) {
      bool quantizer_update = bd_->Lit(1);
      if (quantizer_update) {
        int16_t quantizer_update_value = bd_->Prob7();
        bool quantizer_update_sign = bd_->Lit(1);
        if (segment_feature_mode == SEGMENT_MODE_ABSOLUTE) {
          frame_header_.quantizer_segment.at(i) = 0;
        }
        frame_header_.quantizer_segment.at(i) = quantizer_update_sign
                                                    ? -quantizer_update_value
                                                    : quantizer_update_value;
      }
    }
    for (int i = 0; i < 4; i++) {
      bool loop_filter_update = bd_->Lit(1);
      if (loop_filter_update) {
        int16_t lf_update_value = bd_->Lit(6);
        bool lf_update_sign = bd_->Lit(1);
        if (segment_feature_mode == SEGMENT_MODE_ABSOLUTE) {
          frame_header_.loop_filter_level_segment.at(i) = 0;
        }
        frame_header_.loop_filter_level_segment.at(i) =
            lf_update_sign ? -lf_update_value : lf_update_value;
      }
    }
  }
  if (frame_header_.update_mb_segmentation_map) {
    for (int i = 0; i < 3; i++) {
      bool segment_prob_update = bd_->Lit(1);
      if (segment_prob_update) {
        context_.segment_prob.at(i) = bd_->Lit(8);
      }
    }
  }
}

void BitstreamParser::MbLfAdjust() {
  bool loop_filter_adj_enable = bd_->Lit(1);
  if (loop_filter_adj_enable) {
    bool mode_ref_lf_delta_update = bd_->Lit(1);
    if (mode_ref_lf_delta_update) {
      for (int i = 0; i < 4; i++) {
        bool ref_frame_delta_update_flag = bd_->Lit(1);
        if (ref_frame_delta_update_flag) {
          uint8_t delta_magnitude = bd_->Lit(6);
          bool delta_sign = bd_->Lit(1);
          context_.ref_frame_delta_lf.at(i) =
              delta_sign ? -delta_magnitude : delta_magnitude;
        }
      }
      for (int i = 0; i < 4; i++) {
        bool mb_mode_delta_update_flag = bd_->Lit(1);
        if (mb_mode_delta_update_flag) {
          uint8_t delta_magnitude = bd_->Lit(6);
          bool delta_sign = bd_->Lit(1);
          context_.mb_mode_delta_lf.at(i) =
              delta_sign ? -delta_magnitude : delta_magnitude;
        }
      }
    }
  }
}

QuantIndices BitstreamParser::ReadQuantIndices() {
  QuantIndices result{};
  result.y_ac_qi = bd_->Prob7();
  result.y_dc_delta_present = bd_->Lit(1);
  if (result.y_dc_delta_present) {
    result.y_dc_delta_magnitude = bd_->Lit(4);
    bool y_dc_delta_sign = bd_->Lit(1);
    if (y_dc_delta_sign) {
      result.y_dc_delta_magnitude = -result.y_dc_delta_magnitude;
    }
  }
  result.y2_dc_delta_present = bd_->Lit(1);
  if (result.y2_dc_delta_present) {
    result.y2_dc_delta_magnitude = bd_->Lit(4);
    bool y2_dc_delta_sign = bd_->Lit(1);
    if (y2_dc_delta_sign) {
      result.y2_dc_delta_magnitude = -result.y2_dc_delta_magnitude;
    }
  }
  result.y2_ac_delta_present = bd_->Lit(1);
  if (result.y2_ac_delta_present) {
    result.y2_ac_delta_magnitude = bd_->Lit(4);
    bool y2_ac_delta_sign = bd_->Lit(1);
    if (y2_ac_delta_sign) {
      result.y2_ac_delta_magnitude = -result.y2_ac_delta_magnitude;
    }
  }
  result.uv_dc_delta_present = bd_->Lit(1);
  if (result.uv_dc_delta_present) {
    result.uv_dc_delta_magnitude = bd_->Lit(4);
    bool uv_dc_delta_sign = bd_->Lit(1);
    if (uv_dc_delta_sign) {
      result.uv_dc_delta_magnitude = -result.uv_dc_delta_magnitude;
    }
  }
  result.uv_ac_delta_present = bd_->Lit(1);
  if (result.uv_ac_delta_present) {
    result.uv_ac_delta_magnitude = bd_->Lit(4);
    bool uv_ac_delta_sign = bd_->Lit(1);
    if (uv_ac_delta_sign) {
      result.uv_ac_delta_magnitude = -result.uv_ac_delta_magnitude;
    }
  }
  return result;
}

void BitstreamParser::TokenProbUpdate() {
  if (!frame_header_.refresh_entropy_probs) {
    context_.coeff_prob = std::ref(context_.coeff_prob_persistent);
  } else {
    std::copy(context_.coeff_prob_persistent.begin(),
              context_.coeff_prob_persistent.end(),
              context_.coeff_prob_temp.begin());
    context_.coeff_prob = std::ref(context_.coeff_prob_temp);
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {
      for (int k = 0; k < 3; k++) {
        for (int l = 0; l < 11; l++) {
          bool coeff_prob_update_flag = bd_->Lit(1);
          if (coeff_prob_update_flag) {
            context_.coeff_prob.get().at(i).at(j).at(k).at(l) = bd_->Prob8();
          }
        }
      }
    }
  }
}

void BitstreamParser::MvProbUpdate() {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 19; j++) {
      bool mv_prob_update_flag = bd_->Lit(1);
      if (mv_prob_update_flag) {
        context_.mv_prob.at(i).at(j) = bd_->Prob7();
      }
    }
  }
}

MacroBlockPreHeader BitstreamParser::ReadMacroBlockPreHeader() {
  MacroBlockPreHeader result{};
  if (frame_header_.update_mb_segmentation_map) {
    result.segment_id = bd_->Tree(context_.segment_prob, kMbSegmentTree);
    context_.mb_metadata.at(macroblock_metadata_idx) |= result.segment_id << 2;
  }
  if (frame_header_.mb_no_skip_coeff) {
    result.mb_skip_coeff = bd_->Bool(frame_header_.prob_skip_false);
    context_.mb_metadata.at(macroblock_metadata_idx) |= result.mb_skip_coeff
                                                        << 1;
  }
  if (!frame_tag_.key_frame) {
    result.is_inter_mb = frame_header_.prob_intra;
  }
  return result;
}

SubBlockMode BitstreamParser::ReadSubBlockMode(int sub_mv_context) {
  return SubBlockMode(
      bd_->Tree(kSubMvRefProbs.at(sub_mv_context), kSubBlockMVTree));
}

MotionVector BitstreamParser::ReadSubBlockMV() {
  auto h = ReadMvComponent(false);
  auto w = ReadMvComponent(true);
  return MotionVector(h, w);
}

InterMBHeader BitstreamParser::ReadInterMBHeader(
    const std::array<uint8_t, 4> &cnt) {
  std::array<uint8_t, 4> mv_ref_probs{};
  mv_ref_probs[0] = kSubMvRefProbs[cnt[0]][0];
  mv_ref_probs[1] = kSubMvRefProbs[cnt[1]][1];
  mv_ref_probs[2] = kSubMvRefProbs[cnt[2]][2];
  mv_ref_probs[3] = kSubMvRefProbs[cnt[3]][3];
  InterMBHeader result{};
  bool mb_ref_frame_sel1 = bd_->Bool(frame_header_.prob_last),
       mb_ref_frame_sel2 = false;
  if (mb_ref_frame_sel1) {
    mb_ref_frame_sel2 = bd_->Bool(frame_header_.prob_gf);
  }
  result.ref_frame = mb_ref_frame_sel2 << 1 | mb_ref_frame_sel1;
  result.mv_mode = MacroBlockMV(bd_->Tree(mv_ref_probs, kMvRefTree));
  if (result.mv_mode == MV_SPLIT) {
    result.mv_split_mode =
        MVPartition(bd_->Tree(kMvPartitionProbs, kMvPartitionTree));
    // The rest is up to the caller to call ReadSubBlockMode() &
    // ReadSubBlockMV().
  } else if (result.mv_mode == MV_NEW) {
    auto h = ReadMvComponent(false);
    auto w = ReadMvComponent(true);
    result.mv_new = MotionVector(h, w);
  }
  context_.mb_metadata.at(macroblock_metadata_idx) |=
      (result.mv_mode == MV_SPLIT);
  context_.mb_metadata.at(macroblock_metadata_idx) |= (result.ref_frame << 4);
  macroblock_metadata_idx++;
  return result;
}

SubBlockMode BitstreamParser::ReadSubBlockBMode(int above_bmode,
                                                int left_bmode) {
  return SubBlockMode(bd_->Tree(
      kKeyFrameBModeProbs.at(above_bmode).at(left_bmode), kSubBlockModeTree));
}

MacroBlockMode BitstreamParser::ReadIntraMB_UVMode() {
  return MacroBlockMode(bd_->Tree(context_.intra_chroma_prob, kUvModeProb));
}

MacroBlockMode BitstreamParser::ReadIntraMB_YMode() {
  auto intra_y_mode =
      MacroBlockMode(bd_->Tree(context_.intra_16x16_prob, kYModeTree));
  // The rest is up to the caller to call ReadSubBlockBMode() &
  // ReadMB_UVMode().
  context_.mb_metadata.at(macroblock_metadata_idx) |= (intra_y_mode != B_PRED);
  context_.mb_metadata.at(macroblock_metadata_idx) |= (intra_y_mode << 6);
  macroblock_metadata_idx++;
  return intra_y_mode;
}

int16_t BitstreamParser::ReadMvComponent(bool kind) {
  auto p = context_.mvc_probs.at(kind);
  int16_t a = 0;
  if (bd_->Bool(p.at(MVP_IS_SHORT))) {
    for (int i = 0; i < 3; i++) {
      a += bd_->Bool(p.at(kMvpBits + i)) << i;
    }
    for (int i = 9; i > 3; i--) {
      a += bd_->Bool(p.at(kMvpBits + i)) << i;
    }
    if ((a & 0xFFF0) == 0) {
      a += bd_->Bool(p.at(kMvpBits + 3)) << 3;
    }
  } else {
    a = bd_->Tree(IteratorArray<decltype(p)>(p.begin() + MVP_SHORT, p.end()),
                  kSmallMvTree);
  }
  return a;
}

ResidualData BitstreamParser::ReadResidualData(int first_coeff,
                                               std::array<uint8_t, 4> context) {
  ResidualData result{};
  auto macroblock_metadata = context_.mb_metadata.at(residual_macroblock_idx);
  residual_macroblock_idx++;
  if (macroblock_metadata & 0x2) {
    if (macroblock_metadata & 0x1) {
      result.dct_coeff.at(0) = ReadResidualBlock(first_coeff, context);
    }
    for (int i = 1; i <= 24; i++) {
      result.dct_coeff.at(i) = ReadResidualBlock(first_coeff, context);
    }
  }
  result.segment_id = (macroblock_metadata >> 2) & 0x3;
  result.loop_filter_level = frame_header_.loop_filter_level;
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

std::array<int16_t, 16> BitstreamParser::ReadResidualBlock(
    int first_coeff, std::array<uint8_t, 4> context) {
  std::array<int16_t, 16> result{};
  for (int i = first_coeff; i < 16; i++) {
    DctToken token = DctToken(bd_->Tree(context_.coeff_prob.get()
                                            .at(context.at(0))
                                            .at(context.at(1))
                                            .at(context.at(2)),
                                        kCoeffTree));
    if (token == DCT_EOB) {
      break;
    }
    result.at(i) = kTokenToCoeff.at(token);
    if (token >= DCT_CAT1) {
      size_t idx = size_t(token - DCT_CAT1 + 1);
      uint16_t v = 0;
      for (int i = 0; i < idx; i++) {
        v += v + bd_->Bool(kPcat.at(idx - 1).at(i));
      }
      result.at(i) += v;
    }
    bool sign = bd_->Lit(1);
    if (sign) {
      result.at(i) = -result.at(i);
    }
  }
  return result;
}

}  // namespace vp8
