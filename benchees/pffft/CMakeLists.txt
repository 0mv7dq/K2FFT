##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

add_benchee(pffft SOURCES doit.c LIBRARIES PFFFT::PFFFT)

if(BENCH_AVX2_FLAGS)
    add_library(bench_pffft_avx2 STATIC "${BENCH_PFFFT_DIR}/src/pffft.c" "${BENCH_PFFFT_DIR}/src/pffft_common.c")
    target_include_directories(bench_pffft_avx2 PUBLIC "${BENCH_PFFFT_DIR}/include" PRIVATE "${BENCH_PFFFT_DIR}/include/pffft")
    target_compile_definitions(bench_pffft_avx2 PUBLIC PFFFT_STATIC_DEFINE PRIVATE _USE_MATH_DEFINES)
    target_compile_options(bench_pffft_avx2 PRIVATE ${BENCH_AVX2_FLAGS})
    if(NOT MSVC)
        target_link_libraries(bench_pffft_avx2 PUBLIC m)
    endif()
    add_benchee(pffft_avx2 SOURCES doit.c LIBRARIES bench_pffft_avx2)
elseif(BENCH_X86_64)
    message(STATUS "bench: pffft_avx2 left out: this machine or compiler can't run AVX2 and FMA code")
endif()
