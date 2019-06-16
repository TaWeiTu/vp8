# ITCT Final Project - VP8 Decoder #

## Team members ##
* B07902024 塗大為
* B07902134 黃于軒
* B07902141 林庭風

## Compilation and Execution ##
The decoder supports two modes: decode and display. The only supported input format is ```ivf```.

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


## Test ## 
The decoder is tested with the [test-vectors](https://github.com/webmproject/vp8-test-vectors). The script can be found in ```test/test_vector.py```.

To test the decoder:

```
VP8_TEST_VECTORS=[path to the test-vectors directory] make test
```


## References ##
* https://tools.ietf.org/html/rfc6386
