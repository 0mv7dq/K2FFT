##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#
if(NOT TARGET mkl_dft)
    message(STATUS "bench: mkl left out: the library doesn't use oneMKL on this platform")
    return()
endif()
add_benchee(mkl CLASSES SOURCES doit.c "${K2FFT_DIR}/simd-support/simd.c" LIBRARIES mkl_dft)
add_benchee(mkl-d DOUBLE CLASSES SOURCES doit.c "${K2FFT_DIR}/simd-support/simd.c" LIBRARIES mkl_dft)
target_include_directories(mkl_doit PRIVATE "${K2FFT_DIR}")
target_include_directories(mkl-d_doit PRIVATE "${K2FFT_DIR}")
