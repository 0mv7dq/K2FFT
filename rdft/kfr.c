// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's real DFT, as benchees/kfr: rdft/kfr_rfft.cpp, whose flags stay in its own target */
#include "kernel/kernel.h"

void *kfr_plan(int size, float *in, float *out);
void kfr_forward(void *plan, float *in, float *out);
void kfr_destroy(void *plan);

static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return size>0 && size%2==0;
}

static void *make(const problem *pb){
	return kfr_plan(pb->dims[0].n, pb->in, pb->out);
}

static void execute(const void *plan, void *in, void *out){
	kfr_forward((void *)plan, in, out);
}

const kernel k_kfr = {"kfr", can_do, make, execute, kfr_destroy};
