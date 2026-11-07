// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the kernel of an FFTW interface (rdft/fftw3.c, armpl.c, aocl.c): every problem through its guru planners, F(name) its float names
   and, where it has them, D(name) its double ones, declared here, as each library's fftw3.h would shadow the others'; FFTW_MEASURE
   where the planner races (by estimate w/o timing). Defined first: NAME, F, [D], [CAN(pb)] a further test, [WARM] a transform at
   planning (ArmPL allocates on its first), [LEND] rdft/fftw_heap.h's lending, whose arena its per-transform buffers come from */
#include "kernel/kernel.h"
#include "rdft/fftw_heap.h"
#include <stdlib.h>

#define MEASURE 0U /* fftw3.h's flags */
#define DESTROY_INPUT (1U<<0)
#define PRESERVE_INPUT (1U<<4)
#define ESTIMATE (1U<<6)

typedef struct {
	int n, is, os;
} fftw_like_iodim; /* fftw_iodim's */

#define DECLARE(X, R) \
	struct X(plan_s) *X(plan_guru_dft)(int rank, const fftw_like_iodim *dims, int howmany_rank, const fftw_like_iodim *howmany, \
									   R(*in)[2], R(*out)[2], int sign, unsigned flags); \
	struct X(plan_s) *X(plan_guru_dft_r2c)(int rank, const fftw_like_iodim *dims, int howmany_rank, \
										   const fftw_like_iodim *howmany, R *in, R(*out)[2], unsigned flags); \
	struct X(plan_s) *X(plan_guru_dft_c2r)(int rank, const fftw_like_iodim *dims, int howmany_rank, \
										   const fftw_like_iodim *howmany, R(*in)[2], R *out, unsigned flags); \
	struct X(plan_s) *X(plan_guru_r2r)(int rank, const fftw_like_iodim *dims, int howmany_rank, const fftw_like_iodim *howmany, \
									   R *in, R *out, const int *kind, unsigned flags); \
	void X(execute_dft)(const struct X(plan_s) *p, R(*in)[2], R(*out)[2]); \
	void X(execute_dft_r2c)(const struct X(plan_s) *p, R *in, R(*out)[2]); \
	void X(execute_dft_c2r)(const struct X(plan_s) *p, R(*in)[2], R *out); \
	void X(execute_r2r)(const struct X(plan_s) *p, R *in, R *out); \
	void X(destroy_plan)(struct X(plan_s) *p); \
	k2fft_arena *X(k2fft_lend)(k2fft_arena *a);
DECLARE(F, float)
#ifdef D
DECLARE(D, double)
#endif

#ifndef CAN
#define CAN(pb) 1
#endif

typedef struct plan {
	void *fftw; /* its plan */
	problem_kind kind;
	int single;
	k2fft_arena arena;
	int lend; /* run lends the arena: while room counts, then if a transform wanted one */
} plan;

static int can_do(const problem *pb){
#ifndef D
	if (!pb->single){
		return 0;
	}
#endif
	return pb->rank>0 && CAN(pb);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->fftw && p->single){
		F(destroy_plan)(p->fftw);
	}
#ifdef D
	if (p->fftw && !p->single){
		D(destroy_plan)(p->fftw);
	}
#endif
	free(p->arena.base);
	free(p);
}

