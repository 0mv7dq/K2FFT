// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* count.c for boards w/o a command line: every size in one run, its transform once then three times between bench_marks */
#include "tools.h"

static const int sizes[] = {BENCH_SIZE_LIST};

/* where scripts/benchmark splits the emulator's trace */
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
		bench_mark();
		doit(1, p);
		bench_mark();
		doit(3, p);
		bench_mark();
		done(p);
		problem_destroy(p);
	}
	return 0;
}
