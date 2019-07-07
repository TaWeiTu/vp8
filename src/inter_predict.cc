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
  std::array<uint8_t, 4> cnt{};
  std::array<MotionVector, 4> mv{};
  uint8_t ptr = 0;

  enum { CNT_ZERO, CNT_NEAREST, CNT_NEAR, CNT_SPLIT };

  if (r > 0 && context.at(r - 1).at(c).is_inter_mb) {
    MotionVector v = mb.at(r - 1).at(c).GetMotionVector();
    if (v != kZero) {
      v = Invert(v, context.at(r - 1).at(c).ref_frame, ref_frame,
                 ref_frame_bias);
      mv.at(++ptr) = v;
    }
    cnt.at(ptr) += 2;
  }

  if (c > 0 && context.at(r).at(c - 1).is_inter_mb) {
    MotionVector v = mb.at(r).at(c - 1).GetMotionVector();
    if (v != kZero) {
      v = Invert(v, context.at(r).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
      if (mv.at(ptr) != v) mv.at(++ptr) = v;
      cnt.at(ptr) += 2;
    } else {
      cnt.at(0) += 2;
    }
  }

  if (r > 0 && c > 0 && context.at(r - 1).at(c - 1).is_inter_mb) {
    MotionVector v = mb.at(r - 1).at(c - 1).GetMotionVector();
    if (v != kZero) {
      v = Invert(v, context.at(r - 1).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
      if (mv.at(ptr) != v) mv.at(++ptr) = v;
      cnt.at(ptr) += 1;
    } else {
      cnt.at(0) += 1;
    }
  }

  // found three distinct motion vectors
  if (cnt.at(CNT_SPLIT) && mv.at(ptr) == mv.at(CNT_NEAREST))
    ++cnt.at(CNT_NEAREST);

  cnt.at(CNT_SPLIT) =
      (r > 0 && context.at(r - 1).at(c).mv_mode == MV_SPLIT) * 2 +
      (c > 0 && context.at(r).at(c - 1).mv_mode == MV_SPLIT) * 2 +
      (r > 0 && c > 0 && context.at(r - 1).at(c - 1).mv_mode == MV_SPLIT);

  if (cnt.at(CNT_NEAR) > cnt.at(CNT_NEAREST)) {
    std::swap(cnt.at(CNT_NEAR), cnt.at(CNT_NEAREST));
    std::swap(mv.at(CNT_NEAR), mv.at(CNT_NEAREST));
  }
  if (cnt.at(CNT_NEAREST) >= cnt.at(CNT_ZERO))
    mv.at(CNT_ZERO) = mv.at(CNT_NEAREST);
  best = mv.at(CNT_ZERO);
  nearest = mv.at(CNT_NEAREST);
  near = mv.at(CNT_NEAR);

  return ps.ReadInterMBHeader(cnt);
}

void ClampMV2(int16_t top, int16_t bottom, int16_t left, int16_t right,
              MotionVector &mv) {
  if (mv.dc < (left - (16 << 3)))
    mv.dc = (left - (16 << 3));
  else if (mv.dc > (right + (16 << 3)))
    mv.dc = (right + (16 << 3));
  if (mv.dr < (top - (16 << 3)))
    mv.dr = (top - (16 << 3));
  else if (mv.dr > (bottom + (16 << 3)))
    mv.dr = (bottom + (16 << 3));
}

void ClampMV(int16_t top, int16_t bottom, int16_t left, int16_t right,
             MotionVector &mv) {
  mv.dc = (2 * mv.dc < (left - (19 << 3))) ? (left - (16 << 3)) >> 1 : mv.dc;
  mv.dc = (2 * mv.dc > (right + (18 << 3))) ? (right + (16 << 3)) >> 1 : mv.dc;
  mv.dr = (2 * mv.dr < (top - (19 << 3))) ? (top - (16 << 3)) >> 1 : mv.dr;
  mv.dr =
      (2 * mv.dr > (bottom + (19 << 3))) ? (bottom + (16 << 3)) >> 1 : mv.dr;
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

void ConfigureChromaMVs(const MacroBlock<4> &luma, size_t vblock, size_t hblock,
                        bool trim, MacroBlock<2> &U, MacroBlock<2> &V) {
  for (size_t r = 0; r < 2; ++r) {
    for (size_t c = 0; c < 2; ++c) {
      MotionVector ulv = luma.at(r << 1).at(c << 1).GetMotionVector(),
                   urv = luma.at(r << 1).at(c << 1 | 1).GetMotionVector(),
                   dlv = luma.at(r << 1 | 1).at(c << 1).GetMotionVector(),
                   drv = luma.at(r << 1 | 1).at(c << 1 | 1).GetMotionVector();

      int16_t sr = int16_t(ulv.dr + urv.dr + dlv.dr + drv.dr);
      int16_t sc = int16_t(ulv.dc + urv.dc + dlv.dc + drv.dc);
      int16_t dr = (sr >= 0 ? (sr + 4) / 8 : (sr - 4) / 8);
      int16_t dc = (sc >= 0 ? (sc + 4) / 8 : (sc - 4) / 8);
      if (trim) {
        dr = dr & (~7);
        dc = dc & (~7);
      }
      int16_t top = ((-int16_t(r) * 16) * 8);
      int16_t bottom = ((int16_t(vblock) - 1 - int16_t(r)) * 16) * 8;
      int16_t left = ((-int16_t(c) * 16) * 8);
      int16_t right = ((int16_t(hblock) - 1 - int16_t(c)) * 16) * 8;

      MotionVector mv = MotionVector(dr, dc);
      ClampMV(left, right, top, bottom, mv);
      U.at(r).at(c).SetMotionVector(dr, dc);
      V.at(r).at(c).SetMotionVector(dr, dc);
    }
  }
}

void ConfigureSubBlockMVs(const InterMBHeader &hd, size_t r, size_t c,
                          MotionVector best, BitstreamParser &ps,
                          Plane<4> &mb) {
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

  uint64_t mask = kHead.at(hd.mv_split_mode);
  for (size_t i = 0; i < kNumPartition.at(hd.mv_split_mode); ++i) {
    size_t head = mask & 15;
    mask >>= 4;
    MotionVector left = LeftMotionVector(head);
    MotionVector above = AboveMotionVector(head);
    uint8_t context = SubBlockContext(left, above);
    SubBlockMVMode mode = ps.ReadSubBlockMVMode(context);
    MotionVector mv;
    switch (mode) {
      case LEFT_4x4:
        mv = left;
        break;

      case ABOVE_4x4:
        mv = above;
        break;

      case ZERO_4x4:
        mv = kZero;
        break;

      case NEW_4x4:
        mv = ps.ReadSubBlockMV() + best;
        break;

      default:
        ensure(false,
               "[Error] ConfigureSubBlockMVs: Unknown subblock motion vector "
               "mode.");
        break;
    }
    for (int8_t ptr = int8_t(head); ptr != -1;
         ptr = kNext.at(hd.mv_split_mode).at(size_t(ptr))) {
      size_t ir = size_t(ptr) >> 2, ic = size_t(ptr) & 3;
      mb.at(r).at(c).at(ir).at(ic).SetMotionVector(mv);
    }
  }
}

void ConfigureMVs(size_t r, size_t c, bool trim,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame) {
  int16_t top = ((-int16_t(r) * 16) * 8);
  int16_t bottom = ((int16_t(frame.vblock) - 1 - int16_t(r)) * 16) * 8;
  int16_t left = ((-int16_t(c) * 16) * 8);
  int16_t right = ((int16_t(frame.hblock) - 1 - int16_t(c)) * 16) * 8;

  MotionVector best, nearest, near, mv;
  InterMBHeader hd = SearchMVs(r, c, frame.Y, ref_frame_bias, ref_frame,
                               context, ps, best, nearest, near);

  ClampMV2(top, bottom, left, right, best);
  ClampMV2(top, bottom, left, right, nearest);
  ClampMV2(top, bottom, left, right, near);

  context.at(r).at(c) = InterContext(hd.mv_mode, ref_frame);
  if (hd.mv_mode == MV_SPLIT) skip_lf.at(r).at(c) = 0;

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
      ConfigureSubBlockMVs(hd, r, c, best, ps, frame.Y);
      mv = frame.Y.at(r).at(c).GetSubBlockMV(15);
      frame.Y.at(r).at(c).SetMotionVector(mv);
      break;

    default:
      ensure(false, "[Error] Unknown macroblock motion vector mode.");
      break;
  }

  ConfigureChromaMVs(frame.Y.at(r).at(c), frame.vblock, frame.hblock, trim,
                     frame.U.at(r).at(c), frame.V.at(r).at(c));
}

template <size_t C>
std::array<std::array<int16_t, 4>, 9> HorizontalSixtap(
    const Plane<C> &refer, int32_t r, int32_t c,
    const std::array<int16_t, 6> &filter) {
  std::array<std::array<int16_t, 4>, 9> res;
  auto GetPixel = [&refer](int32_t row, int32_t col) -> int16_t {
    row = std::clamp(row, 0, int32_t(refer.vsize()) - 1);
    col = std::clamp(col, 0, int32_t(refer.hsize()) - 1);
    return refer.GetPixel(size_t(row), size_t(col));
  };

  for (int32_t i = 0; i < 9; ++i) {
    for (int32_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(GetPixel(r + i, c + j - 2)) * filter.at(0) +
                    int32_t(GetPixel(r + i, c + j - 1)) * filter.at(1) +
                    int32_t(GetPixel(r + i, c + j + 0)) * filter.at(2) +
                    int32_t(GetPixel(r + i, c + j + 1)) * filter.at(3) +
                    int32_t(GetPixel(r + i, c + j + 2)) * filter.at(4) +
                    int32_t(GetPixel(r + i, c + j + 3)) * filter.at(5);
      res.at(size_t(i)).at(size_t(j)) = Clamp255(int16_t((sum + 64) >> 7));
    }
  }
  return res;
}

void VerticalSixtap(const std::array<std::array<int16_t, 4>, 9> &refer,
                    const std::array<int16_t, 6> &filter, SubBlock &sub) {
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t sum = int32_t(refer.at(i + 0).at(j)) * filter.at(0) +
                    int32_t(refer.at(i + 1).at(j)) * filter.at(1) +
                    int32_t(refer.at(i + 2).at(j)) * filter.at(2) +
                    int32_t(refer.at(i + 3).at(j)) * filter.at(3) +
                    int32_t(refer.at(i + 4).at(j)) * filter.at(4) +
                    int32_t(refer.at(i + 5).at(j)) * filter.at(5);
      sub.at(i).at(j) = Clamp255(int16_t((sum + 64) >> 7));
    }
  }
}

