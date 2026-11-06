##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
include(CheckCSourceCompiles)

set(SIMD_TEST_avx2_fma "#include <immintrin.h>\nint main(void){ __m256 a = _mm256_set1_ps(1.0f); a = _mm256_fmadd_ps(a, a, a); return (int)_mm256_cvtss_f32(a); }")
set(SIMD_FLAGS_avx2_fma "-mavx2 -mfma" "/arch:AVX2")
set(SIMD_TEST_neon "#include <arm_neon.h>\nint main(void){ float32x4_t a = vdupq_n_f32(1.0f); a = vaddq_f32(a, a); return (int)vgetq_lane_f32(a, 0); }")
set(SIMD_FLAGS_neon "-march=armv7-a -mfpu=neon" "-march=armv7-a -mfpu=neon -mfloat-abi=softfp") # softfp keeps armel's soft-float ABI
set(SIMD_TEST_rvv "#include <riscv_vector.h>\nint main(void){ size_t n = __riscv_vsetvl_e32m1(4); vfloat32m1_t a = __riscv_vfmv_v_f_f32m1(1.0f, n); return (int)__riscv_vfmv_f_s_f32m1_f32(a); }")
set(SIMD_FLAGS_rvv "-march=rv64gcv")

# out = the first of isa's flag sets that builds its test, or empty
function(simd_flags out isa)
    foreach(flags ${SIMD_FLAGS_${isa}})
        string(MAKE_C_IDENTIFIER "SIMD_${isa}_${flags}" ok)
        unset(${ok} CACHE) # CMAKE_C_FLAGS may have changed
        set(CMAKE_REQUIRED_FLAGS "${flags}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_c_source_compiles("${SIMD_TEST_${isa}}" ${ok} FAIL_REGEX "unrecognized|unknown|ignoring|not supported|invalid|D9002")
        if(${ok})
            separate_arguments(list NATIVE_COMMAND "${flags}")
            set(${out} ${list} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out} "" PARENT_SCOPE)
endfunction()

# pffft_simd_copy(<target> <isa> [DEFINITIONS ...]): PFFFT renamed <target>_* for rdft/<target>.c, built w/ isa's flags, unless the compiler has none
function(pffft_simd_copy target isa)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "DEFINITIONS")
    if(TARGET ${target})
        return()
    endif()
    simd_flags(flags ${isa})
    if(flags)
        pffft_copy(${target} ${target})
        target_compile_options(${target} PRIVATE ${flags})
        target_compile_definitions(${target} PRIVATE ${arg_DEFINITIONS})
    else()
        message(STATUS "no ${isa} flags build with this compiler: ${target} is left out")
    endif()
endfunction()
