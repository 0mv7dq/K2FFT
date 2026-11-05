// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include <Accelerate/Accelerate.h>
#include <pffft/pffft.h>
#include <stdlib.h>
#include <string.h>

#define LARGEST_SIZE (1<<26) /* pffft_new_setup refuses larger sizes */

struct fft {
	int size;
	int vdsp;
	vDSP_DFT_Setup dft;
	DSPSplitComplex split;
	PFFFT_Setup *setup;
	float *in;
	float *out;
	float *work; /* so PFFFT doesn't use the stack */
};

static void use_vdsp(fft *f, int path){
	f->vdsp = path;
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
	f->setup = pffft_new_setup(size, PFFFT_REAL);
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->work = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!f->setup || !f->in || !f->out || !f->work){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float));
	f->dft = vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)size, vDSP_DFT_FORWARD);
	if (f->dft){
		f->split.realp = pffft_aligned_malloc((size_t)size/2*sizeof(float));
		f->split.imagp = pffft_aligned_malloc((size_t)size/2*sizeof(float));
	}
	if (f->dft && f->split.realp && f->split.imagp && race_paths(f, size, use_vdsp)){
		f->vdsp = 1;
		pffft_destroy_setup(f->setup);
		f->setup = NULL;
		pffft_aligned_free(f->out);
		f->out = NULL;
		pffft_aligned_free(f->work);
		f->work = NULL;
	} else {
		f->vdsp = 0;
		if (f->dft){
			vDSP_DFT_DestroySetup(f->dft);
			f->dft = NULL;
		}
		pffft_aligned_free(f->split.realp);
		pffft_aligned_free(f->split.imagp);
		f->split.realp = NULL;
		f->split.imagp = NULL;
	}
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	if (f->dft){
		vDSP_DFT_DestroySetup(f->dft);
	}
	pffft_destroy_setup(f->setup);
	pffft_aligned_free(f->split.realp);
	pffft_aligned_free(f->split.imagp);
	pffft_aligned_free(f->in);
	pffft_aligned_free(f->out);
	pffft_aligned_free(f->work);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	if (f->vdsp){
		vDSP_ctoz((const DSPComplex *)f->in, 2, &f->split, 1, (vDSP_Length)f->size/2);
		vDSP_DFT_Execute(f->dft, f->split.realp, f->split.imagp, f->split.realp, f->split.imagp);
	} else {
		pffft_transform_ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
	}
}

float fft_power(const fft *f, float *power, int from){
	if (!f->vdsp){
		return power_packed(f->out, f->size, power, from);
	}
	int half = f->size/2;
	const DSPSplitComplex bins = {f->split.realp+1, f->split.imagp+1}; /* element 0 packs DC and Nyquist */
	const float quarter = 0.25f; /* undoes vDSP's doubling, squared */
	vDSP_zvmags(&bins, 1, power+1, 1, (vDSP_Length)half-1);
	power[0] = f->split.realp[0]*f->split.realp[0];
	power[half] = f->split.imagp[0]*f->split.imagp[0];
	vDSP_vsmul(power, 1, &quarter, power, 1, (vDSP_Length)half+1);
	int k = from<0 ? 0 : from;
	float sum = 0;
	if (k<=half){
		vDSP_sve(power+k, 1, &sum, (vDSP_Length)(half+1-k));
	}
	return sum;
}

void fft_bin(const fft *f, int k, float *re, float *im){
	int half = f->size/2;
	if (!f->vdsp){
		if (k==0 || k==half){
			*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
			*im = 0;
		} else {
			*re = f->out[2*k];
			*im = f->out[2*k+1];
		}
	} else if (k==0 || k==half){
		*re = 0.5f*(k==0 ? f->split.realp[0] : f->split.imagp[0]); /* vDSP packs Nyquist into imagp[0] */
		*im = 0;
	} else {
		*re = 0.5f*f->split.realp[k];
		*im = 0.5f*f->split.imagp[k];
	}
}
