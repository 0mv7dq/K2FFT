// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* IPP's FFT at powers of two, its DFT elsewhere, as rdft/ipp.c: one 1D transform out of place, complex either way, real forward or
   back (CCS, FFTW's n/2+1 layout), in libbench2's precision */
#include "benchee.h"
#include <ipp.h>

static const char *version(void){
	return ippsGetLibVersion()->Version;
}

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("version", version)
BENCH_DOC("author", "Intel")
BENCH_DOC("url", "https://www.intel.com/content/www/us/en/developer/tools/oneapi/ipp.html")
BENCH_DOC("copyright", "Intel Simplified Software License")
END_BENCH_DOC

enum { R2C, C2R, FORWARD, BACKWARD }; /* the transform, + 4 for double, + 8 for the DFT */
static void *spec;
static Ipp8u *mem, *work;
static int how;

/* IPP 2026 faults w/o SSE4.2, as rdft/ipp.c checks */
int can_do(bench_problem *p){
	Ipp64u features = 0;
	Ipp32u regs[4];
	return bench_1d(p)>0 && p->kind!=PROBLEM_R2R && !p->in_place && ippGetCpuFeatures(&features, regs)==ippStsNoErr
		   && (features&ippCPUID_SSE42)!=0;
}

void setup(bench_problem *p){
	const int n = bench_1d(p), cplx = p->kind==PROBLEM_COMPLEX, dbl = sizeof(bench_real)==sizeof(double), flag = IPP_FFT_NODIV_BY_ANY;
	int order = 0, specSize = 0, initSize = 0, workSize = 0;
	while ((1<<order)<n){
		order++;
	}
	const int dft = (1<<order)!=n;
	how = (cplx ? (p->sign<0 ? FORWARD : BACKWARD) : (p->sign<0 ? R2C : C2R))+4*dbl+8*dft;
	IppStatus st;
	switch (dft*4+dbl*2+cplx){
	case 0: st = ippsFFTGetSize_R_32f(order, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 1: st = ippsFFTGetSize_C_32fc(order, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 2: st = ippsFFTGetSize_R_64f(order, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 3: st = ippsFFTGetSize_C_64fc(order, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 4: st = ippsDFTGetSize_R_32f(n, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 5: st = ippsDFTGetSize_C_32fc(n, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	case 6: st = ippsDFTGetSize_R_64f(n, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	default: st = ippsDFTGetSize_C_64fc(n, flag, ippAlgHintFast, &specSize, &initSize, &workSize); break;
	}
	BENCH_ASSERT(st==ippStsNoErr);
	Ipp8u *init = initSize>0 ? ippsMalloc_8u(initSize) : NULL;
	mem = ippsMalloc_8u(specSize>0 ? specSize : 1);
	work = ippsMalloc_8u(workSize>0 ? workSize : 1);
	spec = mem; /* a DFT's spec is its memory */
	switch (dft*4+dbl*2+cplx){
	case 0: st = ippsFFTInit_R_32f((IppsFFTSpec_R_32f **)&spec, order, flag, ippAlgHintFast, mem, init); break;
	case 1: st = ippsFFTInit_C_32fc((IppsFFTSpec_C_32fc **)&spec, order, flag, ippAlgHintFast, mem, init); break;
	case 2: st = ippsFFTInit_R_64f((IppsFFTSpec_R_64f **)&spec, order, flag, ippAlgHintFast, mem, init); break;
	case 3: st = ippsFFTInit_C_64fc((IppsFFTSpec_C_64fc **)&spec, order, flag, ippAlgHintFast, mem, init); break;
	case 4: st = ippsDFTInit_R_32f(n, flag, ippAlgHintFast, spec, init); break;
	case 5: st = ippsDFTInit_C_32fc(n, flag, ippAlgHintFast, spec, init); break;
	case 6: st = ippsDFTInit_R_64f(n, flag, ippAlgHintFast, spec, init); break;
	default: st = ippsDFTInit_C_64fc(n, flag, ippAlgHintFast, spec, init); break;
	}
	BENCH_ASSERT(st==ippStsNoErr);
	ippsFree(init);
}

void doit(int iter, bench_problem *p){
	void *in = p->in, *out = p->out;
	for (int i = 0; i<iter; i++){
		switch (how){
		case R2C: ippsFFTFwd_RToCCS_32f(in, out, spec, work); break;
		case C2R: ippsFFTInv_CCSToR_32f(in, out, spec, work); break;
		case FORWARD: ippsFFTFwd_CToC_32fc(in, out, spec, work); break;
		case BACKWARD: ippsFFTInv_CToC_32fc(in, out, spec, work); break;
		case 4+R2C: ippsFFTFwd_RToCCS_64f(in, out, spec, work); break;
		case 4+C2R: ippsFFTInv_CCSToR_64f(in, out, spec, work); break;
		case 4+FORWARD: ippsFFTFwd_CToC_64fc(in, out, spec, work); break;
		case 4+BACKWARD: ippsFFTInv_CToC_64fc(in, out, spec, work); break;
		case 8+R2C: ippsDFTFwd_RToCCS_32f(in, out, spec, work); break;
		case 8+C2R: ippsDFTInv_CCSToR_32f(in, out, spec, work); break;
		case 8+FORWARD: ippsDFTFwd_CToC_32fc(in, out, spec, work); break;
		case 8+BACKWARD: ippsDFTInv_CToC_32fc(in, out, spec, work); break;
		case 12+R2C: ippsDFTFwd_RToCCS_64f(in, out, spec, work); break;
		case 12+C2R: ippsDFTInv_CCSToR_64f(in, out, spec, work); break;
		case 12+FORWARD: ippsDFTFwd_CToC_64fc(in, out, spec, work); break;
		default: ippsDFTInv_CToC_64fc(in, out, spec, work); break;
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	ippsFree(mem);
	ippsFree(work);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	ippInit();
}

void cleanup(void){
}
