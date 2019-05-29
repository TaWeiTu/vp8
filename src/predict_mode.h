#ifndef PREDICT_MODE_H_
#define PREDICT_MODE_H_

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

}  // namespace vp8

#endif  // PREDICT_MODE_H_
