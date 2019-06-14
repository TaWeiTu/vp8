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
// #ifdef DEBUG
  // std::cerr << "[Debug] Enter SearchMVs()" << std::endl;
// #endif
  std::array<uint8_t, 4> cnt{};
  std::array<MotionVector, 4> mv{};
  uint8_t ptr = 0;

  enum { CNT_ZERO, CNT_NEAREST, CNT_NEAR, CNT_SPLIT };

  if (r > 0 && context.at(r - 1).at(c).is_inter_mb) {
    MotionVector v = r ? mb.at(r - 1).at(c).GetMotionVector() : kZero;
    if (r > 0)
      v = Invert(v, context.at(r - 1).at(c).ref_frame, ref_frame,
                 ref_frame_bias);
    if (v != kZero) mv.at(++ptr) = v;
    cnt.at(ptr) += 2;
  }

  if (c > 0 && context.at(r).at(c - 1).is_inter_mb) {
    MotionVector v = c ? mb.at(r).at(c - 1).GetMotionVector() : kZero;
    if (c > 0)
      v = Invert(v, context.at(r).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
    if (v != kZero) {
      if (mv.at(ptr) != v) mv.at(++ptr) = v;
      cnt.at(ptr) += 2;
    } else {
      cnt.at(0) += 2;
    }
  }

  if (r > 0 && c > 0 && context.at(r - 1).at(c - 1).is_inter_mb) {
    MotionVector v = r && c ? mb.at(r - 1).at(c - 1).GetMotionVector() : kZero;
    if (r > 0 && c > 0)
      v = Invert(v, context.at(r - 1).at(c - 1).ref_frame, ref_frame,
                 ref_frame_bias);
    if (v != kZero) {
      if (mv.at(ptr) != v) mv.at(++ptr) = v;
      cnt.at(ptr) += 1;
    } else {
      cnt.at(0) += 1;
    }
  }

  // found three distinct motion vectors
  if (cnt.at(CNT_SPLIT) && mv.at(CNT_SPLIT) == mv.at(CNT_NEAREST)) ++cnt.at(CNT_NEAREST);

  cnt.at(CNT_SPLIT) =
      (r > 0 && context.at(r - 1).at(c).mv_mode == MV_SPLIT) * 2 +
      (c > 0 && context.at(r).at(c - 1).mv_mode == MV_SPLIT) * 2 +
      (r > 0 && c > 0 && context.at(r - 1).at(c - 1).mv_mode == MV_SPLIT);

  if (cnt.at(CNT_NEAR) > cnt.at(CNT_NEAREST)) {
    std::swap(cnt.at(CNT_NEAR), cnt.at(CNT_NEAREST));
    std::swap(mv.at(CNT_NEAR), mv.at(CNT_NEAREST));
  }
  if (cnt.at(CNT_NEAREST) >= cnt.at(CNT_ZERO)) mv.at(CNT_ZERO) = mv.at(CNT_NEAREST);
  best = mv.at(CNT_ZERO);
  nearest = mv.at(CNT_NEAREST);
  near = mv.at(CNT_NEAR);

#ifdef DEBUG
  for (size_t i = 0; i < 4; ++i)
    std::cerr << mv.at(i).dr << ' ' << mv.at(i).dc << std::endl;
  // std::cerr << "[Debug] Exit SearchMVs()" << std::endl;
  std::cerr << "cnt = {" << int(cnt.at(0)) << ' ' << int(cnt.at(1)) << ' ' << int(cnt.at(2)) << ' ' << int(cnt.at(3)) << "}" << std::endl;
  std::cerr << "mv = {" << mv.at(0).dr << ' ' << mv.at(0).dc << ' ' << mv.at(1).dr << ' ' << mv.at(1).dc << ' ' << mv.at(2).dr << ' ' << mv.at(2).dc << ' ' << mv.at(3).dr << ' ' << mv.at(3).dc << "}" << std::endl;
#endif
  return ps.ReadInterMBHeader(cnt);
}

