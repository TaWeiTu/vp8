#include <array>

#include "bitstream_const.h"
#include "reconstruct.h"

void InitSignBias(const vp8::FrameHeader &header,
                  std::array<bool, 4> &ref_frame_bias) {
  ref_frame_bias[vp8::GOLDEN_FRAME] = header.sign_bias_golden;
  ref_frame_bias[vp8::ALTREF_FRAME] = header.sign_bias_alternate;
}

void RefreshRefFrames(const vp8::FrameHeader &header,
                      std::array<vp8::Frame, 4> &ref_frames,
                      const vp8::Frame &frame) {
  if (header.refresh_golden_frame) {
    ref_frames[vp8::GOLDEN_FRAME] = frame;
  } else {
    if (header.copy_buffer_to_golden == 1)
      ref_frames[vp8::GOLDEN_FRAME] = ref_frames[vp8::LAST_FRAME];
    else if (header.copy_buffer_to_golden == 2)
      ref_frames[vp8::GOLDEN_FRAME] = ref_frames[vp8::ALTREF_FRAME];
  }
  if (header.refresh_alternate_frame) {
    ref_frames[vp8::ALTREF_FRAME] = frame;
  } else {
    if (header.copy_buffer_to_alternate == 1)
      ref_frames[vp8::ALTREF_FRAME] = ref_frames[vp8::LAST_FRAME];
    else if (header.copy_buffer_to_alternate == 2)
      ref_frames[vp8::ALTREF_FRAME] = ref_frames[vp8::GOLDEN_FRAME];
  }
  if (header.refresh_last) ref_frames[vp8::LAST_FRAME] = frame;
}

int main(int argc, const char **argv) {
  // Avoid unused function.
  std::array<vp8::Frame, 4> ref_frames;
  std::array<bool, 4> ref_frame_bias;

  // Decoding loop: reconstruct the frame and update the golden/altref frame (if
  // necessary).
  vp8::FrameHeader header;
  vp8::FrameTag tag;
  vp8::BitstreamParser ps;
  vp8::Frame frame;

  InitSignBias(header, ref_frame_bias);
  vp8::Reconstruct(header, tag, ref_frames, ref_frame_bias, ps, frame);
  RefreshRefFrames(header, ref_frames, frame);
}
