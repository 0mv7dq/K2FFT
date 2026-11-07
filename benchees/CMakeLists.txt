##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

set(BENCH_PFFFT_DIR "${K2FFT_DIR}/libs/pffft")

# bench_runs(<out> <isa> <test>): cmake/simd.cmake's flags for isa if this machine (or its emulator) runs test w/ them, else empty
function(bench_runs out isa test)
    set(${out} "" PARENT_SCOPE)
    simd_flags(flags ${isa})
    if(NOT flags)
        return()
    endif()
    include(CheckCSourceRuns)
    list(JOIN flags " " CMAKE_REQUIRED_FLAGS)
    unset(BENCH_RUNS_${isa} CACHE) # the machine may have changed
    check_c_source_runs("${test}" BENCH_RUNS_${isa})
    if("${BENCH_RUNS_${isa}}" STREQUAL "1") # an emulated crash leaves FAILED_TO_RUN, which if() takes for true
        set(${out} ${flags} PARENT_SCOPE)
    endif()
endfunction()

# the SIMD extensions K2FFT's PFFFT copies (rdft/pffft_*.c) are built for
set(BENCH_AVX2_FLAGS "")
set(BENCH_NEON_FLAGS "")
set(BENCH_RVV_FLAGS "")
if(BENCH_X86_64)
    bench_runs(BENCH_AVX2_FLAGS avx2_fma "#include <immintrin.h>\nint main(void){ __m256 a = _mm256_set1_ps(1.0f); return _mm256_cvtss_f32(_mm256_fmadd_ps(a, a, a))!=2.0f; }")
elseif(ARM32 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    bench_runs(BENCH_NEON_FLAGS neon "#include <arm_neon.h>\nint main(void){ float32x4_t a = vdupq_n_f32(1.0f); return vgetq_lane_f32(vaddq_f32(a, a), 0)!=2.0f; }")
    set(BENCH_NEON_DEFINITIONS PFFFT_ENABLE_NEON=1)
elseif(TARGET pffft_rvv) # where K2FFT builds its copy
    bench_runs(BENCH_RVV_FLAGS rvv "#include <riscv_vector.h>\nint main(void){ size_t n = __riscv_vsetvl_e32m1(4); return __riscv_vfmv_f_s_f32m1_f32(__riscv_vfmv_v_f_f32m1(2.0f, n))!=2.0f; }")
    set(BENCH_RVV_DEFINITIONS PFFFT_ENABLE_RVV=1)
endif()

add_subdirectory(ipp)
add_subdirectory(mkl)
add_subdirectory(aocl)
add_subdirectory(armpl)
add_subdirectory(kfr)
add_subdirectory(ne10)
add_subdirectory(ffts)
add_subdirectory(vdsp)
add_subdirectory(pffft)
add_subdirectory(kissfft)
add_subdirectory(pocketfft)
add_subdirectory(cmsis)
add_subdirectory(fftw3)
add_subdirectory(zldsp)
add_subdirectory(espdsp)
add_benchee(pffft_twin SOURCES pffft/doit.c LIBRARIES PFFFT::PFFFT) # pffft again: the noise floor
