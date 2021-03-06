#include "intra_predict.h"

namespace vp8 {
namespace internal {

void VPredChroma(size_t r, size_t c, Plane<2> &mb) {
  if (r == 0)
    mb.at(r).at(c).FillWith(kUpperPixel);
  else
    mb.at(r).at(c).FillRow(mb.at(r - 1).at(c).GetRow(7));
}

void HPredChroma(size_t r, size_t c, Plane<2> &mb) {
  if (c == 0)
    mb.at(r).at(c).FillWith(kLeftPixel);
  else
    mb.at(r).at(c).FillCol(mb.at(r).at(c - 1).GetCol(7));
}

void DCPredChroma(size_t r, size_t c, Plane<2> &mb) {
  if (r == 0 && c == 0) {
    mb.at(r).at(c).FillWith(kUpperLeftPixel);
    return;
  }

  int32_t sum = 0, shf = 2;
  if (r > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb.at(r - 1).at(c).GetPixel(7, i);
    shf++;
  }
  if (c > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb.at(r).at(c - 1).GetPixel(i, 7);
    shf++;
  }

  int16_t avg = int16_t((sum + (1 << (shf - 1))) >> shf);
  mb.at(r).at(c).FillWith(avg);
}

void TMPredChroma(size_t r, size_t c, Plane<2> &mb) {
  int16_t p =
      (r == 0 ? kUpperPixel
              : c == 0 ? kLeftPixel : mb.at(r - 1).at(c - 1).GetPixel(7, 7));
  for (size_t i = 0; i < 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      int16_t x = (c == 0 ? kLeftPixel : mb.at(r).at(c - 1).GetPixel(i, 7));
      int16_t y = (r == 0 ? kUpperPixel : mb.at(r - 1).at(c).GetPixel(7, j));
      mb.at(r).at(c).SetPixel(i, j, Clamp255(int16_t(x + y - p)));
    }
  }
}

void VPredLuma(size_t r, size_t c, Plane<4> &mb) {
  if (r == 0)
    mb.at(r).at(c).FillWith(kUpperPixel);
  else
    mb.at(r).at(c).FillRow(mb.at(r - 1).at(c).GetRow(15));
}

void HPredLuma(size_t r, size_t c, Plane<4> &mb) {
  if (c == 0)
    mb.at(r).at(c).FillWith(kLeftPixel);
  else
    mb.at(r).at(c).FillCol(mb.at(r).at(c - 1).GetCol(15));
}

void DCPredLuma(size_t r, size_t c, Plane<4> &mb) {
  if (r == 0 && c == 0) {
    mb.at(r).at(c).FillWith(128);
    return;
  }

  int32_t sum = 0, shf = 3;
  if (r > 0) {
    for (size_t i = 0; i < 16; ++i) sum += mb.at(r - 1).at(c).GetPixel(15, i);
    shf++;
  }
  if (c > 0) {
    for (size_t i = 0; i < 16; ++i) sum += mb.at(r).at(c - 1).GetPixel(i, 15);
    shf++;
  }

  int16_t avg = int16_t((sum + (1 << (shf - 1))) >> shf);
  mb.at(r).at(c).FillWith(avg);
}

void TMPredLuma(size_t r, size_t c, Plane<4> &mb) {
  int16_t p =
      (r == 0 ? kUpperPixel
              : c == 0 ? kLeftPixel : mb.at(r - 1).at(c - 1).GetPixel(15, 15));
  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j) {
      int16_t x = (c == 0 ? kLeftPixel : mb.at(r).at(c - 1).GetPixel(i, 15));
      int16_t y = (r == 0 ? kUpperPixel : mb.at(r - 1).at(c).GetPixel(15, j));
      mb.at(r).at(c).SetPixel(i, j, Clamp255(int16_t(x + y - p)));
    }
  }
}

