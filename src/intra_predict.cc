#include "intra_predict.h"

namespace {

void VPred(size_t r, size_t c, Frame &frame) {
  VPredLuma(r, c, frame.YBlocks);
  VPredChroma(r, c, frame.UBlocks);
  VPredChroma(r, c, frame.VBlocks);
}

void HPred(size_t r, size_t c, Frame &frame) {
  HPredLuma(r, c, frame.YBlocks);
  HPredChroma(r, c, frame.UBlocks);
  HPredChroma(r, c, frame.VBlocks);
}

void DCPred(size_t r, size_t c, Frame &frame) {
  DCPredLuma(r, c, frame.YBlocks);
  DCPredChroma(r, c, frame.UBlocks);
  DCPredChroma(r, c, frame.VBlocks);
}

void TMPred(size_t r, size_t c, Frame &frame) {
  TMPredLuma(r, c, frame.YBlocks);
  TMPredChroma(r, c, frame.UBlocks);
  TMPredChroma(r, c, frame.VBlocks);
}

void VPredChroma(size_t r, size_t c, ChromaBlock &mb) {
  if (r == 0)
    mb[r][c].FillWith(127);
  else
    mb[r][c].FillRow(mb[r - 1][c].GetRow(7));
}

void HPredChroma(size_t r, size_t c, ChromaBlock &mb) {
  if (c == 0)
    mb[r][c].FillWith(127);
  else
    mb[r][c].FillCol(mb[r][c - 1].GetCol(7));
}

void DCPredChroma(size_t r, size_t c, ChromaBlock &mb) {
  if (r == 0 && c == 0) {
    mb[r][c].FillWith(128);
    return;
  }

  int32_t sum = 0, shf = 2;
  if (r > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb[r - 1][c].GetPixel(7, i);
    shf++;
  }
  if (c > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb[r][c - 1].GetPixel(i, 7);
    shf++;
  }

  int16_t avg = int16_t((sum + (1 << (shf - 1))) >> shf);
  mb[r][c].FillWith(avg);
}

void TMPredChroma(size_t r, size_t c, ChromaBlock &mb) {
  int16_t p = (r == 0 || c == 0 ? 128 : mb[r - 1][c - 1].GetPixel(7, 7));
  for (size_t i = 0; i < 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      int16_t x = (c == 0 ? 129 : mb[r][c - 1].GetPixel(i, 7));
      int16_t y = (r == 0 ? 127 : mb[r - 1][c].GetPixel(7, i));
      mb[r][c].SetPixel(i, j, x + y - p);
    }
  }
}

void VPredLuma(size_t r, size_t c, LumaBlock &mb) {
  if (r == 0)
    mb[r][c].FillWith(127);
  else
    mb[r][c].FillRow(mb[r - 1][c].GetRow(15));
}

void HPredLuma(size_t r, size_t c, LumaBlock &mb) {
  if (c == 0)
    mb[r][c].FillWith(127);
  else
    mb[r][c].FillCol(mb[r][c - 1].GetCol(15));
}

void DCPredLuma(size_t r, size_t c, LumaBlock &mb) {
  if (r == 0 && c == 0) {
    mb[r][c].FillWith(128);
    return;
  }

  int32_t sum = 0, shf = 3;
  if (r > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb[r - 1][c].GetPixel(15, i);
    shf++;
  }
  if (c > 0) {
    for (size_t i = 0; i < 8; ++i) sum += mb[r][c - 1].GetPixel(i, 15);
    shf++;
  }

  int16_t avg = int16_t((sum + (1 << (shf - 1))) >> shf);
  mb[r][c].FillWith(avg);
}

void TMPredLuma(size_t r, size_t c, LumaBlock &mb) {
  int16_t p = (r == 0 || c == 0 ? 128 : mb[r - 1][c - 1].GetPixel(15, 15));
  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j) {
      int16_t x = (c == 0 ? 129 : mb[r][c - 1].GetPixel(i, 15));
      int16_t y = (r == 0 ? 127 : mb[r - 1][c].GetPixel(15, i));
      mb[r][c].SetPixel(i, j, x + y - p);
    }
  }
}

}  // namespace

void IntraPredict(const FrameHeader &header, Frame &frame) {
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      switch (header.macroblock_header[r][c]) {
        case V_PRED:
          VPred(r, c, frame);
          break;

        case H_PRED:
          HPred(r, c, frame);
          break;

        case DC_PRED:
          DCPred(r, c, frame);
          break;

        case TM_PRED:
          TMPred(r, c, frame);
          break;

        default:
      }
    }
  }
}
