// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include <pffft/pffft.h>
#include <stdlib.h>
#include <string.h>

#define LARGEST_SIZE (1<<26) /* pffft_new_setup refuses larger sizes */

struct fft {
	int size;
	PFFFT_Setup *setup;
	float *in;
	float *out;
	float *work; /* so PFFFT doesn't use the stack */
};

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
	f->setup = pffft_new_setup(size, PFFFT_REAL);
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->work = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!f->setup || !f->in || !f->out || !f->work){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float));
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	pffft_destroy_setup(f->setup);
	pffft_aligned_free(f->in);
	pffft_aligned_free(f->out);
	pffft_aligned_free(f->work);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	pffft_transform_ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
}

float fft_power(const fft *f, float *power, int from){
	return power_packed(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	int half = f->size/2;
	if (k==0 || k==half){
		*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
