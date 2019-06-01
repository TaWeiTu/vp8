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

  if (r == 0 || mh[r - 1][c].is_inter_mb) {
    MotionVector v = r ? mb[r - 1][c].GetMotionVector() : kZero;
    if (r > 0) v = Invert(v, mh[r - 1][c]);
    if (v != kZero) mv.push_back(v);
    cnt[mv.size()] += 2;
  }

  if (c == 0 || mh[r][c - 1].is_inter_mb) {
    MotionVector v = c ? mb[r][c - 1].GetMotionVector() : kZero;
    if (c > 0) v = Invert(v, mh[r][c - 1]);
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt[mv.size()] += 2;
    } else {
      cnt[0] += 2;
    }
  }

  if (r == 0 || c == 0 || mh[r - 1][c - 1].is_inter_mb) {
    MotionVector v = r && c ? mb[r - 1][c - 1].GetMotionVector() : kZero;
    if (r > 0 && c > 0) v = Invert(v, mh[r - 1][c - 1]);
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt[mv.size()] += 1;
    } else {
      cnt[0] += 1;
    }
  }

  // found three distinct motion vectors
  if (mv.size() == 3u && mv[2] == mv[0]) ++cnt[1];
  // unfound motion vectors are set to ZERO
  while (mv.size() < 3u) mv.push_back(kZero);

  cnt[3] = (r > 0 && mh[r - 1][c].mode == MV_SPLIT) +
           (c > 0 && mh[r][c - 1].mode == MV_SPLIT) * 2 +
           (r > 0 && c > 0 && mh[r - 1][c - 1].mode == MV_SPLIT);

  if (cnt[2] > cnt[1]) {
    std::swap(cnt[1], cnt[2]);
    std::swap(mv[0], mv[1]);
  }
  if (cnt[1] >= cnt[0]) mv[0] = mv[1];
  best = mv[0];
  nearest = mv[1];
  near = mv[2];
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

void ConfigureChromaMVs(const MacroBlock<4> &luma, bool trim, MacroBlock<2> &chroma) {
  for (size_t r = 0; r < 2; ++r) {
    for (size_t c = 0; c < 2; ++c) {
      MotionVector ulv = luma[r << 1][c << 1].GetMotionVector(),
                   urv = luma[r << 1][c << 1 | 1].GetMotionVector(),
                   dlv = luma[r << 1 | 1][c << 1].GetMotionVector(),
                   drv = luma[r << 1 | 1][c << 1 | 1].GetMotionVector();

      int16_t sr = int16_t(ulv.dr + urv.dr + dlv.dr + drv.dr);
      int16_t sc = int16_t(ulv.dc + urv.dc + dlv.dc + drv.dc);
      int16_t dr = (sr >= 0 ? (sr + 4) >> 3 : -((-sr + 4) >> 3));
      int16_t dc = (sc >= 0 ? (sc + 4) >> 3 : -((-sc + 4) >> 3));
      if (trim) {
          dr = dr & (-7);
          dc = dc & (-7);
      }
      chroma[r][c].SetMotionVector(dr, dc);
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
    for (size_t j = 0; j < part[i].size(); ++j) {
      size_t ir = part[i][j] >> 2, ic = part[i][j] & 3;
      MotionVector mv;
      switch (mode) {
        case LEFT_4x4:
          mv = ic == 0 ? (c == 0 ? kZero : mb[r][c - 1].GetSubBlockMV(ir, 3))
                       : mb[r][c].GetSubBlockMV(ir, ic - 1);
          break;

        case ABOVE_4x4:
          mv = ir == 0 ? (r == 0 ? kZero : mb[r - 1][c].GetSubBlockMV(3, ir))
                       : mb[r][c].GetSubBlockMV(ir - 1, ic);
          break;

        case ZERO_4x4:
          mv = kZero;
          break;

        case NEW_4x4:
          mv = ReadMotionVector() + mb[r][c].GetMotionVector();
          break;
      }
      mb[r][c][ir][ic].SetMotionVector(mv);
    }
  }
}

void ConfigureMVs(const FrameHeader &header, Frame &frame, bool trim) {
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      if (!header.macroblock_header[r][c].is_inter_mb) continue;
      int16_t left = int16_t(-(1 << 7)), right = int16_t(frame.hblock);
      int16_t up = int16_t(-((r + 1) << 7)), down = int16_t((frame.vblock - r) << 7);
      MotionVector best, nearest, near;
      MacroBlockMV mode = SearchMVs(r, c, header, frame, best, nearest, near);
      ClampMV(best, left, right, up, down);
      ClampMV(nearest, left, right, up, down);
      ClampMV(near, left, right, up, down);

      switch (mode) {
        case MV_NEAREST:
          frame.Y[r][c].SetSubBlockMVs(nearest);
          frame.Y[r][c].SetMotionVector(nearest);
          break;

        case MV_NEAR:
          frame.Y[r][c].SetSubBlockMVs(near);
          frame.Y[r][c].SetMotionVector(near);
          break;

        case MV_ZERO:
          frame.Y[r][c].SetSubBlockMVs(kZero);
          frame.Y[r][c].SetMotionVector(kZero);
          break;

        case MV_NEW:
          // TODO: Read motion vector in mode MV_NEW.
          MotionVector mv = ReadMotionVector() + best;
          frame.Y[r][c].SetSubBlockMVs(mv);
          frame.Y[r][c].SetMotionVector(mv);
          break;

        case MV_SPLIT:
          // TODO: Read how the macroblock is splitted.
          MVPartition part = ReadMVSplit();
          ConfigureSubBlockMVs(part, r, c, frame.Y);
          frame.Y[r][c].SetMotionVector(frame.Y[r][c].GetSubBlockMV(15));
      }
      ConfigureChromaMVs(frame.Y[r][c], trim, frame.U[r][c]);
      ConfigureChromaMVs(frame.Y[r][c], trim, frame.V[r][c]);
    }
  }
}

