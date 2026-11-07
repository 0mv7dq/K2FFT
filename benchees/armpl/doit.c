// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Arm Performance Libraries' FFTW interface, every problem libbench2 poses, as benchees/fftw3: its functions renamed armplf_ and
   armpld_ (cmake/armpl.cmake), and so its header's prototypes, by the build's definitions (benchees/armpl/CMakeLists.txt) */
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include <fftw3.h>

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

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("version", "26.01.1")
BENCH_DOC("planner", VALUE(PLAN))
BENCH_DOC("threads", "1 (sequential)")
BENCH_DOC("author", "Arm")
BENCH_DOC("url", "https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries")
BENCH_DOC("copyright", "Arm's end user license agreement")
END_BENCH_DOC

static API(plan) plan;

/* by estimate, on a copy of p's own, as benchees/fftw3 */
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
	plan = api_plan(p, PLAN);
	BENCH_ASSERT(plan);
	API(execute)(plan); /* ArmPL allocates on a plan's first transform: here, not in doit (tools/alloc-check.c), as rdft/armpl.c */
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

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
	API(cleanup)();
}
