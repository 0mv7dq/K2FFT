// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTS at powers of two, as rdft/ffts.c: one 1D transform out of place, complex either way or real; FFTW's layouts already; a
   backward one's scale measured */
#include "benchee.h"
#include "simd-support/simd.h"
#include <ffts.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Anthony M. Blake")
BENCH_DOC("url", "https://github.com/anthonix/ffts")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static ffts_plan_t *plan;
static float scale;

int can_do(bench_problem *p){
	int n = bench_1d(p);
#if defined(__arm__)
	if (!simd_neon()){
		return 0;
	}
#endif
	return n>=(p->kind==PROBLEM_COMPLEX ? 2 : 4) && power_of_two(n) && p->kind!=PROBLEM_R2R && !p->in_place;
}

static void transform(bench_problem *p, const void *in, void *out){
	ffts_execute(plan, in, out);
	if (scale!=1){
		float *o = out;
		const int m = bench_1d(p)*(p->kind==PROBLEM_COMPLEX ? 2 : 1);
		for (int i = 0; i<m; i++){
			o[i] *= scale;
		}
	}
}

void setup(bench_problem *p){
	int n = bench_1d(p);
	int sign = p->sign<0 ? FFTS_FORWARD : FFTS_BACKWARD;
	plan = p->kind==PROBLEM_COMPLEX ? ffts_init_1d((size_t)n, sign) : ffts_init_1d_real((size_t)n, sign);
	BENCH_ASSERT(plan);
	scale = 1;
	if (p->sign>0){ /* DC alone, whose inverse is 1 everywhere */
		float *in = p->in, *out = p->out;
		memset(in, 0, (size_t)(p->kind==PROBLEM_COMPLEX ? 2*n : n+2)*sizeof(float));
		in[0] = 1;
		transform(p, in, out);
		BENCH_ASSERT(out[0]!=0);
		scale = 1/out[0];
		BENCH_ASSERT(scale==1 || scale==(float)n);
	}
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		transform(p, p->in, p->out); /* libbench2 aligns in and out to 128 bytes */
	}
}

void done(bench_problem *p){
	UNUSED(p);
	ffts_free(plan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
