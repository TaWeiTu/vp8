#ifndef BITSTREAM_PARSER_H_
#define BITSTREAM_PARSER_H_

#include <array>
#include <cstdint>
#include <memory>

#include "bool_decoder.h"

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
  bool y_dc_delta_present;
  uint8_t y_dc_delta_magnitude;
  bool y2_dc_delta_present;
  uint8_t y2_dc_delta_magnitude;
  bool y2_ac_delta_present;
  uint8_t y2_ac_delta_magnitude;
  bool uv_dc_delta_present;
  uint8_t uv_dc_delta_magnitude;
  bool uv_ac_delta_present;
  uint8_t uv_ac_delta_magnitude;
};

struct FrameHeader {
  // if (key_frame) {
  bool color_space;
  bool clamping_type;
  // }
  bool segmentation_enabled;
  // if (segmentation_enabled) {
  // update_segmentation
  // }
  bool filter_type;
  uint8_t loop_filter_level;
  uint8_t sharpness_level;
  // mb_lf_adjustments
  uint8_t log2_nbr_of_dct_partitions;
  QuantIndices quant_indices;
  // if (key_frame) {
  bool refresh_entropy_probs;
  // } else {
  bool refresh_golden_frame;
  bool refreh_alternate_frame;
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
  std::array<int16_t, 4> quantizer_segment;
  std::array<int16_t, 4> loop_filter_level_segment;
};

struct MacroBlockHeader {
	uint8_t segment_id;
	bool mb_skip_coeff;
	bool is_inter_mb;
	uint8_t ref_frame;
	uint8_t mv_mode;
	uint8_t mv_split_mode;
	std::array<uint8_t, 16> sub_mv_mode;
	uint8_t intra_y_mode;
	std::array<uint8_t, 16> intra_b_mode;
	uint8_t intra_uv_mode;
};

struct ResidualData {
  std::array<std::array<int16_t, 16>, 25> dct_coeff;
	uint8_t segment_id;
	int8_t loop_filter_level;
};

struct ParserContext {
  // TODO
	// Bit packed as following: 
	// (prediction_mode << 6) | (ref_frame << 4) |
	// (segment_id << 2) | (mb_skip_coeff << 1) |
	// (is_inter_mb && mv_mode == MV_SPLIT) || (!is_inter_mv && intra_y_mode != B_PRED)
	std::vector<uint8_t> mb_metadata;
	std::array<uint8_t, 4> intra_16x16_prob;
	std::array<uint8_t, 3> intra_chroma_prob;
	std::array<uint8_t, 3> segment_prob;
	std::array<int8_t, 4> ref_frame_delta_lf;
	std::array<int8_t, 4> mb_mode_delta_lf;
	using CoeffProbs std::array<std::array<std::array<std::array, 11>, 3>, 4>, 8>;
	std::reference_wrapper<CoeffProbs> coeff_prob;
	CoeffProbs coeff_prob_persistent;
	CoeffProbs coeff_prob_temp;
	std::array<std::array<uint8_t, 19>, 2> mv_prob;
	std::array<std::array<uint8_t, 10>, 2> mvc_probs;
};

class BitstreamParser {
 private:
  BoolDecoder bd_;
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

  void MvProbUpdate();

  std::array<int16_t, 16> ReadResidualBlock();

  void ReadMvComponent();

 public:
	BitstreamParser() = default;

  std::unique_ptr<BoolDecoder> DropStream();

  std::pair<FrameTag, FrameHeader> ReadFrameTagHeader();

  MacroblockHeader ReadMacroBlockHeader(
      const std::array<uint8_t, 4>& mv_ref_probs, int sub_mv_context);

  ResidualData ReadResidualData();

  const FrameTag& frame_tag() { return frame_tag_; }

  const FrameHeader& frame_header() { return frame_header_; }
};
}  // namespace vp8

#endif  // BITSTREAM_PARSER_H_
