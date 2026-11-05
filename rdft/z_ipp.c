// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include "simd-support/simd.h"
#include <ipp.h>
#include <pffft/pffft.h>
#include <stdlib.h>
#include <string.h>

#define LARGEST_SIZE (1<<26) /* the PFFFT backend's limit */

#ifdef PFFFT_AVX2
/* PFFFT built for AVX2 + FMA by pffft_avx2_copy() in cmake/simd.cmake */
PFFFT_Setup *pffft_avx2_new_setup(int N, pffft_transform_t transform);
void pffft_avx2_destroy_setup(PFFFT_Setup *setup);
void pffft_avx2_transform_ordered(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
#endif

struct fft {
	int size;
	int order; /* log2(size) for IPP's FFT, else 0 */
	int pffft;
	IppsFFTSpec_R_32f *fft; /* inside spec */
	IppsDFTSpec_R_32f *dft; /* inside spec */
	Ipp8u *spec;
	Ipp8u *work; /* so a transform never allocates */
	PFFFT_Setup *setup;
	void (*ordered)(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
	void (*destroy)(PFFFT_Setup *setup);
	float *scratch; /* so PFFFT doesn't use the stack */
	float *in;
	float *out;
};

/* runs even on CPUs too old for IPP's transforms */
static Ipp64u cpu_features(void){
	Ipp64u features = 0;
	Ipp32u regs[4];
	return ippGetCpuFeatures(&features, regs)==ippStsNoErr ? features : 0;
}

/* returns 0 if out of memory */
static int ipp_plan(fft *f){
	static int dispatched = 0; /* fft_new runs on one thread at a time */
	if (!dispatched){
		ippInit(); /* once, so no running transform sees IPP's dispatch rewritten */
		dispatched = 1;
	}
	while ((1<<f->order)<f->size){
		f->order++;
	}
	if ((1<<f->order)!=f->size){
		f->order = 0;
	}
	int specSize = 0;
	int initSize = 0;
	int workSize = 0;
	IppStatus st;
	if (f->order){
		st = ippsFFTGetSize_R_32f(f->order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSize, &initSize, &workSize);
	} else {
		st = ippsDFTGetSize_R_32f(f->size, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSize, &initSize, &workSize);
	}
	Ipp8u *init = initSize>0 ? ippsMalloc_8u(initSize) : NULL;
	f->spec = ippsMalloc_8u(specSize);
	f->work = ippsMalloc_8u(workSize>0 ? workSize : 1);
	if (st!=ippStsNoErr || !f->spec || !f->work || (initSize>0 && !init)){
		st = ippStsMemAllocErr;
	} else if (f->order){
		st = ippsFFTInit_R_32f(&f->fft, f->order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, f->spec, init);
	} else {
		f->dft = (IppsDFTSpec_R_32f *)f->spec;
		st = ippsDFTInit_R_32f(f->size, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, f->dft, init);
	}
	ippsFree(init);
	return st==ippStsNoErr;
}

static void use_pffft(fft *f, int path){
	f->pffft = path;
}

/* PFFFT's sizes, so they don't change with the backend */
int fft_next_size(int n){
	if (n>LARGEST_SIZE){
		return 0; /* also keeps n+31 from overflowing */
	}
	for (int m=(n>32 ? n+31 : 32)/32*32; m<=LARGEST_SIZE; m+=32){
		int r = m/32;
		while (r%2==0){
			r /= 2;
		}
		while (r%3==0){
			r /= 3;
		}
		while (r%5==0){
			r /= 5;
		}
		if (r==1){
			return m;
		}
	}
	return 0;
}

fft *fft_new(int size){
	if (size<32 || size>LARGEST_SIZE){
		return NULL;
	}
	fft *f = calloc(1, sizeof(*f));
	if (!f){
		return NULL;
	}
	f->size = size;
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc(((size_t)size+2)*sizeof(float));
	if (!f->in || !f->out){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float));
	Ipp64u cpu = cpu_features();
	int ipp = (cpu&ippCPUID_SSE42)!=0; /* IPP faults without SSE4.2, which ippInit doesn't report on AMD */
#ifdef PFFFT_AVX2
	int avx2 = ipp && simd_avx2_fma(); /* every AVX2 CPU has SSE4.2 */
#else
	int avx2 = 0;
#endif
	if (ipp && !ipp_plan(f)){
		fft_free(f);
		return NULL;
	}
	if (ipp && (f->order || !avx2)){
		return f;
	}
	if (avx2){
#ifdef PFFFT_AVX2
		f->setup = pffft_avx2_new_setup(size, PFFFT_REAL);
		f->ordered = pffft_avx2_transform_ordered;
		f->destroy = pffft_avx2_destroy_setup;
#endif
	} else {
		f->setup = pffft_new_setup(size, PFFFT_REAL);
		f->ordered = pffft_transform_ordered;
		f->destroy = pffft_destroy_setup;
	}
	f->scratch = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!f->setup || !f->scratch){
		if (!ipp){
			fft_free(f);
			return NULL;
		}
		f->pffft = 0;
	} else if (!ipp || race_paths(f, size, use_pffft)){
		f->pffft = 1;
		if (ipp){
			ippsFree(f->spec);
			ippsFree(f->work);
			f->spec = NULL;
			f->work = NULL;
		}
		return f;
	} else {
		f->pffft = 0;
	}
	if (f->setup){
		f->destroy(f->setup);
		f->setup = NULL;
	}
	pffft_aligned_free(f->scratch);
	f->scratch = NULL;
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	if (f->spec || f->work){ /* set only where IPP runs */
		ippsFree(f->spec);
		ippsFree(f->work);
	}
	if (f->setup){
		f->destroy(f->setup);
	}
	pffft_aligned_free(f->scratch);
	pffft_aligned_free(f->in);
	pffft_aligned_free(f->out);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	if (f->pffft){
		f->ordered(f->setup, f->in, f->out, f->scratch, PFFFT_FORWARD);
	} else if (f->order){
		ippsFFTFwd_RToCCS_32f(f->in, f->out, f->fft, f->work);
	} else {
		ippsDFTFwd_RToCCS_32f(f->in, f->out, f->dft, f->work);
	}
}

float fft_power(const fft *f, float *power, int from){
	if (f->pffft){
		return power_packed(f->out, f->size, power, from);
	}
	return power_interleaved(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	if (f->pffft && (k==0 || k==f->size/2)){
		*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
