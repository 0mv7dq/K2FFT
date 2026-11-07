// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* vDSP's DFT, as rdft/vdsp.c: one 1D transform out of place at f 2^k (f 1, 3, 5 or 15), complex either way (zop), real forward or
   back (zrop), its double functions (name##D) for libbench2's doubles; to and from its split arrays timed, as the kernel does them */
#include "benchee.h"
#include <Accelerate/Accelerate.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Apple")
BENCH_DOC("url", "https://developer.apple.com/documentation/accelerate/vdsp")
BENCH_DOC("copyright", "proprietary, part of macOS")
END_BENCH_DOC

#if defined(BENCHFFT_SINGLE)
#define V(name) name
typedef DSPComplex cplx;
typedef DSPSplitComplex split;
typedef vDSP_DFT_Setup dft;
#else
#define V(name) name##D
typedef DSPDoubleComplex cplx;
typedef DSPDoubleSplitComplex split;
typedef vDSP_DFT_SetupD dft;
#endif

static dft plan;
static bench_real *buffers; /* the input's re and im, then the output's: half each */
static bench_real scale; /* a real one's, to FFTW's */
static int way; /* 0 complex, 1 real forward, -1 real backward */

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

/* in to the split arrays, the DFT, back to FFTW's layout in out, scaled: rdft/vdsp.h's run */
static void run(const bench_real *in, bench_real *out, int n, int half){
	split x = {buffers, buffers+half};
	const split y = {buffers+2*half, buffers+3*half};
	V(vDSP_ctoz)((const cplx *)in, 2, &x, 1, (vDSP_Length)half); /* a real one's even and odd samples */
	if (way<0){ /* vDSP's packed bins: Nyquist's re in DC's im */
		x.imagp[0] = in[n];
	}
	V(vDSP_DFT_Execute)(plan, x.realp, x.imagp, y.realp, y.imagp);
	V(vDSP_ztoc)(&y, 1, (cplx *)out, 2, (vDSP_Length)half);
	if (way==0){
		return;
	}
	V(vDSP_vsmul)(out, 1, &scale, out, 1, (vDSP_Length)n);
	if (way>0){ /* Nyquist last, the im of DC and Nyquist 0 */
		out[n] = out[1];
		out[n+1] = 0;
		out[1] = 0;
	}
}

void setup(bench_problem *p){
	int n = bench_1d(p), half = halfof(p);
	vDSP_DFT_Direction direction = p->sign<0 ? vDSP_DFT_FORWARD : vDSP_DFT_INVERSE;
	way = p->kind==PROBLEM_COMPLEX ? 0 : (p->sign<0 ? 1 : -1);
	plan = way==0 ? V(vDSP_DFT_zop_CreateSetup)(NULL, (vDSP_Length)n, direction)
				  : V(vDSP_DFT_zrop_CreateSetup)(NULL, (vDSP_Length)n, direction);
	buffers = bench_malloc((size_t)half*4*sizeof(bench_real));
	BENCH_ASSERT(plan && buffers);
	scale = 0.5; /* zrop's forward: twice the DFT */
	if (way<0){ /* measured: DC alone, whose inverse is 1 everywhere */
		bench_real *dc = bench_malloc((size_t)(n+2)*sizeof(bench_real)), *y = bench_malloc((size_t)n*sizeof(bench_real));
		memset(dc, 0, (size_t)(n+2)*sizeof(bench_real));
		dc[0] = 1;
		scale = 1;
		run(dc, y, n, half);
		BENCH_ASSERT(y[0]!=0);
		scale = 1/y[0];
		bench_free(dc);
		bench_free(y);
	}
}

void doit(int iter, bench_problem *p){
	int n = bench_1d(p), half = halfof(p);
	for (int i = 0; i<iter; i++){
		run(p->in, p->out, n, half);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	V(vDSP_DFT_DestroySetup)(plan);
	bench_free(buffers);
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
