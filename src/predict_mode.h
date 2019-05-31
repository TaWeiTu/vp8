#ifndef PREDICT_MODE_H_
#define PREDICT_MODE_H_

#include <vector>

namespace vp8 {

enum MacroBlockMode { DC_PRED, V_PRED, H_PRED, TM_PRED, B_PRED };

enum SubBlockMode {
  B_DC_PRED,
  B_TM_PRED,
  B_VE_PRED,
  B_HE_PRED,
  B_LD_PRED,
  B_RD_PRED,
  B_VR_PRED,
  B_VL_PRED,
  B_HD_PRED,
  B_HU_PRED
};

enum MVPartition { MV_TOP_BOTTOM, MV_LEFT_RIGHT, MV_QUARTERS, MV_16 };

enum MacroBlockMV { MV_NEAREST, MV_NEAR, MV_ZERO, MV_NEW, MV_SPLIT };

enum SubBlockMV { LEFT_4x4, ABOVE_4x4, ZERO_4x4, NEW_4x4 };

static const std::vector<int16_t> kKeyFrameYModeTree = {
    -B_PRED, 2, 4, 6, -DC_PRED, -V_PRED, -H_PRED, -TM_PRED};

static const std::vector<int16_t> kYModeTree{
    -DC_PRED, 2, 4, 6, -V_PRED, -H_PRED, -TM_PRED, -B_PRED};

static const std::vector<int16_t> kUVModeTree = {-DC_PRED, 2,       -V_PRED,
                                                 4,        -H_PRED, -TM_PRED};

static const std::vector<int16_t> kSubBlockModeTree{
    -B_DC_PRED, 2,  -B_TM_PRED, 4,  -B_VE_PRED, 6,
    8,          12, -B_HE_PRED, 10, -B_RD_PRED, -B_VR_PRED,
    -B_LD_PRED, 14, -B_VL_PRED, 16, -B_HD_PRED, -B_HU_PRED};

static const std::vector<uint8_t> kModeProb[6]{
    {7, 1, 1, 143},    {14, 18, 14, 107},   {135, 64, 57, 68},
    {60, 56, 128, 65}, {159, 134, 128, 34}, {234, 188, 128, 28}};

}  // namespace vp8

#endif  // PREDICT_MODE_H_
