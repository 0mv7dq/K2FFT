// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* for an instruction counter: scripts/benchmark takes (N(3)-N(1))/2 */
#include "tools.h"
#include <string.h>

int main(int argc, char **argv){
	main_init(&argc, &argv);
	int a = 1;
	while (a<argc && strncmp(argv[a], "-o", 2)==0){
		useropt(argv[a]+2);
		a++;
	}
	if (argc-a<2){
		fprintf(stderr, "usage: count [-o<option> ...] <n> <reps>\n");
		return 2;
	}
	bench_problem *p = rof_problem(atoi(argv[a]));
	if (!p){
		return 1;
	}
	setup(p);
	rof_input(p);
	doit(atoi(argv[a+1]), p);
	done(p);
	problem_destroy(p);
	return 0;
}
