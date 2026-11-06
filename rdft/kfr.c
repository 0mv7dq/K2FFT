// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's real DFT, as benchees/kfr: rdft/kfr_rfft.cpp, whose flags stay in its own target */
#include "kernel/kernel.h"
#include "kernel/pow.h"

void *kfr_plan(int size, float *in, float *out);
void kfr_forward(void *plan, float *in, float *out);
void kfr_destroy(void *plan);

static int can_do(int size){
	return size>0 && size%2==0;
}

const kernel k_kfr = {"kfr", can_do, kfr_plan, kfr_forward, kfr_destroy, power_interleaved, bin_interleaved};
