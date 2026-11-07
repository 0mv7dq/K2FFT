// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"

/* Arm Performance Libraries' FFTW interface, its symbols renamed armplf_ (cmake/armpl.cmake): so are its header's prototypes */
#define fftwf_plan_dft_r2c_1d armplf_plan_dft_r2c_1d
#define fftwf_execute armplf_execute
#define fftwf_destroy_plan armplf_destroy_plan
#define fftwf_cleanup armplf_cleanup
#include <fftw3.h>

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

static fftwf_plan plan;

int can_do(bench_problem *p){
	return rof_size(p)>0;
}

void setup(bench_problem *p){
	plan = fftwf_plan_dft_r2c_1d(rof_size(p), p->in, p->out, PLAN);
	BENCH_ASSERT(plan);
	fftwf_execute(plan); /* ArmPL allocates on a plan's first transform: here, not in doit (tools/alloc-check.c), as rdft/armpl.c */
}

void doit(int iter, bench_problem *p){
	UNUSED(p);
	for (int i = 0; i<iter; i++){
		fftwf_execute(plan);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	fftwf_destroy_plan(plan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
	fftwf_cleanup();
}
