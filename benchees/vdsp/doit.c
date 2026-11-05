// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <Accelerate/Accelerate.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Apple")
BENCH_DOC("url", "https://developer.apple.com/documentation/accelerate/vdsp")
BENCH_DOC("copyright", "proprietary, part of macOS")
END_BENCH_DOC

static vDSP_DFT_Setup plan;
static float *buffers; /* even and odd input samples, then output re and im, n/2 each */

int can_do(bench_problem *p){
	static const int f[] = {1, 3, 5, 15};
	int n = rof_size(p);
	for (int i = 0; i<4; i++){
		if (n>0 && n%f[i]==0 && n/f[i]>=16 && power_of_two(n/f[i])){
			return 1;
		}
	}
	return 0;
}

void setup(bench_problem *p){
	int n = rof_size(p);
	plan = vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)n, vDSP_DFT_FORWARD);
	buffers = bench_malloc((size_t)n*2*sizeof(float));
	BENCH_ASSERT(plan && buffers);
}

void doit(int iter, bench_problem *p){
	int half = rof_size(p)/2;
	DSPSplitComplex in = {buffers, buffers+half};
	for (int i = 0; i<iter; i++){
		vDSP_ctoz(p->in, 2, &in, 1, (vDSP_Length)half);
		vDSP_DFT_Execute(plan, in.realp, in.imagp, buffers+2*half, buffers+3*half);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	vDSP_DFT_DestroySetup(plan);
	bench_free(buffers);
}

/* vDSP returns twice the DFT, with the Nyquist bin in im[0] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int half = rof_size(p)/2;
	const float *re = buffers+2*half, *im = buffers+3*half;
	for (int k = 1; k<half; k++){
		ro[2*k] = re[k]/2;
		io[2*k] = im[k]/2;
	}
	ro[0] = re[0]/2;
	io[0] = 0;
	ro[2*half] = im[0]/2;
	io[2*half] = 0;
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
