// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "pocketfft_single.h"
#include <stdlib.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Martin Reinecke")
BENCH_DOC("url", "https://github.com/hayguen/pocketfft")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static cffts_plan cplan;
static rffts_plan rplan;
static float *packed; /* an out-of-place real backward's input as pocketfft takes it, as libbench2 checks p's is kept */

/* one 1D transform: complex either way, real forward or back, in or out of place */
int can_do(bench_problem *p){
	return bench_1d(p)>0 && p->kind!=PROBLEM_R2R;
}

void setup(bench_problem *p){
	size_t n = (size_t)bench_1d(p);
	if (p->kind==PROBLEM_COMPLEX){
		cplan = make_cffts_plan(n);
	} else {
		rplan = make_rffts_plan(n);
	}
	packed = NULL;
	if (p->kind==PROBLEM_REAL && p->sign>0 && !p->in_place){
		packed = calloc(n, sizeof(float));
		BENCH_ASSERT(packed);
	}
	BENCH_ASSERT(cplan || rplan);
}

/* in place on out, as pocketfft works */
void doit(int iter, bench_problem *p){
	size_t n = (size_t)bench_1d(p), bytes = n*(p->kind==PROBLEM_COMPLEX ? 2 : 1)*sizeof(float);
	for (int i = 0; i<iter; i++){
		if (p->out!=p->in){
			memcpy(p->out, packed ? packed : p->in, bytes);
		}
		if (cplan){
			p->sign<0 ? cffts_forward(cplan, p->out, 1.0f) : cffts_backward(cplan, p->out, 1.0f);
		} else {
			p->sign<0 ? rffts_forward(rplan, p->out, 1.0f) : rffts_backward(rplan, p->out, 1.0f);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	if (cplan){
		destroy_cffts_plan(cplan);
	}
	if (rplan){
		destroy_rffts_plan(rplan);
	}
	cplan = NULL;
	rplan = NULL;
	free(packed);
	packed = NULL;
}

/* a real forward's r0, r1, i1, ..., (r(n/2)) interleaved */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = bench_1d(p);
	if (n%2==0){
		ro[n] = ro[n-1];
		io[n] = 0;
	}
	for (int k = (n-1)/2; k>=1; k--){
		io[2*k] = ro[2*k];
		ro[2*k] = ro[2*k-1];
	}
	io[0] = 0;
}

/* a real backward's (re, im) pairs as r0, r1, i1, ..., (r(n/2)): in packed, or in place */
void after_problem_hccopy_from(bench_problem *p, bench_real *ri, bench_real *ii){
	UNUSED(ii);
	const int n = bench_1d(p);
	bench_real *x = packed ? packed : ri;
	x[0] = ri[0];
	memmove(x+1, ri+2, (size_t)(n-1)*sizeof(bench_real));
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
