#include "inter_predict.h"

namespace vp8 {
namespace {

MacroBlockMV SearchMVs(size_t r, size_t c, const FrameHeader &header,
                       const Plane<4> &mb, MotionVector &best,
                       MotionVector &nearest, MotionVector &near) {
  static std::array<uint8_t, 4> cnt;
  std::fill(cnt.begin(), cnt.end(), 0);
  std::vector<MotionVector> mv;

  auto &mh = header.macroblock_header;

  if (r == 0 || mh.at(r - 1).at(c).is_inter_mb) {
    MotionVector v = r ? mb.at(r - 1).at(c).GetMotionVector() : kZero;
    if (r > 0) v = Invert(v, mh.at(r - 1).at(c));
    if (v != kZero) mv.push_back(v);
    cnt.at(mv.size()) += 2;
  }

  if (c == 0 || mh.at(r).at(c - 1).is_inter_mb) {
    MotionVector v = c ? mb.at(r).at(c - 1).GetMotionVector() : kZero;
    if (c > 0) v = Invert(v, mh.at(r).at(c - 1));
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt.at(mv.size()) += 2;
    } else {
      cnt.at(0) += 2;
    }
  }

  if (r == 0 || c == 0 || mh.at(r - 1).at(c - 1).is_inter_mb) {
    MotionVector v = r && c ? mb.at(r - 1).at(c - 1).GetMotionVector() : kZero;
    if (r > 0 && c > 0) v = Invert(v, mh.at(r - 1).at(c - 1));
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt.at(mv.size()) += 1;
    } else {
      cnt.at(0) += 1;
    }
  }

  // found three distinct motion vectors
  if (mv.size() == 3u && mv.at(2) == mv.at(0)) ++cnt.at(1);
  // unfound motion vectors are set to ZERO
  while (mv.size() < 3u) mv.push_back(kZero);

  cnt.at(3) = (r > 0 && mh.at(r - 1).at(c).mode == MV_SPLIT) +
              (c > 0 && mh.at(r).at(c - 1).mode == MV_SPLIT) * 2 +
              (r > 0 && c > 0 && mh.at(r - 1).at(c - 1).mode == MV_SPLIT);

  if (cnt.at(2) > cnt.at(1)) {
    std::swap(cnt.at(1), cnt.at(2));
    std::swap(mv.at(0), mv.at(1));
  }
  if (cnt.at(1) >= cnt.at(0)) mv.at(0) = mv.at(1);
  best = mv.at(0);
  nearest = mv.at(1);
  near = mv.at(2);
  return ReadMode(cnt);
}

void ClampMV(MotionVector &mv, int16_t left, int16_t right, int16_t up,
             int16_t down) {
  mv.dr = std::clamp(mv.dr, up, down);
  mv.dc = std::clamp(mv.dr, left, right);
}

uint8_t SubBlockProb(const MotionVector &left, const MotionVector &above) {
  uint8_t lez = (left.dr == 0 && left.dc == 0);
  uint8_t aez = (above.dr == 0 && above.dc == 0);
  uint8_t lea = (left.dr == above.dr && left.dc == above.dc);

  if (lea && lez) return 4;
  if (lea) return 3;
  if (aez) return 2;
  if (lez) return 1;
  return 0;
}

void ConfigureChromaMVs(const MacroBlock<4> &luma, bool trim,
                        MacroBlock<2> &chroma) {
  for (size_t r = 0; r < 2; ++r) {
    for (size_t c = 0; c < 2; ++c) {
      MotionVector ulv = luma.at(r << 1).at(c << 1).GetMotionVector(),
                   urv = luma.at(r << 1).at(c << 1 | 1).GetMotionVector(),
                   dlv = luma.at(r << 1 | 1).at(c << 1).GetMotionVector(),
                   drv = luma.at(r << 1 | 1).at(c << 1 | 1).GetMotionVector();

      int16_t sr = int16_t(ulv.dr + urv.dr + dlv.dr + drv.dr);
      int16_t sc = int16_t(ulv.dc + urv.dc + dlv.dc + drv.dc);
      int16_t dr = (sr >= 0 ? (sr + 4) >> 3 : -((-sr + 4) >> 3));
      int16_t dc = (sc >= 0 ? (sc + 4) >> 3 : -((-sc + 4) >> 3));
      if (trim) {
        dr = dr & (-7);
        dc = dc & (-7);
      }
      chroma.at(r).at(c).SetMotionVector(dr, dc);
    }
  }
}

