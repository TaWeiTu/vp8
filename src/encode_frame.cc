#include "encode_frame.h"

namespace vp8 {
namespace internal {

template <size_t C>
uint32_t GetPredictionError(const MacroBlock<C> &target,
                            const MacroBlock<C> &predict) {
  uint32_t res = 0;
  for (size_t i = 0; i < (C << 2); ++i) {
    for (size_t j = 0; j < (C << 2); ++j) {
      int32_t diff = target.GetPixel(i, j) - predict.GetPixel(i, j);
      res += uint32_t(diff * diff);
    }
  }
  return res;
}

uint32_t GetPredictionError(const SubBlock &target, const SubBlock &predict) {
  uint32_t res = 0;
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      int32_t diff = target.at(i).at(j) - predict.at(i).at(j);
      res += uint32_t(diff * diff);
    }
  }
  return res;
}

MacroBlockMode PickIntraModeChroma(size_t r, size_t c,
                                   const MacroBlock<2> &u_target,
                                   const MacroBlock<2> &v_target,
                                   Plane<2> &u_predict, Plane<2> &v_predict) {
  MacroBlockMode best_mode{};
  uint32_t best_error = UINT_MAX;
  uint32_t error = 0;

  VPredChroma(r, c, u_predict);
  VPredChroma(r, c, v_predict);
  error = GetPredictionError(u_target, u_predict.at(r).at(c)) +
          GetPredictionError(v_target, v_predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = V_PRED;
  }

  HPredChroma(r, c, u_predict);
  HPredChroma(r, c, v_predict);
  error = GetPredictionError(u_target, u_predict.at(r).at(c)) +
          GetPredictionError(v_target, v_predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = H_PRED;
  }

  DCPredChroma(r, c, u_predict);
  DCPredChroma(r, c, v_predict);
  error = GetPredictionError(u_target, u_predict.at(r).at(c)) +
          GetPredictionError(v_target, v_predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = DC_PRED;
  }

  TMPredChroma(r, c, u_predict);
  TMPredChroma(r, c, v_predict);
  error = GetPredictionError(u_target, u_predict.at(r).at(c)) +
          GetPredictionError(v_target, v_predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = TM_PRED;
  }

  return best_mode;
}

// This enables pre-increment of SubBlockMode.
static SubBlockMode &operator++(SubBlockMode &mode) {
  switch (mode) {
    case B_DC_PRED:
      return mode = B_TM_PRED;
    case B_TM_PRED:
      return mode = B_VE_PRED;
    case B_VE_PRED:
      return mode = B_HE_PRED;
    case B_HE_PRED:
      return mode = B_LD_PRED;
    case B_LD_PRED:
      return mode = B_RD_PRED;
    case B_RD_PRED:
      return mode = B_VR_PRED;
    case B_VR_PRED:
      return mode = B_VL_PRED;
    case B_VL_PRED:
      return mode = B_HD_PRED;
    case B_HD_PRED:
      return mode = B_HU_PRED;
    default:
      ensure(false,
             "[Error] SubBlockMode::operator++: encounter mode that is "
             "prohibited to be incremented.");
  }
  __builtin_unreachable();
}

std::pair<SubBlockMode, uint32_t> PickIntraSubBlockModeSB(
    const std::array<int16_t, 8> &above, const std::array<int16_t, 4> &left,
    int16_t p, const SubBlock &target, SubBlock &predict) {
  uint32_t best_error = UINT_MAX;
  SubBlockMode best_mode{};

  for (SubBlockMode mode = B_DC_PRED; mode <= B_HU_PRED; ++mode) {
    BPredSubBlock(above, left, p, mode, predict);
    uint32_t error = GetPredictionError(target, predict);
    if (error < best_error) {
      best_error = error;
      best_mode = mode;
    }
  }
  return std::make_pair(best_mode, best_error);
}

