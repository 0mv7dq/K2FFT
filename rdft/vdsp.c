// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* vDSP's real DFT, as benchees/vdsp: f 2^k points, f 1, 3, 5 or 15, 2^k >= 16 */
#include "kernel/kernel.h"
#include <Accelerate/Accelerate.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>

typedef struct plan {
	vDSP_DFT_Setup dft;
	int size;
	float *split; /* even then odd input samples, size/2 each */
} plan;

static int can_do(int size){
	static const int f[] = {1, 3, 5, 15};
	for (int i=0; i<4; i++){
		int m = size/f[i];
		if (size%f[i]==0 && m>=16 && (m&(m-1))==0){
			return 1;
		}
	}
	return 0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->dft){
		vDSP_DFT_DestroySetup(p->dft);
	}
	pffft_aligned_free(p->split);
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->dft = vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)size, vDSP_DFT_FORWARD);
	p->split = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!p->dft || !p->split){
		destroy(p);
		return NULL;
	}
	return p;
}

/* out: re of bins 0..size/2-1 then their im, twice the DFT, Nyquist's re in im[0] */
static void forward(void *p_, float *in, float *out){
	const plan *p = p_;
	int half = p->size/2;
	DSPSplitComplex even = {p->split, p->split+half};
	vDSP_ctoz((const DSPComplex *)in, 2, &even, 1, (vDSP_Length)half);
	vDSP_DFT_Execute(p->dft, even.realp, even.imagp, out, out+half);
}

static float power(const float *out, int size, float *power, int from){
	int half = size/2;
	const DSPSplitComplex bins = {(float *)out+1, (float *)out+half+1}; /* element 0 packs DC and Nyquist */
	const float quarter = 0.25f; /* undoes vDSP's doubling, squared */
	vDSP_zvmags(&bins, 1, power+1, 1, (vDSP_Length)half-1);
	power[0] = out[0]*out[0];
	power[half] = out[half]*out[half];
	vDSP_vsmul(power, 1, &quarter, power, 1, (vDSP_Length)half+1);
	int k = from<0 ? 0 : from;
	float sum = 0;
	if (k<=half){
		vDSP_sve(power+k, 1, &sum, (vDSP_Length)(half+1-k));
	}
	return sum;
}

static void bin(const float *out, int size, int k, float *re, float *im){
	int half = size/2;
	if (k==0 || k==half){
		*re = 0.5f*out[k==0 ? 0 : half];
		*im = 0;
	} else {
		*re = 0.5f*out[k];
		*im = 0.5f*out[half+k];
	}
}

const kernel k_vdsp = {"vdsp", can_do, make, forward, destroy, power, bin};
