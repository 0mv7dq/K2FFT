// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Ne10's real FFT at powers of two, as benchees/ne10: its NEON code where the CPU has NEON, else its C */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>
#include <stdlib.h>

typedef struct plan {
	ne10_fft_r2c_cfg_float32_t cfg;
	void (*r2c)(ne10_fft_cpx_float32_t *fout, ne10_float32_t *fin, ne10_fft_r2c_cfg_float32_t cfg);
} plan;

static int can_do(int size){
	return size>0 && (size&(size-1))==0;
}

static void destroy(void *p_){
	plan *p = p_;
	ne10_fft_destroy_r2c_float32(p->cfg);
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->cfg = ne10_fft_alloc_r2c_float32(size);
	p->r2c = simd_neon() ? ne10_fft_r2c_1d_float32_neon : ne10_fft_r2c_1d_float32_c;
	if (!p->cfg){
		destroy(p);
		return NULL;
	}
	return p;
}

/* size/2+1 (re, im) pairs */
static void forward(void *p_, float *in, float *out){
	const plan *p = p_;
	p->r2c((ne10_fft_cpx_float32_t *)out, in, p->cfg);
}

const kernel k_ne10 = {"ne10", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
