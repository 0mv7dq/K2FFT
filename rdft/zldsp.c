// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* zldsp's real FFT at powers of two, as benchees/zldsp: rdft/zldsp_rfft.cpp built for AVX2 on x86-64, run where the CPU has it */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include "simd-support/simd.h"

void *zldsp_plan(int size, float *in, float *out);
void zldsp_forward(void *plan, float *in, float *out);
void zldsp_destroy(void *plan);

static int can_do(int size){
#if defined(__x86_64__) || defined(_M_X64)
	if (!simd_avx2_fma()){
		return 0;
	}
#endif
	return size>0 && (size&(size-1))==0;
}

const kernel k_zldsp = {"zldsp", can_do, zldsp_plan, zldsp_forward, zldsp_destroy, power_interleaved, bin_interleaved};
