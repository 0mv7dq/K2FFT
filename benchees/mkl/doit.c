// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* oneMKL's DFTI, as rdft/mkl.c: complex either way, real forward or back (CCE, FFTW's n/2+1 layout), any rank and positive strides,
   one batch dimension, in libbench2's precision */
#include "benchee.h"
#include "simd-support/simd.h"
#include <mkl_dfti.h>
#include <mkl_service.h>

static char ver[198];

static const char *version(void){
	mkl_get_version_string(ver, (int)sizeof(ver));
	return ver;
}

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("version", version)
BENCH_DOC("threads", "1 (sequential)")
BENCH_DOC("author", "Intel")
BENCH_DOC("url", "https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html")
BENCH_DOC("copyright", "Intel Simplified Software License")
END_BENCH_DOC

static DFTI_DESCRIPTOR_HANDLE h;

/* oneMKL 2026's code paths start at AVX2, as rdft/mkl.c checks */
int can_do(bench_problem *p){
	const bench_tensor *sz = p->sz, *v = p->vecsz;
	if (!simd_avx2_fma() || p->kind==PROBLEM_R2R || p->split || !BENCH_FINITE_RNK(sz->rnk) || sz->rnk<1 || !BENCH_FINITE_RNK(v->rnk)
		|| v->rnk>1){
		return 0;
	}
	for (int d = 0; d<sz->rnk; d++){
		if (sz->dims[d].is<=0 || sz->dims[d].os<=0){
			return 0;
		}
	}
	return v->rnk==0 || (v->dims[0].is>0 && v->dims[0].os>0);
}

void setup(bench_problem *p){
	const bench_tensor *sz = p->sz;
	const int backward = p->sign>0;
	MKL_LONG n[16], is[17] = {0}, os[17] = {0};
	BENCH_ASSERT(sz->rnk<16);
	for (int d = 0; d<sz->rnk; d++){
		n[d] = sz->dims[d].n;
		is[d+1] = sz->dims[d].is;
		os[d+1] = sz->dims[d].os;
	}
	const enum DFTI_CONFIG_VALUE precision = sizeof(bench_real)==sizeof(float) ? DFTI_SINGLE : DFTI_DOUBLE;
	const enum DFTI_CONFIG_VALUE domain = p->kind==PROBLEM_COMPLEX ? DFTI_COMPLEX : DFTI_REAL;
	BENCH_ASSERT((sz->rnk==1 ? DftiCreateDescriptor(&h, precision, domain, 1, n[0])
							 : DftiCreateDescriptor(&h, precision, domain, (MKL_LONG)sz->rnk, n))==DFTI_NO_ERROR);
	BENCH_ASSERT(DftiSetValue(h, DFTI_PLACEMENT, p->in_place ? DFTI_INPLACE : DFTI_NOT_INPLACE)==DFTI_NO_ERROR);
	BENCH_ASSERT(DftiSetValue(h, DFTI_FWD_STRIDES, backward ? os : is)==DFTI_NO_ERROR); /* a backward's input, the backward domain */
	BENCH_ASSERT(DftiSetValue(h, DFTI_BWD_STRIDES, backward ? is : os)==DFTI_NO_ERROR);
	if (domain==DFTI_REAL){
		BENCH_ASSERT(DftiSetValue(h, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX)==DFTI_NO_ERROR); /* (re, im) pairs */
	}
	if (p->vecsz->rnk==1){
		const bench_iodim *v = &p->vecsz->dims[0];
		BENCH_ASSERT(DftiSetValue(h, DFTI_NUMBER_OF_TRANSFORMS, (MKL_LONG)v->n)==DFTI_NO_ERROR);
		BENCH_ASSERT(DftiSetValue(h, DFTI_FWD_DISTANCE, (MKL_LONG)(backward ? v->os : v->is))==DFTI_NO_ERROR);
		BENCH_ASSERT(DftiSetValue(h, DFTI_BWD_DISTANCE, (MKL_LONG)(backward ? v->is : v->os))==DFTI_NO_ERROR);
	}
	BENCH_ASSERT(DftiCommitDescriptor(h)==DFTI_NO_ERROR);
	doit(1, p); /* one, as rdft/mkl.c's make: MKL takes its workspace at its first (Zen 5's 4096 and up) */
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		if (p->sign>0){
			DftiComputeBackward(h, p->in, p->out);
		} else {
			DftiComputeForward(h, p->in, p->out);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	DftiFreeDescriptor(&h);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
	mkl_free_buffers();
}
