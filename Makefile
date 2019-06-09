CXX := clang++
DBGFLAGS := -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -DDEBUG -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope -fstack-protector-all -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-switch-enum -Wno-undefined-func-template -std=c++17 -Og -g3 -Wno-padded -march=native
CFLAGS := -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-switch-enum -Wno-undefined-func-template -std=c++17 -O3 -march=native
CHECK := cppcheck --enable=all --inconclusive --check-config --suppress=missingIncludeSystem

# CFLAGS := $(DBGFLAGS)

all: decode
	
decode: src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/reconstruct.o src/yuv.o src/residual.o src/decode.cc 
	$(CHECK) src/decode.cc
	$(CXX) $(CFLAGS) -o decode src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o src/quantizer.o src/filter.o src/bitstream_parser.o src/reconstruct.o src/yuv.o src/residual.o src/decode.cc

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

src/filter.o: src/filter.cc src/filter.h src/utils.h src/frame.h
	$(CHECK) src/filter.cc
	$(CXX) $(CFLAGS) -c -o src/filter.o src/filter.cc 

src/bitstream_parser.o: src/bitstream_parser.cc src/bitstream_parser.h src/bitstream_const.h src/bool_decoder.o
	$(CHECK) src/bitstream_parser.cc
	$(CXX) $(CFLAGS) -c -o src/bitstream_parser.o src/bitstream_parser.cc 

src/reconstruct.o: src/reconstruct.cc src/reconstruct.h src/bitstream_parser.o src/intra_predict.o src/inter_predict.o src/filter.o
	$(CHECK) src/reconstruct.cc
	$(CXX) $(CFLAGS) -c -o src/reconstruct.o src/reconstruct.cc 

src/residual.o: src/residual.cc src/residual.h src/quantizer.o src/dct.o
	$(CHECK) src/residual.cc
	$(CXX) $(CFLAGS) -c -o src/residual.o src/residual.cc

.PHONY: clean
clean: 
	rm src/*.o
	rm ./decode

.PHONY: test
test: test/main.cc test/dct_test.h src/dct.o test/yuv_test.h src/yuv.o src/utils.h
	$(CXX) $(CFLAGS) src/dct.o src/yuv.o test/main.cc
	./a.out
	rm ./a.out
