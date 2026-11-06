// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "benchee.h"
#include <dsps_fft2r.h>
#include <dsps_fft4r.h>
#include <sdkconfig.h>
#include <string.h>

BEGIN_BENCH_DOC
BENCH_DOC("name", BENCHEE)
BENCH_DOC("author", "Espressif")
BENCH_DOC("url", "https://github.com/espressif/esp-dsp")
BENCH_DOC("copyright", "Apache-2.0")
END_BENCH_DOC

static int radix;

/* its tables at their largest, made once, as rdft/espdsp.c */
static int tables_cover(int half){
	if (half>CONFIG_DSP_MAX_FFT_SIZE){
		return 0;
	}
	dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
	dsps_fft4r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
	return dsps_fft2r_initialized && dsps_fft4r_initialized && dsps_fft_w_table_size>=half && dsps_fft4r_w_table_size>=2*half;
}

int can_do(bench_problem *p){
	int n = rof_size(p);
	return n>=8 && power_of_two(n) && tables_cover(n/2);
}

void setup(bench_problem *p){
	radix = ((rof_size(p)/2)&0x55555555)!=0 ? 4 : 2; /* radix 4 where n/2 is a power of 4 */
}

void doit(int iter, bench_problem *p){
	int n = rof_size(p);
	int half = n/2;
	float *out = p->out;
	for (int i = 0; i<iter; i++){
		memcpy(out, p->in, (size_t)n*sizeof(float)); /* ESP-DSP works in place */
		if (radix==4){
			dsps_fft4r_fc32(out, half);
			dsps_bit_rev4r_fc32(out, half);
		} else {
			dsps_fft2r_fc32(out, half);
			dsps_bit_rev2r_fc32(out, half);
		}
		dsps_cplx2real_fc32(out, half);
	}
}

void done(bench_problem *p){
	UNUSED(p);
}

/* benchFFT's packed format, as benchees/pffft: the Nyquist bin in io[0], which is ro[1] */
void after_problem_hccopy_to(bench_problem *p, bench_real *ro, bench_real *io){
	int n = rof_size(p);
	ro[n] = ro[1];
	io[0] = 0;
	io[n] = 0;
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){
}
