// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* ESP-DSP's complex FFT on size/2 points, radix 4 where that's a power of 4, else radix 2, then its real split, as benchees/espdsp */
#include "kernel/kernel.h"
#include <dsps_fft2r.h>
#include <dsps_fft4r.h>
#include <sdkconfig.h>
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	int size;
	int radix;
} plan;

/* its tables are never remade, as another fft may be using them */
static int tables_cover(int half){
	if (half>CONFIG_DSP_MAX_FFT_SIZE){
		return 0;
	}
	dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE); /* both do nothing once the tables exist, even if smaller */
	dsps_fft4r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
	return dsps_fft2r_initialized && dsps_fft4r_initialized && dsps_fft_w_table_size>=half && dsps_fft4r_w_table_size>=2*half;
}

static int can_do(const problem *pb){
	const int size = problem_rof(pb);
	return size>=8 && (size&(size-1))==0 && tables_cover(size/2);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = malloc(sizeof(*p));
	if (p){
		p->size = size;
		p->radix = ((size/2)&0x55555555)!=0 ? 4 : 2; /* a power of 4 has its one bit at an even position */
	}
	return p;
}

/* DC, Nyquist, then (re, im) of bins 1..size/2-1 */
static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	int half = p->size/2;
	memcpy(out, in, (size_t)p->size*sizeof(float)); /* ESP-DSP works in place */
	if (p->radix==4){
		dsps_fft4r_fc32(out, half);
		dsps_bit_rev4r_fc32(out, half);
	} else {
		dsps_fft2r_fc32(out, half);
		dsps_bit_rev2r_fc32(out, half);
	}
	dsps_cplx2real_fc32(out, half);
	out[p->size] = out[1]; /* FFTW's layout: Nyquist last, the im of DC and Nyquist 0 */
	out[p->size+1] = 0;
	out[1] = 0;
}

static void destroy(void *p){
	free(p);
}

const kernel k_espdsp = {"espdsp", can_do, make, execute, destroy};
