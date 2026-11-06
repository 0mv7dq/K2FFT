##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(FFTW3F QUIET IMPORTED_TARGET fftw3f)
endif()
if(NOT FFTW3F_FOUND)
    message(STATUS "bench: fftw3 left out: no fftw3f found by pkg-config")
    return()
endif()
list(LENGTH CMAKE_OSX_ARCHITECTURES slices)
if(slices GREATER 1)
    message(STATUS "bench: fftw3 left out: the system's fftw3f has one architecture, this build ${slices}")
    return()
endif()
set(plan FFTW_MEASURE)
set(limit -1.0) # FFTW_NO_TIMELIMIT
if(CMAKE_CROSSCOMPILING) # planning would time the emulator
    set(plan FFTW_ESTIMATE)
elseif(BENCH_ARM64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(limit 1.0) # Debian builds its arm64 fftw3f --with-slow-timer, so FFTW_MEASURE takes minutes a size
endif()
add_benchee(fftw3 SOURCES doit.c LIBRARIES PkgConfig::FFTW3F DEFINITIONS PLAN=${plan} TIMELIMIT=${limit})
