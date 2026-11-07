// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* PFFFT, as benchees/pffft: one 1D transform, complex either way, real forward or back; rdft/pffft_*.c build it again for a copy
   cmake/pffft.cmake's pffft_copy renamed COPY_* */
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
#endif

typedef struct plan {
	PFFFT_Setup *setup;
	float *work; /* so PFFFT doesn't use the stack */
	float *packed; /* a C2R's input in PFFFT's order, where it's kept */
	int size;
	problem_kind kind;
	pffft_direction_t way;
} plan;

/* one contiguous 1D transform of floats, on arrays aligned to PFFFT's vectors, as its loads are */
static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	if (!RUNS || !pb->single || size<=0 || pb->kind==KIND_R2R){
		return 0;
	}
	const uintptr_t align = 4*(uintptr_t)CAT(COPY, simd_size)();
	return ((uintptr_t)pb->in|(uintptr_t)pb->out)%align==0 &&
		   CAT(COPY, is_valid_size)(size, pb->kind==KIND_DFT ? PFFFT_COMPLEX : PFFFT_REAL);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->setup){
		CAT(COPY, destroy_setup)(p->setup);
	}
	pffft_aligned_free(p->work);
	pffft_aligned_free(p->packed);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n, complex = pb->kind==KIND_DFT;
	const int keep = pb->kind==KIND_C2R && pb->preserve && pb->in!=pb->out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->kind = pb->kind;
	p->way = pb->kind==KIND_C2R || (complex && pb->sign>0) ? PFFFT_BACKWARD : PFFFT_FORWARD;
	p->setup = CAT(COPY, new_setup)(size, complex ? PFFFT_COMPLEX : PFFFT_REAL);
	p->work = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*sizeof(float));
	if (keep){
		p->packed = pffft_aligned_malloc((size_t)size*sizeof(float));
	}
	if (!p->setup || !p->work || (keep && !p->packed)){
		destroy(p);
		return NULL;
	}
	return p;
}

/* FFTW's layouts: a real one's n/2+1 (re, im) pairs, PFFFT's w/ the Nyquist re in the DC's im */
static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	const int n = p->size;
	if (p->kind==KIND_C2R){
		float *x = p->packed ? p->packed : in;
		if (p->packed){
			memcpy(x, in, (size_t)n*sizeof(float));
		}
		x[1] = in[n];
		CAT(COPY, transform_ordered)(p->setup, x, out, p->work, PFFFT_BACKWARD);
		return;
	}
	CAT(COPY, transform_ordered)(p->setup, in, out, p->work, p->way);
	if (p->kind==KIND_R2C){
		out[n] = out[1];
		out[n+1] = 0;
		out[1] = 0;
	}
}

const kernel CAT(k, COPY) = {STR(COPY), can_do, make, execute, destroy};
