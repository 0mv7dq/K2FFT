// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <bit>
#include <complex>
#include <memory>
#include "zldsp_fft_rfft.hpp"

static const char *target(void){
	return hwy::TargetName(HWY_STATIC_TARGET);
}

/* *INDENT-OFF* */ /* uncrustify takes BEGIN_BENCH_DOC for a C++ declaration */
BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOCF("target", target)
BENCH_DOC("author", "ZL Audio")
BENCH_DOC("url", "https://github.com/ZL-Audio/zldsp_fft")
BENCH_DOC("copyright", "Apache-2.0")
END_BENCH_DOC
/* *INDENT-ON* */

static std::unique_ptr<zldsp::fft::RFFT<float> > plan;

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>0 && power_of_two(n);
}

void setup(bench_problem *p){
	plan = std::make_unique<zldsp::fft::RFFT<float> >(static_cast<size_t>(std::countr_zero(static_cast<unsigned>(rof_size(p)))));
}

void doit(int iter, bench_problem *p){
	float *in = static_cast<float *>(p->in);
	std::complex<float> *out = static_cast<std::complex<float> *>(p->out);
	for (int i = 0; i<iter; i++){
		plan->forward(in, out);
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
