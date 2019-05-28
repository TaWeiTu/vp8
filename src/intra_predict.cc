#include "intra_predict.h"

namespace vp8 {
namespace {

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
      mb[r][c].SetPixel(i, j, clamp255(x + y - p));
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
      mb[r][c].SetPixel(i, j, clamp255(x + y - p));
    }
  }
}

void BPredLuma(size_t r, size_t c,
               const std::array<std::array<PredictionMode, 4>, 4> &pred,
               LumaBlock &mb) {
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      std::array<int16_t, 8> above;
      std::array<int16_t, 4> left;
      std::array<int16_t, 4> row_above;
      std::array<int16_t, 4> row_right;

      if (i == 0)
        row_above = r == 0 ? std::array<int16_t, 4>{127, 127, 127, 127}
                           : mb[r - 1][c][3][j].GetRow(3);
      else
        row_above = mb[r][c][i - 1][j].GetRow(3);

      if (j == 3) {
        if (r == 0)
          row_right = std::array<int16_t, 4>{127, 127, 127, 127};
        else if (c + 1 == mb[r].size())
          row_right = mb[r - 1][c][3][3].GetRow(3);
        else
          row_right = mb[r - 1][c + 1][3][0].GetRow(3);
      } else {
        if (i == 0)
          row_right = r == 0 ? std::array<int16_t, 4>{127, 127, 127, 127}
                             : mb[r - 1][c][3][j + 1].GetRow(3);
        else
          row_right = mb[r][c][i - 1][j + 1].GetRow(3);
      }

      std::copy(row_above.begin(), row_above.end(), above.begin());
      std::copy(row_right.begin(), row_right.end(), above.begin() + 4);

      if (j == 0)
        left = c == 0 ? std::array<int16_t, 4>{129, 129, 129, 129}
                      : left = mb[r][c - 1][i][3].GetCol(3);
      else
        left = mb[r][c][i][j - 1].GetCol(3);

      int16_t p = 0;
      if (i > 0 && j > 0)
        p = mb[r][c][i - 1][j - 1][3][3];
      else if (i > 0)
        p = c == 0 ? 129 : mb[r][c - 1][i - 1][3][3][3];
      else if (j > 0)
        p = r == 0 ? 127 : mb[r - 1][c][3][j - 1][3][3];
      else
        p = r == 0 && c == 0
                ? 128
                : r == 0 ? 127 : c == 0 ? 129 : mb[r - 1][c - 1][3][3][3][3];

      BPredSubBlock(above, left, p, pred[i][j], mb[r][c][i][j]);
    }
  }
}

