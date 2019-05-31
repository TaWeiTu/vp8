# APIs #

## Bool Decoder ##
* `BoolDecoder(const std::string &filename)` - Initialize the decoder which decodes the context of `filename`.
* `BoolDecoder(std::unique_ptr<std::ifstream> fs)` - Initialize the decoder which decodes the context of `fs`.
* `uint8_t Bool(uint8_t prob)` - Decodes a boolean value based on the given probability `prob`.
* `uint16_t Lit(size_t n)` - Decodes a `n`-bit unsigned literal (each bit is decoded with probability 0.5).
* `int16_t SignedLit(size_t n)` - Decodes a `n`-bit signed literal (each bit is decoded with probability 0.5).
* `uint8_t Prob8()` - Decodes a 8-bit probability.
* `uint8_t Prob7()` - Decodes a 7-bit probability `x` and return `x ? x << 1 : 1`.
* `int16_t Tree(const std::vector<uint8_t> &prob, const std::vector<int16_t> &tree)` - Decodes a token from `tree` (based on the probability table `prob`).
* `uint32_t ReadUncoded(size_t n)` - Reads a `n`-bit unsigned integer from the stream (frame tags).

## Frame ##
A frame is a `struct` holding 
* `size_t hsize, size_t vsize` - The number of pixels (horizontally and vertically, respectively).
* `size_t hblock, size_t vblock` - The number of macroblocks (horizontally and vertically, respectively).
* `Plane<LUMA> Y; Plane<CHROMA> U, V` - The YUV planes.

### SubBlock ###
* `void FillWith(int16_t v)` - Fill the subblock with `v`.
* `void FillRow(const std::array<int16_t, 4> &row)` - Fill each rows of the subblock with `row`.
* `void FillCol(const std::array<int16_t, 4> &col)` - Fill each colums of the subblock with `col`.
* `std::array<int16_t, 4> GetRow(size_t idx)` - Returns the `idx`-th row of the subblock.
* `std::array<int16_t, 4> GetCol(size_t idx)` - Returns the `idx`-th column of the subblock.
* `MotionVector GetMotionVector()` - Returns the motion vector of the subblock.
* `void SetMotionVector(int16_t dr, int16_t rc)` - Set the motion vector of the subblock to `(dr, dc)`.
* `void SetMotionVector(const MotionVector &mv)` - Set the motion vector of the subblock to `mv`.

### MacroBlock ###
* Template argument `C` is required which indicates the number of subblocks in this macroblock (`C` by `C`). `C = 4` for Luma and `C = 2` for Chroma.
* `void FillWith(int16_t v)` - Fill the macroblock with `v`.
* `void FillRow(const std::array<int16_t, C * 4> &row)` - Fill each rows of the macroblock with `row`.
* `void FillCol(const std::array<int16_t, C * 4> &col)` - Fill each colums of the macroblock with `col`.
* `std::array<int16_t, C * 4> GetRow(size_t idx)` - Returns the `idx`-th row of the macroblock.
* `std::array<int16_t, C * 4> GetCol(size_t idx)` - Returns the `idx`-th column of the macroblock.
* `MotionVector GetMotionVector()` - Returns the representative motion vector of the macroblock.
* `void SetMotionVector(int16_t dr, int16_t rc)` - Set the motion vector of the macroblock to `(dr, dc)`.
* `void SetMotionVector(const MotionVector &mv)` - Set the motion vector of the macroblock to `mv`.
* `void SetSubBlockMVs(const MotionVector &mv)` - Set the motion vector of each subblocks in th macroblock to `mv`.
* `MotionVector GetSubBlockMV(size_t idx)` - Returns the motion vector of the `idx`-th subblock in the macroblock (subblocks are numbered by the raster-scan order).
* `MotionVector GetSubBlockMV(size_t r, size_t c)` - Returns the motion vector of the subblock located in `(r, c)`.
* `int16_t GetPixel(size_t r, size_t c)` - Returns the pixel on position `(r, c)`.
* `int16_t SetPixel(size_t r, size_t c, int16_t v)` - Set the pixel on position `(r, c)` to `v`.

### Plane ### 
* Template argument `C` is required which indicates the number of subblocks in each macroblocks (`C` by `C`). `C = 4` for Luma and `C = 2` for Chroma.
* `int16_t GetPixel(size_t r, size_t c)` Returns the pixel on position `(r, c)`.


## Intra Prediction ## 
* `void IntraPredict(const FrameHeader &header, Frame &frame)` - For each macroblock in `frame`, decode it (excluding residuals) if it's intra-coded (based on `header`).

## Inter Prediction ##
* `void InterPredict(const FrameHeader &header, Frame &frame)` - For each macroblock in `frame`, decode it (excluding residuals) if it's inter-coded (based on `header`).
