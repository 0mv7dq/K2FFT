##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

set(dir "${BENCH_PFFFT_DIR}/pocketfft")
if(NOT EXISTS "${dir}/pocketfft_single.c")
    message(STATUS "bench: pocketfft left out: no ${dir} (git submodule update --init --recursive)")
    return()
endif()
add_library(bench_pocketfft STATIC "${dir}/pocketfft_single.c")
target_include_directories(bench_pocketfft PUBLIC "${dir}")
if(NOT MSVC)
    target_link_libraries(bench_pocketfft PUBLIC m)
endif()
add_benchee(pocketfft CLASSES SOURCES doit.c LIBRARIES bench_pocketfft)
if(TEST pocketfft_alloc)
    set_tests_properties(pocketfft_alloc PROPERTIES WILL_FAIL TRUE) # pocketfft mallocs on every transform
endif()