void ClampMV(int16_t left, int16_t right, int16_t up,
             int16_t down, MotionVector &mv) {
  mv.dr = std::clamp(mv.dr, up, down);
  mv.dc = std::clamp(mv.dc, left, right);
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
      int16_t dr = (sr >= 0 ? (sr + 4) / 8 : -((-sr + 4) / 8));
      int16_t dc = (sc >= 0 ? (sc + 4) / 8: -((-sc + 4) / 8));
      if (trim) {
        dr = dr & (~7);
        dc = dc & (~7);
      }
      chroma.at(r).at(c).SetMotionVector(dr, dc);
    }
  }
}

void ConfigureSubBlockMVs(const InterMBHeader &hd, size_t r, size_t c, MotionVector best, 
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
    MotionVector left = LeftMotionVector(part.at(i).at(0));
    MotionVector above = AboveMotionVector(part.at(i).at(0));
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
    for (size_t j = 0; j < part.at(i).size(); ++j) {
      size_t ir = part.at(i).at(j) >> 2, ic = part.at(i).at(j) & 3;
      mb.at(r).at(c).at(ir).at(ic).SetMotionVector(mv);
    }
  }
}

void ConfigureMVs(size_t r, size_t c, bool trim,
                  const std::array<bool, 4> &ref_frame_bias, uint8_t ref_frame,
                  std::vector<std::vector<InterContext>> &context,
                  std::vector<std::vector<uint8_t>> &skip_lf,
                  BitstreamParser &ps, Frame &frame) {
  int16_t left = int16_t(-(1 << 7)), right = int16_t(frame.hblock << 7);
  int16_t up = int16_t(-((r + 1) << 7)),
          down = int16_t((frame.vblock - r) << 7);

#ifdef DEBUG
  std::cerr << "left = " << left << " right = " << right << " up = " << up << " down = " << down << std::endl;
#endif

  MotionVector best, nearest, near, mv;
  InterMBHeader hd = SearchMVs(r, c, frame.Y, ref_frame_bias, ref_frame,
                               context, ps, best, nearest, near);

#ifdef DEBUG
  std::cerr << "Before clamping" << std::endl;
  std::cerr << "best = " << best.dr << ' ' << best.dc << std::endl;
  std::cerr << "nearest = " << nearest.dr << ' ' << nearest.dc << std::endl;
  std::cerr << "near = " << near.dr << ' ' << near.dc << std::endl;
#endif

  ClampMV(left, right, up, down, best);
  ClampMV(left, right, up, down, nearest);
  ClampMV(left, right, up, down, near);

  context.at(r).at(c) = InterContext(hd.mv_mode, ref_frame);
  skip_lf.at(r).at(c) += hd.mv_mode == MV_SPLIT;

#ifdef DEBUG
  std::cerr << "MV_MODE = " << hd.mv_mode << std::endl;
  std::cerr << "best = " << best.dr << ' ' << best.dc << std::endl;
  std::cerr << "nearest = " << nearest.dr << ' ' << nearest.dc << std::endl;
  std::cerr << "near = " << near.dr << ' ' << near.dc << std::endl;
#endif

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
#ifdef DEBUG
      std::cerr << "mv = {" << int(mv.dr) << ' ' << int(mv.dc) << "}" << std::endl;
#endif
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
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.U.at(r).at(c));
  ConfigureChromaMVs(frame.Y.at(r).at(c), trim, frame.V.at(r).at(c));
// #ifdef DEBUG
  // if (hd.mv_mode == MV_SPLIT) {
    // for (size_t i = 0; i < 2; ++i) {
      // for (size_t j = 0; j < 2; ++j) {
        // MotionVector mv = frame.U.at(r).at(c).at(i).at(j).GetMotionVector();
        // std::cout << "uvrow = " << mv.dr << " uvcol = " << mv.dc << std::endl;
      // }
    // }
  // }