template <size_t C>
void Sixtap(const Plane<C> &refer, int32_t r, int32_t c, uint8_t mr, uint8_t mc,
            const std::array<std::array<int16_t, 6>, 8> &filter,
            SubBlock &sub) {
  std::array<std::array<int16_t, 4>, 9> tmp =
      HorizontalSixtap(refer, r - 2, c, filter.at(mc));
  VerticalSixtap(tmp, filter.at(mr), sub);
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
        for (size_t x = 0; x < 4; ++x) {
          for (size_t y = 0; y < 4; ++y)
            mb.at(i).at(j).at(x).at(y) = refer.GetPixel(
                (r << offset) | (i << 2) | x, (c << offset) | (j << 2) | y);
        }
        continue;
      }
      uint8_t mr = mv.dr & 7, mc = mv.dc & 7;
      int32_t tr = int32_t(r << offset | (i << 2)) + (mv.dr >> 3);
      int32_t tc = int32_t(c << offset | (j << 2)) + (mv.dc >> 3);
      if (mr | mc) {
        Sixtap(refer, tr, tc, mr, mc, filter, mb.at(i).at(j));
      } else {
        auto GetPixel = [&refer](int32_t row, int32_t col) -> int16_t {
          row = std::clamp(row, 0, int32_t(refer.vsize()) - 1);
          col = std::clamp(col, 0, int32_t(refer.hsize()) - 1);
          return refer.GetPixel(size_t(row), size_t(col));
        };
        for (int32_t x = 0; x < 4; ++x) {
          for (int32_t y = 0; y < 4; ++y)
            mb.at(i).at(j).at(size_t(x)).at(size_t(y)) =
                GetPixel(tr + x, tc + y);
        }
      }
    }
  }
}

