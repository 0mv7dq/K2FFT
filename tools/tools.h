// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include <stdio.h>
#include <stdlib.h>

/* libbench2's speed.c steps, w/ can_do as a filter */
static inline bench_problem *rof_problem(int n){
	char name[32];
	snprintf(name, sizeof name, "rof%d", n);
	bench_problem *p = problem_parse(name);
	if (!can_do(p)){
		problem_destroy(p);
		return NULL;
	}
	problem_alloc(p);
	problem_zero(p);
	return p;
}

/* must follow setup, whose planning may overwrite the input; libbench2's generator, seeded as bench-main.c */
static inline void rof_input(bench_problem *p){
	bench_real *in = p->in;
	int n = rof_size(p);
	bench_srand(1);
	for (int i = 0; i<n; i++){
		in[i] = (bench_real)bench_drand();
	}
	after_problem_rcopy_from(p, in);
}

static inline int read_sizes(const char *file, int *sizes, int max){
	FILE *f = fopen(file, "r");
	int count = 0;
	if (!f){
		return 0;
	}
	while (count<max && fscanf(f, "%d", &sizes[count])==1){
		count++;
	}
	fclose(f);
	return count;
}
