// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once
#include "libbench2/bench-user.h"

/* n if p is one 1D transform of contiguous elements w/o a batch, any kind, in or out of place, else 0 */
static inline int bench_1d(const bench_problem *p){
	if (p->split || !BENCH_FINITE_RNK(p->sz->rnk) || p->sz->rnk!=1 || !BENCH_FINITE_RNK(p->vecsz->rnk) || p->vecsz->rnk!=0){
		return 0;
	}
	return tensor_unitstridep(p->sz) ? p->sz->dims[0].n : 0;
}

/* n if p is rof<n>, else 0 */
static inline int rof_size(const bench_problem *p){
	if (p->kind!=PROBLEM_REAL || p->sign!=-1 || p->in_place){
		return 0;
	}
	return bench_1d(p);
}
