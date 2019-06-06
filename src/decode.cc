#include <array>
#include <cassert>

#include "bitstream_const.h"
#include "reconstruct.h"
#include "yuv.h"

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
  assert(argc > 2);

  auto fs = std::make_unique<std::ifstream>(argv[1], std::ios::binary);
  auto bd = std::make_unique<vp8::BoolDecoder>(std::move(fs));

  const uint32_t dkif = uint32_t('D') | (uint32_t('K') << 8) |
                        (uint32_t('I') << 16) | (uint32_t('F') << 24);
  const uint32_t vp80 = uint32_t('V') | (uint32_t('P') << 8) |
                        (uint32_t('8') << 16) | (uint32_t('0') << 24);
  assert(bd->Raw(4) == dkif);
  assert(bd->Raw(2) == 0);   // Version
  assert(bd->Raw(2) == 32);  // Header length
  assert(bd->Raw(4) == vp80);
  auto width = bd->Raw(2), height = bd->Raw(2), frame_rate = bd->Raw(4),
       time_scale = bd->Raw(4), num_frames = bd->Raw(4);
  bd->Raw(4);  // Reserved bytes

  // Avoid unused function.
  std::array<vp8::Frame, 4> ref_frames{};
  std::array<bool, 4> ref_frame_bias{};

  vp8::ParserContext ctx{};

  vp8::YUV yuv(argv[2]);

  for (size_t frame_cnt = 0; frame_cnt < num_frames; frame_cnt++) {
    bd->Raw(4);
    bd->Raw(4);
    bd->Raw(4);
    // Decoding loop: reconstruct the frame and update the golden/altref frame
    // (if necessary).
    vp8::BitstreamParser ps(std::move(bd), ctx);
    vp8::FrameHeader header;
    vp8::FrameTag tag;
    std::tie(tag, header) = ps.ReadFrameTagHeader();

    vp8::Frame frame;

    InitSignBias(header, ref_frame_bias);
    vp8::Reconstruct(header, tag, ref_frames, ref_frame_bias, ps, frame);
    RefreshRefFrames(header, ref_frames, frame);

    yuv.WriteFrame(frame);

    std::tie(ctx, bd) = ps.DropStream();
  }
  return 0;
}
