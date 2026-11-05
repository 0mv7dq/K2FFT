// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include "simd-support/simd.h"
#include <Accelerate/Accelerate.h>
#include <kfr/dft.hpp>
#include <pffft/pffft.h>
#include <cstdlib>
#include <cstring>

#define LARGEST_SIZE (1<<26) /* pffft_new_setup refuses larger sizes */

#ifdef PFFFT_AVX2
/* PFFFT built for AVX2 + FMA by pffft_avx2_copy() in cmake/simd.cmake */
extern "C" {
PFFFT_Setup *pffft_avx2_new_setup(int N, pffft_transform_t transform);
void pffft_avx2_destroy_setup(PFFFT_Setup *setup);
void pffft_avx2_transform_ordered(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
}
#endif

struct fft {
	int size;
	int vdsp;
	kfr::dft_plan_real<float> *plan;
	kfr::u8 *temp; /* so a transform never allocates */
	PFFFT_Setup *setup;
	void (*ordered)(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);
	void (*destroy)(PFFFT_Setup *setup);
	vDSP_DFT_Setup dft;
	DSPSplitComplex split;
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
	fft *f = static_cast<fft *>(calloc(1, sizeof(*f)));
	if (!f){
		return NULL;
	}
	f->size = size;
	bool planned;
	if ((size&(size-1))==0){
		try {
			f->plan = new kfr::dft_plan_real<float>(static_cast<size_t>(size));
			f->temp = static_cast<kfr::u8 *>(pffft_aligned_malloc(f->plan->temp_size+1));
		} catch (...){
			f->plan = nullptr; /* an exception must not reach C */
		}
		planned = f->plan && f->temp;
	} else {
#ifdef PFFFT_AVX2
		if (simd_avx2_fma()){
			f->setup = pffft_avx2_new_setup(size, PFFFT_REAL);
			f->ordered = pffft_avx2_transform_ordered;
			f->destroy = pffft_avx2_destroy_setup;
		}
#endif
		if (!f->ordered){
			f->setup = pffft_new_setup(size, PFFFT_REAL);
			f->ordered = pffft_transform_ordered;
			f->destroy = pffft_destroy_setup;
		}
		f->work =static_cast<float *>(pffft_aligned_malloc(static_cast<size_t>(size)*sizeof(float)));
		planned = f->setup && f->work;
	}
	f->in = static_cast<float *>(pffft_aligned_malloc(static_cast<size_t>(size)*sizeof(float)));
	f->out = static_cast<float *>(pffft_aligned_malloc((static_cast<size_t>(size)+2)*sizeof(float)));
	if (!planned || !f->in || !f->out){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, static_cast<size_t>(size)*sizeof(float));
	f->dft = vDSP_DFT_zrop_CreateSetup(nullptr, static_cast<vDSP_Length>(size), vDSP_DFT_FORWARD);
	if (f->dft){
		f->split.realp = static_cast<float *>(pffft_aligned_malloc(static_cast<size_t>(size)/2*sizeof(float)));
		f->split.imagp = static_cast<float *>(pffft_aligned_malloc(static_cast<size_t>(size)/2*sizeof(float)));
	}
	if (f->dft && f->split.realp && f->split.imagp && race_paths(f, size, use_vdsp)){
		f->vdsp = 1;
		delete f->plan;
		f->plan = nullptr;
		pffft_aligned_free(f->temp);
		f->temp = nullptr;
		if (f->setup){
			f->destroy(f->setup);
			f->setup = nullptr;
		}
		pffft_aligned_free(f->work);
		f->work = nullptr;
		pffft_aligned_free(f->out);
		f->out = nullptr;
	} else {
		f->vdsp = 0;
		if (f->dft){
			vDSP_DFT_DestroySetup(f->dft);
			f->dft = nullptr;
		}
		pffft_aligned_free(f->split.realp);
		pffft_aligned_free(f->split.imagp);
		f->split.realp = nullptr;
		f->split.imagp = nullptr;
	}
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	delete f->plan;
	pffft_aligned_free(f->temp);
	if (f->setup){
		f->destroy(f->setup);
	}
	if (f->dft){
		vDSP_DFT_DestroySetup(f->dft);
	}
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
		vDSP_ctoz(reinterpret_cast<const DSPComplex *>(f->in), 2, &f->split, 1, static_cast<vDSP_Length>(f->size)/2);
		vDSP_DFT_Execute(f->dft, f->split.realp, f->split.imagp, f->split.realp, f->split.imagp);
	} else if (f->plan){
		f->plan->execute(reinterpret_cast<kfr::complex<float> *>(f->out), f->in, f->temp);
	} else {
		f->ordered(f->setup, f->in, f->out, f->work, PFFFT_FORWARD);
	}
}

float fft_power(const fft *f, float *power, int from){
	if (f->plan && !f->vdsp){
		return power_interleaved(f->out, f->size, power, from);
	}
	if (!f->vdsp){
		return power_packed(f->out, f->size, power, from);
	}
	int half = f->size/2;
	const DSPSplitComplex bins = {f->split.realp+1, f->split.imagp+1}; /* element 0 packs DC and Nyquist */
	const float quarter = 0.25f; /* undoes vDSP's doubling, squared */
	vDSP_zvmags(&bins, 1, power+1, 1, static_cast<vDSP_Length>(half)-1);
	power[0] = f->split.realp[0]*f->split.realp[0];
	power[half] = f->split.imagp[0]*f->split.imagp[0];
	vDSP_vsmul(power, 1, &quarter, power, 1, static_cast<vDSP_Length>(half)+1);
	int k = from<0 ? 0 : from;
	float sum = 0;
	if (k<=half){
		vDSP_sve(power+k, 1, &sum, static_cast<vDSP_Length>(half+1-k));
	}
	return sum;
}

void fft_bin(const fft *f, int k, float *re, float *im){
	int half = f->size/2;
	if (f->vdsp){
		if (k==0 || k==half){
			*re = 0.5f*(k==0 ? f->split.realp[0] : f->split.imagp[0]); /* vDSP packs Nyquist into imagp[0] */
			*im = 0;
		} else {
			*re = 0.5f*f->split.realp[k];
			*im = 0.5f*f->split.imagp[k];
		}
	} else if (!f->plan && (k==0 || k==half)){
		*re = f->out[k==0 ? 0 : 1]; /* PFFFT packs its two real bins first */
		*im = 0;
	} else {
		*re = f->out[2*k];
		*im = f->out[2*k+1];
	}
}
