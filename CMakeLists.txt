cmake_minimum_required(VERSION 3.9.1)

project(VP8)
SET(CMAKE_CXX_COMPILER clang++)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
find_package(OpenCV REQUIRED)

include(AddCXXCompilerFlag)

option(DEBUG "Debug mode" OFF)

add_cxx_compiler_flag(-std=c++17)
add_cxx_compiler_flag(-Weverything)
add_cxx_compiler_flag(-Wno-c++98-compat-pedantic)
add_cxx_compiler_flag(-Wno-padded)
add_cxx_compiler_flag(-Wno-switch-enum)
add_cxx_compiler_flag(-Wno-weak-vtables)
add_cxx_compiler_flag(-march=native)
add_cxx_compiler_flag(-flto=full)
add_cxx_compiler_flag(-pthread)

if (DEBUG)
  add_cxx_compiler_flag(-fsanitize=undefined)
  add_cxx_compiler_flag(-fsanitize=address)
  add_cxx_compiler_flag(-fsanitize-address-use-after-scope)
  add_cxx_compiler_flag(-fstack-protector-all)
  add_cxx_compiler_flag(-fprofile-instr-generate)
  add_cxx_compiler_flag(-fcoverage-mapping)
  add_cxx_compiler_flag(-Og)
  add_cxx_compiler_flag(-g3)
  add_compile_definitions(DEBUG)
  add_compile_definitions(_GLIBCXX_DEBUG)
  add_compile_definitions(_GLIBCXX_DEBUG_PENDANTIC)
else ()
  add_cxx_compiler_flag(-O3)
endif (DEBUG)

file(GLOB LIB_SRC "src/*.cc")

list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/decode.cc")
list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/display.cc")
list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/encode.cc")

add_library(vp8 STATIC "${LIB_SRC}")

add_executable(decode src/decode.cc)
add_executable(display src/display.cc)

target_link_libraries(decode vp8)
target_link_libraries(display vp8 ${OpenCV_LIBS})
