// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "ARM")
BENCH_DOC("url", "https://github.com/projectNe10/Ne10")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static ne10_fft_r2c_cfg_float32_t plan;
static int neon;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && power_of_two(n);
}

void setup(bench_problem *p){
	plan = ne10_fft_alloc_r2c_float32(rof_size(p));
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		if (neon){
			ne10_fft_r2c_1d_float32_neon(p->out, p->in, plan);
		} else {
			ne10_fft_r2c_1d_float32_c(p->out, p->in, plan);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	ne10_fft_destroy_r2c_float32(plan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	neon = simd_neon();
}

void cleanup(void){
}
