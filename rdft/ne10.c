// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Ne10's FFT at powers of two, as benchees/ne10: one 1D transform of floats out of place, complex either way (its backward unscaled,
   as FFTW's), real forward or back; NEON where the CPU has it */
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>
#include <stdlib.h>

typedef struct plan {
	ne10_fft_cfg_float32_t c2c;
	ne10_fft_r2c_cfg_float32_t r2c;
	problem_kind kind;
	int backward;
	int neon;
} plan;

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
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

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->kind;
	p->backward = pb->sign>0;
	p->neon = simd_neon();
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
	return p;
}

/* FFTW's layouts: (re, im) pairs, a real one's size/2+1 */
static void execute(const void *p_, void *in, void *out){
	const plan *p = p_;
	switch (p->kind){
	case KIND_DFT:
		if (p->neon){
			ne10_fft_c2c_1d_float32_neon(out, in, p->c2c, p->backward);
		} else {
			ne10_fft_c2c_1d_float32_c(out, in, p->c2c, p->backward);
		}
		break;
	case KIND_R2C:
		if (p->neon){
			ne10_fft_r2c_1d_float32_neon(out, in, p->r2c);
		} else {
			ne10_fft_r2c_1d_float32_c(out, in, p->r2c);
		}
		break;
	default:
		if (p->neon){
			ne10_fft_c2r_1d_float32_neon(out, in, p->r2c);
		} else {
			ne10_fft_c2r_1d_float32_c(out, in, p->r2c);
		}
		break;
	}
}

const kernel k_ne10 = {"ne10", can_do, make, execute, destroy};
