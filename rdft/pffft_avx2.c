// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT built for AVX2 + FMA (cmake/simd.cmake's pffft_avx2_copy), where the CPU has them */
#include "simd-support/simd.h"
#define COPY pffft_avx2
#define RUNS simd_avx2_fma()
#include "pffft.c"
