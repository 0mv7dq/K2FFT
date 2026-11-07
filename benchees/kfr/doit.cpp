// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's DFTs, as rdft/kfr_rfft.cpp: one 1D transform out of place, complex either way, real forward or back at even sizes, in
   libbench2's precision */
#include "benchee.h"
#include <kfr/dft.hpp>
#include <memory>
#include <vector>

/* *INDENT-OFF* */ /* uncrustify takes BEGIN_BENCH_DOC for a C++ declaration */
BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "KFR")
BENCH_DOC("url", "https://github.com/kfrlib/kfr")
BENCH_DOC("copyright", "GPL-2.0-or-later, or commercial")
END_BENCH_DOC
/* *INDENT-ON* */

using R = bench_real;
static std::unique_ptr<kfr::dft_plan<R> > cplan;
static std::unique_ptr<kfr::dft_plan_real<R> > rplan;
static std::vector<kfr::u8> temp;

int can_do(bench_problem *p){
	int n = bench_1d(p);
	return n>0 && p->kind!=PROBLEM_R2R && !p->in_place && (p->kind==PROBLEM_COMPLEX || n%2==0);
}

void setup(bench_problem *p){
	const size_t n = static_cast<size_t>(bench_1d(p));
	if (p->kind==PROBLEM_COMPLEX){
		cplan = std::make_unique<kfr::dft_plan<R> >(n);
		temp.resize(cplan->temp_size+1);
	} else {
		rplan = std::make_unique<kfr::dft_plan_real<R> >(n);
		temp.resize(rplan->temp_size+1);
	}
}

void doit(int iter, bench_problem *p){
	const bool back = p->sign>0;
	for (int i = 0; i<iter; i++){
		if (cplan){
			cplan->execute(static_cast<kfr::complex<R> *>(p->out), static_cast<const kfr::complex<R> *>(p->in), temp.data(), back);
		} else if (back){
			rplan->execute(static_cast<R *>(p->out), static_cast<const kfr::complex<R> *>(p->in), temp.data());
		} else {
			rplan->execute(static_cast<kfr::complex<R> *>(p->out), static_cast<const R *>(p->in), temp.data());
		}
	}
}

void done(bench_problem *p){
	UNUSED(p);
	cplan.reset();
	rplan.reset();
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
