#ifndef BITSTREAM_PARSER_H_
#define BITSTREAM_PARSER_H_

#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "bitstream_const.h"
#include "bool_decoder.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {

struct FrameTag {
  // frame_tag {
  bool key_frame;
  uint8_t version;
  bool show_frame;
  // first_part_size
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
  int16_t y_ac_qi;
  int16_t y_dc_delta_q;
  int16_t y2_dc_delta_q;
  int16_t y2_ac_delta_q;
  int16_t uv_dc_delta_q;
  int16_t uv_ac_delta_q;
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
  // nbr_of_dct_partitions
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
  uint8_t loop_filter_level;
  bool has_y2, is_zero;
};

struct ParserContext {
  // Bit packed as following:
  // (prediction_mode << 6) | (ref_frame << 4) |
  // (segment_id << 2) | (mb_skip_coeff << 1) |
  // (is_inter_mb && mv_mode == MV_SPLIT) || (!is_inter_mv && intra_y_mode !=
  // B_PRED)
  std::vector<uint16_t> mb_metadata;
  std::vector<uint8_t> segment_id;
  std::array<uint8_t, kNumYModeProb> intra_16x16_prob_persistent;
  std::array<uint8_t, kNumYModeProb> intra_16x16_prob_temp;
  std::reference_wrapper<std::array<uint8_t, kNumYModeProb>> intra_16x16_prob;
  std::array<uint8_t, kNumUVModeProb> intra_chroma_prob_persistent;
  std::array<uint8_t, kNumUVModeProb> intra_chroma_prob_temp;
  std::reference_wrapper<std::array<uint8_t, kNumUVModeProb>> intra_chroma_prob;
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
  std::array<std::array<uint8_t, kMVPCount>, kNumMVDimen> mv_prob_persistent;
  std::array<std::array<uint8_t, kMVPCount>, kNumMVDimen> mv_prob_temp;
  std::reference_wrapper<
      std::array<std::array<uint8_t, kMVPCount>, kNumMVDimen>>
      mv_prob;
  SegmentMode segment_feature_mode;
  std::array<int16_t, kMaxMacroBlockSegments> quantizer_segment;
  std::array<int16_t, kMaxMacroBlockSegments> loop_filter_level_segment;
  uint16_t mb_num_rows, mb_num_cols;

  ParserContext()
      : mb_metadata(),
        segment_id(),
        intra_16x16_prob_persistent(kYModeProb),
        intra_16x16_prob_temp(),  // Temporary buffer; normal to be zero
        intra_16x16_prob(std::ref(intra_16x16_prob_persistent)),
        intra_chroma_prob_persistent(kUVModeProb),
        intra_chroma_prob_temp(),  // Temporary buffer; normal to be zero
        intra_chroma_prob(std::ref(intra_chroma_prob_persistent)),
        segment_prob(),        // Set to 255 if not updated
        ref_frame_delta_lf(),  // Deltas; normal to be zero
        mb_mode_delta_lf(),    // Deltas; normal to be zero
        coeff_prob_persistent(kDefaultCoeffProbs),
        coeff_prob_temp(),  // Temporary buffer; normal to be zero
        coeff_prob(std::ref(coeff_prob_persistent)),
        mv_prob_persistent(kDefaultMVContext),
        mv_prob_temp(),  // Temporary buffer; normal to be zero
        mv_prob(std::ref(mv_prob_persistent)),
        segment_feature_mode(),
        quantizer_segment(),
        loop_filter_level_segment(),
        mb_num_rows(),
        mb_num_cols() {}
};

struct ResidualParam {
  uint8_t y2_nonzero;
  uint8_t y1_above, y1_left;
  uint8_t u_above, u_left;
  uint8_t v_above, v_left;

  ResidualParam() = default;
  explicit ResidualParam(uint8_t y2_nonzero_, uint8_t y1_above_,
                         uint8_t y1_left_, uint8_t u_above_, uint8_t u_left_,
                         uint8_t v_above_, uint8_t v_left_)
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
  SpanReader<uint8_t> buffer_;
  BoolDecoder bd_;
  FrameTag frame_tag_;
  FrameHeader frame_header_;
  std::reference_wrapper<ParserContext> context_;
  size_t macroblock_metadata_idx_, residual_macroblock_idx_;
  uint8_t nbr_of_dct_partitions_, cur_partition_;
  uint16_t mb_cur_row_, mb_cur_col_;
  uint32_t first_part_size_;
  std::array<BoolDecoder, 8> residual_bd_;
  bool loop_filter_adj_enable_;

  FrameHeader ReadFrameHeader();

  void UpdateSegmentation();

  void MbLfAdjust();

  QuantIndices ReadQuantIndices();

  void TokenProbUpdate();

  void MVProbUpdate();

  std::pair<std::array<int16_t, 16>, bool> ReadResidualBlock(
      unsigned block_type, unsigned zero_cnt);

  int16_t ReadMVComponent(bool kind);

 public:
  BitstreamParser(SpanReader<uint8_t> buffer, ParserContext& ctx)
      : buffer_(buffer),
        bd_(),
        frame_tag_(),
        frame_header_(),
        context_(std::ref(ctx)),
        macroblock_metadata_idx_(),
        residual_macroblock_idx_(),
        nbr_of_dct_partitions_(),
        cur_partition_(),
        mb_cur_row_(),
        mb_cur_col_(),
        first_part_size_(),
        loop_filter_adj_enable_() {}

  std::pair<FrameTag, FrameHeader> ReadFrameTagHeader();

  MacroBlockPreHeader ReadMacroBlockPreHeader();

  SubBlockMVMode ReadSubBlockMVMode(uint8_t sub_mv_context);

  MotionVector ReadSubBlockMV();

  InterMBHeader ReadInterMBHeader(const std::array<uint8_t, 4>& cnt);

  SubBlockMode ReadSubBlockBModeKF(int above_bmode, int left_bmode);

  SubBlockMode ReadSubBlockBModeNonKF();

  MacroBlockMode ReadIntraMB_UVModeKF();

  MacroBlockMode ReadIntraMB_UVModeNonKF();

  IntraMBHeader ReadIntraMBHeaderKF();

  IntraMBHeader ReadIntraMBHeaderNonKF();

  ResidualData ReadResidualData(const ResidualParam& residual_ctx);

  FrameTag ReadFrameTag();

  const FrameTag& frame_tag() { return frame_tag_; }

  const FrameHeader& frame_header() { return frame_header_; }
};

}  // namespace vp8

#endif  // BITSTREAM_PARSER_H_
