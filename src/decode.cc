#include <array>
#include <cassert>
#include <memory>
#include <utility>

#include "bitstream_const.h"
#include "loop.h"
#include "reconstruct.h"
#include "utils.h"
#include "yuv.h"

int main(int argc, const char **argv) {
  ensure(argc == 3, "[Usage] ./decode [input] [output]");

  auto fs = std::ifstream(argv[1], std::ios::binary);
  auto read_bytes = [&fs](size_t n) {
    uint32_t res = 0;
    for (size_t i = 0; i < n; ++i) {
      res |= uint32_t(fs.get()) << (i << 3);
    }
    return res;
  };

  const uint32_t dkif = uint32_t('D') | (uint32_t('K') << 8) |
                        (uint32_t('I') << 16) | (uint32_t('F') << 24);
  const uint32_t vp80 = uint32_t('V') | (uint32_t('P') << 8) |
                        (uint32_t('8') << 16) | (uint32_t('0') << 24);
  assert(read_bytes(4) == dkif);
  assert(read_bytes(2) == 0);   // Version
  assert(read_bytes(2) == 32);  // Header length
  assert(read_bytes(4) == vp80);
  read_bytes(2);
  read_bytes(2);
  read_bytes(4);
  read_bytes(4);
  auto num_frames = read_bytes(4);
  read_bytes(4);  // Reserved bytes

  std::array<std::shared_ptr<vp8::Frame>, 4> ref_frames;
  std::array<bool, 4> ref_frame_bias{};

  vp8::ParserContext ctx{};

  vp8::YUV<vp8::WRITE> yuv(argv[2]);

  std::vector<uint8_t> buffer;

  size_t height = 0, width = 0;
  for (size_t frame_cnt = 0; frame_cnt < num_frames; frame_cnt++) {
    auto frame_size = read_bytes(4);
    read_bytes(4);
    read_bytes(4);
    // Decoding loop: reconstruct the frame and update the golden/altref frame
    // (if necessary).
    buffer.resize(frame_size);
    // TODO: (Improvement) This part is a bit ugly.
    fs.read(reinterpret_cast<char *>(buffer.data()), frame_size);
    vp8::BitstreamParser ps(
        vp8::SpanReader(buffer.data(), buffer.data() + buffer.size()), ctx);
    vp8::FrameHeader header;
    vp8::FrameTag tag;
    std::tie(tag, header) = ps.ReadFrameTagHeader();
    if (tag.key_frame) {
      height = tag.height;
      width = tag.width;
    }

    std::shared_ptr<vp8::Frame> &frame = ref_frames.at(vp8::CURRENT_FRAME);
    frame = std::make_shared<vp8::Frame>(height, width);

    vp8::InitSignBias(header, ref_frame_bias);
    vp8::Reconstruct(header, tag, ref_frames, ref_frame_bias, ps, frame);
    vp8::RefreshRefFrames(header, ref_frames);
    if (tag.show_frame) yuv.WriteFrame(frame);
  }
  return 0;
}
