// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* AOCL-FFTW, AMD's FFTW, float, as benchees/aocl: renamed aoclf_, its AVX2 build, its heap through rdft/fftw_heap.h as K2FFT's FFTW's
   (cmake/aocl.cmake) */
#include "simd-support/simd.h"
#define F(name) aoclf_##name
#define CAN(pb) simd_avx2_fma()
#define LEND
#include "rdft/fftw_like.h"

const kernel k_aocl = {"aocl", can_do, make, execute, destroy, 0};
