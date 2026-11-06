##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

add_benchee(pffft SOURCES doit.c LIBRARIES PFFFT::PFFFT)

# PFFFT built for an extension the baseline lacks, as K2FFT's copies (rdft/pffft_*.c)
foreach(isa avx2 neon rvv)
    string(TOUPPER ${isa} ISA)
    if(NOT BENCH_${ISA}_FLAGS)
        continue()
    endif()
    add_library(bench_pffft_${isa} STATIC "${BENCH_PFFFT_DIR}/src/pffft.c" "${BENCH_PFFFT_DIR}/src/pffft_common.c")
    target_include_directories(bench_pffft_${isa} PUBLIC "${BENCH_PFFFT_DIR}/include" PRIVATE "${BENCH_PFFFT_DIR}/include/pffft")
    target_compile_definitions(bench_pffft_${isa} PUBLIC PFFFT_STATIC_DEFINE PRIVATE _USE_MATH_DEFINES ${BENCH_${ISA}_DEFINITIONS})
    target_compile_options(bench_pffft_${isa} PRIVATE ${BENCH_${ISA}_FLAGS})
    if(NOT MSVC)
        target_link_libraries(bench_pffft_${isa} PUBLIC m)
    endif()
    add_benchee(pffft_${isa} SOURCES doit.c LIBRARIES bench_pffft_${isa})
endforeach()
if(BENCH_X86_64 AND NOT BENCH_AVX2_FLAGS)
    message(STATUS "bench: pffft_avx2 left out: this machine or compiler can't run AVX2 and FMA code")
endif()