static void run(plan *p, void *in, void *out){
	if (p->single){
#ifdef LEND
		k2fft_arena *was = p->lend ? F(k2fft_lend)(&p->arena) : NULL; /* none if planning saw no heap call, as most */
#endif
		switch (p->kind){
		case KIND_DFT:
			F(execute_dft)(p->fftw, in, out);
			break;
		case KIND_R2C:
			F(execute_dft_r2c)(p->fftw, in, out);
			break;
		case KIND_C2R:
			F(execute_dft_c2r)(p->fftw, in, out);
			break;
		default:
			F(execute_r2r)(p->fftw, in, out);
			break;
		}
#ifdef LEND
		if (p->lend){
			F(k2fft_lend)(was);
		}
#endif
		return;
	}
#ifdef D
#ifdef LEND
	k2fft_arena *was = p->lend ? D(k2fft_lend)(&p->arena) : NULL;
#endif
	switch (p->kind){
	case KIND_DFT:
		D(execute_dft)(p->fftw, in, out);
		break;
	case KIND_R2C:
		D(execute_dft_r2c)(p->fftw, in, out);
		break;
	case KIND_C2R:
		D(execute_dft_c2r)(p->fftw, in, out);
		break;
	default:
		D(execute_r2r)(p->fftw, in, out);
		break;
	}
#ifdef LEND
	if (p->lend){
		D(k2fft_lend)(was);
	}
#endif
#endif
}

/* w/ LEND, the arena's room: a transform w/ none takes what it wants from the heap and counts it, then one in the room it counted
   mustn't; w/o, the transforms WARM asks for */
static int room(plan *p, const problem *pb){
#ifdef LEND
	p->lend = 1;
	run(p, pb->in, pb->out);
	p->lend = p->arena.want>0; /* most want none: run skips the lending's two calls */
	for (int tries=0; p->arena.want>0 && tries<4; tries++){
		const size_t want = p->arena.want*(tries ? 2 : 1);
		free(p->arena.base);
		p->arena.base = malloc(want);
		p->arena.size = p->arena.base ? want : 0;
		if (!p->arena.base){
			return 0;
		}
		run(p, pb->in, pb->out);
		if (!p->arena.over){
			return 1;
		}
	}
	return p->arena.want==0;
#else
#ifdef WARM
	run(p, pb->in, pb->out);
#endif
	(void)p;
	(void)pb;
	return 1;
#endif
}

static void *make(const problem *pb){
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->kind;
	p->single = pb->single;
	fftw_like_iodim d[RANKS], v[RANKS];
	int k[RANKS];
	for (int i=0; i<pb->rank; i++){
		d[i] = (fftw_like_iodim){pb->dims[i].n, pb->dims[i].is, pb->dims[i].os};
		k[i] = (int)pb->r2r[i]; /* k2fft_r2r_kind's FFTW's order */
	}
	for (int i=0; i<pb->vrank; i++){
		v[i] = (fftw_like_iodim){pb->vdims[i].n, pb->vdims[i].is, pb->vdims[i].os};
	}
	const unsigned flags = (pb->measure ? MEASURE : ESTIMATE)|(pb->kind==KIND_C2R && !pb->preserve ? DESTROY_INPUT : PRESERVE_INPUT);
	if (pb->single){
		switch (pb->kind){
		case KIND_DFT:
			p->fftw = F(plan_guru_dft)(pb->rank, d, pb->vrank, v, pb->in, pb->out, pb->sign, flags);
			break;
		case KIND_R2C:
			p->fftw = F(plan_guru_dft_r2c)(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		case KIND_C2R:
			p->fftw = F(plan_guru_dft_c2r)(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		default:
			p->fftw = F(plan_guru_r2r)(pb->rank, d, pb->vrank, v, pb->in, pb->out, k, flags);
			break;
		}
	}
#ifdef D
	if (!pb->single){
		switch (pb->kind){
		case KIND_DFT:
			p->fftw = D(plan_guru_dft)(pb->rank, d, pb->vrank, v, pb->in, pb->out, pb->sign, flags);
			break;
		case KIND_R2C:
			p->fftw = D(plan_guru_dft_r2c)(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		case KIND_C2R:
			p->fftw = D(plan_guru_dft_c2r)(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		default:
			p->fftw = D(plan_guru_r2r)(pb->rank, d, pb->vrank, v, pb->in, pb->out, k, flags);
			break;
		}
	}
#endif
	if (!p->fftw || !room(p, pb)){
		destroy(p);
		return NULL;
	}
	return p;
}

static void execute(const void *p, void *in, void *out){
	run((plan *)p, in, out); /* its arena's count changes */
}