void ConfigureSubBlockMVs(MVPartition p, size_t r, size_t c, Plane<4> &mb) {
  std::vector<std::vector<uint8_t>> part;
  switch (p) {
    case MV_TOP_BOTTOM:
      part.push_back({0, 1, 2, 3, 4, 5, 6, 7});
      part.push_back({8, 9, 10, 11, 12, 13, 14, 15});
      break;

    case MV_LEFT_RIGHT:
      part.push_back({0, 1, 4, 5, 8, 9, 12, 13});
      part.push_back({2, 3, 6, 7, 10, 11, 14, 15});
      break;

    case MV_QUARTERS:
      part.push_back({0, 1, 4, 5});
      part.push_back({2, 3, 6, 7});
      part.push_back({8, 9, 12, 13});
      part.push_back({10, 11, 14, 15});
      break;

    case MV_16:
      for (uint8_t i = 0; i < 16; ++i) part.push_back({i});
      break;
  }

  for (size_t i = 0; i < part.size(); ++i) {
    SubBlockMV mode = ReadSubBlockMV();
    for (size_t j = 0; j < part.at(i).size(); ++j) {
      size_t ir = part.at(i).at(j) >> 2, ic = part.at(i).at(j) & 3;
      MotionVector mv;
      switch (mode) {
        case LEFT_4x4:
          mv = ic == 0
                   ? (c == 0 ? kZero : mb.at(r).at(c - 1).GetSubBlockMV(ir, 3))
                   : mb.at(r).at(c).GetSubBlockMV(ir, ic - 1);
          break;

        case ABOVE_4x4:
          mv = ir == 0
                   ? (r == 0 ? kZero : mb.at(r - 1).at(c).GetSubBlockMV(3, ir))
                   : mb.at(r).at(c).GetSubBlockMV(ir - 1, ic);
          break;

        case ZERO_4x4:
          mv = kZero;
          break;

        case NEW_4x4:
          mv = ReadMotionVector() + mb.at(r).at(c).GetMotionVector();
          break;
      }
      mb.at(r).at(c).at(ir).at(ic).SetMotionVector(mv);
    }
  }
}

void ConfigureMVs(const FrameHeader &header, size_t r, size_t c, bool trim,
                  Frame &frame) {
  int16_t left = int16_t(-(1 << 7)), right = int16_t(frame.hblock);
  int16_t up = int16_t(-((r + 1) << 7)),
          down = int16_t((frame.vblock - r) << 7);
  MotionVector best, nearest, near;
  MacroBlockMV mode = SearchMVs(r, c, header, frame.Y, best, nearest, near);
  ClampMV(best, left, right, up, down);
  ClampMV(nearest, left, right, up, down);
  ClampMV(near, left, right, up, down);

  switch (mode) {
    case MV_NEAREST:
      frame.Y.at(r).at(c).SetSubBlockMVs(nearest);
      frame.Y.at(r).at(c).SetMotionVector(nearest);
      break;

    case MV_NEAR:
      frame.Y.at(r).at(c).SetSubBlockMVs(near);
      frame.Y.at(r).at(c).SetMotionVector(near);
      break;

    case MV_ZERO:
      frame.Y.at(r).at(c).SetSubBlockMVs(kZero);
      frame.Y.at(r).at(c).SetMotionVector(kZero);
      break;

    case MV_NEW:
      // TODO: Read motion vector in mode MV_NEW.
      MotionVector mv = ReadMotionVector() + best;
      frame.Y.at(r).at(c).SetSubBlockMVs(mv);
      frame.Y.at(r).at(c).SetMotionVector(mv);
      break;

    case MV_SPLIT:
      // TODO: Read how the macroblock is splitted.
      MVPartition part = ReadMVSplit();
      ConfigureSubBlockMVs(part, r, c, frame.Y);
      frame.Y.at(r).at(c).SetMotionVector(
          frame.Y.at(r).at(c).GetSubBlockMV(15));
  }
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.U.at(r).at(c));
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.V.at(r).at(c));
}

template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorSixtap(
    const Plane<C> &refer, size_t r, size_t c,
    const std::array<int16_t, 6> &filter) {
  std::array<std::array<int16_t, 4>, 9> res;
  for (size_t i = 0; i < 9; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(refer.GetPixel(r + i, c + j - 2)) * filter.at(0) +
                    int32_t(refer.GetPixel(r + i, c + j - 1)) * filter.at(1) +
                    int32_t(refer.GetPixel(r + i, c + j + 0)) * filter.at(2) +
                    int32_t(refer.GetPixel(r + i, c + j + 1)) * filter.at(3) +
                    int32_t(refer.GetPixel(r + i, c + j + 2)) * filter.at(4) +
                    int32_t(refer.GetPixel(r + i, c + j + 3)) * filter.at(5);
      res.at(i).at(j) = int16_t((sum + 64) >> 7);
    }
  }
  return res;
}

