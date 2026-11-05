// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include "simd-support/simd.h"
#include <fftw3.h>
#include <pffft/pffft.h>
#include <stdlib.h>
#include <string.h>

#define LARGEST_SIZE (1<<26) /* the PFFFT backend's limit */

#define PLAN_FLAGS (FFTW_ESTIMATE|FFTW_PRESERVE_INPUT|FFTW_NO_BUFFERING) /* buffered plans allocate on every transform */

struct fft {
	int size;
	int rvv;
	fftwf_plan plan;
	PFFFT_Setup *setup;
	float *in;
	float *out;
	float *work; /* so PFFFT doesn't use the stack */
};

/* cmake/fftw.cmake sets PFFFT_RVV where the compiler vectorises PFFFT */
static int has_rvv(void){
#ifdef PFFFT_RVV
	return simd_rvv();
#else
	return 0;
#endif
}

static void use_rvv(fft *f, int path){
	f->rvv = path;
}

/* PFFFT's sizes, so they don't change with the backend */
int fft_next_size(int n){
	if (n>LARGEST_SIZE){
		return 0; /* also keeps n+31 from overflowing */
	}
	for (int m=(n>32 ? n+31 : 32)/32*32; m<=LARGEST_SIZE; m+=32){
		int r = m/32;
		while (r%2==0){
			r /= 2;
		}
		while (r%3==0){
			r /= 3;
		}
		while (r%5==0){
			r /= 5;
		}
		if (r==1){
			return m;
		}
	}
	return 0;
}

fft *fft_new(int size){
	if (size<32 || size>LARGEST_SIZE){
		return NULL;
	}
	fft *f = calloc(1, sizeof(*f));
	if (!f){
		return NULL;
	}
	f->size = size;
	f->in = fftwf_alloc_real((size_t)size);
	f->out = fftwf_alloc_real((size_t)size+2);
	if (f->in && f->out){
		f->plan = fftwf_plan_dft_r2c_1d(size, f->in, (fftwf_complex *)f->out, PLAN_FLAGS);
	}
	if (!f->plan){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float));
	if (has_rvv()){
		f->setup = pffft_new_setup(size, PFFFT_REAL);
		f->work = fftwf_alloc_real((size_t)size);
		if (f->setup && f->work && race_paths(f, size, use_rvv)){
			f->rvv = 1;
			fftwf_destroy_plan(f->plan);
			f->plan = NULL;
		} else {
			f->rvv = 0;
			pffft_destroy_setup(f->setup);
			f->setup = NULL;
			fftwf_free(f->work);
			f->work = NULL;
		}
	}
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	if (f->plan){
		fftwf_destroy_plan(f->plan);
	}
	pffft_destroy_setup(f->setup);
	fftwf_free(f->in);
	fftwf_free(f->out);
	fftwf_free(f->work);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	if (f->rvv){
		pffft_transform_ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
	} else {
		fftwf_execute(f->plan);
	}
}

float fft_power(const fft *f, float *power, int from){
	if (f->rvv){
		return power_packed(f->out, f->size, power, from);
	}
	return power_interleaved(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	if (f->rvv && (k==0 || k==f->size/2)){
		*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
