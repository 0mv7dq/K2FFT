// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTW, K2FFT's build (cmake/fftw.cmake), float and double, as benchees/fftw3; the buffers its solvers take per transform from the
   plan's arena (rdft/fftw_heap.h), sized by a transform at planning, so fftw_execute never reaches the heap */
#define F(name) fftwf_##name
#define D(name) fftw_##name
#define LEND
#include "rdft/fftw_like.h"

const kernel k_fftw3 = {"fftw3", can_do, make, execute, destroy};
