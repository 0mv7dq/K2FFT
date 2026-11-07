// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "kernel/problem.h"
#include <stdio.h>
#include <string.h>

/* dimension d of the input's elements from base on: the transform's, then the batch's; a C2R's last has n/2+1 */
static void zero(const problem *p, char *base, int d, size_t bytes){
	if (d==p->rank+p->vrank){
		memset(base, 0, bytes);
		return;
	}
	const k2fft_iodim *x = d<p->rank ? &p->dims[d] : &p->vdims[d-p->rank];
	const int n = d==p->rank-1 && p->kind==KIND_C2R ? x->n/2+1 : x->n;
	for (int i=0; i<n; i++){
		zero(p, base+(ptrdiff_t)i*x->is*(ptrdiff_t)bytes, d+1, bytes);
	}
}

void problem_zero_input(const problem *p){
	const size_t real = p->single ? sizeof(float) : sizeof(double);
	zero(p, p->in, 0, p->kind==KIND_DFT || p->kind==KIND_C2R ? 2*real : real);
}

int problem_1d(const problem *p){
	return p->rank==1 && p->vrank==0 && p->dims[0].is==1 && p->dims[0].os==1 ? p->dims[0].n : 0;
}

int problem_rof(const problem *p){
	return p->kind==KIND_R2C && p->single && p->in!=p->out ? problem_1d(p) : 0;
}

/* libbench2's letters for an r2r kind (problem.c's parsetensor) */
static const char *letters(k2fft_r2r_kind k){
	static const char *const l[] = {"f", "b", "h", "e00", "e01", "e10", "e11", "o00", "o01", "o10", "o11"};
	return (unsigned)k<sizeof(l)/sizeof(l[0]) ? l[k] : "?";
}

int problem_name(const problem *p, char *cls, size_t cls_room, char *tensor, size_t tensor_room){
	const char *way = p->kind==KIND_R2R ? "" : (p->kind==KIND_C2R || (p->kind==KIND_DFT && p->sign>0) ? "b" : "f");
	int n = snprintf(cls, cls_room, "%c%c%c%s", p->single ? 's' : 'd', p->kind==KIND_DFT ? 'c' : (p->kind==KIND_R2R ? 'k' : 'r'),
					 p->in==p->out ? 'i' : 'o', way);
	if (n<0 || (size_t)n>=cls_room){
		return 0;
	}
	size_t used = 0;
	for (int d=0; d<p->rank+p->vrank; d++){
		const char *sep = d==0 ? "" : (d==p->rank ? "*" : "x");
		const int size = d<p->rank ? p->dims[d].n : p->vdims[d-p->rank].n;
		n = snprintf(tensor+used, tensor_room-used, "%s%d%s", sep, size, d<p->rank && p->kind==KIND_R2R ? letters(p->r2r[d]) : "");
		if (n<0 || (size_t)n>=tensor_room-used){
			return 0;
		}
		used += (size_t)n;
	}
	return 1;
}
