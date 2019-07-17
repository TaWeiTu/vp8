#ifndef CONTEXT_H_
#define CONTEXT_H_

namespace vp8 {

struct Context {
  bool is_inter_mb;
  union {
    uint16_t as_intra;
    struct {
      MacroBlockMV mv_mode;
      uint8_t ref_frame;
    } as_inter;
  } ctx;

  Context() : is_inter_mb(false) { ctx.as_intra = 0; }

  Context(bool is_inter_mb_) : is_inter_mb(is_inter_mb_) {
    if (!is_inter_mb) ctx.as_intra = 0;
  }

  Context(uint16_t modes) : is_inter_mb(false) { ctx.as_intra = modes; }

  Context(MacroBlockMV mv_mode_, uint8_t ref_frame_) : is_inter_mb(true) {
    ctx.as_inter = {mv_mode_, ref_frame_};
  }
};

}  // namespace vp8

#endif  // CONTEXT_H_
