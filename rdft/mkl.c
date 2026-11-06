// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* oneMKL's DFTI, as benchees/mkl: real, out of place, size/2+1 (re, im) pairs; sequential */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include "simd-support/simd.h"
#include <mkl_dfti.h>
#include <stddef.h>

/* oneMKL 2026's code paths start at AVX2 */
static int can_do(int size){
	return size>0 && simd_avx2_fma();
}

static void destroy(void *p){
	DFTI_DESCRIPTOR_HANDLE h = p;
	DftiFreeDescriptor(&h);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
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

static void forward(void *p, float *in, float *out){
	DftiComputeForward((DFTI_DESCRIPTOR_HANDLE)p, in, out);
}

const kernel k_mkl = {"mkl", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