// TODO(waynetu): residuals should be taken into consideration.
uint32_t PickIntraSubBlockModeMB(size_t r, size_t c,
                                 const MacroBlock<4> &target, Plane<4> &predict,
                                 std::array<SubBlockMode, 16> &sub_mode) {
  std::array<int16_t, 8> above{};
  std::array<int16_t, 4> left{};
  std::array<int16_t, 4> row_above{};
  std::array<int16_t, 4> row_right{};

  uint32_t error = 0;

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      if (i > 0) {
        row_above = predict.at(r).at(c).at(i - 1).at(j).GetRow(3);
      } else {
        if (r == 0)
          std::fill(row_above.begin(), row_above.end(), kUpperPixel);
        else
          row_above = predict.at(r - 1).at(c).at(3).at(j).GetRow(3);
      }

      if (j == 3) {
        if (r == 0) {
          std::fill(row_right.begin(), row_right.end(), kUpperPixel);
        } else if (c + 1 == predict.at(r).size()) {
          int16_t x = predict.at(r - 1).at(c).at(3).at(3).at(3).at(3);
          std::fill(row_right.begin(), row_right.end(), x);
        } else {
          row_right = predict.at(r - 1).at(c + 1).at(3).at(0).GetRow(3);
        }
      } else {
        if (i > 0) {
          row_right = predict.at(r).at(c).at(i - 1).at(j + 1).GetRow(3);
        } else {
          if (r == 0)
            std::fill(row_right.begin(), row_right.end(), kUpperPixel);
          else
            row_right = predict.at(r - 1).at(c).at(3).at(j + 1).GetRow(3);
        }
      }

      std::copy(row_above.begin(), row_above.end(), above.begin());
      std::copy(row_right.begin(), row_right.end(), above.begin() + 4);

      if (j > 0) {
        left = predict.at(r).at(c).at(i).at(j - 1).GetCol(3);
      } else {
        if (c == 0)
          std::fill(left.begin(), left.end(), kLeftPixel);
        else
          left = predict.at(r).at(c - 1).at(i).at(3).GetCol(3);
      }

      int16_t p = 0;
      if (i > 0 && j > 0)
        p = predict.at(r).at(c).at(i - 1).at(j - 1).at(3).at(3);
      else if (i > 0)
        p = c == 0 ? kLeftPixel : predict.at(r).at(c - 1).at(i - 1).at(3).at(3).at(3);
      else if (j > 0)
        p = r == 0 ? kUpperPixel : predict.at(r - 1).at(c).at(3).at(j - 1).at(3).at(3);
      else
        p = r == 0 && c == 0
                ? kUpperPixel
                : r == 0
                      ? kUpperPixel
                      : c == 0
                            ? kLeftPixel
                            : predict.at(r - 1).at(c - 1).at(3).at(3).at(3).at(
                                  3);

      std::pair<SubBlockMode, uint32_t> info = PickIntraSubBlockModeSB(
          above, left, p, target.at(i).at(j), predict.at(r).at(c).at(i).at(j));

      error += info.second;
      sub_mode.at(i << 2 | j) = info.first;
    }
  }
  return error;
}

MacroBlockMode PickIntraModeLuma(size_t r, size_t c,
                                 const MacroBlock<4> &target, Plane<4> &predict,
                                 std::array<SubBlockMode, 16> &sub_mode) {
  MacroBlockMode best_mode{};
  uint32_t best_error = UINT_MAX, error = 0;

  VPredLuma(r, c, predict);
  error = GetPredictionError(target, predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = V_PRED;
  }

  HPredLuma(r, c, predict);
  error = GetPredictionError(target, predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = H_PRED;
  }

  DCPredLuma(r, c, predict);
  error = GetPredictionError(target, predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = DC_PRED;
  }

  TMPredLuma(r, c, predict);
  error = GetPredictionError(target, predict.at(r).at(c));
  if (error < best_error) {
    best_error = error;
    best_mode = TM_PRED;
  }

  error = PickIntraSubBlockModeMB(r, c, target, predict, sub_mode);
  if (error < best_error) {
    best_error = error;
    best_mode = B_PRED;
  }
  return best_mode;
}

}  // namespace internal

}  // namespace vp8
