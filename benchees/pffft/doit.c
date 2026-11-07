// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <pffft/pffft.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("target", pffft_simd_arch)
BENCH_DOC("author", "Julien Pommier, Hayati Ayguen and others")
BENCH_DOC("url", "https://github.com/marton78/pffft")
BENCH_DOC("copyright", "BSD-like (FFTPACKv4)")
END_BENCH_DOC

static PFFFT_Setup *plan;
static float *work;
static float *packed; /* an out-of-place real backward's input in PFFFT's order, as libbench2 checks p's is kept */

/* one 1D transform: complex either way, real forward or back, in or out of place */
int can_do(bench_problem *p){
	int n = bench_1d(p);
	if (n==0 || p->kind==PROBLEM_R2R){
		return 0;
	}
	return pffft_is_valid_size(n, p->kind==PROBLEM_COMPLEX ? PFFFT_COMPLEX : PFFFT_REAL);
}

void setup(bench_problem *p){
	int n = bench_1d(p), complex = p->kind==PROBLEM_COMPLEX;
	plan = pffft_new_setup(n, complex ? PFFFT_COMPLEX : PFFFT_REAL);
	work = pffft_aligned_malloc((size_t)n*(complex ? 2 : 1)*sizeof(float));
	packed = NULL;
	if (p->kind==PROBLEM_REAL && p->sign>0 && !p->in_place){
		packed = pffft_aligned_malloc((size_t)n*sizeof(float));
		BENCH_ASSERT(packed);
		memset(packed, 0, (size_t)n*sizeof(float));
	}
	BENCH_ASSERT(plan && work);
}

void doit(int iter, bench_problem *p){
	const pffft_direction_t way = p->sign<0 ? PFFFT_FORWARD : PFFFT_BACKWARD;
	for (int i = 0; i<iter; i++){
		pffft_transform_ordered(plan, packed ? packed : p->in, p->out, work, way); /* libbench2 aligns in and out to 128 bytes */
	}
}

void done(bench_problem *p){
	UNUSED(p);
	pffft_destroy_setup(plan);
	pffft_aligned_free(work);
	pffft_aligned_free(packed);
	packed = NULL;
}

/* a real forward's Nyquist bin from io[0], which is ro[1] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = bench_1d(p);
	ro[n] = ro[1];
	io[0] = 0;
	io[n] = 0;
}

/* a real backward's Nyquist bin into io[0], where PFFFT reads it: in packed, or in place */
void after_problem_hccopy_from(bench_problem *p, bench_real *ri, bench_real *ii){
	const int n = bench_1d(p);
	if (packed){
		memcpy(packed, ri, (size_t)n*sizeof(float));
		packed[1] = ri[n];
	} else {
		ii[0] = ri[n];
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
