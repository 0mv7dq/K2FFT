// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* IPP's FFT at powers of two, its DFT elsewhere, as benchees/ipp: one 1D transform out of place, complex either way, real forward or
   back (CCS, FFTW's n/2+1 layout), float and double */
#include "kernel/kernel.h"
#include <ipp.h>
#include <stdlib.h>

enum { R2C, C2R, FORWARD, BACKWARD }; /* the transform, + 4 for double, + 8 for the DFT */

typedef struct plan {
	void *spec; /* inside mem: an Ipps{FFT,DFT}Spec_{R,C}_{32f,32fc,64f,64fc} */
	Ipp8u *mem;
	Ipp8u *work; /* so a transform never allocates */
	int how;
} plan;

/* IPP 2026 faults w/o SSE4.2, which ippInit doesn't report on AMD; ippGetCpuFeatures runs on any x86-64 */
static int can_do(const problem *pb){
	Ipp64u features = 0;
	Ipp32u regs[4];
	return problem_1d(pb)>0 && pb->kind!=KIND_R2R && pb->in!=pb->out && ippGetCpuFeatures(&features, regs)==ippStsNoErr
		   && (features&ippCPUID_SSE42)!=0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->mem){
		ippsFree(p->mem);
	}
	if (p->work){
		ippsFree(p->work);
	}
	free(p);
}

/* its spec's, init's and work's sizes, then its spec, for one of the 8 FFTs and DFTs */
static IppStatus sizes(int dft, int dbl, int cplx, int n, int order, int *spec, int *init, int *work){
	const int flag = IPP_FFT_NODIV_BY_ANY;
	switch (dft*4+dbl*2+cplx){
	case 0: return ippsFFTGetSize_R_32f(order, flag, ippAlgHintFast, spec, init, work);
	case 1: return ippsFFTGetSize_C_32fc(order, flag, ippAlgHintFast, spec, init, work);
	case 2: return ippsFFTGetSize_R_64f(order, flag, ippAlgHintFast, spec, init, work);
	case 3: return ippsFFTGetSize_C_64fc(order, flag, ippAlgHintFast, spec, init, work);
	case 4: return ippsDFTGetSize_R_32f(n, flag, ippAlgHintFast, spec, init, work);
	case 5: return ippsDFTGetSize_C_32fc(n, flag, ippAlgHintFast, spec, init, work);
	case 6: return ippsDFTGetSize_R_64f(n, flag, ippAlgHintFast, spec, init, work);
	default: return ippsDFTGetSize_C_64fc(n, flag, ippAlgHintFast, spec, init, work);
	}
}

static IppStatus init(plan *p, int dft, int dbl, int cplx, int n, int order, Ipp8u *buf){
	const int flag = IPP_FFT_NODIV_BY_ANY;
	switch (dft*4+dbl*2+cplx){
	case 0: return ippsFFTInit_R_32f((IppsFFTSpec_R_32f **)&p->spec, order, flag, ippAlgHintFast, p->mem, buf);
	case 1: return ippsFFTInit_C_32fc((IppsFFTSpec_C_32fc **)&p->spec, order, flag, ippAlgHintFast, p->mem, buf);
	case 2: return ippsFFTInit_R_64f((IppsFFTSpec_R_64f **)&p->spec, order, flag, ippAlgHintFast, p->mem, buf);
	case 3: return ippsFFTInit_C_64fc((IppsFFTSpec_C_64fc **)&p->spec, order, flag, ippAlgHintFast, p->mem, buf);
	default:
		p->spec = p->mem; /* a DFT's spec is its memory */
		switch (dbl*2+cplx){
		case 0: return ippsDFTInit_R_32f(n, flag, ippAlgHintFast, p->spec, buf);
		case 1: return ippsDFTInit_C_32fc(n, flag, ippAlgHintFast, p->spec, buf);
		case 2: return ippsDFTInit_R_64f(n, flag, ippAlgHintFast, p->spec, buf);
		default: return ippsDFTInit_C_64fc(n, flag, ippAlgHintFast, p->spec, buf);
		}
	}
}

static void *make(const problem *pb){
	static int dispatched = 0; /* the planner plans on one thread at a time (race_lock) */
	if (!dispatched){
		ippInit(); /* once, so no running transform sees IPP's dispatch rewritten */
		dispatched = 1;
	}
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	const int n = pb->dims[0].n, cplx = pb->kind==KIND_DFT, dbl = !pb->single;
	int order = 0;
	while ((1<<order)<n){
		order++;
	}
	const int dft = (1<<order)!=n;
	p->how = (cplx ? (pb->sign<0 ? FORWARD : BACKWARD) : (pb->kind==KIND_R2C ? R2C : C2R))+4*dbl+8*dft;
	int spec = 0, buf = 0, work = 0;
	IppStatus st = sizes(dft, dbl, cplx, n, order, &spec, &buf, &work);
	Ipp8u *initbuf = st==ippStsNoErr && buf>0 ? ippsMalloc_8u(buf) : NULL;
	p->mem = ippsMalloc_8u(spec>0 ? spec : 1);
	p->work = ippsMalloc_8u(work>0 ? work : 1);
	if (st!=ippStsNoErr || !p->mem || !p->work || (buf>0 && !initbuf)){
		st = ippStsMemAllocErr;
	} else {
		st = init(p, dft, dbl, cplx, n, order, initbuf);
	}
	if (initbuf){
		ippsFree(initbuf);
	}
	if (st!=ippStsNoErr){
		destroy(p);
		return NULL;
	}
	return p;
}

static void execute(const void *p_, void *in, void *out){
	const plan *p = p_;
	void *s = p->spec;
	Ipp8u *w = p->work;
	switch (p->how){
	case R2C: ippsFFTFwd_RToCCS_32f(in, out, s, w); break;
	case C2R: ippsFFTInv_CCSToR_32f(in, out, s, w); break;
	case FORWARD: ippsFFTFwd_CToC_32fc(in, out, s, w); break;
	case BACKWARD: ippsFFTInv_CToC_32fc(in, out, s, w); break;
	case 4+R2C: ippsFFTFwd_RToCCS_64f(in, out, s, w); break;
	case 4+C2R: ippsFFTInv_CCSToR_64f(in, out, s, w); break;
	case 4+FORWARD: ippsFFTFwd_CToC_64fc(in, out, s, w); break;
	case 4+BACKWARD: ippsFFTInv_CToC_64fc(in, out, s, w); break;
	case 8+R2C: ippsDFTFwd_RToCCS_32f(in, out, s, w); break;
	case 8+C2R: ippsDFTInv_CCSToR_32f(in, out, s, w); break;
	case 8+FORWARD: ippsDFTFwd_CToC_32fc(in, out, s, w); break;
	case 8+BACKWARD: ippsDFTInv_CToC_32fc(in, out, s, w); break;
	case 12+R2C: ippsDFTFwd_RToCCS_64f(in, out, s, w); break;
	case 12+C2R: ippsDFTInv_CCSToR_64f(in, out, s, w); break;
	case 12+FORWARD: ippsDFTFwd_CToC_64fc(in, out, s, w); break;
	default: ippsDFTInv_CToC_64fc(in, out, s, w); break;
	}
}

const kernel k_ipp = {"ipp", can_do, make, execute, destroy, 0};
