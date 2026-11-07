// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* count.c w/o argv, as libbench2's bench-main.c and speed.c w/o a timer: every size in one run */
#include "tools.h"

static const int sizes[] = {BENCH_SIZE_LIST};

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

int main(void){
	int argc = 0;
	char **argv = NULL;
	main_init(&argc, &argv);
	report_info_all();
	for (size_t i = 0; i<sizeof sizes/sizeof *sizes; i++){
		bench_problem *p = rof_problem(sizes[i]);
		if (!p){
			continue;
		}
		setup(p);
		problem_input(p);
		printf("%d\n", sizes[i]);
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
		printf("ticks %d %lu\n", sizes[i], ((t2-t1)-(t1-t0))/2);
#else
		(void)t0, (void)t1, (void)t2;
#endif
		done(p);
		problem_destroy(p);
	}
	cleanup();
	return 0;
}
