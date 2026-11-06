// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Arm Performance Libraries' real DFT through its FFTW interface, as benchees/armpl: renamed armplf_ (cmake/armpl.cmake), declared
   here, as its fftw3.h would shadow K2FFT's own FFTW's */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include "kernel/race.h"

typedef struct armplf_plan_s *armplf_plan;
armplf_plan armplf_plan_dft_r2c_1d(int n, float *in, float (*out)[2], unsigned flags);
void armplf_execute_dft_r2c(const armplf_plan p, float *in, float (*out)[2]);
void armplf_destroy_plan(armplf_plan p);
#define MEASURE 0U /* fftw3.h's FFTW_ flags */
#define PRESERVE_INPUT (1U<<4)
#define ESTIMATE (1U<<6)

static int can_do(int size){
	return size>0;
}

static void *make(int size, float *in, float *out){
	return armplf_plan_dft_r2c_1d(size, in, (float (*)[2])out, (race_timing() ? MEASURE : ESTIMATE)|PRESERVE_INPUT);
}

/* size/2+1 (re, im) pairs */
static void forward(void *p, float *in, float *out){
	armplf_execute_dft_r2c(p, in, (float (*)[2])out);
}

static void destroy(void *p){
	armplf_destroy_plan(p);
}

const kernel k_armpl = {"armpl", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
