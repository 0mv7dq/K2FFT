// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

enum { DESC = 64 }; /* a problem's room, as libbench2's "rof1024" or "cib64x64" */

/* libbench2's speed.c steps, w/ can_do as a filter */
static inline bench_problem *problem_of(const char *desc){
	bench_problem *p = problem_parse(desc);
	if (!can_do(p)){
		problem_destroy(p);
		return NULL;
	}
	problem_alloc(p);
	problem_zero(p);
	return p;
}

static inline bench_problem *rof_problem(int n){
	char desc[DESC];
	snprintf(desc, sizeof desc, "rof%d", n);
	return problem_of(desc);
}

/* must follow setup, whose planning may overwrite the input: libbench2's generator, seeded as bench-main.c, into p's input, then the
   copy hook libbench2's verifiers call for p's kind */
static inline void problem_input(bench_problem *p){
	const int complex = p->kind==PROBLEM_COMPLEX || (p->kind==PROBLEM_REAL && p->sign>0);
	const size_t n = (size_t)p->iphyssz*(complex ? 2 : 1);
	bench_real *in = p->inphys, *ri = p->in;
	bench_srand(1);
	for (size_t i = 0; i<n; i++){
		in[i] = (bench_real)bench_drand();
	}
	if (p->kind==PROBLEM_COMPLEX){
		after_problem_ccopy_from(p, ri, ri+1);
	} else if (complex){
		after_problem_hccopy_from(p, ri, ri+1);
	} else {
		after_problem_rcopy_from(p, ri);
	}
}

/* word as libbench2 names a problem: a bare size n is rof<n> */
static inline void desc_of(char *desc, const char *word){
	snprintf(desc, DESC, "%s%s", isdigit((unsigned char)word[0]) ? "rof" : "", word);
}

/* file's problems, by desc_of */
static inline int read_problems(const char *file, char (*descs)[DESC], int max){
	FILE *f = fopen(file, "r");
	int count = 0;
	char word[DESC];
	if (!f){
		return 0;
	}
	while (count<max && fscanf(f, "%63s", word)==1){
		desc_of(descs[count++], word);
	}
	fclose(f);
	return count;
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
