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
SubBlockMode &operator++(SubBlockMode &mode) {
  switch (mode) {
    case B_DC_PRED: return mode = B_TM_PRED;
    case B_TM_PRED: return mode = B_VE_PRED;
    case B_VE_PRED: return mode = B_HE_PRED;
    case B_HE_PRED: return mode = B_LD_PRED;
    case B_LD_PRED: return mode = B_RD_PRED;
    case B_RD_PRED: return mode = B_VR_PRED;
    case B_VR_PRED: return mode = B_VL_PRED;
    case B_VL_PRED: return mode = B_HD_PRED;
    case B_HD_PRED: return mode = B_HU_PRED;
    default: assert(false);
  }
}

std::pair<SubBlockMode, uint32_t> PickIntraSubBlockMode(
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

}  // namespace internal

}  // namespace vp8
