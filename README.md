# VP8 Decoder #

## Compilation and Execution ##
The decoder supports two modes: decode and display. The only supported input format is ```ivf```.

* decode:
```
make
./decode [path to the compressed input video] [path to the output video]
```

In decode mode, the input data is decoded into ```yuv``` (I420p) format.

To play the `yuv` output, one can use the following command (requires `ffmpeg` to be installed):

```
ffplay -video_size [width]x[height] -framerate [framerate] -pix_fmt yuv420p [path to YUV file]
```


* display
```
make display
./display [path to the compressed input video]
```

(One may need to modify the OpenCV path `/usr/include/opencv4/` on a distro other than Arch Linux.)

In display mode, the decoded video is displayed simultaneously. Note that pressing the `Right` key allows the user to fast-forward (seek) the video.

For a longer video to try out the seeking feature, one can use <https://www.csie.ntu.edu.tw/~b07902134/aimer.ivf>. (Not included due to its size.)


## Test ## 
The decoder is tested with the [test-vectors](https://github.com/webmproject/vp8-test-vectors). The script can be found in ```test/test_vector.py```.

To test the decoder:

```
VP8_TEST_VECTORS=example/vp8-test-vectors/ make test
```

## VP8 ##
[VP8](https://www.webmproject.org/) is a video codec that is comparable to H.264 in terms of compression / quality. However, unlike H.264, VP8 is royality-free, and can usually be decoded at a higher speed. In addition, VP8, being in the VP family of codecs, can be said to be a predecessor of the new anticipated AV1 codec.


## References ##
* https://tools.ietf.org/html/rfc6386
