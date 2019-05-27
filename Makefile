CXX = clang++ 
DBGFLAGS = -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope -fstack-protector-all -Weverything -Wno-c++98-compat-pedantic -std=c++17 -Og -g3 -Wno-padded -march=native
CFLAGS = -Weverything -Wno-c++98-compat-pedantic -Wno-padded -std=c++17 -O3 -march=native

all: bool_decoder.o intra_predict.o

bool_decoder.o: src/bool_decoder.cc src/bool_decoder.h
	$(CXX) $(CFLAGS) -c -o src/bool_decoder.o src/bool_decoder.cc 

intra_predict.o: src/intra_predict.cc src/intra_predict.h src/frame.h
	$(CXX) $(CFLAGS) -c -o src/intra_predict.o src/intra_predict.cc 
