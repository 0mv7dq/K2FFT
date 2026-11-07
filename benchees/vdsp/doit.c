// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* vDSP's DFT, as rdft/vdsp.c: one 1D transform out of place at f 2^k (f 1, 3, 5 or 15), complex either way (zop), real forward or
   back (zrop); the input to its split arrays timed, the output back from them not */
#include "benchee.h"
#include <Accelerate/Accelerate.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Apple")
BENCH_DOC("url", "https://developer.apple.com/documentation/accelerate/vdsp")
BENCH_DOC("copyright", "proprietary, part of macOS")
END_BENCH_DOC

static vDSP_DFT_Setup plan;
static float *buffers; /* the input's re and im, then the output's: half each */
static float scale; /* a real backward's, to FFTW's */

static int halfof(const bench_problem *p){
	return p->kind==PROBLEM_COMPLEX ? bench_1d(p) : bench_1d(p)/2;
}

int can_do(bench_problem *p){
	static const int f[] = {1, 3, 5, 15};
	int n = bench_1d(p);
	if (n==0 || p->kind==PROBLEM_R2R || p->in_place){
		return 0;
	}
	for (int i = 0; i<4; i++){
		if (n%f[i]==0 && n/f[i]>=(p->kind==PROBLEM_COMPLEX ? 8 : 16) && power_of_two(n/f[i])){
			return 1;
		}
	}
	return 0;
}

static void run(const float *in, int n, int half, int packed){
	DSPSplitComplex x = {buffers, buffers+half};
	vDSP_ctoz((const DSPComplex *)in, 2, &x, 1, (vDSP_Length)half);
	if (packed){ /* Nyquist's re in DC's im */
		x.imagp[0] = in[n];
	}
	vDSP_DFT_Execute(plan, x.realp, x.imagp, buffers+2*half, buffers+3*half);
}

void setup(bench_problem *p){
	int n = bench_1d(p), half = halfof(p);
	vDSP_DFT_Direction way = p->sign<0 ? vDSP_DFT_FORWARD : vDSP_DFT_INVERSE;
	plan = p->kind==PROBLEM_COMPLEX ? vDSP_DFT_zop_CreateSetup(NULL, (vDSP_Length)n, way) : vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)n, way);
	buffers = bench_malloc((size_t)half*4*sizeof(float));
	BENCH_ASSERT(plan && buffers);
	scale = 0.5f; /* zrop's forward: twice the DFT */
	if (p->kind==PROBLEM_REAL && p->sign>0){ /* measured: DC alone, whose inverse is 1 everywhere */
		float *dc = bench_malloc((size_t)(n+2)*sizeof(float));
		memset(dc, 0, (size_t)(n+2)*sizeof(float));
		dc[0] = 1;
		run(dc, n, half, 1);
		bench_free(dc);
		BENCH_ASSERT(buffers[2*half]!=0);
		scale = 1/buffers[2*half];
	}
}

void doit(int iter, bench_problem *p){
	int n = bench_1d(p), half = halfof(p), packed = p->kind==PROBLEM_REAL && p->sign>0;
	for (int i = 0; i<iter; i++){
		run(p->in, n, half, packed);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	vDSP_DFT_DestroySetup(plan);
	bench_free(buffers);
}

/* a complex one's split output interleaved */
void after_problem_ccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = bench_1d(p);
	const float *re = buffers+2*n, *im = buffers+3*n;
	for (int k = 0; k<n; k++){
		ro[2*k] = re[k];
		io[2*k] = im[k];
	}
}

/* a real forward's: twice the DFT, the Nyquist bin in im[0] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int half = halfof(p);
	const float *re = buffers+2*half, *im = buffers+3*half;
	for (int k = 1; k<half; k++){
		ro[2*k] = re[k]*scale;
		io[2*k] = im[k]*scale;
	}
	ro[0] = re[0]*scale;
	io[0] = 0;
	ro[2*half] = im[0]*scale;
	io[2*half] = 0;
}

/* a real backward's even and odd samples, interleaved and scaled */
void after_problem_rcopy_to(bench_problem *p, bench_real *ro){
	int half = halfof(p);
	const float *even = buffers+2*half, *odd = buffers+3*half;
	for (int k = 0; k<half; k++){
		ro[2*k] = even[k]*scale;
		ro[2*k+1] = odd[k]*scale;
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
