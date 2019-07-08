#ifndef LOOP_H_
#define LOOP_H_

#include "bitstream_const.h"
#include "bitstream_parser.h"
#include "frame.h"

namespace vp8 {

// Set the sign-bias of both GOLDEN and ALTREF reference frame.
void InitSignBias(const FrameHeader &header,
                  std::array<bool, 4> &ref_frame_bias) {
  ref_frame_bias.at(GOLDEN_FRAME) = header.sign_bias_golden;
  ref_frame_bias.at(ALTREF_FRAME) = header.sign_bias_alternate;
}

void RefreshRefFrames(const FrameHeader &header,
                      std::array<Frame, 4> &ref_frames) {
  bool golden_to_altref =
      !header.refresh_alternate_frame && header.copy_buffer_to_alternate == 2;
  bool altref_to_golden =
      !header.refresh_golden_frame && header.copy_buffer_to_golden == 2;

  if (golden_to_altref && altref_to_golden) {
    std::swap(ref_frames.at(GOLDEN_FRAME), ref_frames.at(ALTREF_FRAME));
  } else if (golden_to_altref) {
    ref_frames.at(ALTREF_FRAME) = ref_frames.at(GOLDEN_FRAME);
  } else if (altref_to_golden) {
    ref_frames.at(GOLDEN_FRAME) = ref_frames.at(ALTREF_FRAME);
  }

  if (header.refresh_golden_frame)
    ref_frames.at(GOLDEN_FRAME) = ref_frames.at(CURRENT_FRAME);
  else if (header.copy_buffer_to_golden == 1)
    ref_frames.at(GOLDEN_FRAME) = ref_frames.at(LAST_FRAME);

  if (header.refresh_alternate_frame)
    ref_frames.at(ALTREF_FRAME) = ref_frames.at(CURRENT_FRAME);
  else if (header.copy_buffer_to_alternate == 1)
    ref_frames.at(ALTREF_FRAME) = ref_frames.at(LAST_FRAME);

  if (header.refresh_last)
    ref_frames.at(LAST_FRAME) = ref_frames.at(CURRENT_FRAME);
}

}  // namespace vp8

#endif  // LOOP_H_
