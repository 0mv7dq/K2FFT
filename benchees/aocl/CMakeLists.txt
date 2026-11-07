##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#
if(NOT TARGET aocl_fftw3f)
    message(STATUS "bench: aocl left out: the library doesn't use AOCL-FFTW on this platform")
    return()
endif()
set(plan FFTW_MEASURE)
if(CMAKE_CROSSCOMPILING) # planning would time the emulator
    set(plan FFTW_ESTIMATE)
endif()
add_benchee(aocl CLASSES SOURCES doit.c "${K2FFT_DIR}/simd-support/simd.c" LIBRARIES aocl_fftw3f DEFINITIONS PLAN=${plan})
target_include_directories(aocl_doit PRIVATE "${K2FFT_DIR}")
target_include_directories(aocl_doit PRIVATE "${AOCL_DIR}/include")
