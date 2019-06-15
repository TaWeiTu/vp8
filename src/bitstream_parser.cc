#include "bitstream_parser.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "frame.h"
#include "utils.h"

namespace vp8 {

std::pair<FrameTag, FrameHeader> BitstreamParser::ReadFrameTagHeader() {
  auto ft = ReadFrameTag();
  auto fh = ReadFrameHeader();
  return std::make_pair(ft, fh);
}

FrameTag BitstreamParser::ReadFrameTag() {
  uint32_t tag = buffer_.ReadBytes(3);
  frame_tag_.key_frame = !(tag & 0x1);
  frame_tag_.version = (tag >> 1) & 0x7;
  frame_tag_.show_frame = (tag >> 4) & 0x1;
  first_part_size_ = (tag >> 5) & 0x7FFFF;
  ensure(!(frame_tag_.version >> 2),
         "[Error] ReadFrameTag: Experimental streams unsupported.");
  if (frame_tag_.key_frame) {
    uint32_t start_code = buffer_.ReadBytes(3);
    ensure(start_code == 0x2A019D,
           "[Error] ReadFrameTag: Incorrect start_code.");
    uint32_t horizontal_size_code = buffer_.ReadBytes(2);
    frame_tag_.width = horizontal_size_code & 0x3FFF;
    frame_tag_.horizontal_scale = uint16_t(horizontal_size_code >> 14);
    uint32_t vertical_size_code = buffer_.ReadBytes(2);
    frame_tag_.height = vertical_size_code & 0x3FFF;
    frame_tag_.vertical_scale = uint16_t(vertical_size_code >> 14);
  }
  size_t tag_size = frame_tag_.key_frame ? 10 : 3;
  bd_ = BoolDecoder(buffer_.SubSpan(tag_size, first_part_size_));
  return frame_tag_;
}

FrameHeader BitstreamParser::ReadFrameHeader() {
  if (frame_tag_.key_frame) {
    context_.get() = ParserContext();
    frame_header_.color_space = bd_.LitU8(1);
    frame_header_.clamping_type = bd_.LitU8(1);
    ensure(!frame_header_.color_space && !frame_header_.clamping_type,
           "[Error] ReadFrameHeader: Unsupported color_space / clamping_type");
    context_.get().mb_num_cols = (frame_tag_.width + 15) / 16;
    context_.get().mb_num_rows = (frame_tag_.height + 15) / 16;
    context_.get().mb_metadata.resize(uint32_t(context_.get().mb_num_cols) *
                                      context_.get().mb_num_rows);
    context_.get().segment_id.resize(uint32_t(context_.get().mb_num_cols) *
                                     context_.get().mb_num_rows);
    fill(context_.get().segment_id.begin(), context_.get().segment_id.end(), 0);
  }
  fill(context_.get().mb_metadata.begin(), context_.get().mb_metadata.end(), 0);
  frame_header_.segmentation_enabled = bd_.LitU8(1);
  if (frame_header_.segmentation_enabled) {
    UpdateSegmentation();
  }
  frame_header_.filter_type = bd_.LitU8(1);
  frame_header_.loop_filter_level = bd_.LitU8(6);
  frame_header_.sharpness_level = bd_.LitU8(3);
  MbLfAdjust();
  nbr_of_dct_partitions_ = uint8_t(1 << bd_.LitU8(2));
  size_t tag_size = frame_tag_.key_frame ? 10 : 3;
  auto size_span = buffer_.SubSpan(tag_size + first_part_size_,
                                   3 * (nbr_of_dct_partitions_ - 1));
  size_t offset =
      tag_size + first_part_size_ + 3 * (nbr_of_dct_partitions_ - 1);
  for (size_t i = 0; i < nbr_of_dct_partitions_ - 1; i++) {
    auto count = size_span.ReadBytes(3);
    residual_bd_.at(i) = BoolDecoder(buffer_.SubSpan(offset, count));
    offset += count;
  }
  residual_bd_.at(nbr_of_dct_partitions_ - 1) =
      BoolDecoder(buffer_.SubSpan(offset));
  frame_header_.quant_indices = ReadQuantIndices();
  if (frame_tag_.key_frame) {
    frame_header_.refresh_entropy_probs = bd_.LitU8(1);
    frame_header_.refresh_golden_frame = true;
    frame_header_.refresh_alternate_frame = true;
    frame_header_.refresh_last = true;
  } else {
    frame_header_.refresh_golden_frame = bd_.LitU8(1);
    frame_header_.refresh_alternate_frame = bd_.LitU8(1);
    if (!frame_header_.refresh_golden_frame) {
      frame_header_.copy_buffer_to_golden = bd_.LitU8(2);
    }
    if (!frame_header_.refresh_alternate_frame) {
      frame_header_.copy_buffer_to_alternate = bd_.LitU8(2);
    }
    frame_header_.sign_bias_golden = bd_.LitU8(1);
    frame_header_.sign_bias_alternate = bd_.LitU8(1);
    frame_header_.refresh_entropy_probs = bd_.LitU8(1);
    frame_header_.refresh_last = bd_.LitU8(1);
  }
  TokenProbUpdate();
  frame_header_.mb_no_skip_coeff = bd_.LitU8(1);
  if (frame_header_.mb_no_skip_coeff) {
    frame_header_.prob_skip_false = bd_.Prob8();
  }
  if (!frame_tag_.key_frame) {
    frame_header_.prob_intra = bd_.Prob8();
    frame_header_.prob_last = bd_.Prob8();
    frame_header_.prob_gf = bd_.Prob8();
    bool intra_16x16_prob_update_flag = bd_.LitU8(1);
    if (intra_16x16_prob_update_flag) {
      for (unsigned i = 0; i < kNumYModeProb; i++) {
        context_.get().intra_16x16_prob.at(i) = bd_.Prob8();
      }
    }
    bool intra_chroma_prob_update_flag = bd_.LitU8(1);
    if (intra_chroma_prob_update_flag) {
      for (unsigned i = 0; i < kNumUVModeProb; i++) {
        context_.get().intra_chroma_prob.at(i) = bd_.Prob8();
      }
    }
    MVProbUpdate();
  }
  return frame_header_;
}

void BitstreamParser::UpdateSegmentation() {
  frame_header_.update_mb_segmentation_map = bd_.LitU8(1);
  bool update_segment_feature_data = bd_.LitU8(1);
  if (update_segment_feature_data) {
    context_.get().segment_feature_mode = SegmentMode(bd_.LitU8(1));
    for (unsigned i = 0; i < kMaxMacroBlockSegments; i++) {
      bool quantizer_update = bd_.LitU8(1);
      if (quantizer_update) {
        int16_t quantizer_update_value = bd_.Prob7();
        bool quantizer_update_sign = bd_.LitU8(1);
        context_.get().quantizer_segment.at(i) = quantizer_update_sign
                                                     ? -quantizer_update_value
                                                     : quantizer_update_value;
      }
    }
    for (unsigned i = 0; i < kMaxMacroBlockSegments; i++) {
      bool loop_filter_update = bd_.LitU8(1);
      if (loop_filter_update) {
        int16_t lf_update_value = bd_.LitU8(6);
        bool lf_update_sign = bd_.LitU8(1);
        context_.get().loop_filter_level_segment.at(i) =
            lf_update_sign ? -lf_update_value : lf_update_value;
      }
    }
  }
  if (frame_header_.update_mb_segmentation_map) {
    for (unsigned i = 0; i < kNumMacroBlockSegmentProb; i++) {
      bool segment_prob_update = bd_.LitU8(1);
      if (segment_prob_update) {
        context_.get().segment_prob.at(i) = bd_.Prob8();
      } else {
        context_.get().segment_prob.at(i) = UINT8_MAX;
      }
    }
  }
  // TODO: This is fucking slow.
  frame_header_.segment_feature_mode = context_.get().segment_feature_mode;
  std::copy(context_.get().quantizer_segment.begin(),
            context_.get().quantizer_segment.end(),
            frame_header_.quantizer_segment.begin());
  std::copy(context_.get().loop_filter_level_segment.begin(),
            context_.get().loop_filter_level_segment.end(),
            frame_header_.loop_filter_level_segment.begin());
}

void BitstreamParser::MbLfAdjust() {
  loop_filter_adj_enable_ = bd_.LitU8(1);
  if (loop_filter_adj_enable_) {
    bool mode_ref_lf_delta_update = bd_.LitU8(1);
    if (mode_ref_lf_delta_update) {
      for (unsigned i = 0; i < kNumRefFrames; i++) {
        bool ref_frame_delta_update_flag = bd_.LitU8(1);
        if (ref_frame_delta_update_flag) {
          int8_t delta_q = int8_t(bd_.LitU8(6));
          bool delta_sign = bd_.LitU8(1);
          context_.get().ref_frame_delta_lf.at(i) =
              delta_sign ? -delta_q : delta_q;
        }
      }
      for (unsigned i = 0; i < kNumLfPredictionDelta; i++) {
        bool mb_mode_delta_update_flag = bd_.LitU8(1);
        if (mb_mode_delta_update_flag) {
          int8_t delta_q = int8_t(bd_.LitU8(6));
          bool delta_sign = bd_.LitU8(1);
          context_.get().mb_mode_delta_lf.at(i) =
              delta_sign ? -delta_q : delta_q;
        }
      }
    }
  }
}

QuantIndices BitstreamParser::ReadQuantIndices() {
  QuantIndices result{};
  result.y_ac_qi = bd_.LitU8(7);
  bool y_dc_delta_present = bd_.LitU8(1);
  if (y_dc_delta_present) {
    result.y_dc_delta_q = bd_.LitU8(4);
    bool y_dc_delta_sign = bd_.LitU8(1);
    if (y_dc_delta_sign) {
      result.y_dc_delta_q = -result.y_dc_delta_q;
    }
  }
  bool y2_dc_delta_present = bd_.LitU8(1);
  if (y2_dc_delta_present) {
    result.y2_dc_delta_q = bd_.LitU8(4);
    bool y2_dc_delta_sign = bd_.LitU8(1);
    if (y2_dc_delta_sign) {
      result.y2_dc_delta_q = -result.y2_dc_delta_q;
    }
  }
  bool y2_ac_delta_present = bd_.LitU8(1);
  if (y2_ac_delta_present) {
    result.y2_ac_delta_q = bd_.LitU8(4);
    bool y2_ac_delta_sign = bd_.LitU8(1);
    if (y2_ac_delta_sign) {
      result.y2_ac_delta_q = -result.y2_ac_delta_q;
    }
  }
  bool uv_dc_delta_present = bd_.LitU8(1);
  if (uv_dc_delta_present) {
    result.uv_dc_delta_q = bd_.LitU8(4);
    bool uv_dc_delta_sign = bd_.LitU8(1);
    if (uv_dc_delta_sign) {
      result.uv_dc_delta_q = -result.uv_dc_delta_q;
    }
  }
  bool uv_ac_delta_present = bd_.LitU8(1);
  if (uv_ac_delta_present) {
    result.uv_ac_delta_q = bd_.LitU8(4);
    bool uv_ac_delta_sign = bd_.LitU8(1);
    if (uv_ac_delta_sign) {
      result.uv_ac_delta_q = -result.uv_ac_delta_q;
    }
  }
  return result;
}

void BitstreamParser::TokenProbUpdate() {
  if (frame_header_.refresh_entropy_probs) {
    context_.get().coeff_prob = std::ref(context_.get().coeff_prob_persistent);
  } else {
    // TODO: (Improvement) Write a safe_copy with bounds checking
    std::copy(context_.get().coeff_prob_persistent.begin(),
              context_.get().coeff_prob_persistent.end(),
              context_.get().coeff_prob_temp.begin());
    context_.get().coeff_prob = std::ref(context_.get().coeff_prob_temp);
  }
  for (unsigned i = 0; i < kNumBlockType; i++) {
    for (unsigned j = 0; j < kNumCoeffBand; j++) {
      for (unsigned k = 0; k < kNumDctContextType; k++) {
        for (unsigned l = 0; l < kNumCoeffProb; l++) {
          bool coeff_prob_update_flag =
              bd_.Bool(kCoeffUpdateProbs.at(i).at(j).at(k).at(l));
          if (coeff_prob_update_flag) {
            context_.get().coeff_prob.get().at(i).at(j).at(k).at(l) =
                bd_.Prob8();
          }
        }
      }
    }
  }
}

void BitstreamParser::MVProbUpdate() {
  for (unsigned i = 0; i < kNumMVDimen; i++) {
    for (unsigned j = 0; j < kMVPCount; j++) {
      bool mv_prob_update_flag = bd_.Bool(kMVUpdateProbs.at(i).at(j));
      if (mv_prob_update_flag) {
        context_.get().mv_prob.at(i).at(j) = bd_.Prob7();
      }
    }
  }
}

MacroBlockPreHeader BitstreamParser::ReadMacroBlockPreHeader() {
  MacroBlockPreHeader result{};
  if (frame_header_.update_mb_segmentation_map) {
    result.segment_id =
        uint8_t(bd_.Tree(context_.get().segment_prob, kMbSegmentTree));
    context_.get().segment_id.at(macroblock_metadata_idx_) = result.segment_id;
  } else {
    result.segment_id = context_.get().segment_id.at(macroblock_metadata_idx_);
  }
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |= result.segment_id
                                                             << 2;
  if (frame_header_.mb_no_skip_coeff) {
    result.mb_skip_coeff = bd_.Bool(frame_header_.prob_skip_false);
    context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
        uint16_t(result.mb_skip_coeff) << 1;
  }
  if (!frame_tag_.key_frame) {
    result.is_inter_mb = bd_.Bool(frame_header_.prob_intra);
  }
  result.ref_frame = CURRENT_FRAME;
  if (result.is_inter_mb) {
    bool mb_ref_frame_sel1 = bd_.Bool(frame_header_.prob_last);
    if (mb_ref_frame_sel1) {
      bool mb_ref_frame_sel2 = bd_.Bool(frame_header_.prob_gf);
      result.ref_frame = GOLDEN_FRAME + mb_ref_frame_sel2;
    } else {
      result.ref_frame = LAST_FRAME;
    }
    context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
        (result.ref_frame << 4);
  }
  return result;
}

SubBlockMVMode BitstreamParser::ReadSubBlockMVMode(uint8_t sub_mv_context) {
  return SubBlockMVMode(
      bd_.Tree(kSubMVRefProbs.at(sub_mv_context), kSubBlockMVTree));
}

MotionVector BitstreamParser::ReadSubBlockMV() {
  int16_t h = int16_t(ReadMVComponent(false) * 2);
  int16_t w = int16_t(ReadMVComponent(true) * 2);
  return MotionVector(h, w);
}

InterMBHeader BitstreamParser::ReadInterMBHeader(
    const std::array<uint8_t, 4> &cnt) {
#ifdef DEBUG
  std::cerr << "[Debug] Enter ReadInterMBHeader()" << std::endl;
#endif
  std::array<uint8_t, 4> mv_ref_probs{};
  mv_ref_probs.at(0) = kModeProb.at(cnt.at(0)).at(0);
  mv_ref_probs.at(1) = kModeProb.at(cnt.at(1)).at(1);
  mv_ref_probs.at(2) = kModeProb.at(cnt.at(2)).at(2);
  mv_ref_probs.at(3) = kModeProb.at(cnt.at(3)).at(3);
  InterMBHeader result{};
  result.mv_mode = MacroBlockMV(bd_.Tree(mv_ref_probs, kMVRefTree));
  if (result.mv_mode == MV_SPLIT) {
    result.mv_split_mode =
        MVPartition(bd_.Tree(kMVPartitionProbs, kMVPartitionTree));
    // The rest is up to the caller to call ReadSubBlockMode() &
    // ReadSubBlockMV().
  } else if (result.mv_mode == MV_NEW) {
    int16_t h = int16_t(ReadMVComponent(false) * 2);
    int16_t w = int16_t(ReadMVComponent(true) * 2);
    result.mv_new = MotionVector(h, w);
  }
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (result.mv_mode != MV_SPLIT);
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (uint16_t(result.mv_mode) << 6);
  macroblock_metadata_idx_++;
#ifdef DEBUG
  std::cerr << "[Debug] Exit ReadInterMBHeader()" << std::endl;
#endif
  return result;
}

SubBlockMode BitstreamParser::ReadSubBlockBModeKF(int above_bmode,
                                                  int left_bmode) {
  auto x = SubBlockMode(bd_.Tree(
      kKeyFrameBModeProbs.at(size_t(above_bmode)).at(size_t(left_bmode)),
      kSubBlockModeTree));
  return x;
}

SubBlockMode BitstreamParser::ReadSubBlockBModeNonKF() {
  return SubBlockMode(bd_.Tree(kBModeProb, kSubBlockModeTree));
}

MacroBlockMode BitstreamParser::ReadIntraMB_UVModeKF() {
  return MacroBlockMode(bd_.Tree(kKeyFrameUVModeProb, kUVModeTree));
}

MacroBlockMode BitstreamParser::ReadIntraMB_UVModeNonKF() {
  return MacroBlockMode(
      bd_.Tree(context_.get().intra_chroma_prob, kUVModeTree));
}

IntraMBHeader BitstreamParser::ReadIntraMBHeaderKF() {
  IntraMBHeader result{};
  result.intra_y_mode =
      MacroBlockMode(bd_.Tree(kKeyFrameYModeProb, kKeyFrameYModeTree));
  // The rest is up to the caller to call ReadSubBlockBMode() &
  // ReadMB_UVMode().
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (result.intra_y_mode != B_PRED);
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (uint16_t(result.intra_y_mode) << 6);
  macroblock_metadata_idx_++;
  return result;
}

IntraMBHeader BitstreamParser::ReadIntraMBHeaderNonKF() {
  IntraMBHeader result{};
  result.intra_y_mode =
      MacroBlockMode(bd_.Tree(context_.get().intra_16x16_prob, kYModeTree));
  // The rest is up to the caller to call ReadSubBlockBMode() &
  // ReadMB_UVMode().
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (result.intra_y_mode != B_PRED);
  context_.get().mb_metadata.at(macroblock_metadata_idx_) |=
      (uint16_t(result.intra_y_mode) << 6);
  macroblock_metadata_idx_++;
  return result;
}

int16_t BitstreamParser::ReadMVComponent(bool kind) {
  auto p = context_.get().mv_prob.at(kind);
  int16_t a = 0;
  if (bd_.Bool(p.at(MVP_IS_SHORT))) {
    for (unsigned i = 0; i < 3; i++) {
      a += bd_.Bool(p.at(kMVPBits + i)) << i;
    }
    for (unsigned i = 9; i > 3; i--) {
      a += bd_.Bool(p.at(kMVPBits + i)) << i;
    }
    if ((a & 0xFFF0) == 0 || bd_.Bool(p.at(kMVPBits + 3))) {
      a += 1 << 3;
    }
  } else {
    a = int16_t(
        bd_.Tree(IteratorArray<decltype(p)>(p.begin() + MVP_SHORT, p.end()),
                 kSmallMVTree));
  }
  if (a) {
    bool sign = bd_.Bool(p.at(MVP_SIGN));
    if (sign) a = -a;
  }
  return a;
}

ResidualData BitstreamParser::ReadResidualData(
    const ResidualParam &residual_ctx) {
  if (mb_cur_col_ == context_.get().mb_num_cols) {
    mb_cur_row_++;
    mb_cur_col_ = 0;
    if (nbr_of_dct_partitions_ > 1) {
      cur_partition_++;
      if (cur_partition_ == nbr_of_dct_partitions_) {
        cur_partition_ = 0;
      }
    }
  }
  mb_cur_col_++;
  ensure(
      mb_cur_row_ < context_.get().mb_num_rows,
      "[Error] ReadResidualData: Consumed too many macroblocks; vomiting...");
  ensure(residual_macroblock_idx_ < macroblock_metadata_idx_,
         "[Error] ReadResidualData: Corresponding macroblock not yet read.");
  ResidualData result{};
  auto macroblock_metadata =
      context_.get().mb_metadata.at(residual_macroblock_idx_);
  auto first_coeff = (macroblock_metadata & 0x1) ? 1 : 0;
  result.has_y2 = first_coeff;
  residual_macroblock_idx_++;
  if (!((macroblock_metadata >> 1) & 0x1)) {
    std::array<bool, 25> non_zero{};
    if (macroblock_metadata & 0x1) {
      tie(result.dct_coeff.at(0), non_zero.at(0)) =
          ReadResidualBlock(1, residual_ctx.y2_nonzero);
    }
    unsigned block_type_y = first_coeff ? 0 : 3;
    for (unsigned i = 1; i <= 16; i++) {
      unsigned above_nonzero =
          (i <= 4) ? residual_ctx.y1_above.at(i - 1) : non_zero.at(i - 4);
      unsigned left_nonzero = ((i - 1) & 3)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.y1_left.at((i - 1) >> 2);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(block_type_y, above_nonzero + left_nonzero);
    }
    for (unsigned i = 17; i <= 20; i++) {
      unsigned above_nonzero =
          (i <= 18) ? residual_ctx.u_above.at(i - 17) : non_zero.at(i - 2);
      unsigned left_nonzero = ((i - 17) & 1)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.u_left.at((i - 17) >> 1);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(2, above_nonzero + left_nonzero);
    }
    for (unsigned i = 21; i <= 24; i++) {
      unsigned above_nonzero =
          (i <= 22) ? residual_ctx.v_above.at(i - 21) : non_zero.at(i - 2);
      unsigned left_nonzero = ((i - 21) & 1)
                                  ? non_zero.at(i - 1)
                                  : residual_ctx.v_left.at((i - 21) >> 1);
      std::tie(result.dct_coeff.at(i), non_zero.at(i)) =
          ReadResidualBlock(2, above_nonzero + left_nonzero);
    }
  }
  result.segment_id = (macroblock_metadata >> 2) & 0x3;
  int loop_filter_level = frame_header_.loop_filter_level;
  if (frame_header_.segmentation_enabled) {
    if (frame_header_.segment_feature_mode == SEGMENT_MODE_ABSOLUTE) {
      loop_filter_level =
          context_.get().loop_filter_level_segment.at(result.segment_id);
    } else {
      loop_filter_level +=
          context_.get().loop_filter_level_segment.at(result.segment_id);
    }
    loop_filter_level = std::clamp(loop_filter_level, 0, 63);
  }
  if (loop_filter_adj_enable_) {
    loop_filter_level +=
        context_.get().ref_frame_delta_lf.at((macroblock_metadata >> 4) & 0x3);
    auto prediction_mode = (macroblock_metadata >> 6) & 0x7;
    if (((macroblock_metadata >> 4) & 0x3) == CURRENT_FRAME) {
      if (prediction_mode == B_PRED) {
        loop_filter_level += context_.get().mb_mode_delta_lf.at(0);
      }
    } else if (prediction_mode == MV_ZERO) {
      loop_filter_level += context_.get().mb_mode_delta_lf.at(1);
    } else if (prediction_mode == MV_SPLIT) {
      loop_filter_level += context_.get().mb_mode_delta_lf.at(3);
    } else {
      loop_filter_level += context_.get().mb_mode_delta_lf.at(2);
    }
    loop_filter_level = std::clamp(loop_filter_level, 0, 63);
  }
  result.loop_filter_level = uint8_t(loop_filter_level);
  return result;
}

std::pair<std::array<int16_t, 16>, bool> BitstreamParser::ReadResidualBlock(
    unsigned block_type, unsigned zero_cnt) {
  std::array<int16_t, 16> result{};
  bool non_zero = false;
  bool last_zero = false;
  unsigned ctx3 = zero_cnt;
  for (unsigned n = (block_type == 0 ? 1 : 0); n < 16; n++) {
    unsigned i = kZigZag.at(n);
    auto &prob = context_.get()
                     .coeff_prob.get()
                     .at(block_type)
                     .at(kCoeffBands.at(n))
                     .at(ctx3);
    DctToken token;
    if (last_zero) {
      auto prob_no_eob =
          IteratorArray<std::remove_reference<decltype(prob)>::type>(
              prob.begin() + 1, prob.end());
      token = DctToken(
          residual_bd_.at(cur_partition_).Tree(prob_no_eob, kCoeffTreeNoEOB));
    } else {
      token = DctToken(residual_bd_.at(cur_partition_).Tree(prob, kCoeffTree));
    }
    if (token == DCT_EOB) {
      break;
    }
    result.at(i) = kTokenToCoeff.at(token);
    if (result.at(i) != DCT_0) {
      non_zero = true;
      last_zero = false;
      if (token >= DCT_CAT1) {
        size_t idx = size_t(token - DCT_CAT1);
        unsigned v = 0;
        for (unsigned j = 0; kPcat.at(idx).at(j); j++) {
          v += v + residual_bd_.at(cur_partition_).Bool(kPcat.at(idx).at(j));
        }
        result.at(i) += uint16_t(v);
      }
      ctx3 = result.at(i) > 1 ? 2 : unsigned(result.at(i));
      bool sign = residual_bd_.at(cur_partition_).LitU8(1);
      if (sign) {
        result.at(i) = -result.at(i);
      }
    } else {
      last_zero = true;
      ctx3 = 0;
    }
  }
  return make_pair(result, non_zero);
}

}  // namespace vp8
