// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* libbench2's problems through an FFTW-style interface (fftw3.h's, api/k2fft.h's, ArmPL's or AOCL's renamed), as FFTW's
   tests/bench.c: the basic, advanced or guru planner by the problem's shape. Defined first: API(name), its prefixed names (fftwf_name),
   API_IODIM and API_KIND, its iodim and r2r kind types, API_PRESERVE and API_DESTROY, its flags */
#pragma once
#include "benchee.h"

static int *api_sizes(const bench_tensor *t){
	int *n = bench_malloc(sizeof(int)*(size_t)t->rnk);
	for (int i = 0; i<t->rnk; i++){
		n[i] = t->dims[i].n;
	}
	return n;
}

static API_IODIM *api_iodims(const bench_tensor *t){
	if (t->rnk==0){
		return NULL;
	}
	API_IODIM *d = bench_malloc(sizeof(*d)*(size_t)t->rnk);
	for (int i = 0; i<t->rnk; i++){
		d[i].n = t->dims[i].n;
		d[i].is = t->dims[i].is;
		d[i].os = t->dims[i].os;
	}
	return d;
}

/* whether t's strides are its last's times the embeds, as the advanced interface's */
static int api_embedded(const bench_tensor *t){
	for (int i = t->rnk-2; i>=0; i--){
		const bench_iodim *d = &t->dims[i];
		if (!d[1].is || !d[1].os || d[0].is%d[1].is || d[0].os%d[1].os){
			return 0;
		}
	}
	return 1;
}

static void api_embeds(const bench_tensor *t, int **inembed, int **onembed){
	*inembed = bench_malloc(sizeof(int)*(size_t)t->rnk);
	*onembed = bench_malloc(sizeof(int)*(size_t)t->rnk);
	(*inembed)[0] = (*onembed)[0] = t->dims[0].n; /* unread */
	for (int i = t->rnk-2; i>=0; i--){
		(*inembed)[i+1] = t->dims[i].is/t->dims[i+1].is;
		(*onembed)[i+1] = t->dims[i].os/t->dims[i+1].os;
	}
}

static API(plan) api_basic(bench_problem *p, const API_KIND *k, unsigned flags){
	const bench_tensor *sz = p->sz;
	const int r2c = p->sign<0;
	if (sz->rnk==1){
		const int n = sz->dims[0].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return API(plan_dft_1d)(n, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? API(plan_dft_r2c_1d)(n, p->in, p->out, flags) : API(plan_dft_c2r_1d)(n, p->in, p->out, flags);
		default:
			return API(plan_r2r_1d)(n, p->in, p->out, k[0], flags);
		}
	}
	if (sz->rnk==2){
		const int n0 = sz->dims[0].n, n1 = sz->dims[1].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return API(plan_dft_2d)(n0, n1, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? API(plan_dft_r2c_2d)(n0, n1, p->in, p->out, flags) : API(plan_dft_c2r_2d)(n0, n1, p->in, p->out, flags);
		default:
			return API(plan_r2r_2d)(n0, n1, p->in, p->out, k[0], k[1], flags);
		}
	}
	if (sz->rnk==3){
		const int n0 = sz->dims[0].n, n1 = sz->dims[1].n, n2 = sz->dims[2].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return API(plan_dft_3d)(n0, n1, n2, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? API(plan_dft_r2c_3d)(n0, n1, n2, p->in, p->out, flags) : API(plan_dft_c2r_3d)(n0, n1, n2, p->in, p->out, flags);
		default:
			return API(plan_r2r_3d)(n0, n1, n2, p->in, p->out, k[0], k[1], k[2], flags);
		}
	}
	int *n = api_sizes(sz);
	API(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = API(plan_dft)(sz->rnk, n, p->in, p->out, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = r2c ? API(plan_dft_r2c)(sz->rnk, n, p->in, p->out, flags) : API(plan_dft_c2r)(sz->rnk, n, p->in, p->out, flags);
		break;
	default:
		x = API(plan_r2r)(sz->rnk, n, p->in, p->out, k, flags);
		break;
	}
	bench_free(n);
	return x;
}

static API(plan) api_advanced(bench_problem *p, const API_KIND *k, unsigned flags){
	const bench_tensor *sz = p->sz;
	const bench_iodim *v = &p->vecsz->dims[0], *last = &sz->dims[sz->rnk-1];
	int *n = api_sizes(sz), *ie, *oe;
	api_embeds(sz, &ie, &oe);
	API(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = API(plan_many_dft)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = p->sign<0 ? API(plan_many_dft_r2c)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, flags)
					  : API(plan_many_dft_c2r)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, flags);
		break;
	default:
		x = API(plan_many_r2r)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, k, flags);
		break;
	}
	bench_free(n);
	bench_free(ie);
	bench_free(oe);
	return x;
}

static API(plan) api_guru(bench_problem *p, const API_KIND *k, unsigned flags){
	const int rank = p->sz->rnk, vrank = p->vecsz->rnk;
	API_IODIM *d = api_iodims(p->sz), *v = api_iodims(p->vecsz);
	API(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = API(plan_guru_dft)(rank, d, vrank, v, p->in, p->out, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = p->sign<0 ? API(plan_guru_dft_r2c)(rank, d, vrank, v, p->in, p->out, flags)
					  : API(plan_guru_dft_c2r)(rank, d, vrank, v, p->in, p->out, flags);
		break;
	default:
		x = API(plan_guru_r2r)(rank, d, vrank, v, p->in, p->out, k, flags);
		break;
	}
	bench_free0(d);
	bench_free0(v);
	return x;
}

/* as FFTW's tests: a multidimensional c2r out of place may destroy its input */
static unsigned api_preserving(bench_problem *p){
	if (p->kind==PROBLEM_REAL && p->sign>0 && !p->in_place && p->sz->rnk>1){
		p->destroy_input = 1;
	}
	return p->destroy_input ? API_DESTROY : API_PRESERVE;
}

/* the planner FFTW's tests/bench.c picks: the basic for a contiguous row-major transform, the advanced for one batch dimension; NULL
   for what the interface can't name (split arrays, rank 0, an infinite rank) */
static API(plan) api_plan(bench_problem *p, unsigned flags){
	const bench_tensor *sz = p->sz, *v = p->vecsz;
	if (p->split || !BENCH_FINITE_RNK(sz->rnk) || !BENCH_FINITE_RNK(v->rnk) || sz->rnk<1){
		return NULL;
	}
	API_KIND *k = NULL;
	if (p->kind==PROBLEM_R2R){
		k = bench_malloc(sizeof(*k)*(size_t)sz->rnk);
		for (int i = 0; i<sz->rnk; i++){
			k[i] = (API_KIND)p->k[i]; /* r2r_kind_t's FFTW's order */
		}
	}
	flags |= api_preserving(p);
	const int rowmajor = p->kind==PROBLEM_REAL ? tensor_real_rowmajorp(p->sz, p->sign, p->in_place) : tensor_rowmajorp(p->sz);
	API(plan) x;
	if (v->rnk==0 && tensor_unitstridep(p->sz) && rowmajor){
		x = api_basic(p, k, flags);
	} else if (v->rnk==1 && api_embedded(sz)){
		x = api_advanced(p, k, flags);
	} else {
		x = api_guru(p, k, flags);
	}
	bench_free0(k);
	return x;
}