template std::array<std::array<int16_t, 4>, 9> HorizontalSixtap<4>(
    const Plane<4> &, int32_t, int32_t, const std::array<int16_t, 6> &);
template std::array<std::array<int16_t, 4>, 9> HorizontalSixtap<2>(
    const Plane<2> &, int32_t, int32_t, const std::array<int16_t, 6> &);

template void Sixtap<4>(const Plane<4> &, int32_t, int32_t, uint8_t, uint8_t,
                        const std::array<std::array<int16_t, 6>, 8> &,
                        SubBlock &);
template void Sixtap<2>(const Plane<2> &, int32_t, int32_t, uint8_t, uint8_t,
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
  ConfigureMVs(r, c, tag.version == 3, ref_frame_bias, ref_frame, context,
               skip_lf, ps, frame);
  std::array<std::array<int16_t, 6>, 8> subpixel_filters =
      tag.version == 0 ? kBicubicFilter : kBilinearFilter;

  InterpBlock(refs.at(ref_frame).Y, subpixel_filters, r, c,
              frame.Y.at(r).at(c));
  InterpBlock(refs.at(ref_frame).U, subpixel_filters, r, c,
              frame.U.at(r).at(c));
  InterpBlock(refs.at(ref_frame).V, subpixel_filters, r, c,
              frame.V.at(r).at(c));
}

}  // namespace vp8
