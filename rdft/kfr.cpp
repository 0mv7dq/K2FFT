// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's real DFT, as benchees/kfr */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include <kfr/dft.hpp>
#include <cstdlib>

namespace {
struct plan {
	kfr::dft_plan_real<float> dft;
	kfr::u8 *temp; /* so a transform never allocates */
	explicit plan(int size) : dft(static_cast<size_t>(size)), temp(nullptr){
	}
};
}

static int can_do(int size){
	return size>0 && size%2==0;
}

static void destroy(void *p_){
	plan *p = static_cast<plan *>(p_);
	std::free(p->temp);
	delete p;
}

static void *make(int size, float *, float *){
	try {
		plan *p = new plan(size);
		p->temp = static_cast<kfr::u8 *>(std::malloc(p->dft.temp_size+1));
		if (!p->temp){
			delete p;
			return nullptr;
		}
		return p;
	} catch (...){
		return nullptr; /* an exception must not reach C */
	}
}

/* size/2+1 (re, im) pairs */
static void forward(void *p_, float *in, float *out){
	plan *p = static_cast<plan *>(p_);
	p->dft.execute(reinterpret_cast<kfr::complex<float> *>(out), in, p->temp);
}

extern "C" const kernel k_kfr = {"kfr", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
