// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

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

static std::unique_ptr<kfr::dft_plan_real<float> > plan;
static std::vector<kfr::u8> temp;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && n%2==0;
}

void setup(bench_problem *p){
	plan = std::make_unique<kfr::dft_plan_real<float> >(static_cast<size_t>(rof_size(p)));
	temp.resize(plan->temp_size+1);
}

void doit(int iter, bench_problem *p){
	kfr::complex<float> *out = static_cast<kfr::complex<float> *>(p->out);
	const float *in = static_cast<const float *>(p->in);
	for (int i = 0; i<iter; i++){
		plan->execute(out, in, temp.data());
	}
}

void done(bench_problem *p){
	UNUSED(p);
	plan.reset();
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
