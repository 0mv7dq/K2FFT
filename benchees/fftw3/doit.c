// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTW, every problem libbench2 poses, as its own tests/bench.c; fftw3f's in single precision, fftw3's in double */
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include <fftw3.h>
#include <stdio.h>
#include <string.h>

#if defined(BENCHFFT_SINGLE)
#define API(name) fftwf_##name
#define API_IODIM fftwf_iodim
#define API_KIND fftwf_r2r_kind
#else
#define API(name) fftw_##name
#define API_IODIM fftw_iodim
#define API_KIND fftw_r2r_kind
#endif
#define API_PRESERVE FFTW_PRESERVE_INPUT
#define API_DESTROY FFTW_DESTROY_INPUT
#include "fftw_api.h"

#define STRING(x) #x
#define VALUE(x) STRING(x)

static const char *version(void){
	return API(version);
}

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
#ifdef RELEASE /* K2FFT's */
BENCH_DOC("version", RELEASE)
BENCH_DOCF("build", version) /* its SIMD */
#else
BENCH_DOCF("version", version)
#endif
BENCH_DOC("planner", VALUE(PLAN))
BENCH_DOC("timelimit", VALUE(TIMELIMIT))
BENCH_DOC("author", "Matteo Frigo and Steven G. Johnson")
BENCH_DOC("url", "https://www.fftw.org")
BENCH_DOC("copyright", "GPL-2.0-or-later")
END_BENCH_DOC

static unsigned flags = PLAN; /* FFTW_MEASURE, or FFTW_ESTIMATE when cross-compiling */
static API(plan) plan;

/* by estimate, on a copy of p's own: libbench2 asks before it allocates p's arrays, and FFTW takes in==out for in place */
int can_do(bench_problem *p){
	bench_problem *q = problem_parse(p->pstring);
	problem_alloc(q);
	API(plan) x = api_plan(q, FFTW_ESTIMATE);
	if (x){
		API(destroy_plan)(x);
	}
	problem_destroy(q);
	return x!=NULL;
}

void setup(bench_problem *p){
	plan = api_plan(p, flags);
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	UNUSED(p);
	for (int i = 0; i<iter; i++){
		API(execute)(plan);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	API(destroy_plan)(plan);
}

void useropt(const char *arg){
	double limit;
	if (!strcmp(arg, "estimate")){
		flags |= FFTW_ESTIMATE;
	} else if (!strcmp(arg, "patient")){
		flags |= FFTW_PATIENT;
	} else if (!strcmp(arg, "exhaustive")){
		flags |= FFTW_EXHAUSTIVE;
	} else if (sscanf(arg, "timelimit=%lg", &limit)==1){ /* as FFTW's own bench */
		API(set_timelimit)(limit);
	} else {
		fprintf(stderr, "unknown user option: %s.  Ignoring.\n", arg);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	API(set_timelimit)(TIMELIMIT); /* seconds a plan, or FFTW_NO_TIMELIMIT; before -o timelimit= */
}

void cleanup(void){
	API(cleanup)();
}
