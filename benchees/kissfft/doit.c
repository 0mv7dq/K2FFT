// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "kiss_fftr.h"

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Mark Borgerding")
BENCH_DOC("url", "https://github.com/mborgerding/kissfft")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static kiss_fftr_cfg plan;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && n%2==0;
}

void setup(bench_problem *p){
	plan = kiss_fftr_alloc(rof_size(p), 0, NULL, NULL);
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		kiss_fftr(plan, p->in, p->out);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	kiss_fftr_free(plan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
