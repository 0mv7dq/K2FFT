##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

option(BENCH_ZLDSP "build the zldsp benchee (Highway is the bench's largest build)" ON)
if(NOT BENCH_ZLDSP)
    return()
endif()
if(NOT BENCH_AVX2_FLAGS AND NOT (BENCH_ARM64 AND NOT MSVC))
    message(STATUS "bench: zldsp left out: it needs AVX2 on x86-64 or NEON on 64-bit ARM (with GCC or Clang)")
    return()
endif()
include("${K2FFT_DIR}/cmake/zldsp.cmake") # the sources and flags rdft/zldsp.c is built from
add_benchee(zldsp SOURCES doit.cpp LIBRARIES zldsp_fft DEFINITIONS HWY_COMPILE_ONLY_STATIC)
set_target_properties(zldsp_doit PROPERTIES CXX_STANDARD 20) # zldsp uses std::numbers and std::bit_width
target_compile_options(zldsp_doit PRIVATE ${ZLDSP_FLAGS})
