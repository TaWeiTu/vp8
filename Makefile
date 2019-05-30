CXX = clang++ 
DBGFLAGS = -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope -fstack-protector-all -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -std=c++17 -Og -g3 -Wno-padded -march=native
CFLAGS = -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -std=c++17 -O3 -march=native 
CHECK = cppcheck --enable=all --inconclusive --check-config

all: src/bool_decoder.o src/intra_predict.o src/inter_predict.o src/dct.o

src/bool_decoder.o: src/bool_decoder.cc src/bool_decoder.h
	$(CHECK) src/bool_decoder.cc
	$(CXX) $(CFLAGS) -c -o src/bool_decoder.o src/bool_decoder.cc 

src/intra_predict.o: src/intra_predict.cc src/intra_predict.h src/frame.h
	$(CHECK) src/intra_predict.cc
	$(CXX) $(CFLAGS) -c -o src/intra_predict.o src/intra_predict.cc 

src/inter_predict.o: src/inter_predict.cc src/inter_predict.h src/frame.h
	$(CHECK) src/inter_predict.cc
	$(CXX) $(CFLAGS) -c -o src/inter_predict.o src/inter_predict.cc 

src/dct.o: src/dct.cc src/dct.h
	$(CHECK) src/dct.cc
	$(CXX) $(CFLAGS) -c -o src/dct.o src/dct.cc 

.PHONY: clean
clean: 
	rm src/*.o

.PHONY: test
test: test/main.cc test/dct_test.h src/dct.o
	$(CXX) $(CFLAGS) src/dct.o test/main.cc
	./a.out
	rm ./a.out
