#include <array>

#include "reconstruct.h"

int main() {
  // Avoid unused function.
  vp8::FrameHeader header;
  vp8::FrameTag tag;
  std::array<vp8::Frame, 4> refs;
  std::array<bool, 4> ref_frame_bias;
  vp8::BitstreamParser ps;
  vp8::Frame frame;
  vp8::Reconstruct(header, tag, refs, ref_frame_bias, ps, frame);
}
