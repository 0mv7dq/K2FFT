// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <arm_math.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Arm Limited")
BENCH_DOC("url", "https://github.com/ARM-software/CMSIS-DSP")
BENCH_DOC("copyright", "Apache-2.0")
#ifdef ARM_MATH_MVEF /* arm_math_types.h's, from __ARM_FEATURE_MVE */
BENCH_DOC("target", "Helium")
#else
BENCH_DOC("target", "scalar C")
#endif
END_BENCH_DOC

static arm_rfft_fast_instance_f32 plan;
static arm_cfft_instance_f32 cplan;
static float32_t *work;

/* one 1D forward transform, real at arm_rfft_fast_init_f32's sizes (powers of two, 32 to 4096) or complex at arm_cfft_init_f32's
   (16 to 4096); w/o the backward ones, which scale by 1/n */
int can_do(bench_problem *p){
	int n = rof_size(p);
	if (n>0){
		return n>=32 && n<=4096 && power_of_two(n);
	}
	n = bench_1d(p);
	return p->kind==PROBLEM_COMPLEX && p->sign<0 && !p->in_place && n>=16 && n<=4096 && power_of_two(n);
}

void setup(bench_problem *p){
	int n = bench_1d(p);
	work = NULL;
	if (p->kind==PROBLEM_COMPLEX){
		BENCH_ASSERT(arm_cfft_init_f32(&cplan, (uint16_t)n)==ARM_MATH_SUCCESS);
		return;
	}
	BENCH_ASSERT(arm_rfft_fast_init_f32(&plan, (uint16_t)n)==ARM_MATH_SUCCESS);
	work = bench_malloc((size_t)n*sizeof(float32_t)); /* asserts by itself */
}

void doit(int iter, bench_problem *p){
	const int n = bench_1d(p);
	for (int i = 0; i<iter; i++){
		if (!work){ /* in place on out, of a copy, as rof keeps its input */
			memcpy(p->out, p->in, 2*(size_t)n*sizeof(float32_t));
			arm_cfft_f32(&cplan, p->out, 0, 1);
			continue;
		}
		memcpy(work, p->in, (size_t)n*sizeof(float32_t)); /* arm_rfft_fast_f32 overwrites its input, which rof keeps */
		arm_rfft_fast_f32(&plan, work, p->out, 0);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	if (work){
		bench_free(work);
	}
}

/* benchFFT's packed format, as benchees/pffft: the Nyquist bin in io[0], which is ro[1] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = bench_1d(p);
	ro[n] = ro[1];
	io[0] = 0;
	io[n] = 0;
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
