// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Ne10's FFT at powers of two, as benchees/ne10: one 1D transform of floats out of place, complex either way or real, NEON where the
   CPU has it; a backward one's scale to FFTW's, measured, as its NEON code scales some sizes by 1/n whatever is_backward_scaled says */
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	ne10_fft_cfg_float32_t c2c;
	ne10_fft_r2c_cfg_float32_t r2c;
	problem_kind kind;
	int backward;
	int neon;
	int size;
	float scale; /* 1, or n where it scaled by 1/n */
} plan;

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
#if !defined(__aarch64__)
	if (pb->kind==KIND_C2R && simd_neon()){
		return 0; /* its 32-bit NEON c2r leaves its c2c's is_backward_scaled unset, a stack struct's */
	}
#endif
	return pb->single && pb->kind!=KIND_R2R && pb->in!=pb->out && size>0 && (size&(size-1))==0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->c2c){
		ne10_fft_destroy_c2c_float32(p->c2c);
	}
	if (p->r2c){
		ne10_fft_destroy_r2c_float32(p->r2c);
	}
	free(p);
}

/* FFTW's layouts: (re, im) pairs, a real one's size/2+1 */
static void execute(const void *p_, void *in, void *out){
	const plan *p = p_;
	float *o = out;
	int n = p->size;
	switch (p->kind){
	case KIND_DFT:
		if (p->neon){
			ne10_fft_c2c_1d_float32_neon(out, in, p->c2c, p->backward);
		} else {
			ne10_fft_c2c_1d_float32_c(out, in, p->c2c, p->backward);
		}
		n *= 2;
		break;
	case KIND_R2C:
		if (p->neon){
			ne10_fft_r2c_1d_float32_neon(out, in, p->r2c);
		} else {
			ne10_fft_r2c_1d_float32_c(out, in, p->r2c);
		}
		break;
	default: {
		float *x = in, dc = x[1]; /* its AArch64 c2r zeroes the DC's im, which FFTW's ignores */
		if (p->neon){
			ne10_fft_c2r_1d_float32_neon(out, in, p->r2c);
		} else {
			ne10_fft_c2r_1d_float32_c(out, in, p->r2c);
		}
		x[1] = dc;
		break;
	}
	}
	if (p->scale!=1){
		for (int i=0; i<n; i++){
			o[i] *= p->scale;
		}
	}
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->kind;
	p->backward = pb->kind==KIND_C2R || (pb->kind==KIND_DFT && pb->sign>0);
	p->neon = simd_neon();
	p->size = size;
	p->scale = 1;
	if (pb->kind==KIND_DFT){
		p->c2c = p->neon ? ne10_fft_alloc_c2c_float32_neon(size) : ne10_fft_alloc_c2c_float32_c(size);
		if (p->c2c){
			p->c2c->is_backward_scaled = 0;
		}
	} else {
		p->r2c = ne10_fft_alloc_r2c_float32(size);
	}
	if (!p->c2c && !p->r2c){
		destroy(p);
		return NULL;
	}
	if (p->backward){ /* DC alone, whose inverse is 1 everywhere */
		float *in = pb->in, *out = pb->out;
		memset(in, 0, (size_t)(pb->kind==KIND_DFT ? 2*size : size+2)*sizeof(float));
		in[0] = 1;
		execute(p, in, out);
		p->scale = out[0]!=0 ? 1/out[0] : 0;
		if (p->scale!=1 && p->scale!=(float)size){
			destroy(p); /* neither FFTW's nor 1/n */
			return NULL;
		}
	}
	return p;
}

const kernel k_ne10 = {"ne10", can_do, make, execute, destroy};
