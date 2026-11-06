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
static float32_t *work;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>=32 && n<=4096 && power_of_two(n); /* arm_rfft_fast_init_f32's sizes */
}

void setup(bench_problem *p){
	int n = rof_size(p);
	BENCH_ASSERT(arm_rfft_fast_init_f32(&plan, (uint16_t)n)==ARM_MATH_SUCCESS);
	work = bench_malloc((size_t)n*sizeof(float32_t)); /* asserts by itself */
}

void doit(int iter, bench_problem *p){
	size_t bytes = (size_t)rof_size(p)*sizeof(float32_t);
	for (int i = 0; i<iter; i++){
		memcpy(work, p->in, bytes); /* arm_rfft_fast_f32 overwrites its input, which rof keeps */
		arm_rfft_fast_f32(&plan, work, p->out, 0);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	bench_free(work);
}

/* benchFFT's packed format, as benchees/pffft: the Nyquist bin in io[0], which is ro[1] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = rof_size(p);
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
