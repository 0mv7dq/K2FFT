// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* count.c w/o argv, as libbench2's bench-main.c and speed.c w/o a timer: every size in one run */
#include "tools.h"

static const int sizes[] = {BENCH_SIZE_LIST};

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
		rof_input(p);
		printf("%d\n", sizes[i]);
		doit(1, p); /* first-call work, out of the counts as count.c's (N(3)-N(1))/2 leaves it */
		bench_mark();
		doit(1, p);
		bench_mark();
		doit(3, p);
		bench_mark();
		done(p);
		problem_destroy(p);
	}
	cleanup();
	return 0;
}
