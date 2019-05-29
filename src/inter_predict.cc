#include "inter_predict.h"

namespace vp8 {
namespace {

MacroBlockMV SearchMVs(size_t r, size_t c, const FrameHeader &header,
                       const LumaBlock &mb, MotionVector &best,
                       MotionVector &nearest, MotionVector &near) {
  static std::array<uint8_t, 4> cnt;
  std::fill(cnt.begin(), cnt.end(), 0);
  std::vector<MotionVector> mv;

  auto &mh = header.macroblock_header;

  if (r == 0 || mh.[r - 1][c].is_inter_mb) {
    MotionVector v = r ? mb[r - 1][c].GetMotionVector() : kZero;
    if (r > 0) v = Invert(v, mh[r - 1][c]);
    if (v != kZero) mv.push_back(v);
    cnt[mv.size()] += 2;
  }

  if (c == 0 || mh.[r][c - 1].is_inter_mb) {
    MotionVector v = c ? mb[r][c - 1].GetMotionVector() : kZero;
    if (c > 0) v = Invert(v, mh[r][c - 1]);
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt[mv.size()] += 2;
    } else {
      cnt[0] += 2;
    }
  }

  if (r == 0 || c == 0 || mh.[r - 1][c - 1].is_inter_mb) {
    MotionVector v = r && c ? mb[r - 1][c - 1].GetMotionVector() : kZero;
    if (r > 0 && c > 0) v = Invert(v, mh[r - 1][c - 1]);
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt[mv.size()] += 2;
    } else {
      cnt[0] += 2;
    }
  }

  for (size_t i = 0; i < mv.size(); ++i) ClampMV(mv[i]);

  // found three distinct motion vectors
  if (mv.size() == 3u && mv[2] == mv[0]) ++cnt[1];
  // unfound motion vectors are set to ZERO
  while (mv.size() < 3u) mv.push_back(ZERO);

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

MotionVector Average(size_t r, size_t c, const LumbBlock &mb) {
  int16_t sr =
      int16_t(mb[r << 1][c << 1].dr + mb[r << 1][c << 1 | 1].dr +
              mb[r << 1 | 1][c << 1].dr + mb[r << 1 | 1][c << 1 | 1].dr);
  int16_t sc =
      int16_t(mb[r << 1][c << 1].dc + mb[r << 1][c << 1 | 1].dc +
              mb[r << 1 | 1][c << 1].dc + mb[r << 1 | 1][c << 1 | 1].dc);
  int16_t dr = (sr >= 0 ? (sr + 4) >> 3 : -((-sr + 4) >> 3));
  int16_t dc = (sc >= 0 ? (sc + 4) >> 3 : -((-sc + 4) >> 3));

  return MotionVector(dr, dc);
}

void ConfigureSubBlockMVs(MVPartition p, LumaBlock &mb) {
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

    default:
  }

  for (size_t i = 0; i < part.size(); ++i) {
    SubBlockMV mode = ReadSubBlockMV();
    for (size_t j = 0; j < part[i].size(); ++j) {
      size_t r = part[i][j] >> 2, c = part[i][j] & 3;
      MotionVector mv;
      switch (mode) {
        case LEFT_4x4:
          mv = (c == 0 ? kZero : mb[r][c - 1].GetMotionVector());
          break;

        case ABOVE_4x4:
          mv = (r == 0 ? kZero : mb[r - 1][c].GetMotionVector());
          break;

        case ZERO_4x4:
          mv = kZero;
          break;

        case NEW_4x4:
          mv = ReadMotionVector();
          break;

        default:
      }
      mb[r][c].SetMotionVector(mv);
    }
  }
}

}  // namespace

void InterPredict(const FrameHeader &header, const Frame &frame) {
  auto &Y = frame.YBlocks, &U = frame.UBlocks, &V = frame.VBlocks;
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      if (!header.macroblock_header[r][c].is_inter_mb) continue;
      MotionVector best, nearest, near, mv;
      MacroBlockMV mode = SearchMVs(r, c, header, frame, best, nearest, near);
      Y[r][c].SetMotionVector(mode);

      switch (mode) {
        case MV_NEARST:
          Y.SetSubBlockMVs(nearest);
          break;

        case MV_NEAR:
          Y.SetSubBlockMVs(near);
          break;

        case MV_ZERO:
          Y.SetSubBlockMVs(kZero);
          break;

        case MV_NEW:
          // TODO: Read motion vector in mode MV_NEW.
          mv = ReadMotionVector();
          break;

        case MV_SPLIT:
          // TODO: Read how the macroblock is splitted.
          MVPartition part = ReadMVSplit();
          ConfigureSubBlockMVs(part, Y[r][c]);
      }
    }
  }
}

}  // namespace vp8
