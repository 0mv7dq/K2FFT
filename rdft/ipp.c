// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* IPP's real FFT at powers of two, its DFT elsewhere, as benchees/ipp */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include <ipp.h>
#include <stdlib.h>

typedef struct plan {
	IppsFFTSpec_R_32f *fft; /* inside spec, at powers of two */
	IppsDFTSpec_R_32f *dft; /* inside spec, elsewhere */
	Ipp8u *spec;
	Ipp8u *work; /* so a transform never allocates */
} plan;

/* IPP 2026 faults w/o SSE4.2, which ippInit doesn't report on AMD; ippGetCpuFeatures runs on any x86-64 */
static int can_do(int size){
	Ipp64u features = 0;
	Ipp32u regs[4];
	return size>0 && ippGetCpuFeatures(&features, regs)==ippStsNoErr && (features&ippCPUID_SSE42)!=0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->spec){
		ippsFree(p->spec);
	}
	if (p->work){
		ippsFree(p->work);
	}
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	static int dispatched = 0; /* fft_new runs on one thread at a time */
	if (!dispatched){
		ippInit(); /* once, so no running transform sees IPP's dispatch rewritten */
		dispatched = 1;
	}
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	int order = 0;
	while ((1<<order)<size){
		order++;
	}
	int pow2 = (1<<order)==size;
	int spec_size = 0;
	int init_size = 0;
	int work_size = 0;
	IppStatus st = pow2 ? ippsFFTGetSize_R_32f(order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &spec_size, &init_size, &work_size)
	               : ippsDFTGetSize_R_32f(size, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &spec_size, &init_size, &work_size);
	Ipp8u *init = init_size>0 ? ippsMalloc_8u(init_size) : NULL;
	p->spec = ippsMalloc_8u(spec_size>0 ? spec_size : 1);
	p->work = ippsMalloc_8u(work_size>0 ? work_size : 1);
	if (st!=ippStsNoErr || !p->spec || !p->work || (init_size>0 && !init)){
		st = ippStsMemAllocErr;
	} else if (pow2){
		st = ippsFFTInit_R_32f(&p->fft, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, p->spec, init);
	} else {
		p->dft = (IppsDFTSpec_R_32f *)p->spec;
		st = ippsDFTInit_R_32f(size, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, p->dft, init);
	}
	if (init){
		ippsFree(init);
	}
	if (st!=ippStsNoErr){
		destroy(p);
		return NULL;
	}
	return p;
}

/* CCS: size/2+1 (re, im) pairs */
static void forward(void *p_, float *in, float *out){
	const plan *p = p_;
	if (p->fft){
		ippsFFTFwd_RToCCS_32f(in, out, p->fft, p->work);
	} else {
		ippsDFTFwd_RToCCS_32f(in, out, p->dft, p->work);
	}
}

const kernel k_ipp = {"ipp", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
