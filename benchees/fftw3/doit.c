// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <fftw3.h>
#include <stdio.h>
#include <string.h>

#define STRING(x) #x
#define VALUE(x) STRING(x)

static const char *version(void){
	return fftwf_version;
}

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("version", version)
BENCH_DOC("planner", VALUE(PLAN))
BENCH_DOC("timelimit", VALUE(TIMELIMIT))
BENCH_DOC("author", "Matteo Frigo and Steven G. Johnson")
BENCH_DOC("url", "https://www.fftw.org")
BENCH_DOC("copyright", "GPL-2.0-or-later")
END_BENCH_DOC

static unsigned flags = PLAN; /* FFTW_MEASURE, or FFTW_ESTIMATE when cross-compiling */
static fftwf_plan plan;

int can_do(bench_problem *p){
	return rof_size(p)>0;
}

void setup(bench_problem *p){
	plan = fftwf_plan_dft_r2c_1d(rof_size(p), p->in, p->out, flags);
	BENCH_ASSERT(plan);
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

void useropt(const char *arg){
	double limit;
	if (!strcmp(arg, "estimate")){
		flags |= FFTW_ESTIMATE;
	} else if (!strcmp(arg, "patient")){
		flags |= FFTW_PATIENT;
	} else if (!strcmp(arg, "exhaustive")){
		flags |= FFTW_EXHAUSTIVE;
	} else if (sscanf(arg, "timelimit=%lg", &limit)==1){ /* as FFTW's own bench */
		fftwf_set_timelimit(limit);
	} else {
		fprintf(stderr, "unknown user option: %s.  Ignoring.\n", arg);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	fftwf_set_timelimit(TIMELIMIT); /* seconds a plan, or FFTW_NO_TIMELIMIT; before -o timelimit= */
}

void cleanup(void){
	fftwf_cleanup();
}
