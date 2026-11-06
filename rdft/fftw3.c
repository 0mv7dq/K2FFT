// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTW's real DFT, as benchees/fftw3: FFTW_MEASURE (by estimate w/o timing, as there when cross-compiled),
   w/o the buffered plans, which allocate in fftwf_execute */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include <fftw3.h>

static int can_do(int size){
	return size>0;
}

static void *make(int size, float *in, float *out){
	unsigned flags = (race_timing() ? FFTW_MEASURE : FFTW_ESTIMATE)|FFTW_PRESERVE_INPUT|FFTW_NO_BUFFERING;
	return fftwf_plan_dft_r2c_1d(size, in, (fftwf_complex *)out, flags);
}

/* size/2+1 (re, im) pairs */
static void forward(void *p, float *in, float *out){
	fftwf_execute_dft_r2c(p, in, (fftwf_complex *)out);
}

static void destroy(void *p){
	fftwf_destroy_plan(p);
}

const kernel k_fftw3 = {"fftw3", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
