// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT, as benchees/pffft; rdft/pffft_*.c build it again for a copy cmake/pffft.cmake's pffft_copy renamed COPY_* */
#include "kernel/kernel.h"
#include "kernel/pow.h"
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
} plan;

static int can_do(int size){
	return RUNS && CAT(COPY, is_valid_size)(size, PFFFT_REAL);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->setup){
		CAT(COPY, destroy_setup)(p->setup);
	}
	pffft_aligned_free(p->work);
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->setup = CAT(COPY, new_setup)(size, PFFFT_REAL);
	p->work = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!p->setup || !p->work){
		destroy(p);
		return NULL;
	}
	return p;
}

static void forward(void *p_, float *in, float *out){
	const plan *p = p_;
	CAT(COPY, transform_ordered)(p->setup, in, out, p->work, PFFFT_FORWARD);
}

const kernel CAT(k, COPY) = {STR(COPY), can_do, make, forward, destroy, power_packed, bin_packed};
