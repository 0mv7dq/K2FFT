// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* vDSP's DFT, as benchees/vdsp: one 1D transform of floats out of place at f 2^k (f 1, 3, 5 or 15), complex either way (zop), real
   forward or back (zrop); its split arrays to and from FFTW's (re, im) pairs, its real ones' scales to FFTW's */
#include "kernel/kernel.h"
#include <Accelerate/Accelerate.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	vDSP_DFT_Setup dft;
	problem_kind kind;
	int size;
	float scale; /* a real one's, to FFTW's */
	float *split; /* the input's re, then its im: size/2 each for a real one, size for a complex one */
	float *bins; /* the output's */
} plan;

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	if (!pb->single || pb->kind==KIND_R2R || pb->in==pb->out || size<=0){
		return 0;
	}
	static const int f[] = {1, 3, 5, 15};
	for (int i=0; i<4; i++){
		int m = size/f[i];
		if (size%f[i]==0 && m>=(pb->kind==KIND_DFT ? 8 : 16) && (m&(m-1))==0){
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

static void run(const plan *p, const float *in, float *out){
	const int n = p->size, half = p->kind==KIND_DFT ? n : n/2;
	DSPSplitComplex x = {p->split, p->split+half};
	const DSPSplitComplex y = {p->bins, p->bins+half};
	if (p->kind==KIND_C2R){ /* vDSP's packed bins: Nyquist's re in DC's im */
		vDSP_ctoz((const DSPComplex *)in, 2, &x, 1, (vDSP_Length)half);
		x.imagp[0] = in[n];
	} else {
		vDSP_ctoz((const DSPComplex *)in, 2, &x, 1, (vDSP_Length)half); /* a real one's even and odd samples */
	}
	vDSP_DFT_Execute(p->dft, x.realp, x.imagp, y.realp, y.imagp);
	vDSP_ztoc(&y, 1, (DSPComplex *)out, 2, (vDSP_Length)half);
	if (p->kind==KIND_DFT){
		return;
	}
	vDSP_vsmul(out, 1, &p->scale, out, 1, (vDSP_Length)n);
	if (p->kind==KIND_R2C){
		out[n] = out[1];
		out[n+1] = 0;
		out[1] = 0;
	}
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n, complex = pb->kind==KIND_DFT;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->kind = pb->kind;
	p->scale = 0.5f; /* zrop's forward: twice the DFT */
	const vDSP_DFT_Direction way = pb->kind==KIND_C2R || (complex && pb->sign>0) ? vDSP_DFT_INVERSE : vDSP_DFT_FORWARD;
	p->dft = complex ? vDSP_DFT_zop_CreateSetup(NULL, (vDSP_Length)size, way) : vDSP_DFT_zrop_CreateSetup(NULL, (vDSP_Length)size, way);
	p->split = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*sizeof(float));
	p->bins = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*sizeof(float));
	if (!p->dft || !p->split || !p->bins){
		destroy(p);
		return NULL;
	}
	if (pb->kind==KIND_C2R){ /* its scale, measured: DC alone, whose inverse is 1 everywhere */
		float *in = pb->in, *out = pb->out;
		memset(in, 0, (size_t)(size+2)*sizeof(float));
		in[0] = 1;
		p->scale = 1;
		run(p, in, out);
		p->scale = out[0]!=0 ? 1/out[0] : 0;
		if (p->scale==0){
			destroy(p);
			return NULL;
		}
	}
	return p;
}

static void execute(const void *p, void *in, void *out){
	run(p, in, out);
}

const kernel k_vdsp = {"vdsp", can_do, make, execute, destroy};
