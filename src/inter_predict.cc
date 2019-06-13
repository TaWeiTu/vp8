#include "inter_predict.h"

#include <utility>

namespace vp8 {
namespace internal {

InterMBHeader SearchMVs(size_t r, size_t c, const Plane<4> &mb,
                        const std::array<bool, 4> &ref_frame_bias,
                        uint8_t ref_frame,
                        const std::vector<std::vector<InterContext>> &context,
                        BitstreamParser &ps, MotionVector &best,
                        MotionVector &nearest, MotionVector &near) {
#ifdef DEBUG
  std::cerr << "[Debug] Enter SearchMVs()" << std::endl;
#endif
  static std::array<uint8_t, 4> cnt;
  std::fill(cnt.begin(), cnt.end(), 0);
  std::vector<MotionVector> mv;

  if (r == 0 || context.at(r - 1).at(c).is_inter_mb) {
    MotionVector v = r ? mb.at(r - 1).at(c).GetMotionVector() : kZero;
    if (r > 0)
      v = Invert(v, context.at(r - 1).at(c).ref_frame, ref_frame,
                 ref_frame_bias);
    if (v != kZero) mv.push_back(v);
    cnt.at(mv.size()) += 2;
  }

  if (c == 0 || context.at(r).at(c - 1).is_inter_mb) {
    MotionVector v = c ? mb.at(r).at(c - 1).GetMotionVector() : kZero;
    if (c > 0)
      v = Invert(v, context.at(r).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
    if (v != kZero) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt.at(mv.size()) += 2;
    } else {
      cnt.at(0) += 2;
    }
  }

  if (r == 0 || c == 0 || context.at(r - 1).at(c - 1).is_inter_mb) {
    MotionVector v = r && c ? mb.at(r - 1).at(c - 1).GetMotionVector() : kZero;
    if (r > 0 && c > 0)
      v = Invert(v, context.at(r - 1).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
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

  cnt.at(3) =
      (r > 0 && context.at(r - 1).at(c).mv_mode == MV_SPLIT) +
      (c > 0 && context.at(r).at(c - 1).mv_mode == MV_SPLIT) * 2 +
      (r > 0 && c > 0 && context.at(r - 1).at(c - 1).mv_mode == MV_SPLIT);

  if (cnt.at(2) > cnt.at(1)) {
    std::swap(cnt.at(1), cnt.at(2));
    std::swap(mv.at(0), mv.at(1));
  }
  if (cnt.at(1) >= cnt.at(0)) mv.at(0) = mv.at(1);
  best = mv.at(0);
  nearest = mv.at(1);
  near = mv.at(2);

#ifdef DEBUG
  std::cerr << "[Debug] Exit SearchMVs()" << std::endl;
  std::cerr << "[Debug] cnt = {" << int(cnt.at(0)) << ' ' << int(cnt.at(1)) << ' ' << int(cnt.at(2)) << ' ' << int(cnt.at(3)) << "}" << std::endl;
#endif
  return ps.ReadInterMBHeader(cnt);
}

void ClampMV(MotionVector &mv, int16_t left, int16_t right, int16_t up,
             int16_t down) {
  mv.dr = std::clamp(mv.dr, up, down);
  mv.dc = std::clamp(mv.dr, left, right);
}

MotionVector Invert(const MotionVector &mv, uint8_t ref_frame1,
                    uint8_t ref_frame2,
                    const std::array<bool, 4> &ref_frame_bias) {
  if (ref_frame_bias[ref_frame1] != ref_frame_bias[ref_frame2])
    return MotionVector(-mv.dr, -mv.dc);
  return mv;
}

uint8_t SubBlockContext(const MotionVector &left, const MotionVector &above) {
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

void ConfigureSubBlockMVs(const InterMBHeader &hd, size_t r, size_t c,
                          BitstreamParser &ps, Plane<4> &mb) {
  std::vector<std::vector<uint8_t>> part;
  switch (hd.mv_split_mode) {
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
      ensure(false, "[Error] Unknown subblock partition.");
      break;
  }

  auto LeftMotionVector = [&mb, r, c](size_t idx) {
    if ((idx & 3) == 0) {
      if (c == 0) return kZero;
      return mb.at(r).at(c - 1).GetSubBlockMV(idx + 3);
    }
    return mb.at(r).at(c).GetSubBlockMV(idx - 1);
  };

  auto AboveMotionVector = [&mb, r, c](size_t idx) {
    if (idx < 4) {
      if (r == 0) return kZero;
      return mb.at(r - 1).at(c).GetSubBlockMV(idx + 12);
    }
    return mb.at(r).at(c).GetSubBlockMV(idx - 4);
  };

  for (size_t i = 0; i < part.size(); ++i) {
    uint8_t context = SubBlockContext(LeftMotionVector(part.at(i).at(0)),
                                      AboveMotionVector(part.at(i).at(0)));
    SubBlockMVMode mode = ps.ReadSubBlockMVMode(context);
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
          mv = ps.ReadSubBlockMV() + mb.at(r).at(c).GetMotionVector();
          break;

        default:
          ensure(false,
                 "[Error] ConfigureSubBlockMVs: Unknown subblock motion vector "
                 "mode.");
          break;
      }
      mb.at(r).at(c).at(ir).at(ic).SetMotionVector(mv);
    }
  }
}

void ConfigureMVs(size_t r, size_t c, bool trim,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame) {
  int16_t left = int16_t(-(1 << 7)), right = int16_t(frame.hblock);
  int16_t up = int16_t(-((r + 1) << 7)),
          down = int16_t((frame.vblock - r) << 7);

  MotionVector best, nearest, near, mv;
  InterMBHeader hd = SearchMVs(r, c, frame.Y, ref_frame_bias, ref_frame,
                               context, ps, best, nearest, near);
  ClampMV(best, left, right, up, down);
  ClampMV(nearest, left, right, up, down);
  ClampMV(near, left, right, up, down);

  context.at(r).at(c) = InterContext(hd.mv_mode, ref_frame);
  skip_lf.at(r).at(c) += hd.mv_mode == MV_SPLIT;

  switch (hd.mv_mode) {
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
      mv = hd.mv_new + best;
      frame.Y.at(r).at(c).SetSubBlockMVs(mv);
      frame.Y.at(r).at(c).SetMotionVector(mv);
      break;

    case MV_SPLIT:
      ConfigureSubBlockMVs(hd, r, c, ps, frame.Y);
      mv = frame.Y.at(r).at(c).GetSubBlockMV(15);
      frame.Y.at(r).at(c).SetMotionVector(mv);
      break;

    default:
      ensure(false, "[Error] Unknown macroblock motion vector mode.");
      break;
  }
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.U.at(r).at(c));
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.V.at(r).at(c));
}

template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorizontalSixtap(
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

void VerticalSixtap(const std::array<std::array<int16_t, 4>, 9> &refer,
                    size_t r, size_t c, const std::array<int16_t, 6> &filter,
                    SubBlock &sub) {
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
      HorizontalSixtap(refer, r - 2, c, filter.at(mr));
  VerticalSixtap(tmp, r, c, filter.at(mc), sub);
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
      if (mr | mc) Sixtap(refer, tr, tc, mr, mc, filter, mb.at(i).at(j));
    }
  }
}

