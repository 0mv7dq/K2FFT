// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

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
	return rof_size(p)>0 && simd_avx2_fma();
}

void setup(bench_problem *p){
	BENCH_ASSERT(DftiCreateDescriptor(&h, DFTI_SINGLE, DFTI_REAL, 1, (MKL_LONG)rof_size(p))==DFTI_NO_ERROR);
	BENCH_ASSERT(DftiSetValue(h, DFTI_PLACEMENT, DFTI_NOT_INPLACE)==DFTI_NO_ERROR);
	BENCH_ASSERT(DftiSetValue(h, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX)==DFTI_NO_ERROR); /* size/2+1 (re, im) pairs */
	BENCH_ASSERT(DftiCommitDescriptor(h)==DFTI_NO_ERROR);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		DftiComputeForward(h, p->in, p->out);
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
