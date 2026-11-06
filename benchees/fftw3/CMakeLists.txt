##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# K2FFT's FFTW (cmake/fftw.cmake), which rdft/fftw3.c runs, else the system's
set(definitions "")
if(TARGET fftw3f)
    set(fftw fftw3f)
    set(definitions "RELEASE=\"${FFTW_RELEASE}\"") # FFTW 3.3.11's CMakeLists.txt still names 3.3.10, which fftwf_version reports
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3F QUIET IMPORTED_TARGET fftw3f)
    endif()
    if(NOT FFTW3F_FOUND)
        message(STATUS "bench: fftw3 left out: K2FFT builds no FFTW here, and pkg-config finds no fftw3f")
        return()
    endif()
    list(LENGTH CMAKE_OSX_ARCHITECTURES slices)
    if(slices GREATER 1)
        message(STATUS "bench: fftw3 left out: the system's fftw3f has one architecture, this build ${slices}")
        return()
    endif()
    set(fftw PkgConfig::FFTW3F)
endif()
set(plan FFTW_MEASURE)
if(CMAKE_CROSSCOMPILING) # planning would time the emulator
    set(plan FFTW_ESTIMATE)
endif()
add_benchee(fftw3 SOURCES doit.c LIBRARIES ${fftw} DEFINITIONS PLAN=${plan} TIMELIMIT=-1.0 ${definitions}) # -1: FFTW_NO_TIMELIMIT, as rdft/fftw3.c