std::array<Context, 2> BPredLuma(size_t r, size_t c, bool is_key_frame,
                                 const ResidualValue &rv,
                                 const std::array<Context, 2> &context,
                                 const std::unique_ptr<BitstreamParser> &ps,
                                 Plane<4> &mb) {
  Context row = context.at(1).ctx;
  Context col = context.at(0).ctx;

  uint32_t zero = rv.zero;
  std::array<Context, 2> ctx{};

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      std::array<int16_t, 8> above{};
      std::array<int16_t, 4> left{};
      std::array<int16_t, 4> row_above{};
      std::array<int16_t, 4> row_right{};

      if (i > 0) {
        row_above = mb.at(r).at(c).at(i - 1).at(j).GetRow(3);
      } else {
        if (r == 0)
          std::fill(row_above.begin(), row_above.end(), kUpperPixel);
        else
          row_above = mb.at(r - 1).at(c).at(3).at(j).GetRow(3);
      }

      if (j == 3) {
        if (r == 0) {
          std::fill(row_right.begin(), row_right.end(), kUpperPixel);
        } else if (c + 1 == mb.at(r).size()) {
          int16_t x = mb.at(r - 1).at(c).at(3).at(3).at(3).at(3);
          std::fill(row_right.begin(), row_right.end(), x);
        } else {
          row_right = mb.at(r - 1).at(c + 1).at(3).at(0).GetRow(3);
        }
      } else {
        if (i > 0) {
          row_right = mb.at(r).at(c).at(i - 1).at(j + 1).GetRow(3);
        } else {
          if (r == 0)
            std::fill(row_right.begin(), row_right.end(), kUpperPixel);
          else
            row_right = mb.at(r - 1).at(c).at(3).at(j + 1).GetRow(3);
        }
      }

      std::copy(row_above.begin(), row_above.end(), above.begin());
      std::copy(row_right.begin(), row_right.end(), above.begin() + 4);

      if (j > 0) {
        left = mb.at(r).at(c).at(i).at(j - 1).GetCol(3);
      } else {
        if (c == 0)
          std::fill(left.begin(), left.end(), kLeftPixel);
        else
          left = mb.at(r).at(c - 1).at(i).at(3).GetCol(3);
      }

      int16_t p = 0;
      if (i > 0 && j > 0)
        p = mb.at(r).at(c).at(i - 1).at(j - 1).at(3).at(3);
      else if (i > 0)
        p = c == 0 ? kLeftPixel
                   : mb.at(r).at(c - 1).at(i - 1).at(3).at(3).at(3);
      else if (j > 0)
        p = r == 0 ? kUpperPixel
                   : mb.at(r - 1).at(c).at(3).at(j - 1).at(3).at(3);
      else
        p = r == 0 && c == 0
                ? kUpperPixel
                : r == 0
                      ? kUpperPixel
                      : c == 0 ? kLeftPixel
                               : mb.at(r - 1).at(c - 1).at(3).at(3).at(3).at(3);

      SubBlockMode mode =
          is_key_frame ? ps->ReadSubBlockBModeKF(col.mode(j), row.mode(i))
                       : ps->ReadSubBlockBModeNonKF();

      if (i == 3) ctx.at(0).append(j, mode);
      if (j == 3) ctx.at(1).append(i, mode);
      col.append(j, mode);
      row.append(i, mode);
      BPredSubBlock(above, left, p, mode, mb.at(r).at(c).at(i).at(j));
      ApplySBResidual(rv.y.at(i << 2 | j), zero & 1,
                      mb.at(r).at(c).at(i).at(j));
      zero >>= 1;
    }
  }
  return ctx;
}

