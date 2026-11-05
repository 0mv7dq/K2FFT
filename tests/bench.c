// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "api/fft.h"
#include "kernel/race.h"
#include <stdio.h>
#include <string.h>


BEGIN_BENCH_DOC
    BENCH_DOC("name", BENCHEE)
BENCH_DOC("package", "PUREfft")
END_BENCH_DOC


fft *plan; /* hook.c swaps each contestant in */
static int paranoid;
void hook(fft *f, int size, int path);

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && fft_next_size(n)==n;
}

void setup(bench_problem *p){
	if (paranoid){ race_size(rof_size(p)); } /* so the hook checks the sizes spectrum_init would race */
	plan = fft_new(rof_size(p));
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	UNUSED(p);
	for (int i=0; i<iter; i++){ fft_forward(plan); }
}

void done(bench_problem *p){
	UNUSED(p);
	fft_free(plan);
	plan = NULL;
}

void after_problem_rcopy_from(bench_problem *p, bench_real *ri){
	memcpy(fft_input(plan), ri, (size_t)rof_size(p)*sizeof(bench_real));
}

/* bins interleaved, io = ro + 1 */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	for (int k=0; k<=rof_size(p)/2; k++){
		fft_bin(plan, k, &ro[2*k], &io[2*k]);
	}
}

void useropt(const char *arg){
	if (strcmp(arg, "paranoid")==0){
		paranoid = 1;
		race_hook = hook;
	} else { fprintf(stderr, "unknown user option: %s.  Ignoring.\n", arg); }
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){}
