CXX = clang++ 
DBGFLAGS = -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope -fstack-protector-all -Weverything -Wno-c++98-compat-pedantic -std=c++17 -Og -g3 -march=native
CFLAGS = -Weverything -Wno-c++98-compat-pedantic -std=c++17 -O3 -march=native

