#ifndef BITSTREAM_PARSER_H_
#define BITSTREAM_PARSER_H_

#include <array>
#include <cstdint>
#include <memory>

#include "bitstream_const.h"
#include "bool_decoder.h"
#include "frame.h"

namespace vp8 {

struct FrameTag {
  // frame_tag {
  bool key_frame;
  uint8_t version;
  bool show_frame;
  uint32_t first_part_size;
  // }
  // if (key_frame) {
  // start_code
  uint16_t height;
  uint16_t horizontal_scale;
  uint16_t width;
  uint16_t vertical_scale;
  // }
};

struct QuantIndices {
  uint8_t y_ac_qi;
  uint8_t y_dc_delta_q;
  uint8_t y2_dc_delta_q;
  uint8_t y2_ac_delta_q;
  uint8_t uv_dc_delta_q;
  uint8_t uv_ac_delta_q;
};

struct FrameHeader {
  // if (key_frame) {
  bool color_space;
  bool clamping_type;
  // }
  bool segmentation_enabled;
  // if (segmentation_enabled) {
  // update_segmentation
  SegmentMode segment_feature_mode;
  bool update_mb_segmentation_map;
  // }
  bool filter_type;
  uint8_t loop_filter_level;
  uint8_t sharpness_level;
  // mb_lf_adjustments
  uint8_t nbr_of_dct_partitions;
  QuantIndices quant_indices;
  // if (key_frame) {
  bool refresh_entropy_probs;
  // } else {
  bool refresh_golden_frame;
  bool refresh_alternate_frame;
  // if (!refresh_golden_frame) {
  uint8_t copy_buffer_to_golden;
  // }
  // if (!refresh_alternate_frame) {
  uint8_t copy_buffer_to_alternate;
  // }
  bool sign_bias_golden;
  bool sign_bias_alternate;
  // refresh_entropy_probs
  bool refresh_last;
  // }
  // token_prob_update
  bool mb_no_skip_coeff;
  // if (mb_no_skip_coeff) {
  uint8_t prob_skip_false;
  // }
  // if (!key_frame) {
  uint8_t prob_intra;
  uint8_t prob_last;
  uint8_t prob_gf;
  // intra_16x16_prob_update_flag
  // if (intra_chroma_prob_update_flag) {
  // intra_16x16_prob
  // }
  // intra_chroma_prob_update_flag
  // if (intra_chroma_prob_update_flag) {
  // intra_chroma_prob
  // }
  // mv_prob_update
  // }
  // --- Other fields not directly in the header ---
  std::array<int16_t, kMaxMacroBlockSegments> quantizer_segment;
  std::array<int16_t, kMaxMacroBlockSegments> loop_filter_level_segment;
};

struct MacroBlockPreHeader {
  uint8_t segment_id;
  bool mb_skip_coeff;
  bool is_inter_mb;
  // Only used when is_inter_mb == true
  uint8_t ref_frame;
};

struct InterMBHeader {
  MacroBlockMV mv_mode;
  MVPartition mv_split_mode;
  MotionVector mv_new;
};

struct IntraMBHeader {
  MacroBlockMode intra_y_mode;
};

struct ResidualData {
  std::array<std::array<int16_t, 16>, 25> dct_coeff;
  uint8_t segment_id;
  int8_t loop_filter_level;
};

struct ParserContext {
  // Bit packed as following:
  // (prediction_mode << 6) | (ref_frame << 4) |
  // (segment_id << 2) | (mb_skip_coeff << 1) |
  // (is_inter_mb && mv_mode == MV_SPLIT) || (!is_inter_mv && intra_y_mode !=
  // B_PRED)
  std::vector<uint8_t> mb_metadata;
  std::array<uint8_t, kNumYModeProb> intra_16x16_prob;
  std::array<uint8_t, kNumUVModeProb> intra_chroma_prob;
  std::array<uint8_t, kNumMacroBlockSegmentProb> segment_prob;
  std::array<int8_t, kNumRefFrames> ref_frame_delta_lf;
  std::array<int8_t, kNumLfPredictionDelta> mb_mode_delta_lf;
  using CoeffProbs =
      std::array<std::array<std::array<std::array<Prob, kNumCoeffProb>,
                                       kNumDctContextType>,
                            kNumCoeffBand>,
                 kNumBlockType>;
  CoeffProbs coeff_prob_persistent;
  CoeffProbs coeff_prob_temp;
  std::reference_wrapper<CoeffProbs> coeff_prob;
  std::array<std::array<uint8_t, kMVPCount>, kNumMVDimen> mv_prob;