template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorSixtap(
    const Plane<C> &refer, size_t r, size_t c,
    const std::array<int16_t, 6> &filter) {
  std::array<std::array<int16_t, 4>, 9> res;
  for (size_t i = 0; i < 9; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(refer.GetPixel(r + i, c + j - 2)) * filter[0] +
                    int32_t(refer.GetPixel(r + i, c + j - 1)) * filter[1] +
                    int32_t(refer.GetPixel(r + i, c + j + 0)) * filter[2] +
                    int32_t(refer.GetPixel(r + i, c + j + 1)) * filter[3] +
                    int32_t(refer.GetPixel(r + i, c + j + 2)) * filter[4] +
                    int32_t(refer.GetPixel(r + i, c + j + 3)) * filter[5];
      res[i][j] = int16_t((sum + 64) >> 7);
    }
  }
  return res;
}

void VerSixtap(const std::array<std::array<int16_t, 4>, 9> &refer, size_t r,
               size_t c, const std::array<int16_t, 6> &filter, SubBlock &sub) {
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(refer[r + i + 0][c + j]) * filter[0] +
                    int32_t(refer[r + i + 1][c + j]) * filter[1] +
                    int32_t(refer[r + i + 2][c + j]) * filter[2] +
                    int32_t(refer[r + i + 3][c + j]) * filter[3] +
                    int32_t(refer[r + i + 4][c + j]) * filter[4] +
                    int32_t(refer[r + i + 5][c + j]) * filter[5];
      sub[i][j] = int16_t((sum + 64) >> 7);
    }
  }
}

template <size_t C>
void Sixtap(const Plane<C> &refer, size_t r, size_t c, uint8_t mr, uint8_t mc,
            const std::array<std::array<int16_t, 6>, 8> &filter,
            SubBlock &sub) {
  std::array<std::array<int16_t, 4>, 9> tmp =
      HorSixtap(refer, r - 2, c, filter[mr]);
  VerSixtap(tmp, r, c, filter[mc], sub);
}

template <size_t C>
void InterpBlock(const Plane<C> &refer,
                 const std::array<std::array<int16_t, 6>, 8> &filter, size_t r,
                 size_t c, MacroBlock<C> &mb) {
  size_t offset = C / 2 + 2;
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j) {
      MotionVector mv = mb[i][j].GetMotionVector();
      if (mv == kZero) {
        for (size_t x = 0; x < C; ++x) {
          for (size_t y = 0; y < C; ++y)
            mb[i][j][x][y] = refer.GetPixel((r << offset) | (i << 2) | x,
                                            (c << offset) | (j << 2) | y);
        }
        continue;
      }
      uint8_t mr = mv.dr & 7, mc = mv.dc & 7;
      assert(int32_t(r << offset | (i << 2)) + (mv.dr >> 3) >= 0);
      assert(int32_t(c << offset | (j << 2)) + (mv.dc >> 3) >= 0);
      size_t tr = size_t(int32_t(r << offset | (i << 2)) + (mv.dr >> 3));
      size_t tc = size_t(int32_t(c << offset | (j << 2)) + (mv.dc >> 3));
      if (mr | mc) Sixtap(refer, tr, tc, mr, mc, filter, mb[r][c]);
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

void InterPredict(const FrameHeader &header, const FrameTag &tag, Frame &frame) {
  ConfigureMVs(header, tag.version == 3, frame);
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      if (!header.macroblock_header[r][c].is_inter_mb) continue;
      InterpBlock(header.ref_frame.Y, header.subpixel_filters, r, c,
                  frame.Y[r].at(c));
      InterpBlock(header.ref_frame.U, header.subpixel_filters, r, c,
                  frame.U[r].at(c));
      InterpBlock(header.ref_frame.V, header.subpixel_filters, r, c,
                  frame.V[r].at(c));
    }
  }
}

}  // namespace vp8
