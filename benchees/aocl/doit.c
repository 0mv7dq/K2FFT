// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* AOCL-FFTW, every problem libbench2 poses in single precision, as benchees/fftw3 */
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include "simd-support/simd.h"
#include <fftw3.h> /* AOCL-FFTW's, its names aoclf_ (cmake/aocl.cmake) */

#define API(name) aoclf_##name
#define API_IODIM aoclf_iodim
#define API_KIND aoclf_r2r_kind
#define API_PRESERVE FFTW_PRESERVE_INPUT
#define API_DESTROY FFTW_DESTROY_INPUT
#include "fftw_api.h"

#define STRING(x) #x
#define VALUE(x) STRING(x)

static const char *version(void){
	return aoclf_version;
}

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("version", version)
BENCH_DOC("planner", VALUE(PLAN))
BENCH_DOC("author", "AMD, after Matteo Frigo and Steven G. Johnson")
BENCH_DOC("url", "https://github.com/amd/amd-fftw")
BENCH_DOC("copyright", "GPL-2.0-or-later")
END_BENCH_DOC

static aoclf_plan plan;

/* its AVX2 build, as rdft/aocl.c checks; by estimate on a copy of p's own, as benchees/fftw3 */
int can_do(bench_problem *p){
	if (!simd_avx2_fma()){
		return 0;
	}
	bench_problem *q = problem_parse(p->pstring);
	problem_alloc(q);
	aoclf_plan x = api_plan(q, FFTW_ESTIMATE);
	if (x){
		aoclf_destroy_plan(x);
	}
	problem_destroy(q);
	return x!=NULL;
}

void setup(bench_problem *p){
	plan = api_plan(p, PLAN);
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	UNUSED(p);
	for (int i = 0; i<iter; i++){
		aoclf_execute(plan);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	aoclf_destroy_plan(plan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
	aoclf_cleanup();
}
