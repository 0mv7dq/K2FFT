// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "simd-support/simd.h"
#include <fftw3.h> /* AOCL-FFTW's, its names aoclf_ (cmake/aocl.cmake) */

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

/* its AVX2 build, as rdft/aocl.c checks */
int can_do(bench_problem *p){
	return rof_size(p)>0 && simd_avx2_fma();
}

void setup(bench_problem *p){
	plan = aoclf_plan_dft_r2c_1d(rof_size(p), p->in, p->out, PLAN);
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
