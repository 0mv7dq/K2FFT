##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

set(dir "${BENCH_PFFFT_DIR}/kissfft")
if(NOT EXISTS "${dir}/tools/kiss_fftr.c")
    message(STATUS "bench: kissfft left out: no ${dir} (git submodule update --init --recursive)")
    return()
endif()
add_library(bench_kissfft STATIC "${dir}/kiss_fft.c" "${dir}/tools/kiss_fftr.c")
target_include_directories(bench_kissfft PUBLIC "${dir}" "${dir}/tools")
target_compile_definitions(bench_kissfft PUBLIC kiss_fft_scalar=float)
if(NOT MSVC)
    target_link_libraries(bench_kissfft PUBLIC m)
endif()
add_benchee(kissfft CLASSES SOURCES doit.c LIBRARIES bench_kissfft)
