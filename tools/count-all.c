// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* count.c w/o argv, as libbench2's bench-main.c and speed.c w/o a timer: every problem in one run, BENCH_PROBLEM_LIST's
   (cmake/bench.cmake's, of K2FFT_CLASSES's single-precision classes) */
#include "tools.h"

static const char *const problems[] = {BENCH_PROBLEM_LIST};

#ifdef BENCH_TICKS /* a real board's cycle counter, as FFTW's cycle.h getticks */
unsigned long BENCH_TICKS(void);
#define TICKS() BENCH_TICKS()
#else
#define TICKS() 0ul
#endif

/* scripts/marks splits the trace here */
__attribute__((noinline)) void bench_mark(void){
	__asm__ volatile ("" ::: "memory");
}

#ifndef BENCH_VERIFY_LARGEST
#define BENCH_VERIFY_LARGEST 0 /* every size; an emulated board's, slow under the trace, 1024 */
#endif

/* libbench2's verify, after the counts and out of them, w/ tests/hook.c's tolerance; where its 7 arrays fit */
static void verify_counted(bench_problem *p, const char *name){
	const size_t n = tensor_sz(p->sz)*tensor_sz(p->vecsz);
	void *room = BENCH_VERIFY_LARGEST && n>BENCH_VERIFY_LARGEST ? NULL : malloc(8*n*sizeof(bench_complex));
	if (room){
		free(room);
		verify_problem(p, 2, sizeof(bench_real)==sizeof(float) ? 1.0e-3 : 1.0e-10); /* exits on an error, to stderr */
	}
	printf("%s %s\n", room ? "verified" : "unverified", name);
	fflush(stdout); /* scripts/marks counts these lines, and bare metal's exit may not flush */
}

int main(void){
	int argc = 0;
	char **argv = NULL;
	main_init(&argc, &argv);
	report_info_all();
	for (size_t i = 0; i<sizeof problems/sizeof *problems; i++){
		bench_problem *p = problem_of(problems[i]);
		if (!p){
			continue;
		}
		setup(p);
		problem_input(p);
		printf("%s\n", problems[i]);
		doit(1, p); /* first-call work, out of the counts as count.c's (N(3)-N(1))/2 leaves it */
		bench_mark();
		unsigned long t0 = TICKS();
		doit(1, p);
		bench_mark();
		unsigned long t1 = TICKS();
		doit(3, p);
		bench_mark();
		unsigned long t2 = TICKS();
#ifdef BENCH_TICKS
		printf("ticks %s %lu\n", problems[i], ((t2-t1)-(t1-t0))/2);
#else
		(void)t0, (void)t1, (void)t2;
#endif
		verify_counted(p, problems[i]);
		done(p);
		problem_destroy(p);
	}
	cleanup();
	return 0;
}
