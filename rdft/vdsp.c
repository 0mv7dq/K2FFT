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
	float *bins; /* its re, then its im */
} plan;

static int can_do(const problem *pb){
	const int size = problem_rof(pb);
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
	pffft_aligned_free(p->bins);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->dft = vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)size, vDSP_DFT_FORWARD);
	p->split = pffft_aligned_malloc((size_t)size*sizeof(float));
	p->bins = pffft_aligned_malloc((size_t)size*sizeof(float));
	if (!p->dft || !p->split || !p->bins){
		destroy(p);
		return NULL;
	}
	return p;
}

/* vDSP's bins, re then im, twice the DFT, Nyquist's re in DC's im, to FFTW's layout */
static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	int half = p->size/2;
	DSPSplitComplex even = {p->split, p->split+half};
	const DSPSplitComplex bins = {p->bins, p->bins+half};
	const float h = 0.5f;
	vDSP_ctoz((const DSPComplex *)in, 2, &even, 1, (vDSP_Length)half);
	vDSP_DFT_Execute(p->dft, even.realp, even.imagp, bins.realp, bins.imagp);
	vDSP_ztoc(&bins, 1, (DSPComplex *)out, 2, (vDSP_Length)half);
	vDSP_vsmul(out, 1, &h, out, 1, (vDSP_Length)p->size);
	out[p->size] = out[1];
	out[p->size+1] = 0;
	out[1] = 0;
}

const kernel k_vdsp = {"vdsp", can_do, make, execute, destroy};
