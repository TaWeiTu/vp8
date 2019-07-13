CXX = clang++
DBGFLAGS = -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -DDEBUG -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope -fstack-protector-all -fprofile-instr-generate -fcoverage-mapping -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-switch-enum -Wno-undefined-func-template -std=c++17 -Og -g3 -Wno-padded -march=native
CFLAGS = -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-switch-enum -Wno-undefined-func-template -Wno-missing-prototypes -Wno-implicitly-unsigned-literal -std=c++17 -O3 -march=native
CVPATH ?= /usr/include/opencv4/
OPENCV = -I$(CVPATH) -lopencv_core -lopencv_imgproc -lopencv_highgui
CHECK = cppcheck --enable=all --inconclusive --check-config --suppress=missingIncludeSystem

all: decode

debug: CFLAGS = $(DBGFLAGS)
debug: decode
	
decode: src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/decode_frame.o src/yuv.o src/residual.o src/decode.cc 
	$(CHECK) src/decode.cc
	$(CXX) $(CFLAGS) -o decode src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/decode_frame.o src/yuv.o src/residual.o src/decode.cc


display: src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/decode_frame.o src/yuv.o src/residual.o src/display.cc
	$(CHECK) src/display.cc
	$(CXX) $(CFLAGS) $(OPENCV) -o display src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/decode_frame.o src/yuv.o src/residual.o src/display.cc

src/bool_decoder.o: src/bool_decoder.cc src/bool_decoder.h src/utils.h
	$(CHECK) src/bool_decoder.cc
	$(CXX) $(CFLAGS) -c -o src/bool_decoder.o src/bool_decoder.cc 

src/intra_predict.o: src/intra_predict.cc src/intra_predict.h src/utils.h src/frame.h src/bitstream_parser.o
	$(CHECK) src/intra_predict.cc
	$(CXX) $(CFLAGS) -c -o src/intra_predict.o src/intra_predict.cc 

src/inter_predict.o: src/inter_predict.cc src/inter_predict.h src/utils.h src/frame.h src/bitstream_parser.o
	$(CHECK) src/inter_predict.cc
	$(CXX) $(CFLAGS) -c -o src/inter_predict.o src/inter_predict.cc 

src/dct.o: src/dct.cc src/dct.h
	$(CHECK) src/dct.cc
	$(CXX) $(CFLAGS) -c -o src/dct.o src/dct.cc 

src/quantizer.o: src/quantizer.cc src/quantizer.h src/utils.h src/bitstream_parser.o
	$(CHECK) src/quantizer.cc
	$(CXX) $(CFLAGS) -c -o src/quantizer.o src/quantizer.cc 

src/yuv.o: src/yuv.cc src/yuv.h src/utils.h src/frame.h
	$(CHECK) src/yuv.cc
	$(CXX) $(CFLAGS) -c -o src/yuv.o src/yuv.cc 

src/filter.o: src/filter.cc src/filter.h src/utils.h src/frame.h src/intra_predict.o src/inter_predict.o
	$(CHECK) src/filter.cc
	$(CXX) $(CFLAGS) -c -o src/filter.o src/filter.cc 

src/bitstream_parser.o: src/bitstream_parser.cc src/bitstream_parser.h src/bitstream_const.h src/bool_decoder.o
	$(CHECK) src/bitstream_parser.cc
	$(CXX) $(CFLAGS) -c -o src/bitstream_parser.o src/bitstream_parser.cc 

src/decode_frame.o: src/decode_frame.cc src/decode_frame.h src/bitstream_parser.o src/intra_predict.o src/inter_predict.o src/filter.o
	$(CHECK) src/decode_frame.cc
	$(CXX) $(CFLAGS) -c -o src/decode_frame.o src/decode_frame.cc 

src/residual.o: src/residual.cc src/residual.h src/quantizer.o src/dct.o
	$(CHECK) src/residual.cc
	$(CXX) $(CFLAGS) -c -o src/residual.o src/residual.cc

src/bool_encoder.o: src/bool_encoder.cc src/bool_encoder.h
	$(CHECK) src/bool_encoder.cc
	$(CXX) $(CFLAGS) -c -o src/bool_encoder.o src/bool_encoder.cc
	
src/encode_frame.o: src/encode_frame.cc src/encode_frame.h src/intra_predict.o src/inter_predict.o
	$(CHECK) src/encode_frame.cc
	$(CXX) $(CFLAGS) -c -o src/encode_frame.o src/encode_frame.cc

.PHONY: clean
clean: 
	rm src/*.o
	rm ./decode
	rm ./display

.PHONY: test
test: test/main.cc test/dct_test.h src/dct.o test/yuv_test.h src/yuv.o src/utils.h test/intra_test.py decode
	$(CXX) $(CFLAGS) src/dct.o src/yuv.o test/main.cc
	@./a.out
	@rm ./a.out
	@echo '[Info] Start testing test vectors'
	@test/test_vector.py
	@echo '[Info] Done testing test vectors'
	@echo '[Info] Start testing comprehensives'
	@test/test_comprehensive.py
	@echo '[Info] Done testing comprehensives'
	@rm ./tmp_split.yuv
	@rm ./tmp_test.yuv

