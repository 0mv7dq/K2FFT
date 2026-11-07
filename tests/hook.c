// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include "libbench2/bench.h"
#include "kernel/race.h"
#include <stdio.h>

extern const racer *contestant;

/* for -o paranoid, libbench2 verifies each contestant before the race times it, on a problem of its own, whose arrays tests/bench.c's
   doit runs it on; one libbench2 can't name (strided) or of the other precision is left to the race */
void hook(const racer *r, int path){
	UNUSED(path);
	char cls[8], tensor[160], desc[176];
	if (r->p->single!=(sizeof(bench_real)==sizeof(float)) || !problem_name(r->p, cls, sizeof(cls), tensor, sizeof(tensor))){
		return;
	}
	snprintf(desc, sizeof(desc), "%s%s", cls+1, tensor); /* w/o the precision */
	bench_problem *p = problem_parse(desc);
	p->destroy_input = r->p->kind==KIND_C2R && !r->p->preserve;
	problem_alloc(p);
	problem_zero(p);
	const racer *own = contestant;
	contestant = r;
	verify_problem(p, 5, sizeof(bench_real)==sizeof(float) ? 1.0e-3 : 1.0e-10);
	contestant = own;
	problem_destroy(p);
}
