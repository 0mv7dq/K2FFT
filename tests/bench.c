// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* every problem libbench2 poses, through api/k2fft.h as benchees/fftw3 through fftw3.h; a FORCED_KERNEL build does only what its
   kernel does */
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include "api/k2fft.h"
#include "kernel/race.h"
#include <stdio.h>
#include <string.h>

#if defined(BENCHFFT_LDOUBLE) || defined(BENCHFFT_QUAD)
#error "K2FFT plans floats and doubles"
#elif defined(BENCHFFT_SINGLE)
#define API(name) k2fftf_##name
#else
#define API(name) k2fft_##name
#endif
#define API_IODIM k2fft_iodim
#define API_KIND k2fft_r2r_kind
#define API_PRESERVE K2FFT_PRESERVE_INPUT
#define API_DESTROY 0U
#include "fftw_api.h"


BEGIN_BENCH_DOC
    BENCH_DOC("name", BENCHEE)
BENCH_DOC("package", "K2FFT")
END_BENCH_DOC


static API(plan) plan;
const racer *contestant; /* tests/hook.c's, run on the problem's arrays in plan's place */
static int paranoid;
void hook(const racer *r, int path);

/* untimed, on a copy of p's own: libbench2 asks before it allocates p's arrays, which planning may write */
int can_do(bench_problem *p){
	bench_problem *q = problem_parse(p->pstring);
	problem_alloc(q);
	API(plan) x = api_plan(q, K2FFT_ESTIMATE);
	int ok = x!=NULL;
#ifdef FORCED_KERNEL
	ok = ok && strcmp(API(plan_kernel)(x), FORCED_KERNEL)==0;
#endif
	API(destroy_plan)(x);
	problem_destroy(q);
	return ok;
}

void setup(bench_problem *p){
	if (paranoid && rof_size(p)){
		race_size(rof_size(p)); /* so the hook also checks the sizes spectrum_init races */
	}
	plan = api_plan(p, K2FFT_MEASURE);
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	if (contestant){ /* on p's arrays, as k2fft_execute_dft and the others allow */
		for (int i=0; i<iter; i++){
			contestant->k->execute(contestant->plan, p->in, p->out);
		}
		return;
	}
	for (int i=0; i<iter; i++){
		API(execute)(plan);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	API(destroy_plan)(plan);
	plan = NULL;
}

void useropt(const char *arg){
	if (strcmp(arg, "paranoid")==0){
		paranoid = 1;
		race_hook = hook;
	} else if (strcmp(arg, "estimate")!=0){ /* scripts/benchmark's, for FFTW under an emulator, where K2FFT_NORACE plans untimed */
		fprintf(stderr, "unknown user option: %s.  Ignoring.\n", arg);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){}
