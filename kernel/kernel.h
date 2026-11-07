// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "kernel/problem.h"

#ifdef __cplusplus
extern "C" {
#endif


/* one library's transforms, as FFTW's solvers: kernel/planner.c picks among those that can do a problem by kernel/table.h and a race */
typedef struct kernel {
	const char *name; /* its benchee's, as benchmark/ and kernel/table.h name it */
	int (*can_do)(const problem *p); /* here, after the CPU's run-time checks */
	void *(*plan)(const problem *p); /* on p's arrays, which it may overwrite; allocates; NULL if it can't */
	void (*execute)(const void *plan, void *in, void *out); /* FFTW's layouts, on arrays laid out and aligned as p's; never allocates;
															   leaves in as it was but a C2R w/o p->preserve */
	void (*destroy)(void *plan);
	const struct conv_ops *conv; /* NULL, or its spectra in its own order for a convolution, on a plan of a real forward one */
} kernel;

/* FFTW's TODO's convolution problem type: a real forward one's plan's transforms to and from its own order (n floats of n+2), which
   only mac reads, so a reorder can be left out (PFFFT's unordered transforms and zconvolve) */
typedef struct conv_ops {
	void (*forward)(const void *plan, const float *in, float *spectrum);
	void (*backward)(const void *plan, float *spectrum, float *out); /* unnormalized; may overwrite spectrum */
	void (*mac)(const void *plan, const float *a, const float *b, float *sum, float scale); /* sum += a b scale, bin by bin */
} conv_ops;

/* the kernels built here, NULL-terminated, in the order the planner falls back on (kernel/kernels.c) */
extern const kernel *const kernels[];


#ifdef __cplusplus
}
#endif
