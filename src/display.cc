#include <array>
#include <cassert>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "bitstream_const.h"
#include "reconstruct.h"

// Set the sign-bias of both GOLDEN and ALTREF reference frame.
void InitSignBias(const vp8::FrameHeader &header,
                  std::array<bool, 4> &ref_frame_bias) {
  ref_frame_bias.at(vp8::GOLDEN_FRAME) = header.sign_bias_golden;
  ref_frame_bias.at(vp8::ALTREF_FRAME) = header.sign_bias_alternate;
}

void RefreshRefFrames(const vp8::FrameHeader &header,
                      std::array<vp8::Frame, 4> &ref_frames,
                      const vp8::Frame &frame) {
  if (header.refresh_golden_frame) {
    ref_frames.at(vp8::GOLDEN_FRAME) = frame;
  } else {
    if (header.copy_buffer_to_golden == 1)
      ref_frames.at(vp8::GOLDEN_FRAME) = ref_frames.at(vp8::LAST_FRAME);
    else if (header.copy_buffer_to_golden == 2)
      ref_frames.at(vp8::GOLDEN_FRAME) = ref_frames.at(vp8::ALTREF_FRAME);
  }
  if (header.refresh_alternate_frame) {
    ref_frames.at(vp8::ALTREF_FRAME) = frame;
  } else {
    if (header.copy_buffer_to_alternate == 1)
      ref_frames.at(vp8::ALTREF_FRAME) = ref_frames.at(vp8::LAST_FRAME);
    else if (header.copy_buffer_to_alternate == 2)
      ref_frames.at(vp8::ALTREF_FRAME) = ref_frames.at(vp8::GOLDEN_FRAME);
  }
  if (header.refresh_last) {
    ref_frames.at(vp8::LAST_FRAME) = frame;
  }
}

int main(int argc, const char **argv) {
  assert(argc > 1);

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
  auto width_ = read_bytes(2), height_ = read_bytes(2),
       frame_rate = read_bytes(4), time_scale = read_bytes(4),
       num_frames = read_bytes(4);
  read_bytes(4);  // Reserved bytes

  std::array<vp8::Frame, 4> ref_frames{};
  std::array<bool, 4> ref_frame_bias{};

  vp8::ParserContext ctx{};

  std::vector<uint8_t> buffer;

  size_t height = 0, width = 0;

  cv::namedWindow(argv[1], cv::WINDOW_AUTOSIZE);

  bool fast_forward = false;

  for (size_t frame_cnt = 0; frame_cnt < num_frames; frame_cnt++) {
    if (fast_forward) {
      auto cur_frame = frame_cnt;
      while (true) {
        long pos = fs.tellg();
        auto frame_size = read_bytes(4);
        read_bytes(4);
        read_bytes(4);
        buffer.resize(frame_size);
        // TODO: (Improvement) This part is a bit ugly.
        fs.read(reinterpret_cast<char *>(buffer.data()), frame_size);
        vp8::BitstreamParser ps(
            vp8::SpanReader(buffer.data(), buffer.data() + buffer.size()), ctx);
        auto tag = ps.ReadFrameTag();
        fs.seekg(pos);
        if (tag.key_frame && cur_frame - frame_cnt > 30) {
          break;
        }
        fs.seekg(4 + 4 + 4 + frame_size, std::ios::cur);
        cur_frame++;
      }
      frame_cnt = cur_frame;
      fast_forward = false;
    }
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

    vp8::Frame frame(height, width);

    InitSignBias(header, ref_frame_bias);
    vp8::Reconstruct(header, tag, ref_frames, ref_frame_bias, ps, frame);
    RefreshRefFrames(header, ref_frames, frame);

    if (!tag.show_frame) continue;
    cv::Mat mYUV((height + (height >> 1)), width, CV_8UC1);
    auto it = mYUV.begin<uint8_t>();

    for (size_t r = 0; r < frame.vsize; ++r) {
      for (size_t c = 0; c < frame.hsize; ++c) {
        uint8_t y =
            uint8_t(frame.Y.at(r >> 4).at(c >> 4).GetPixel(r & 15, c & 15));
        *it++ = y;
      }
    }
    for (size_t r = 0; r < frame.vsize; r += 2) {
      for (size_t c = 0; c < frame.hsize; c += 2) {
        uint8_t u = uint8_t(
            frame.U.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
        *it++ = u;
      }
    }
    for (size_t r = 0; r < frame.vsize; r += 2) {
      for (size_t c = 0; c < frame.hsize; c += 2) {
        int8_t v = uint8_t(
            frame.V.at(r >> 4).at(c >> 4).GetPixel((r >> 1) & 7, (c >> 1) & 7));
        *it++ = v;
      }
    }

    cv::Mat mRGB(height, width, CV_8UC3);
    cv::cvtColor(mYUV, mRGB, cv::COLOR_YUV2BGR_I420, 3);
    cv::imshow(argv[1], mRGB);
    if (cv::waitKey(25) == 83) {
      fast_forward = true;
    }
  }
  return 0;
}
