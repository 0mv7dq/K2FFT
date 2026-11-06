// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* zldsp's RFFT for the one Highway target this file's flags select (HWY_COMPILE_ONLY_STATIC), as benchees/zldsp; rdft/zldsp.c checks the CPU */
#include <bit>
#include <complex>
#include "zldsp_fft_rfft.hpp"

typedef zldsp::fft::RFFT<float> rfft;

extern "C" void *zldsp_plan(int size, float *, float *){
	try {
		return new rfft(static_cast<size_t>(std::countr_zero(static_cast<unsigned>(size))));
	} catch (...){
		return nullptr; /* an exception must not reach C */
	}
}

/* size/2+1 (re, im) pairs */
extern "C" void zldsp_forward(void *plan, float *in, float *out){
	static_cast<rfft *>(plan)->forward(in, reinterpret_cast<std::complex<float> *>(out));
}

extern "C" void zldsp_destroy(void *plan){
	delete static_cast<rfft *>(plan);
}
