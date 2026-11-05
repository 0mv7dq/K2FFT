// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "pocketfft_single.h"
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Martin Reinecke")
BENCH_DOC("url", "https://github.com/hayguen/pocketfft")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static rffts_plan plan;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && n%2==0;
}

void setup(bench_problem *p){
	plan = make_rffts_plan((size_t)rof_size(p));
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	size_t bytes = (size_t)rof_size(p)*sizeof(float);
	for (int i = 0; i<iter; i++){
		memcpy(p->out, p->in, bytes); /* rffts_forward works in place */
		rffts_forward(plan, p->out, 1.0f);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	destroy_rffts_plan(plan);
}

/* interleave r0, r1, i1, ..., r(n/2) */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = rof_size(p);
	ro[n] = ro[n-1];
	io[n] = 0;
	for (int k = n/2-1; k>=1; k--){
		io[2*k] = ro[2*k];
		ro[2*k] = ro[2*k-1];
	}
	io[0] = 0;
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
