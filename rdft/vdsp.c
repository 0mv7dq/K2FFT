// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* vDSP's DFT, as benchees/vdsp: one 1D transform of floats or doubles out of place at f 2^k (f 1, 3, 5 or 15), complex either way
   (zop), real forward or back (zrop); its split arrays to and from FFTW's (re, im) pairs, its real ones' scales to FFTW's */
#include "kernel/kernel.h"
#include <Accelerate/Accelerate.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	void *dft; /* vDSP_DFT_Setup, or vDSP_DFT_SetupD */
	problem_kind kind;
	int size;
	int single;
	double scale; /* a real one's, to FFTW's */
	void *split; /* the input's re, then its im: size/2 each for a real one, size for a complex one */
	void *bins; /* the output's */
} plan;

#define R float
#define T(name) name##_f
#define V(name) name
#define CPLX DSPComplex
#define SPLIT DSPSplitComplex
#include "rdft/vdsp.h"
#undef R
#undef T
#undef V
#undef CPLX
#undef SPLIT
#define R double
#define T(name) name##_d
#define V(name) name##D
#define CPLX DSPDoubleComplex
#define SPLIT DSPDoubleSplitComplex
#include "rdft/vdsp.h"
#undef R
#undef T
#undef V
#undef CPLX
#undef SPLIT

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	if (pb->kind==KIND_R2R || pb->in==pb->out || size<=0){
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
		if (p->single){
			unsetup_f(p->dft);
		} else {
			unsetup_d(p->dft);
		}
	}
	pffft_aligned_free(p->split);
	pffft_aligned_free(p->bins);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n, complex = pb->kind==KIND_DFT;
	const size_t e = pb->single ? sizeof(float) : sizeof(double);
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->kind = pb->kind;
	p->single = pb->single;
	p->scale = 0.5; /* zrop's forward: twice the DFT */
	const vDSP_DFT_Direction way = pb->kind==KIND_C2R || (complex && pb->sign>0) ? vDSP_DFT_INVERSE : vDSP_DFT_FORWARD;
	p->dft = p->single ? setup_f(size, complex, way) : setup_d(size, complex, way);
	p->split = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*e);
	p->bins = pffft_aligned_malloc((size_t)size*(complex ? 2 : 1)*e);
	if (!p->dft || !p->split || !p->bins){
		destroy(p);
		return NULL;
	}
	if (pb->kind==KIND_C2R){ /* its scale, measured */
		const double dc = p->single ? dc_f(p, pb->in, pb->out) : dc_d(p, pb->in, pb->out);
		if (dc==0){
			destroy(p);
			return NULL;
		}
		p->scale = 1/dc;
	}
	return p;
}

static void execute(const void *p_, void *in, void *out){
	const plan *p = p_;
	if (p->single){
		run_f(p, in, out);
	} else {
		run_d(p, in, out);
	}
}

const kernel k_vdsp = {"vdsp", can_do, make, execute, destroy};
