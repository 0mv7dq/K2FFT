// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once
#include "libbench2/bench-user.h"

/* n if p is rof<n>, else 0 */
static inline int rof_size(const bench_problem *p){
	if (p->kind!=PROBLEM_REAL || p->sign!=-1 || p->in_place || p->split){
		return 0;
	}
	if (!BENCH_FINITE_RNK(p->sz->rnk) || p->sz->rnk!=1 || !BENCH_FINITE_RNK(p->vecsz->rnk) || p->vecsz->rnk!=0){
		return 0;
	}
	return tensor_unitstridep(p->sz) ? p->sz->dims[0].n : 0;
}
