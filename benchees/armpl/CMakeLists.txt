##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#
if(NOT TARGET armpl_fft)
    message(STATUS "bench: armpl left out: the library doesn't use Arm Performance Libraries on this platform")
    return()
endif()
set(plan FFTW_MEASURE)
if(CMAKE_CROSSCOMPILING) # planning would time the emulator
    set(plan FFTW_ESTIMATE)
endif()
# its fftw3.h's names as cmake/armpl.cmake renamed its symbols, so the header's prototypes are the library's
file(STRINGS "${ARMPL_DIR}/rename.txt" pairs)
list(TRANSFORM pairs REPLACE " " "=" OUTPUT_VARIABLE renamed)
add_benchee(armpl CLASSES SOURCES doit.c LIBRARIES armpl_fft DEFINITIONS PLAN=${plan} ${renamed})
target_include_directories(armpl_doit PRIVATE "${ARMPL_ROOT}/include")
add_benchee(armpl-d DOUBLE CLASSES SOURCES doit.c LIBRARIES armpl_fft DEFINITIONS PLAN=${plan} ${renamed})
target_include_directories(armpl-d_doit PRIVATE "${ARMPL_ROOT}/include")
