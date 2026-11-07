// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT, as benchees/pffft; rdft/pffft_*.c build it again for a copy cmake/pffft.cmake's pffft_copy renamed COPY_* */
#include "kernel/kernel.h"
#include <pffft/pffft.h>
#include <stdlib.h>

#define CAT_(a, b) a##_##b
#define CAT(a, b) CAT_(a, b)
#define STR_(a) #a
#define STR(a) STR_(a)

#ifdef COPY
int CAT(COPY, is_valid_size)(int N, pffft_transform_t cplx);
PFFFT_Setup *CAT(COPY, new_setup)(int N, pffft_transform_t transform);
void CAT(COPY, destroy_setup)(PFFFT_Setup *setup);
void CAT(COPY, transform_ordered)(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
#else
#define COPY pffft
#define RUNS 1 /* the CPU runs what the compiler targets */
#endif

typedef struct plan {
	PFFFT_Setup *setup;
	float *work; /* so PFFFT doesn't use the stack */
	int size;
} plan;

static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return RUNS && size>0 && CAT(COPY, is_valid_size)(size, PFFFT_REAL);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->setup){
		CAT(COPY, destroy_setup)(p->setup);
	}
	pffft_aligned_free(p->work);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->setup = CAT(COPY, new_setup)(size, PFFFT_REAL);
	p->work = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!p->setup || !p->work){
		destroy(p);
		return NULL;
	}
	return p;
}

static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	CAT(COPY, transform_ordered)(p->setup, in, out, p->work, PFFFT_FORWARD);
	out[p->size] = out[1]; /* FFTW's layout: Nyquist last, the im of DC and Nyquist 0 */
	out[p->size+1] = 0;
	out[1] = 0;
}

const kernel CAT(k, COPY) = {STR(COPY), can_do, make, execute, destroy};
