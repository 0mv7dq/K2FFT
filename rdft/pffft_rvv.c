// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT built for RVV 1.0 (cmake/fftw.cmake's copy), where the CPU has it */
#include "simd-support/simd.h"
#define COPY pffft_rvv
#define RUNS simd_rvv()
#include "pffft.c"
