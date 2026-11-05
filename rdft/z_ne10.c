// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include "simd-support/simd.h"
#include <NE10_dsp.h>
#include <pffft/pffft.h>
#include <stdlib.h>
#include <string.h>
#ifndef __ARM_NEON
/* PFFFT built for NEON by pffft_copy() in cmake/ne10.cmake */
PFFFT_Setup *pffft_neon_new_setup(int N, pffft_transform_t transform);
void pffft_neon_destroy_setup(PFFFT_Setup *setup);
void pffft_neon_transform_ordered(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
#endif

#define LARGEST_SIZE (1<<26) /* pffft_new_setup refuses larger sizes */

struct fft {
	int size;
	int ne10;
	ne10_fft_r2c_cfg_float32_t cfg;
	PFFFT_Setup *setup;
	void (*ordered)(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
	void (*destroy)(PFFFT_Setup *setup);
	float *in;
	float *out;
	float *work; /* so PFFFT doesn't use the stack */
};

static void pffft_plan(fft *f, int neon){
#ifdef __ARM_NEON
	(void)neon; /* PFFFT is NEON code already */
#else
	if (neon){
		f->setup = pffft_neon_new_setup(f->size, PFFFT_REAL);
		f->ordered = pffft_neon_transform_ordered;
		f->destroy = pffft_neon_destroy_setup;
		return;
	}
#endif
	f->setup = pffft_new_setup(f->size, PFFFT_REAL);
	f->ordered = pffft_transform_ordered;
	f->destroy = pffft_destroy_setup;
}

static void use_ne10(fft *f, int path){
	f->ne10 = path;
}

int fft_next_size(int n){
	if (n>LARGEST_SIZE){
		return 0;
	}
	return pffft_nearest_transform_size(n, PFFFT_REAL, 1);
}

fft *fft_new(int size){
	if (size>LARGEST_SIZE || !pffft_is_valid_size(size, PFFFT_REAL)){
		return NULL;
	}
	fft *f = calloc(1, sizeof(*f));
	if (!f){
		return NULL;
	}
	f->size = size;
	int neon = simd_neon();
	pffft_plan(f, neon);
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc(((size_t)size+2)*sizeof(float));
	f->work = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!f->setup || !f->in || !f->out || !f->work){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float));
	if (neon && (size&(size-1))==0){
		f->cfg = ne10_fft_alloc_r2c_float32(size);
		if (f->cfg && race_paths(f, size, use_ne10)){
			f->ne10 = 1;
			f->destroy(f->setup);
			f->setup = NULL;
			pffft_aligned_free(f->work);
			f->work = NULL;
		} else {
			f->ne10 = 0;
			ne10_fft_destroy_r2c_float32(f->cfg);
			f->cfg = NULL;
		}
	}
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	ne10_fft_destroy_r2c_float32(f->cfg);
	if (f->setup){
		f->destroy(f->setup);
	}
	pffft_aligned_free(f->in);
	pffft_aligned_free(f->out);
	pffft_aligned_free(f->work);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	if (f->ne10){
		ne10_fft_r2c_1d_float32_neon((ne10_fft_cpx_float32_t *)f->out, f->in, f->cfg);
	} else {
		f->ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
	}
}

float fft_power(const fft *f, float *power, int from){
	if (f->ne10){
		return power_interleaved(f->out, f->size, power, from);
	}
	return power_packed(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	if (!f->ne10 && (k==0 || k==f->size/2)){
		*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
