// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* AOCL-FFTW's real DFT, as rdft/fftw3.c and benchees/aocl: its API renamed aoclf_ (cmake/aocl.cmake), declared here, as its
   fftw3.h would shadow K2FFT's own FFTW's */
#include "kernel/kernel.h"
#include "simd-support/simd.h"

typedef struct aoclf_plan_s *aoclf_plan;
aoclf_plan aoclf_plan_dft_r2c_1d(int n, float *in, float (*out)[2], unsigned flags);
void aoclf_execute_dft_r2c(const aoclf_plan p, float *in, float (*out)[2]);
void aoclf_destroy_plan(aoclf_plan p);
#define MEASURE 0U /* fftw3.h's FFTW_ flags */
#define PRESERVE_INPUT (1U<<4)
#define ESTIMATE (1U<<6)
#define NO_BUFFERING (1U<<11)

/* its AVX2 build (cmake/aocl.cmake) */
static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return size>0 && simd_avx2_fma();
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	float *in = pb->in, *out = pb->out;
	unsigned flags = (pb->measure ? MEASURE : ESTIMATE)|PRESERVE_INPUT|NO_BUFFERING;
	return aoclf_plan_dft_r2c_1d(size, in, (float (*)[2])out, flags);
}

/* size/2+1 (re, im) pairs */
static void execute(const void *p_, void *in_, void *out_){
	void *p = (void *)p_; /* the library's execute takes it w/o const */
	float *in = in_, *out = out_;
	aoclf_execute_dft_r2c(p, in, (float (*)[2])out);
}

static void destroy(void *p){
	aoclf_destroy_plan(p);
}

const kernel k_aocl = {"aocl", can_do, make, execute, destroy};
