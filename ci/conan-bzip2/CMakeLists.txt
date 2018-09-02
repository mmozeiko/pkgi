project(bzip2)
cmake_minimum_required(VERSION 2.8)
include(../conanbuildinfo.cmake)
CONAN_BASIC_SETUP()

if(MSVC OR MSVC90 OR MSVC10)
  set(MSVC ON)
endif (MSVC OR MSVC90 OR MSVC10)

set(bzip2_STAT_SRCS blocksort.c bzlib.c compress.c  crctable.c  decompress.c  huffman.c randtable.c)

add_library(bz2 ${bzip2_STAT_SRCS})

if(FPIC)
    MESSAGE(WARNING "ENABLED FPIC")
	SET_TARGET_PROPERTIES(bz2 PROPERTIES COMPILE_FLAGS "-fPIC")
endif()