void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p,
                   SubBlockMode mode, SubBlock &sub) {
  const std::array<int16_t, 9> edge = {
      left.at(3),  left.at(2),  left.at(1),  left.at(0),  p,
      above.at(0), above.at(1), above.at(2), above.at(3),
  };
  switch (mode) {
    case B_VE_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        int16_t x = i == 0 ? p : above.at(i - 1);
        int16_t y = above.at(i);
        int16_t z = above.at(i + 1);
        int16_t avg = (x + y + y + z + 2) >> 2;
        sub.at(0).at(i) = sub.at(1).at(i) = sub.at(2).at(i) = sub.at(3).at(i) =
            avg;
      }
      break;
    }

    case B_HE_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        int16_t x = i == 0 ? p : left.at(i - 1);
        int16_t y = left.at(i);
        int16_t z = i == 3 ? left.at(3) : left.at(i + 1);
        int16_t avg = (x + y + y + z + 2) >> 2;
        sub.at(i).at(0) = sub.at(i).at(1) = sub.at(i).at(2) = sub.at(i).at(3) =
            avg;
      }
      break;
    }

    case B_DC_PRED: {
      int16_t v = 4;
      for (size_t i = 0; i < 4; ++i) v += above.at(i) + left.at(i);
      v >>= 3;
      sub.FillWith(v);
      break;
    }

    case B_TM_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j)
          sub.at(i).at(j) = Clamp255(int16_t(left.at(i) + above.at(j) - p));
      }
      break;
    }

    case B_LD_PRED: {
      for (size_t d = 0; d < 7; ++d) {
        int16_t x = above.at(d);
        int16_t y = above.at(d + 1);
        int16_t z = d + 2 < 8 ? above.at(d + 2) : above.at(7);
        int16_t avg = (x + y + y + z + 2) >> 2;
        for (size_t i = 0; i < 4 && int(d) - int(i) >= 0; ++i) {
          if (int(d) - int(i) < 4) sub.at(i).at(d - i) = avg;
        }
      }
      break;
    }

    case B_RD_PRED: {
      sub.at(3).at(0) =
          (edge.at(0) + edge.at(1) + edge.at(1) + edge.at(2) + 2) >> 2;
      sub.at(3).at(1) = sub.at(2).at(0) =
          (edge.at(1) + edge.at(2) + edge.at(2) + edge.at(3) + 2) >> 2;
      sub.at(3).at(2) = sub.at(2).at(1) = sub.at(1).at(0) =
          (edge.at(2) + edge.at(3) + edge.at(3) + edge.at(4) + 2) >> 2;
      sub.at(3).at(3) = sub.at(2).at(2) = sub.at(1).at(1) = sub.at(0).at(0) =
          (edge.at(3) + edge.at(4) + edge.at(4) + edge.at(5) + 2) >> 2;
      sub.at(2).at(3) = sub.at(1).at(2) = sub.at(0).at(1) =
          (edge.at(4) + edge.at(5) + edge.at(5) + edge.at(6) + 2) >> 2;
      sub.at(1).at(3) = sub.at(0).at(2) =
          (edge.at(5) + edge.at(6) + edge.at(6) + edge.at(7) + 2) >> 2;
      sub.at(0).at(3) =
          (edge.at(6) + edge.at(7) + edge.at(7) + edge.at(8) + 2) >> 2;
      break;
    }

    case B_VR_PRED: {
      sub.at(3).at(0) =
          (edge.at(1) + edge.at(2) + edge.at(2) + edge.at(3) + 2) >> 2;
      sub.at(2).at(0) =
          (edge.at(2) + edge.at(3) + edge.at(3) + edge.at(4) + 2) >> 2;
      sub.at(3).at(1) = sub.at(1).at(0) =
          (edge.at(3) + edge.at(4) + edge.at(4) + edge.at(5) + 2) >> 2;
      sub.at(2).at(1) = sub.at(0).at(0) = (edge.at(4) + edge.at(5) + 1) >> 1;
      sub.at(3).at(2) = sub.at(1).at(1) =
          (edge.at(4) + edge.at(5) + edge.at(5) + edge.at(6) + 2) >> 2;
      sub.at(2).at(2) = sub.at(0).at(1) = (edge.at(5) + edge.at(6) + 1) >> 1;
      sub.at(3).at(3) = sub.at(1).at(2) =
          (edge.at(5) + edge.at(6) + edge.at(6) + edge.at(7) + 2) >> 2;
      sub.at(2).at(3) = sub.at(0).at(2) = (edge.at(6) + edge.at(7) + 1) >> 1;
      sub.at(1).at(3) =
          (edge.at(6) + edge.at(7) + edge.at(7) + edge.at(8) + 2) >> 2;
      sub.at(0).at(3) = (edge.at(7) + edge.at(8) + 1) >> 1;
      break;
    }

    case B_VL_PRED: {
      sub.at(0).at(0) = (above.at(0) + above.at(1) + 1) >> 1;
      sub.at(1).at(0) =
          (above.at(0) + above.at(1) + above.at(1) + above.at(2) + 2) >> 2;
      sub.at(2).at(0) = sub.at(0).at(1) = (above.at(1) + above.at(2) + 1) >> 1;
      sub.at(1).at(1) = sub.at(3).at(0) =
          (above.at(1) + above.at(2) + above.at(2) + above.at(3) + 2) >> 2;
      sub.at(2).at(1) = sub.at(0).at(2) = (above.at(2) + above.at(3) + 1) >> 1;
      sub.at(3).at(1) = sub.at(1).at(2) =
          (above.at(2) + above.at(3) + above.at(3) + above.at(4) + 2) >> 2;
      sub.at(2).at(2) = sub.at(0).at(3) = (above.at(3) + above.at(4) + 1) >> 1;
      sub.at(3).at(2) = sub.at(1).at(3) =
          (above.at(3) + above.at(4) + above.at(4) + above.at(5) + 2) >> 2;
      sub.at(2).at(3) =
          (above.at(4) + above.at(5) + above.at(5) + above.at(6) + 2) >> 2;
      sub.at(3).at(3) =
          (above.at(5) + above.at(6) + above.at(6) + above.at(7) + 2) >> 2;
      break;
    }

    case B_HD_PRED: {
      sub.at(3).at(0) = (edge.at(0) + edge.at(1) + 1) >> 1;
      sub.at(3).at(1) =
          (edge.at(0) + edge.at(1) + edge.at(1) + edge.at(2) + 2) >> 2;
      sub.at(2).at(0) = sub.at(3).at(2) = (edge.at(1) + edge.at(2) + 1) >> 1;
      sub.at(2).at(1) = sub.at(3).at(3) =
          (edge.at(1) + edge.at(2) + edge.at(2) + edge.at(3) + 2) >> 2;
      sub.at(2).at(2) = sub.at(1).at(0) = (edge.at(2) + edge.at(3) + 1) >> 1;
      sub.at(2).at(3) = sub.at(1).at(1) =
          (edge.at(2) + edge.at(3) + edge.at(3) + edge.at(4) + 2) >> 2;
      sub.at(1).at(2) = sub.at(0).at(0) = (edge.at(3) + edge.at(4) + 1) >> 1;
      sub.at(1).at(3) = sub.at(0).at(1) =
          (edge.at(3) + edge.at(4) + edge.at(4) + edge.at(5) + 2) >> 2;
      sub.at(0).at(2) =
          (edge.at(4) + edge.at(5) + edge.at(5) + edge.at(6) + 2) >> 2;
      sub.at(0).at(3) =
          (edge.at(5) + edge.at(6) + edge.at(6) + edge.at(7) + 2) >> 2;
      break;
    }

    case B_HU_PRED: {
      sub.at(0).at(0) = (left.at(0) + left.at(1) + 1) >> 1;
      sub.at(0).at(1) =
          (left.at(0) + left.at(1) + left.at(1) + left.at(2) + 2) >> 2;
      sub.at(0).at(2) = sub.at(1).at(0) = (left.at(1) + left.at(2) + 1) >> 1;
      sub.at(0).at(3) = sub.at(1).at(1) =
          (left.at(1) + left.at(2) + left.at(2) + left.at(3) + 2) >> 2;
      sub.at(1).at(2) = sub.at(2).at(0) = (left.at(2) + left.at(3) + 1) >> 1;
      sub.at(1).at(3) = sub.at(2).at(1) =
          (left.at(2) + left.at(3) + left.at(3) + left.at(3) + 2) >> 2;
      sub.at(2).at(2) = sub.at(2).at(3) = sub.at(3).at(0) = sub.at(3).at(1) =
          sub.at(3).at(2) = sub.at(3).at(3) = left.at(3);
      break;
    }

    default:
      ensure(false, "[Error] BPredSubBlock: Unknown subblock mode.");
      break;
  }
}

}  // namespace internal

