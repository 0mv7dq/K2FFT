// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTS, as benchees/ffts: one 1D transform of floats out of place at a power of two, complex either way, real forward or back, on
   arrays aligned to 16 bytes; FFTW's layouts already; a backward one's scale measured, as Ne10's */
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <ffts.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	ffts_plan_t *ffts;
	problem_kind kind;
	int size;
	float scale; /* 1, or n where it scaled by 1/n */
} plan;

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
#if defined(__arm__)
	if (!simd_neon()){
		return 0;
	}
#endif
	if (!pb->single || pb->kind==KIND_R2R || pb->in==pb->out || size<(pb->kind==KIND_DFT ? 2 : 4) || (size&(size-1))!=0){
		return 0;
	}
	return ((uintptr_t)pb->in|(uintptr_t)pb->out)%16==0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->ffts){
		ffts_free(p->ffts);
	}
	free(p);
}

static void execute(const void *p_, void *in, void *out){
	const plan *p = p_;
	ffts_execute(p->ffts, in, out);
	if (p->scale!=1){
		float *o = out;
		const int n = p->kind==KIND_DFT ? 2*p->size : p->size;
		for (int i=0; i<n; i++){
			o[i] *= p->scale;
		}
	}
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	const int backward = pb->kind==KIND_C2R || (pb->kind==KIND_DFT && pb->sign>0);
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->kind;
	p->size = size;
	p->scale = 1;
	p->ffts = pb->kind==KIND_DFT ? ffts_init_1d((size_t)size, backward ? FFTS_BACKWARD : FFTS_FORWARD)
								 : ffts_init_1d_real((size_t)size, backward ? FFTS_BACKWARD : FFTS_FORWARD);
	if (!p->ffts){
		destroy(p);
		return NULL;
	}
	if (backward){ /* DC alone, whose inverse is 1 everywhere */
		float *in = pb->in, *out = pb->out;
		memset(in, 0, (size_t)(pb->kind==KIND_DFT ? 2*size : size+2)*sizeof(float));
		in[0] = 1;
		execute(p, in, out);
		p->scale = out[0]!=0 ? 1/out[0] : 0;
		if (p->scale!=1 && p->scale!=(float)size){
			destroy(p);
			return NULL;
		}
	}
	return p;
}

const kernel k_ffts = {"ffts", can_do, make, execute, destroy, 0};
