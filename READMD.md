# ITCT Final Project - VP8 Decoder #

## Team members ##
* B07902024 塗大為
* B07902134 黃于軒
* B07902141 林庭風

## Compilation and Execution ##
The decoder supports two modes: decode and display.

* decode:
```
make
./decode [path to the compressed input video] [path to the output video]
```

In decode mode, the input data is decoded into ```yuv``` format.


* display
```
make display
./display [path to the compressed input video]
```

In display mode, the decoded video is displayed simultaneously.
