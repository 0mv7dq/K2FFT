// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include <dsps_fft2r.h>
#include <dsps_fft4r.h>
#include <pffft/pffft.h>
#include <sdkconfig.h>
#include <stdlib.h>
#include <string.h>

#define LARGEST_SIZE (1<<26) /* pffft_new_setup refuses larger sizes */

struct fft {
	int size;
	int radix; /* 2 or 4, or 0 for PFFFT */
	PFFFT_Setup *setup;
	float *in;
	float *out;
	float *work; /* so PFFFT doesn't use the stack */
};

/* ESP-DSP's tables are never remade, as another fft may be using them */
static int tables_cover(int half){
	if (half>CONFIG_DSP_MAX_FFT_SIZE){
		return 0;
	}
	dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE); /* both do nothing once the tables exist, even if smaller */
	dsps_fft4r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
	return dsps_fft2r_initialized && dsps_fft4r_initialized && dsps_fft_w_table_size>=half && dsps_fft4r_w_table_size>=2*half;
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
	int half = size/2;
	int planned = 1;
	if ((size&(size-1))==0 && tables_cover(half)){
		f->radix = (half&0x55555555)!=0 ? 4 : 2; /* a power of 4 has its one bit at an even position */
	} else {
		f->setup = pffft_new_setup(size, PFFFT_REAL);
		f->work = pffft_aligned_malloc((size_t)size*sizeof(float));
		planned = f->setup && f->work;
	}
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!planned || !f->in || !f->out){
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
	if (!f->radix){
		pffft_transform_ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
		return;
	}
	int half = f->size/2;
	memcpy(f->out, f->in, (size_t)f->size*sizeof(float)); /* ESP-DSP works in place */
	if (f->radix==4){
		dsps_fft4r_fc32(f->out, half);
		dsps_bit_rev4r_fc32(f->out, half);
	} else {
		dsps_fft2r_fc32(f->out, half);
		dsps_bit_rev2r_fc32(f->out, half);
	}
	dsps_cplx2real_fc32(f->out, half);
}

float fft_power(const fft *f, float *power, int from){
	return power_packed(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	int half = f->size/2;
	if (k==0 || k==half){
		*re = f->out[k==0 ? 0 : 1]; /* both libraries pack the two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
