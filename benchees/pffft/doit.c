// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <pffft/pffft.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("target", pffft_simd_arch)
BENCH_DOC("author", "Julien Pommier, Hayati Ayguen and others")
BENCH_DOC("url", "https://github.com/marton78/pffft")
BENCH_DOC("copyright", "BSD-like (FFTPACKv4)")
END_BENCH_DOC

static PFFFT_Setup *plan;
static float *work;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && pffft_is_valid_size(n, PFFFT_REAL);
}

void setup(bench_problem *p){
	int n = rof_size(p);
	plan = pffft_new_setup(n, PFFFT_REAL);
	work = pffft_aligned_malloc((size_t)n*sizeof(float));
	BENCH_ASSERT(plan && work);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		pffft_transform_ordered(plan, p->in, p->out, work, PFFFT_FORWARD); /* libbench2 aligns in and out to 128 bytes */
	}
}

void done(bench_problem *p){
	UNUSED(p);
	pffft_destroy_setup(plan);
	pffft_aligned_free(work);
}

/* unpack the Nyquist bin from io[0], which is ro[1] */
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
