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
} kernel;

/* the kernels built here, NULL-terminated, in the order the planner falls back on (kernel/kernels.c) */
extern const kernel *const kernels[];


#ifdef __cplusplus
}
#endif
