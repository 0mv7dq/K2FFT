// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* kissfft: one 1D transform out of place (in place, kiss_fft allocates), complex either way, real forward or back at even sizes */
#include "benchee.h"
#include "kiss_fftr.h"

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Mark Borgerding")
BENCH_DOC("url", "https://github.com/mborgerding/kissfft")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static kiss_fft_cfg cplan;
static kiss_fftr_cfg rplan;

int can_do(bench_problem *p){
	int n = bench_1d(p);
	return n>0 && p->kind!=PROBLEM_R2R && !p->in_place && (p->kind==PROBLEM_COMPLEX || n%2==0);
}

void setup(bench_problem *p){
	int n = bench_1d(p), inverse = p->sign>0;
	cplan = NULL;
	rplan = NULL;
	if (p->kind==PROBLEM_COMPLEX){
		cplan = kiss_fft_alloc(n, inverse, NULL, NULL);
	} else {
		rplan = kiss_fftr_alloc(n, inverse, NULL, NULL);
	}
	BENCH_ASSERT(cplan || rplan);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		if (cplan){
			kiss_fft(cplan, p->in, p->out);
		} else if (p->sign<0){
			kiss_fftr(rplan, p->in, p->out);
		} else {
			kiss_fftri(rplan, p->in, p->out);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	kiss_fft_free(cplan);
	kiss_fftr_free(rplan);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
