// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Arm Performance Libraries' real DFT through its FFTW interface, as benchees/armpl: renamed armplf_ (cmake/armpl.cmake), declared
   here, as its fftw3.h would shadow K2FFT's own FFTW's */
#include "kernel/kernel.h"
#include <string.h>

typedef struct armplf_plan_s *armplf_plan;
armplf_plan armplf_plan_dft_r2c_1d(int n, float *in, float (*out)[2], unsigned flags);
void armplf_execute_dft_r2c(const armplf_plan p, float *in, float (*out)[2]);
void armplf_destroy_plan(armplf_plan p);
#define MEASURE 0U /* fftw3.h's FFTW_ flags */
#define PRESERVE_INPUT (1U<<4)
#define ESTIMATE (1U<<6)

static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return size>0;
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	float *in = pb->in, *out = pb->out;
	armplf_plan p = armplf_plan_dft_r2c_1d(size, in, (float (*)[2])out, (pb->measure ? MEASURE : ESTIMATE)|PRESERVE_INPUT);
	if (p){
		memset(in, 0, (size_t)size*sizeof(float));
		armplf_execute_dft_r2c(p, in, (float (*)[2])out); /* ArmPL allocates on a plan's first transform: here, not in Pd's DSP */
	}
	return p;
}

/* size/2+1 (re, im) pairs */
static void execute(const void *p_, void *in_, void *out_){
	void *p = (void *)p_; /* the library's execute takes it w/o const */
	float *in = in_, *out = out_;
	armplf_execute_dft_r2c(p, in, (float (*)[2])out);
}

static void destroy(void *p){
	armplf_destroy_plan(p);
}

const kernel k_armpl = {"armpl", can_do, make, execute, destroy};
