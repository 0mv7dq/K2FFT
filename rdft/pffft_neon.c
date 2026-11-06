// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT built for 32-bit ARM's NEON (cmake/ne10.cmake's copy), where the CPU has it */
#include "simd-support/simd.h"
#define COPY pffft_neon
#define RUNS simd_neon()
#include "pffft.c"
