#ifndef FILTER_H_
#define FILTER_H_

#include <vector>

#include "frame.h"
#include "inter_predict.h"
#include "intra_predict.h"
#include "utils.h"

namespace vp8 {
namespace internal {

inline int16_t minus128(int16_t x) {
  return int16_t(Clamp128(x - int16_t(128)));
}
inline int16_t plus128(int16_t x) {
  return int16_t(Clamp255(x + int16_t(128)));
}

namespace filter {

static int16_t p3_, p2_, p1_, p0_;
static int16_t q0_, q1_, q2_, q3_;

bool IsFilterNormal(int16_t interior, int16_t edge);

bool IsFilterSimple(int16_t edge);

bool IsHighVariance(int16_t threshold);

void Adjust(bool use_outer_taps);

void SubBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                    int16_t edge_limit);

void MacroBlockFilter(int16_t hev_threshold, int16_t interior_limit,
                      int16_t edge_limit);

void SimpleFilter(int16_t limit);

void InitHorizontal(const SubBlock &lsb, const SubBlock &rsb, size_t idx);
void FillHorizontal(SubBlock &lsb, SubBlock &rsb, size_t idx);

void InitVertical(const SubBlock &usb, const SubBlock &dsb, size_t idx);
void FillVertical(SubBlock &usb, SubBlock &dsb, size_t idx);

}  // namespace filter

void CalculateCoeffs(uint8_t loop_filter_level, uint8_t sharpness_level,
                     bool is_key_frame, uint8_t &interior_limit,
                     uint8_t &hev_threshold, int16_t &edge_limit_mb,
                     int16_t &edge_limit_sb);

template <size_t C>
void PlaneFilterNormal(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<InterContext>> &interc,
                       const std::vector<std::vector<IntraContext>> &intrac,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &nonzero,
                       Plane<C> &frame);

void PlaneFilterSimple(const FrameHeader &header, size_t hblock, size_t vblock,
                       bool is_key_frame,
                       const std::vector<std::vector<uint8_t>> &lf,
                       const std::vector<std::vector<uint8_t>> &skip_lf,
                       Plane<4> &frame);

}  // namespace internal

void FrameFilter(const FrameHeader &header, bool is_key_frame,
                 const std::vector<std::vector<uint8_t>> &lf,
                 const std::vector<std::vector<uint8_t>> &skip_lf,
                 Frame &frame);

}  // namespace vp8

#endif  // FILTER_H_
