// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* oneMKL's DFTI, as benchees/mkl: complex either way, real forward or back (FFTW's n/2+1 layout), any rank and strides, one batch
   dimension, float and double; sequential; a transform at planning, as its first may allocate */
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <mkl_dfti.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct plan {
	DFTI_DESCRIPTOR_HANDLE h;
	int backward;
} plan;

/* oneMKL 2026's code paths start at AVX2; its strides positive */
static int can_do(const problem *pb){
	if (pb->kind==KIND_R2R || pb->vrank>1 || !simd_avx2_fma()){
		return 0;
	}
	for (int d=0; d<pb->rank; d++){
		if (pb->dims[d].is<=0 || pb->dims[d].os<=0){
			return 0;
		}
	}
	return pb->vrank==0 || (pb->vdims[0].is>0 && pb->vdims[0].os>0);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->h){
		DftiFreeDescriptor(&p->h);
	}
	free(p);
}

static void run(const plan *p, void *in, void *out){
	DFTI_DESCRIPTOR_HANDLE h = p->h; /* its compute takes it w/o const */
	if (p->backward){
		DftiComputeBackward(h, in, out);
	} else {
		DftiComputeForward(h, in, out);
	}
}

static void *make(const problem *pb){
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->backward = pb->kind==KIND_C2R || (pb->kind==KIND_DFT && pb->sign>0);
	MKL_LONG n[RANKS], is[RANKS+1] = {0}, os[RANKS+1] = {0}; /* offset 0, then each dimension's */
	for (int d=0; d<pb->rank; d++){
		n[d] = pb->dims[d].n;
		is[d+1] = pb->dims[d].is;
		os[d+1] = pb->dims[d].os;
	}
	const enum DFTI_CONFIG_VALUE precision = pb->single ? DFTI_SINGLE : DFTI_DOUBLE, domain = pb->kind==KIND_DFT ? DFTI_COMPLEX : DFTI_REAL;
	MKL_LONG failed = pb->rank==1 ? DftiCreateDescriptor(&p->h, precision, domain, 1, n[0])
								  : DftiCreateDescriptor(&p->h, precision, domain, (MKL_LONG)pb->rank, n);
	/* the forward domain's the real one, or a complex forward's input; a backward's input is the backward domain's */
	const MKL_LONG *fwd = p->backward ? os : is, *bwd = p->backward ? is : os;
	failed = failed || DftiSetValue(p->h, DFTI_PLACEMENT, pb->in==pb->out ? DFTI_INPLACE : DFTI_NOT_INPLACE)
			 || DftiSetValue(p->h, DFTI_FWD_STRIDES, fwd) || DftiSetValue(p->h, DFTI_BWD_STRIDES, bwd);
	if (!failed && domain==DFTI_REAL){
		failed = DftiSetValue(p->h, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX);
	}
	if (!failed && pb->vrank==1){
		const MKL_LONG vi = pb->vdims[0].is, vo = pb->vdims[0].os;
		failed = DftiSetValue(p->h, DFTI_NUMBER_OF_TRANSFORMS, (MKL_LONG)pb->vdims[0].n)
				 || DftiSetValue(p->h, DFTI_FWD_DISTANCE, p->backward ? vo : vi) || DftiSetValue(p->h, DFTI_BWD_DISTANCE, p->backward ? vi : vo);
	}
	if (failed || DftiCommitDescriptor(p->h)!=DFTI_NO_ERROR){
		destroy(p);
		return NULL;
	}
	run(p, pb->in, pb->out);
	return p;
}

static void execute(const void *p, void *in, void *out){
	run(p, in, out);
}

const kernel k_mkl = {"mkl", can_do, make, execute, destroy};
