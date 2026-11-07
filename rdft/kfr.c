// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's DFTs, as benchees/kfr: one 1D transform out of place, complex either way, real forward or back at even sizes, float and
   double; rdft/kfr_rfft.cpp, whose flags stay in its own target */
#include "kernel/kernel.h"

void *kfr_plan(int how, int size);
void kfr_execute(void *plan, void *in, void *out);
void kfr_destroy(void *plan);

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	return size>0 && pb->kind!=KIND_R2R && pb->in!=pb->out && (pb->kind==KIND_DFT || size%2==0);
}

static void *make(const problem *pb){
	const int how = (pb->kind==KIND_DFT ? (pb->sign<0 ? 0 : 1) : (pb->kind==KIND_R2C ? 2 : 3))+(pb->single ? 0 : 4);
	return kfr_plan(how, pb->dims[0].n);
}

static void execute(const void *plan, void *in, void *out){
	kfr_execute((void *)plan, in, out);
}

const kernel k_kfr = {"kfr", can_do, make, execute, kfr_destroy};
