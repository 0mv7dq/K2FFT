// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "libbench2/bench.h"
#include "api/fft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern fft *plan;

/* for -o paranoid, verify each contestant before the race times it */
void hook(fft *f, int size, int path){
	UNUSED(path);
	char desc[32];
	snprintf(desc, sizeof(desc), "rof%d", size);
	bench_problem *p = problem_parse(desc);
	problem_alloc(p);
	float *in = fft_input(f);
	float *saved = malloc((size_t)size*sizeof(float));
	BENCH_ASSERT(saved);
	memcpy(saved, in, (size_t)size*sizeof(float)); /* f's input must be left as it was */
	fft *own = plan;
	plan = f;
	verify_problem(p, 5, 1.0e-3); /* single-precision tolerance */
	plan = own;
	memcpy(in, saved, (size_t)size*sizeof(float));
	free(saved);
	problem_destroy(p);
}