// #endif
#ifdef DEBUG
  std::cerr << "[Debug] Exit ConfigureMVs()" << std::endl;
#endif
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
                    const std::array<int16_t, 6> &filter,
                    SubBlock &sub) {
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
  VerticalSixtap(tmp, filter.at(mr), sub); }

template <size_t C>
void InterpBlock(const Plane<C> &refer,
                 const std::array<std::array<int16_t, 6>, 8> &filter, size_t r,
                 size_t c, MacroBlock<C> &mb) {
#ifdef DEBUG
  std::cerr << "[Debug] Enter InterpBlock()" << std::endl;
#endif
  size_t offset = C / 2 + 2;
  for (size_t i = 0; i < C; ++i) {
    for (size_t j = 0; j < C; ++j) {
      MotionVector mv = mb.at(i).at(j).GetMotionVector();
      if (mv == kZero) {
#ifdef DEBUG
        std::cerr << "[Debug] zero motion vector" << std::endl;
#endif
        for (size_t x = 0; x < 4; ++x) {
          for (size_t y = 0; y < 4; ++y)
            mb.at(i).at(j).at(x).at(y) = refer.GetPixel(
                (r << offset) | (i << 2) | x, (c << offset) | (j << 2) | y);
        }
        continue;
      }
#ifdef DEBUG
      std::cerr << "mv.dr = " << int(mv.dr) << "mv.dr & 7 = " << int(mv.dr & 7) << ' ' << "mv.dc = " << int(mv.dc) << "mv.dc & 7 = " << int(mv.dc & 7) << std::endl;
#endif
      uint8_t mr = mv.dr & 7, mc = mv.dc & 7;
      int32_t tr = int32_t(r << offset | (i << 2)) + (mv.dr >> 3);
      int32_t tc = int32_t(c << offset | (j << 2)) + (mv.dc >> 3);
#ifdef DEBUG
      std::cerr << "tr = " << tr << " tc = " << tc << std::endl;
#endif
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
            mb.at(i).at(j).at(size_t(x)).at(size_t(y)) = GetPixel(tr + x, tc + y);
        }
      }
    }
  }
#ifdef DEBUG
  std::cerr << "result" << std::endl;
  for (size_t i = 0; i < C * 4; ++i) {
    for (size_t j = 0; j < C * 4; ++j)
      std::cerr << mb.GetPixel(i, j) << ' ';
    std::cerr << std::endl;
  }
#endif
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

#ifdef DEBUG
  std::cerr << "Inter-Predict" << std::endl;
#endif
  ConfigureMVs(r, c, tag.version == 3, ref_frame_bias, ref_frame, context, skip_lf, ps,
               frame);
  std::array<std::array<int16_t, 6>, 8> subpixel_filters =
      tag.version == 0 ? kBicubicFilter : kBilinearFilter;

#ifdef DEBUG
  std::cerr << "[Debug] ref_frame = " << int(ref_frame) << std::endl;
#endif

  InterpBlock(refs.at(ref_frame).Y, subpixel_filters, r, c, frame.Y.at(r).at(c));
  InterpBlock(refs.at(ref_frame).U, subpixel_filters, r, c, frame.U.at(r).at(c));
  InterpBlock(refs.at(ref_frame).V, subpixel_filters, r, c, frame.V.at(r).at(c));
#ifdef DEBUG
  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j)
      std::cout << frame.Y.at(r).at(c).GetPixel(i, j) << ' ';
    std::cout << std::endl;
  }
  for (size_t i = 0; i < 8; ++i) {
    for (size_t j = 0; j < 8; ++j)
      std::cout << frame.U.at(r).at(c).GetPixel(i, j) << ' ';
    std::cout << std::endl;
  }
  for (size_t i = 0; i < 8; ++i) {
    for (size_t j = 0; j < 8; ++j)
      std::cout << frame.V.at(r).at(c).GetPixel(i, j) << ' ';
    std::cout << std::endl;
  }
#endif
}

}  // namespace vp8
