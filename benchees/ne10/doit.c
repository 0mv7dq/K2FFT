// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Ne10 at powers of two, as rdft/ne10.c: one 1D transform out of place, complex either way or real; a backward one's scale to FFTW's,
   measured, as its NEON code scales some sizes by 1/n whatever is_backward_scaled says */
#include "benchee.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "ARM")
BENCH_DOC("url", "https://github.com/projectNe10/Ne10")
BENCH_DOC("copyright", "BSD-3-Clause")
END_BENCH_DOC

static ne10_fft_cfg_float32_t c2c;
static ne10_fft_r2c_cfg_float32_t r2c;
static int neon;
static float scale;

int can_do(bench_problem *p){
	int n = bench_1d(p);
#if !defined(__aarch64__)
	if (p->kind==PROBLEM_REAL && p->sign>0 && neon){
		return 0; /* its 32-bit NEON c2r leaves its c2c's is_backward_scaled unset, a stack struct's */
	}
#endif
	return n>0 && power_of_two(n) && p->kind!=PROBLEM_R2R && !p->in_place;
}

static void transform(bench_problem *p, void *in, void *out){
	const int backward = p->sign>0;
	if (c2c){
		if (neon){
			ne10_fft_c2c_1d_float32_neon(out, in, c2c, backward);
		} else {
			ne10_fft_c2c_1d_float32_c(out, in, c2c, backward);
		}
	} else if (!backward){
		if (neon){
			ne10_fft_r2c_1d_float32_neon(out, in, r2c);
		} else {
			ne10_fft_r2c_1d_float32_c(out, in, r2c);
		}
	} else {
		float *x = in, dc = x[1]; /* its AArch64 c2r zeroes the DC's im, which FFTW's ignores */
		if (neon){
			ne10_fft_c2r_1d_float32_neon(out, in, r2c);
		} else {
			ne10_fft_c2r_1d_float32_c(out, in, r2c);
		}
		x[1] = dc;
	}
	if (scale!=1){
		float *o = out;
		const int m = bench_1d(p)*(c2c ? 2 : 1);
		for (int i = 0; i<m; i++){
			o[i] *= scale;
		}
	}
}

void setup(bench_problem *p){
	int n = bench_1d(p);
	c2c = NULL;
	r2c = NULL;
	scale = 1;
	if (p->kind==PROBLEM_COMPLEX){
		c2c = neon ? ne10_fft_alloc_c2c_float32_neon(n) : ne10_fft_alloc_c2c_float32_c(n);
		BENCH_ASSERT(c2c);
		c2c->is_backward_scaled = 0;
	} else {
		r2c = ne10_fft_alloc_r2c_float32(n);
		BENCH_ASSERT(r2c);
	}
	if (p->sign>0){ /* DC alone, whose inverse is 1 everywhere */
		float *in = p->in, *out = p->out;
		memset(in, 0, (size_t)(c2c ? 2*n : n+2)*sizeof(float));
		in[0] = 1;
		transform(p, in, out);
		BENCH_ASSERT(out[0]!=0);
		scale = 1/out[0];
		BENCH_ASSERT(scale==1 || scale==(float)n);
	}
}

void doit(int iter, bench_problem *p){
	for (int i = 0; i<iter; i++){
		transform(p, p->in, p->out);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	if (c2c){
		ne10_fft_destroy_c2c_float32(c2c);
	}
	if (r2c){
		ne10_fft_destroy_r2c_float32(r2c);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
	neon = simd_neon();
}

void cleanup(void){
}