void BPredSubBlock(const std::array<int16_t, 8> &above,
                   const std::array<int16_t, 4> &left, int16_t p,
                   PredictionMode mode, SubBlock &sub) {
  std::array<int16_t, 9> edge = {
      left[3],  left[2],  left[1],  left[0],  p,
      above[3], above[2], above[1], above[0],
  };
  switch (mode) {
    case B_VE_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        int16_t x = i == 0 ? p : above[i - 1];
        int16_t y = above[i];
        int16_t z = above[i + 1];
        int16_t avg = (x + y + y + z + 2) >> 2;
        sub[0][i] = sub[1][i] = sub[2][i] = sub[3][i] = avg;
      }
      break;
    }

    case B_HE_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        int16_t x = i == 0 ? p : left[i - 1];
        int16_t y = left[i];
        int16_t z = i == 3 ? above[3] : above[i + 1];
        int16_t avg = (x + y + y + z + 2) >> 2;
        sub[i][0] = sub[i][1] = sub[i][2] = sub[i][3] = avg;
      }
      break;
    }

    case B_DC_PRED: {
      int16_t v = 4;
      for (size_t i = 0; i < 4; ++i) v += above[i] + left[i];
      v >>= 8;
      sub.FillWith(v);
      break;
    }

    case B_TM_PRED: {
      for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j)
          sub[i][j] = clamp255(left[i] + above[j] - p);
      }
      break;
    }

    case B_LD_PRED: {
      for (int d = 0; d < 7; ++d) {
        int16_t x = above[d];
        int16_t y = above[d + 1];
        int16_t z = d + 2 < 8 ? above[d + 2] : above[7];
        int16_t avg = (x + y + y + z + 2) >> 2;
        for (size_t i = 0; d - i >= 0; ++i) {
          if (d - i < 4) sub[i][d - i] = avg;
        }
      }
      break;
    }

    case B_RD_PRED: {
      for (int d = 0; d < 7; ++d) {
        int16_t x = edge[d];
        int16_t y = edge[d + 1];
        int16_t z = edge[d + 2];
        int16_t avg = (x + y + y + z + 2) >> 2;
        for (size_t i = 0; i + d < 4; ++i) sub[i][d - i] = avg;
      }
      break;
    }

    case B_VR_PRED: {
      sub[3][0] = (edge[1] + edge[2] + edge[2] + edge[3] + 2) >> 2;
      sub[2][0] = (edge[2] + edge[3] + edge[3] + edge[4] + 2) >> 2;
      sub[3][1] = sub[1][0] = (edge[3] + edge[4] + edge[4] + edge[5] + 2) >> 2;
      sub[2][1] = sub[0][0] = (edge[4] + edge[5] + 1) >> 1;
      sub[3][2] = sub[1][1] = (edge[4] + edge[5] + edge[5] + edge[6] + 2) >> 2;
      sub[2][2] = sub[0][1] = (edge[5] + edge[6] + 1) >> 1;
      sub[3][3] = sub[1][2] = (edge[5] + edge[6] + edge[6] + edge[7] + 2) >> 2;
      sub[2][3] = sub[0][2] = (edge[6] + edge[7] + 1) >> 1;
      sub[1][3] = (edge[6] + edge[7] + edge[7] + edge[8] + 2) >> 2;
      sub[0][3] = (edge[7] + edge[8] + 1) >> 1;
      break;
    }

    case B_VL_PRED: {
      sub[0][0] = (above[0] + above[1] + 1) >> 1;
      sub[1][0] = (above[0] + above[1] + above[1] + above[2] + 2) >> 2;
      sub[2][0] = sub[0][1] = (above[1] + above[2] + 1) >> 1;
      sub[1][1] = sub[3][0] =
          (above[1] + above[2] + above[2] + above[3] + 2) >> 2;
      sub[2][1] = sub[0][2] = (above[2] + above[3] + 1) >> 1;
      sub[3][1] = sub[1][2] =
          (above[2] + above[3] + above[3] + above[4] + 2) >> 2;
      sub[2][2] = sub[0][3] = (above[3] + above[4] + 1) >> 1;
      sub[3][2] = sub[1][3] =
          (above[3] + above[4] + above[4] + above[5] + 2) >> 2;
      sub[2][3] = (above[4] + above[5] + above[5] + above[6] + 2) >> 2;
      sub[3][3] = (above[5] + above[6] + above[6] + above[7] + 2) >> 2;
      break;
    }

    case B_HD_PRED: {
      sub[3][0] = (edge[0] + edge[1] + 1) >> 1;
      sub[3][1] = (edge[0] + edge[1] + edge[1] + edge[2] + 2) >> 2;
      sub[2][0] = sub[3][2] = (edge[1] + edge[2] + 1) >> 1;
      sub[2][1] = sub[3][3] = (edge[1] + edge[2] + edge[2] + edge[3] + 2) >> 2;
      sub[2][2] = sub[1][0] = (edge[2] + edge[3] + 1) >> 1;
      sub[2][3] = sub[1][1] = (edge[2] + edge[3] + edge[3] + edge[4] + 2) >> 2;
      sub[1][2] = sub[0][0] = (edge[3] + edge[4] + 1) >> 1;
      sub[1][3] = sub[0][1] = (edge[3] + edge[4] + edge[4] + edge[5] + 2) >> 2;
      sub[0][2] = (edge[4] + edge[5] + edge[5] + edge[6] + 2) >> 2;
      sub[0][3] = (edge[5] + edge[6] + edge[6] + edge[7] + 2) >> 2;
      break;
    }

    case B_HU_PRED: {
      sub[0][0] = (left[0] + left[1] + 1) >> 1;
      sub[0][1] = (left[0] + left[1] + left[1] + left[2] + 2) >> 2;
      sub[0][2] = sub[1][0] = (left[1] + left[2] + 1) >> 1;
      sub[0][3] = sub[1][1] = (left[1] + left[2] + left[2] + left[3] + 2) >> 2;
      sub[1][2] = sub[2][0] = (left[2] + left[3] + 1) >> 1;
      sub[1][3] = sub[2][1] = (left[2] + left[3] + left[3] + left[3] + 2) >> 2;
      sub[2][2] = sub[2][3] = sub[3][0] = sub[3][1] = sub[3][2] = sub[3][3] =
          left[3];
      break;
    }

    default:
  }
}

}  // namespace

void IntraPredict(const FrameHeader &header, Frame &frame) {
  auto &Y = frame.YBlocks, &U = frame.UBlocks, &V = frame.VBlocks;
  for (size_t r = 0; r < frame.vblock; ++r) {
    for (size_t c = 0; c < frame.hblock; ++c) {
      if (header.macroblock_header[r][c].is_inter_mb) continue;

      auto &mh = header.macroblock_header[r][c];
      switch (mh.intra_y_mode) {
        case V_PRED:
          VPredLuma(r, c, Y);
          break;

        case H_PRED:
          HPredLuma(r, c, Y);
          break;

        case DC_PRED:
          DCPredLuma(r, c, Y);
          break;

        case TM_PRED:
          TMPredLuma(r, c, Y);
          break;

        case B_PRED:
          BPredLuma(r, c, mh.intra_b_mode, Y);
          break;

        default:
      }
      switch (mh.intra_uv_mode) {
        case V_PRED:
          VPredChroma(r, c, U);
          VPredChroma(r, c, V);
          break;

        case H_PRED:
          HPredChroma(r, c, U);
          HPredChroma(r, c, V);
          break;

        case DC_PRED:
          DCPredChroma(r, c, U);
          DCPredChroma(r, c, V);
          break;

        case TM_PRED:
          TMPredChroma(r, c, U);
          TMPredChroma(r, c, V);
          break;

        default:
      }
    }
  }
}

}  // namespace vp8
