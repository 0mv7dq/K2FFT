// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Ne10 at powers of two, as rdft/ne10.c: one 1D forward transform out of place, complex or real; w/o its backward ones, which scale by
   1/n */
#include "benchee.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "ARM")
BENCH_DOC("url", "https://github.com/projectNe10/Ne10")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static ne10_fft_cfg_float32_t c2c;
static ne10_fft_r2c_cfg_float32_t r2c;
static int neon;

int can_do(bench_problem *p){
	int n = bench_1d(p);
	return n>0 && power_of_two(n) && p->kind!=PROBLEM_R2R && p->sign<0 && !p->in_place;
}

void setup(bench_problem *p){
	int n = bench_1d(p);
	c2c = NULL;
	r2c = NULL;
	if (p->kind==PROBLEM_COMPLEX){
		c2c = neon ? ne10_fft_alloc_c2c_float32_neon(n) : ne10_fft_alloc_c2c_float32_c(n);
		BENCH_ASSERT(c2c);
		c2c->is_backward_scaled = 0;
	} else {
		r2c = ne10_fft_alloc_r2c_float32(n);
		BENCH_ASSERT(r2c);
	}
}

void doit(int iter, bench_problem *p){
	const int backward = p->sign>0;
	for (int i = 0; i<iter; i++){
		if (c2c){
			if (neon){
				ne10_fft_c2c_1d_float32_neon(p->out, p->in, c2c, backward);
			} else {
				ne10_fft_c2c_1d_float32_c(p->out, p->in, c2c, backward);
			}
		} else if (!backward){
			if (neon){
				ne10_fft_r2c_1d_float32_neon(p->out, p->in, r2c);
			} else {
				ne10_fft_r2c_1d_float32_c(p->out, p->in, r2c);
			}
		} else if (neon){
			ne10_fft_c2r_1d_float32_neon(p->out, p->in, r2c);
		} else {
			ne10_fft_c2r_1d_float32_c(p->out, p->in, r2c);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	if (c2c){
		ne10_fft_destroy_c2c_float32(c2c);
	}
	if (r2c){
		ne10_fft_destroy_r2c_float32(r2c);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	neon = simd_neon();
}

void cleanup(void){
}
