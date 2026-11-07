// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* FFTW: every problem, through its guru planners, in float (fftw3f) and double (fftw3), as benchees/fftw3; FFTW_MEASURE where the
   planner races (by estimate w/o timing, as there when cross-compiled); the buffers its solvers take per transform from the plan's
   arena (rdft/fftw_heap.h), sized by a transform at planning, so fftw_execute never reaches the heap */
#include "kernel/kernel.h"
#include "rdft/fftw_heap.h"
#include <fftw3.h>
#include <stdlib.h>

k2fft_arena *fftwf_k2fft_lend(k2fft_arena *a);
k2fft_arena *fftw_k2fft_lend(k2fft_arena *a);

typedef struct plan {
	void *fftw; /* an fftwf_plan, or an fftw_plan */
	problem_kind kind;
	int single;
	k2fft_arena arena;
} plan;

static int can_do(const problem *pb){
	return pb->rank>0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->fftw && p->single){
		fftwf_destroy_plan(p->fftw);
	} else if (p->fftw){
		fftw_destroy_plan(p->fftw);
	}
	free(p->arena.base);
	free(p);
}

static void run(plan *p, void *in, void *out){
	k2fft_arena *was = p->single ? fftwf_k2fft_lend(&p->arena) : fftw_k2fft_lend(&p->arena);
	if (p->single){
		switch (p->kind){
		case KIND_DFT:
			fftwf_execute_dft(p->fftw, in, out);
			break;
		case KIND_R2C:
			fftwf_execute_dft_r2c(p->fftw, in, out);
			break;
		case KIND_C2R:
			fftwf_execute_dft_c2r(p->fftw, in, out);
			break;
		default:
			fftwf_execute_r2r(p->fftw, in, out);
			break;
		}
		fftwf_k2fft_lend(was);
	} else {
		switch (p->kind){
		case KIND_DFT:
			fftw_execute_dft(p->fftw, in, out);
			break;
		case KIND_R2C:
			fftw_execute_dft_r2c(p->fftw, in, out);
			break;
		case KIND_C2R:
			fftw_execute_dft_c2r(p->fftw, in, out);
			break;
		default:
			fftw_execute_r2r(p->fftw, in, out);
			break;
		}
		fftw_k2fft_lend(was);
	}
}

/* the arena's room: a transform w/ none takes what it wants from the heap and counts it, then one in the room it counted mustn't */
static int room(plan *p, const problem *pb){
	run(p, pb->in, pb->out);
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
}

static void *make(const problem *pb){
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->kind = pb->kind;
	p->single = pb->single;
	fftw_iodim d[RANKS], v[RANKS]; /* fftwf_iodim's the same */
	fftw_r2r_kind k[RANKS];
	for (int i=0; i<pb->rank; i++){
		d[i] = (fftw_iodim){pb->dims[i].n, pb->dims[i].is, pb->dims[i].os};
		k[i] = (fftw_r2r_kind)pb->r2r[i]; /* k2fft_r2r_kind's FFTW's order */
	}
	for (int i=0; i<pb->vrank; i++){
		v[i] = (fftw_iodim){pb->vdims[i].n, pb->vdims[i].is, pb->vdims[i].os};
	}
	const unsigned flags = (pb->measure ? FFTW_MEASURE : FFTW_ESTIMATE)|
						   (pb->kind==KIND_C2R && !pb->preserve ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);
	if (pb->single){
		switch (pb->kind){
		case KIND_DFT:
			p->fftw = fftwf_plan_guru_dft(pb->rank, d, pb->vrank, v, pb->in, pb->out, pb->sign, flags);
			break;
		case KIND_R2C:
			p->fftw = fftwf_plan_guru_dft_r2c(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		case KIND_C2R:
			p->fftw = fftwf_plan_guru_dft_c2r(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		default:
			p->fftw = fftwf_plan_guru_r2r(pb->rank, d, pb->vrank, v, pb->in, pb->out, k, flags);
			break;
		}
	} else {
		switch (pb->kind){
		case KIND_DFT:
			p->fftw = fftw_plan_guru_dft(pb->rank, d, pb->vrank, v, pb->in, pb->out, pb->sign, flags);
			break;
		case KIND_R2C:
			p->fftw = fftw_plan_guru_dft_r2c(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		case KIND_C2R:
			p->fftw = fftw_plan_guru_dft_c2r(pb->rank, d, pb->vrank, v, pb->in, pb->out, flags);
			break;
		default:
			p->fftw = fftw_plan_guru_r2r(pb->rank, d, pb->vrank, v, pb->in, pb->out, k, flags);
			break;
		}
	}
	if (!p->fftw || !room(p, pb)){
		destroy(p);
		return NULL;
	}
	return p;
}

static void execute(const void *p, void *in, void *out){
	run((plan *)p, in, out); /* its arena's count changes */
}

const kernel k_fftw3 = {"fftw3", can_do, make, execute, destroy};
