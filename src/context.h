#ifndef CONTEXT_H_
#define CONTEXT_H_

namespace vp8 {

struct Context {
  bool is_inter_mb;
  uint16_t ctx;

  // Inter-prediction context.
  uint8_t ref_frame() const { return uint8_t(ctx & 255); }
  MacroBlockMV mv_mode() const { return MacroBlockMV(ctx >> 8); }

  // Intra-prediction context.
  SubBlockMode mode(size_t i) const {
    return SubBlockMode(ctx >> (i << 2) & 15);
  }
  void append(size_t i, SubBlockMode sub) {
    ctx &= ~(15 << (i << 2));
    ctx |= uint16_t(sub) << (i << 2);
  }

  Context() : is_inter_mb(false), ctx(0) {}

  Context(bool b) : is_inter_mb(b), ctx(0) {}

  Context(uint16_t modes) : is_inter_mb(false), ctx(modes) {}

  Context(MacroBlockMV mv_mode_, uint8_t ref_frame_)
      : is_inter_mb(true), ctx(uint16_t(mv_mode_ << 8 | ref_frame_)) {}
};

}  // namespace vp8

#endif  // CONTEXT_H_
