// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* CMSIS-DSP's arm_rfft_fast_f32, as benchees/cmsis: powers of two from 32 to 4096 */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include <arm_math.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	arm_rfft_fast_instance_f32 rfft;
	float32_t *work; /* arm_rfft_fast_f32 overwrites its input */
	int size;
} plan;

static int can_do(int size){
	return size>=32 && size<=4096 && (size&(size-1))==0;
}

static void destroy(void *p_){
	plan *p = p_;
	pffft_aligned_free(p->work);
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->work = pffft_aligned_malloc((size_t)size*sizeof(float32_t));
	if (!p->work || arm_rfft_fast_init_f32(&p->rfft, (uint16_t)size)!=ARM_MATH_SUCCESS){
		destroy(p);
		return NULL;
	}
	return p;
}

/* DC, Nyquist, then (re, im) of bins 1..size/2-1 */
static void forward(void *p_, float *in, float *out){
	plan *p = p_;
	memcpy(p->work, in, (size_t)p->size*sizeof(float32_t));
	arm_rfft_fast_f32(&p->rfft, p->work, out, 0);
}

const kernel k_cmsis = {"cmsis", can_do, make, forward, destroy, power_packed, bin_packed};
