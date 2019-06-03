#ifndef INTRA_PREDICT_H_
#define INTRA_PREDICT_H_

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include "bitstream_parser.h"
#include "frame.h"
#include "utils.h"

namespace vp8 {

struct IntraContext {
  bool is_intra_mb;
  SubBlockMode mode;

  IntraContext() : is_intra_mb(false) {}

  explicit IntraContext(SubBlockMode mode_) : is_intra_mb(true), mode(mode_) {}

  explicit IntraContext(bool is_intra_mb_, SubBlockMode mode_)
      : is_intra_mb(is_intra_mb_), mode(mode_) {}
};

namespace {

void VPredChroma(size_t r, size_t c, Plane<2> &mb);
void HPredChroma(size_t r, size_t c, Plane<2> &mb);
void DCPredChroma(size_t r, size_t c, Plane<2> &mb);
void TMPredChroma(size_t r, size_t c, Plane<2> &mb);

void VPredLuma(size_t r, size_t c, Plane<4> &mb);
void HPredLuma(size_t r, size_t c, Plane<4> &mb);
void DCPredLuma(size_t r, size_t c, Plane<4> &mb);
void TMPredLuma(size_t r, size_t c, Plane<4> &mb);

void BPredLuma(size_t r, size_t c,
               std::vector<std::vector<IntraContext>> &context,
               BitstreamParser &ps, Plane<4> &mb);
void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p,
                   SubBlockMode mode, SubBlock &sub);
}  // namespace

void IntraPredict(size_t r, size_t c,
                  std::vector<std::vector<IntraContext>> &context,
                  BitstreamParser &ps, Frame &frame);

}  // namespace vp8

#endif  // INTRA_PREDICT_H_
