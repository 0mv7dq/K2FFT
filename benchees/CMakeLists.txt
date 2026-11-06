##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

set(BENCH_PFFFT_DIR "${PUREFFT_DIR}/libs/pffft")

# AVX2 + FMA, if this machine runs them
set(BENCH_AVX2_FLAGS "")
if(BENCH_X86_64)
    include(CheckCSourceRuns)
    if(MSVC)
        set(flags /arch:AVX2)
    else()
        set(flags -mavx2 -mfma)
    endif()
    list(JOIN flags " " CMAKE_REQUIRED_FLAGS)
    unset(BENCH_AVX2 CACHE) # the machine may have changed
    check_c_source_runs("#include <immintrin.h>\nint main(void){ __m256 a = _mm256_set1_ps(1.0f); return _mm256_cvtss_f32(_mm256_fmadd_ps(a, a, a))!=2.0f; }" BENCH_AVX2)
    unset(CMAKE_REQUIRED_FLAGS)
    if(BENCH_AVX2)
        set(BENCH_AVX2_FLAGS ${flags})
    endif()
endif()

add_subdirectory(ipp)
add_subdirectory(kfr)
add_subdirectory(ne10)
add_subdirectory(vdsp)
add_subdirectory(pffft)
add_subdirectory(kissfft)
add_subdirectory(pocketfft)
add_subdirectory(fftw3)
add_subdirectory(zldsp)
add_benchee(pffft_twin SOURCES pffft/doit.c LIBRARIES PFFFT::PFFFT) # pffft again: the noise floor