  ParserContext()
      : mb_metadata(),
        intra_16x16_prob(kYModeProb),
        intra_chroma_prob(kUVModeProb),
        segment_prob(),        // Set to 255 if not updated
        ref_frame_delta_lf(),  // Deltas; normal to be zero
        mb_mode_delta_lf(),    // Deltas; normal to be zero
        coeff_prob_persistent(kDefaultCoeffProbs),
        coeff_prob_temp(),  // Temporary buffer; normal to be zero
        coeff_prob(std::ref(coeff_prob_persistent)),
        mv_prob(kDefaultMVContext) {}
};

struct ResidualParam {
  uint8_t y2_nonzero;
  std::array<bool, 4> y1_above;
  std::array<bool, 4> y1_left;
  std::array<bool, 2> u_above;
  std::array<bool, 2> u_left;
  std::array<bool, 2> v_above;
  std::array<bool, 2> v_left;

  ResidualParam() = default;
  ResidualParam(uint8_t y2_nonzero_, std::array<bool, 4> y1_above_,
                std::array<bool, 4> y1_left_, std::array<bool, 2> u_above_,
                std::array<bool, 2> u_left_, std::array<bool, 2> v_above_,
                std::array<bool, 2> v_left_)
      : y2_nonzero(y2_nonzero_),
        y1_above(y1_above_),
        y1_left(y1_left_),
        u_above(u_above_),
        u_left(u_left_),
        v_above(v_above_),
        v_left(v_left_) {}
};

class BitstreamParser {
 private:
  std::unique_ptr<BoolDecoder> bd_;
  FrameTag frame_tag_;
  FrameHeader frame_header_;
  ParserContext context_;
  size_t macroblock_metadata_idx, residual_macroblock_idx;

  FrameTag ReadFrameTag();

  FrameHeader ReadFrameHeader();

  void UpdateSegmentation();

  void MbLfAdjust();

  QuantIndices ReadQuantIndices();

  void TokenProbUpdate();

  void MVProbUpdate();

  std::pair<std::array<int16_t, 16>, bool> ReadResidualBlock(
      int first_coeff, const std::array<Prob, kNumCoeffProb>& prob);

  int16_t ReadMVComponent(bool kind);

 public:
  BitstreamParser() = default;

  std::unique_ptr<BoolDecoder> DropStream();

  std::pair<FrameTag, FrameHeader> ReadFrameTagHeader();

  MacroBlockPreHeader ReadMacroBlockPreHeader();

  SubBlockMVMode ReadSubBlockMVMode(uint8_t sub_mv_context);

  MotionVector ReadSubBlockMV();

  InterMBHeader ReadInterMBHeader(const std::array<uint8_t, 4>& cnt);

  SubBlockMode ReadSubBlockBModeKF(int above_bmode, int left_bmode);

  SubBlockMode ReadSubBlockBModeNonKF();

  MacroBlockMode ReadIntraMB_UVMode();

  IntraMBHeader ReadIntraMBHeader();

  ResidualData ReadResidualData(const ResidualParam& residual_ctx);

  const FrameTag& frame_tag() { return frame_tag_; }

  const FrameHeader& frame_header() { return frame_header_; }
};

}  // namespace vp8

#endif  // BITSTREAM_PARSER_H_
