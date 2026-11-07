// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT, as benchees/pffft: one 1D transform, complex either way, real forward or back, of floats, or of doubles (pffftd_*) where
   cmake/pffft.cmake built them; rdft/pffft_*.c build it again for a copy cmake/pffft.cmake's pffft_copy renamed COPY_*, floats alone */
#include "kernel/kernel.h"
#include <pffft/pffft.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CAT_(a, b) a##_##b
#define CAT(a, b) CAT_(a, b)
#define STR_(a) #a
#define STR(a) STR_(a)

#ifdef COPY
int CAT(COPY, is_valid_size)(int N, pffft_transform_t cplx);
int CAT(COPY, simd_size)(void);
PFFFT_Setup *CAT(COPY, new_setup)(int N, pffft_transform_t transform);
void CAT(COPY, destroy_setup)(PFFFT_Setup *setup);
void CAT(COPY, transform_ordered)(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
#else
#define COPY pffft
#define RUNS 1 /* the CPU runs what the compiler targets */
#ifdef K2FFT_PFFFTD
#include <pffft/pffft_double.h>
#define DOUBLES 1
#endif
#endif
#ifndef DOUBLES
#define DOUBLES 0
#endif

typedef struct plan {
	PFFFT_Setup *setup;
	void *dsetup; /* a double one's PFFFTD_Setup */
	void *work; /* so PFFFT doesn't use the stack */
	void *packed; /* a C2R's input in PFFFT's order, where it's kept */
	int size;
	int single;
	problem_kind kind;
	pffft_direction_t way;
} plan;

/* one contiguous 1D transform, on arrays aligned to PFFFT's vectors, as its loads are */
static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	if (!RUNS || size<=0 || pb->kind==KIND_R2R || (!pb->single && !DOUBLES)){
		return 0;
	}
	const pffft_transform_t t = pb->kind==KIND_DFT ? PFFFT_COMPLEX : PFFFT_REAL;
#if DOUBLES
	if (!pb->single){
		return ((uintptr_t)pb->in|(uintptr_t)pb->out)%(8*(uintptr_t)pffftd_simd_size())==0 && pffftd_is_valid_size(size, t);
	}
#endif
	return ((uintptr_t)pb->in|(uintptr_t)pb->out)%(4*(uintptr_t)CAT(COPY, simd_size)())==0 && CAT(COPY, is_valid_size)(size, t);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->setup){
		CAT(COPY, destroy_setup)(p->setup);
	}
#if DOUBLES
	if (p->dsetup){
		pffftd_destroy_setup(p->dsetup);
	}
#endif
	pffft_aligned_free(p->work);
	pffft_aligned_free(p->packed);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n, complex = pb->kind==KIND_DFT;
	const int keep = pb->kind==KIND_C2R && pb->preserve && pb->in!=pb->out;
	const size_t e = pb->single ? sizeof(float) : sizeof(double);
	const pffft_transform_t t = complex ? PFFFT_COMPLEX : PFFFT_REAL;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->single = pb->single;
	p->kind = pb->kind;
	p->way = pb->kind==KIND_C2R || (complex && pb->sign>0) ? PFFFT_BACKWARD : PFFFT_FORWARD;
#if DOUBLES
	if (!p->single){
		p->dsetup = pffftd_new_setup(size, t);
	} else
#endif
	{
		p->setup = CAT(COPY, new_setup)(size, t);
	}
	p->work = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*e);
	if (keep){
		p->packed = pffft_aligned_malloc((size_t)size*e);
	}
	if ((!p->setup && !p->dsetup) || !p->work || (keep && !p->packed)){
		destroy(p);
		return NULL;
	}
	return p;
}

/* FFTW's layouts: a real one's n/2+1 (re, im) pairs, PFFFT's w/ the Nyquist re in the DC's im */
#define RUN(R, transform, setup) \
	do { \
		R *in = in_, *out = out_; \
		const int n = p->size; \
		if (p->kind==KIND_C2R){ \
			R *x = p->packed ? p->packed : in; \
			if (p->packed){ \
				memcpy(x, in, (size_t)n*sizeof(R)); \
			} \
			x[1] = in[n]; \
			transform(setup, x, out, p->work, PFFFT_BACKWARD); \
			return; \
		} \
		transform(setup, in, out, p->work, p->way); \
		if (p->kind==KIND_R2C){ \
			out[n] = out[1]; \
			out[n+1] = 0; \
			out[1] = 0; \
		} \
	} while (0)

static void execute(const void *p_, void *in_, void *out_){
	const plan *p = p_;
#if DOUBLES
	if (!p->single){
		RUN(double, pffftd_transform_ordered, p->dsetup);
		return;
	}
#endif
	RUN(float, CAT(COPY, transform_ordered), p->setup);
}

const kernel CAT(k, COPY) = {STR(COPY), can_do, make, execute, destroy};