void VerSixtap(const std::array<std::array<int16_t, 4>, 9> &refer, size_t r,
               size_t c, const std::array<int16_t, 6> &filter, SubBlock &sub) {
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(refer.at(r + i + 0).at(c + j)) * filter.at(0) +
                    int32_t(refer.at(r + i + 1).at(c + j)) * filter.at(1) +
                    int32_t(refer.at(r + i + 2).at(c + j)) * filter.at(2) +
                    int32_t(refer.at(r + i + 3).at(c + j)) * filter.at(3) +
                    int32_t(refer.at(r + i + 4).at(c + j)) * filter.at(4) +
                    int32_t(refer.at(r + i + 5).at(c + j)) * filter.at(5);
      sub.at(i).at(j) = int16_t((sum + 64) >> 7);
    }
  }
}

template <size_t C>
void Sixtap(const Plane<C> &refer, size_t r, size_t c, uint8_t mr, uint8_t mc,
            const std::array<std::array<int16_t, 6>, 8> &filter,
            SubBlock &sub) {
  std::array<std::array<int16_t, 4>, 9> tmp =
      HorSixtap(refer, r - 2, c, filter.at(mr));
  VerSixtap(tmp, r, c, filter.at(mc), sub);
}

template <size_t C>
void InterpBlock(const Plane<C> &refer,
                 const std::array<std::array<int16_t, 6>, 8> &filter, size_t r,
                 size_t c, MacroBlock<C> &mb) {
  size_t offset = C / 2 + 2;
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j) {
      MotionVector mv = mb.at(i).at(j).GetMotionVector();
      if (mv == kZero) {
        for (size_t x = 0; x < C; ++x) {
          for (size_t y = 0; y < C; ++y)
            mb.at(i).at(j).at(x).at(y) = refer.GetPixel(
                (r << offset) | (i << 2) | x, (c << offset) | (j << 2) | y);
        }
        continue;
      }
      uint8_t mr = mv.dr & 7, mc = mv.dc & 7;
      ensure(int32_t(r << offset | (i << 2)) + (mv.dr >> 3) >= 0,
             "[Error] InterpBlock: the motion vectors is out of bound.");
      ensure(int32_t(c << offset | (j << 2)) + (mv.dc >> 3) >= 0,
             "[Error] InterpBlock: the motion vectors is out of bound.");
      size_t tr = size_t(int32_t(r << offset | (i << 2)) + (mv.dr >> 3));
      size_t tc = size_t(int32_t(c << offset | (j << 2)) + (mv.dc >> 3));
      if (mr | mc) Sixtap(refer, tr, tc, mr, mc, filter, mb.at(r).at(c));
    }
  }
}

// Ugly hacks to allow definitions of template funcitons in the .cc file.
template std::array<std::array<int16_t, 4>, 9> HorSixtap(
    const Plane<4> &, size_t, size_t, const std::array<int16_t, 6> &);
template std::array<std::array<int16_t, 4>, 9> HorSixtap(
    const Plane<2> &, size_t, size_t, const std::array<int16_t, 6> &);

template void Sixtap(const Plane<4> &, size_t, size_t, uint8_t, uint8_t,
                     const std::array<std::array<int16_t, 6>, 8> &, SubBlock &);
template void Sixtap(const Plane<2> &, size_t, size_t, uint8_t, uint8_t,
                     const std::array<std::array<int16_t, 6>, 8> &, SubBlock &);

template void InterpBlock(const Plane<4> &,
                          const std::array<std::array<int16_t, 6>, 8> &, size_t,
                          size_t, MacroBlock<4> &);
template void InterpBlock(const Plane<2> &,
                          const std::array<std::array<int16_t, 6>, 8> &, size_t,
                          size_t, MacroBlock<2> &);

}  // namespace

void InterPredict(const FrameHeader &header, const FrameTag &tag, size_t r,
                  size_t c, Frame &frame) {
  ConfigureMVs(header, r, c, tag.version == 3, frame);
  InterpBlock(header.ref_frame.Y, header.subpixel_filters, r, c,
              frame.Y.at(r).at(c));
  InterpBlock(header.ref_frame.U, header.subpixel_filters, r, c,
              frame.U.at(r).at(c));
  InterpBlock(header.ref_frame.V, header.subpixel_filters, r, c,
              frame.V.at(r).at(c));
}

}  // namespace vp8
