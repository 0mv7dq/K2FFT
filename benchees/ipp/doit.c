// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

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

static IppsFFTSpec_R_32f *fft;
static IppsDFTSpec_R_32f *dft;
static Ipp8u *spec, *work;

int can_do(bench_problem *p){
	return rof_size(p)>0;
}

void setup(bench_problem *p){
	int n = rof_size(p), order = 0, specSize = 0, initSize = 0, workSize = 0;
	while ((1<<order)<n){
		order++;
	}
	fft = NULL;
	dft = NULL;
	if (power_of_two(n)){
		BENCH_ASSERT(ippsFFTGetSize_R_32f(order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSize, &initSize, &workSize)==ippStsNoErr);
	} else {
		BENCH_ASSERT(ippsDFTGetSize_R_32f(n, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSize, &initSize, &workSize)==ippStsNoErr);
	}
	Ipp8u *init = initSize>0 ? ippsMalloc_8u(initSize) : NULL;
	spec = ippsMalloc_8u(specSize);
	work = ippsMalloc_8u(workSize>0 ? workSize : 1);
	if (power_of_two(n)){
		BENCH_ASSERT(ippsFFTInit_R_32f(&fft, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, spec, init)==ippStsNoErr);
	} else {
		dft = (IppsDFTSpec_R_32f *)spec;
		BENCH_ASSERT(ippsDFTInit_R_32f(n, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, dft, init)==ippStsNoErr);
	}
	ippsFree(init);
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		if (fft){
			ippsFFTFwd_RToCCS_32f(p->in, p->out, fft, work);
		} else {
			ippsDFTFwd_RToCCS_32f(p->in, p->out, dft, work);
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	ippsFree(spec);
	ippsFree(work);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	ippInit();
}

void cleanup(void){
}