template std::array<std::array<int16_t, 4>, 9> HorizontalSixtap<4>(
    const Plane<4> &, size_t, size_t, const std::array<int16_t, 6> &);
template std::array<std::array<int16_t, 4>, 9> HorizontalSixtap<2>(
    const Plane<2> &, size_t, size_t, const std::array<int16_t, 6> &);

template void Sixtap<4>(const Plane<4> &, size_t, size_t, uint8_t, uint8_t,
                        const std::array<std::array<int16_t, 6>, 8> &,
                        SubBlock &);
template void Sixtap<2>(const Plane<2> &, size_t, size_t, uint8_t, uint8_t,
                        const std::array<std::array<int16_t, 6>, 8> &,
                        SubBlock &);

template void InterpBlock<4>(
    const Plane<4> &refer, const std::array<std::array<int16_t, 6>, 8> &filter,
    size_t r, size_t c, MacroBlock<4> &mb);
template void InterpBlock<2>(
    const Plane<2> &refer, const std::array<std::array<int16_t, 6>, 8> &filter,
    size_t r, size_t c, MacroBlock<2> &mb);

}  // namespace internal

using namespace internal;

void InterPredict(const FrameTag &tag, size_t r, size_t c,
                  const std::array<Frame, 4> &refs,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame) {

#ifdef DEBUG
  std::cerr << "Inter-Predict" << std::endl;
#endif
  ConfigureMVs(r, c, tag.version == 3, ref_frame_bias, ref_frame, context, skip_lf, ps,
               frame);
  std::array<std::array<int16_t, 6>, 8> subpixel_filters =
      tag.version == 0 ? kBicubicFilter : kBilinearFilter;

  InterpBlock(refs[ref_frame].Y, subpixel_filters, r, c, frame.Y.at(r).at(c));
  InterpBlock(refs[ref_frame].U, subpixel_filters, r, c, frame.U.at(r).at(c));
  InterpBlock(refs[ref_frame].V, subpixel_filters, r, c, frame.V.at(r).at(c));
}

}  // namespace vp8
