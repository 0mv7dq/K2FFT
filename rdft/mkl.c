// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* oneMKL's DFTI, as benchees/mkl: real, out of place, size/2+1 (re, im) pairs; sequential */
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <mkl_dfti.h>
#include <stddef.h>

/* oneMKL 2026's code paths start at AVX2 */
static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return size>0 && simd_avx2_fma();
}

static void destroy(void *p){
	DFTI_DESCRIPTOR_HANDLE h = p;
	DftiFreeDescriptor(&h);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	DFTI_DESCRIPTOR_HANDLE h = NULL;
	if (DftiCreateDescriptor(&h, DFTI_SINGLE, DFTI_REAL, 1, (MKL_LONG)size)!=DFTI_NO_ERROR){
		return NULL;
	}
	if (DftiSetValue(h, DFTI_PLACEMENT, DFTI_NOT_INPLACE)!=DFTI_NO_ERROR
		|| DftiSetValue(h, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX)!=DFTI_NO_ERROR || DftiCommitDescriptor(h)!=DFTI_NO_ERROR){
		destroy(h);
		return NULL;
	}
	return h;
}

static void execute(const void *p_, void *in_, void *out_){
	void *p = (void *)p_; /* the library's execute takes it w/o const */
	float *in = in_, *out = out_;
	DftiComputeForward((DFTI_DESCRIPTOR_HANDLE)p, in, out);
}

const kernel k_mkl = {"mkl", can_do, make, execute, destroy};