std::array<Context, 2> IntraPredict(const FrameTag &tag, size_t r, size_t c,
                                    const ResidualValue &rv,
                                    const IntraMBHeader &mh,
                                    const std::array<Context, 2> &context,
                                    std::vector<std::vector<uint8_t>> &skip_lf,
                                    const std::unique_ptr<BitstreamParser> &ps,
                                    const std::shared_ptr<Frame> &frame) {
  if (mh.intra_y_mode == B_PRED) skip_lf.at(r).at(c) = 0;
  std::array<Context, 2> ctx{};

  switch (mh.intra_y_mode) {
    case V_PRED:
      internal::VPredLuma(r, c, frame->Y);
      ctx.at(0) = ctx.at(1) = Context(kAllVPred);
      ApplyMBResidual(rv.y, rv.zero, frame->Y.at(r).at(c));
      break;

    case H_PRED:
      internal::HPredLuma(r, c, frame->Y);
      ctx.at(0) = ctx.at(1) = Context(kAllHPred);
      ApplyMBResidual(rv.y, rv.zero, frame->Y.at(r).at(c));
      break;

    case DC_PRED:
      internal::DCPredLuma(r, c, frame->Y);
      ctx.at(0) = ctx.at(1) = Context(kAllDCPred);
      ApplyMBResidual(rv.y, rv.zero, frame->Y.at(r).at(c));
      break;

    case TM_PRED:
      internal::TMPredLuma(r, c, frame->Y);
      ctx.at(0) = ctx.at(1) = Context(kAllTMPred);
      ApplyMBResidual(rv.y, rv.zero, frame->Y.at(r).at(c));
      break;

    case B_PRED:
      ctx = internal::BPredLuma(r, c, tag.key_frame, rv, context, ps, frame->Y);
      break;

    default:
      ensure(false, "[Error] IntraPredict: Unknown Y mode.");
      break;
  }
  MacroBlockMode intra_uv_mode = tag.key_frame ? ps->ReadIntraMB_UVModeKF()
                                               : ps->ReadIntraMB_UVModeNonKF();
  switch (intra_uv_mode) {
    case V_PRED:
      internal::VPredChroma(r, c, frame->U);
      internal::VPredChroma(r, c, frame->V);
      break;

    case H_PRED:
      internal::HPredChroma(r, c, frame->U);
      internal::HPredChroma(r, c, frame->V);
      break;

    case DC_PRED:
      internal::DCPredChroma(r, c, frame->U);
      internal::DCPredChroma(r, c, frame->V);
      break;

    case TM_PRED:
      internal::TMPredChroma(r, c, frame->U);
      internal::TMPredChroma(r, c, frame->V);
      break;

    default:
      ensure(false, "[Error] IntraPredict: Unknown UV mode.");
      break;
  }
  ApplyMBResidual(rv.u, rv.zero >> 16, frame->U.at(r).at(c));
  ApplyMBResidual(rv.v, rv.zero >> 20, frame->V.at(r).at(c));
  return ctx;
}

}  // namespace vp8
