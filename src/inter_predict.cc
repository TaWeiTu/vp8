#include "inter_predict.h"

namespace vp8 {
namespace {

std::array<MotionVector, 3> SearchMVs(size_t r, size_t c,
                                      const FrameHeader &header,
                                      const LumaBlock &mb) {
  static std::array<uint8_t, 4> cnt;
  static const MotionVector ZERO = MotionVector(0, 0);
  std::fill(cnt.begin(), cnt.end(), 0);
  std::vector<MotionVector> mv;

  auto &mh = header.macroblock_header;

  if (r == 0 || mh.[r - 1][c].is_inter_mb) {
    MotionVector v = r ? mb[r - 1][c].GetMotionVector() : ZERO;
    if (r > 0) v = Invert(v, mh[r - 1][c]);
    if (v != ZERO) mv.push_back(v);
    cnt[mv.size()] += 2;
  }

  if (c == 0 || mh.[r][c - 1].is_inter_mb) {
    MotionVector v = c ? mb[r][c - 1].GetMotionVector() : ZERO;
    if (c > 0) v = Invert(v, mh[r][c - 1]);
    if (v != ZERO) {
      if (!mv.empty() && mv.back() != v) mv.push_back(v);
      cnt[mv.size()] += 2;
    } else {
      cnt[0] += 2;
    }
  }

  if (r == 0 || c == 0 || mh.[r - 1][c - 1].is_inter_mb) {
    MotionVector v = r && c ? mb[r - 1][c - 1].GetMotionVector() : ZERO;
    if (r > 0 && c > 0) v = Invert(v, mh[r - 1][c - 1]);
    if (v != ZERO) {
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
  return std::array<MotionVector, 3>{mv[0], mv[1], mv[2]};
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

}  // namespace
}  // namespace vp8
