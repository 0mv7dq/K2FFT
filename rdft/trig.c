// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTW's 1D r2r kinds as FFTPACK computes its trigonometric transforms (Swarztrauber's, libs/pffft/src/fftpack.c's cost, sint, cosq,
   sinq): O(n) before and after one FFT, which kernel/planner.c plans as any other, so each rides the fastest this CPU has: a real one
   of n (DCT-II/III, DST-II/III, R2HC, HC2R, DHT; Makhoul's for the DCTs), of 2(n-1) or 2(n+1) (DCT-I, DST-I: their extensions,
   not FFTPACK's shorter, less accurate ones), a complex one of n/2 (DCT-IV, DST-IV); buffers and twiddles at planning */
#include "kernel/kernel.h"
#include "kernel/planner.h"
#include <math.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct trig {
	struct k2fft_plan_s *fft;
	k2fft_r2r_kind kind;
	int n, single;
	void *a, *b, *w; /* the FFT's input and output, the twiddles */
} trig;

#define R float
#define T(name) name##_f
#include "rdft/trig.h"
#undef R
#undef T
#define R double
#define T(name) name##_d
#include "rdft/trig.h"
#undef R
#undef T

static int can_do(const problem *pb){
	const int n = problem_1d(pb);
	if (pb->kind!=KIND_R2R || n<=0){
		return 0;
	}
	switch (pb->r2r[0]){
	case K2FFT_REDFT00: return n>=2;
	case K2FFT_REDFT11: case K2FFT_RODFT11: return n%2==0;
	default: return 1;
	}
}

static void destroy(void *p_){
	trig *p = p_;
	planner_destroy(p->fft);
	pffft_aligned_free(p->a);
	pffft_aligned_free(p->b);
	pffft_aligned_free(p->w);
	free(p);
}

static void *make(const problem *pb){
	trig *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->r2r[0];
	p->n = pb->dims[0].n;
	p->single = pb->single;
	const int n = p->n;
	problem sub = {.kind = KIND_R2C, .sign = K2FFT_FORWARD, .single = pb->single, .rank = 1};
	int m = n;
	switch (p->kind){
	case K2FFT_REDFT01: case K2FFT_RODFT01: case K2FFT_HC2R:
		sub.kind = KIND_C2R;
		sub.sign = K2FFT_BACKWARD;
		break;
	case K2FFT_REDFT11: case K2FFT_RODFT11:
		sub.kind = KIND_DFT;
		m = n/2;
		break;
	case K2FFT_REDFT00:
		m = 2*(n-1);
		break;
	case K2FFT_RODFT00:
		m = 2*(n+1);
		break;
	default:
		break;
	}
	const size_t real = pb->single ? sizeof(float) : sizeof(double), room = (size_t)(2*m+2)*real;
	p->a = pffft_aligned_malloc(room);
	p->b = pffft_aligned_malloc(room);
	p->w = pffft_aligned_malloc((size_t)(2*n+2)*real);
	if (!p->a || !p->b || !p->w){
		destroy(p);
		return NULL;
	}
	if (p->single){
		twiddles_f(p->w, n, p->kind);
	} else {
		twiddles_d(p->w, n, p->kind);
	}
	sub.dims[0] = (k2fft_iodim){m, 1, 1};
	sub.in = p->a;
	sub.out = p->b;
	p->fft = planner_plan(&sub, pb->measure ? K2FFT_MEASURE : K2FFT_ESTIMATE);
	if (!p->fft){
		destroy(p);
		return NULL;
	}
	return p;
}

static void execute(const void *p_, void *in, void *out){
	const trig *p = p_;
	if (p->single){
		run_f(p, in, out);
	} else {
		run_d(p, in, out);
	}
}

const kernel k_trig = {"trig", can_do, make, execute, destroy};
