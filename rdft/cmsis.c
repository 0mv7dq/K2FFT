// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* CMSIS-DSP, as benchees/cmsis: one 1D forward transform of floats at a power of two, real (arm_rfft_fast_f32, 32 to 4096) or
   complex (arm_cfft_f32, 16 to 4096, in place on out); w/o its backward ones, which scale by 1/n */
#include "kernel/kernel.h"
#include <arm_math.h>
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

typedef struct plan {
	arm_rfft_fast_instance_f32 rfft;
	arm_cfft_instance_f32 cfft;
	float32_t *work; /* arm_rfft_fast_f32 overwrites its input */
	int size;
	int complex;
} plan;

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	if (!pb->single || size<=0 || (size&(size-1))!=0 || size>4096){
		return 0;
	}
	return (pb->kind==KIND_R2C && size>=32 && pb->in!=pb->out) || (pb->kind==KIND_DFT && pb->sign<0 && size>=16);
}

static void destroy(void *p_){
	plan *p = p_;
	pffft_aligned_free(p->work);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->complex = pb->kind==KIND_DFT;
	if (p->complex){
		if (arm_cfft_init_f32(&p->cfft, (uint16_t)size)!=ARM_MATH_SUCCESS){
			destroy(p);
			return NULL;
		}
		return p;
	}
	p->work = pffft_aligned_malloc((size_t)size*sizeof(float32_t));
	if (!p->work || arm_rfft_fast_init_f32(&p->rfft, (uint16_t)size)!=ARM_MATH_SUCCESS){
		destroy(p);
		return NULL;
	}
	return p;
}

/* FFTW's layouts: a complex one's (re, im) pairs; a real one's DC, Nyquist, then (re, im) of bins 1..size/2-1, rearranged */
static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	if (p->complex){
		if (out!=in){
			memcpy(out, in, 2*(size_t)p->size*sizeof(float32_t));
		}
		arm_cfft_f32(&p->cfft, out, 0, 1);
		return;
	}
	memcpy(p->work, in, (size_t)p->size*sizeof(float32_t));
	arm_rfft_fast_f32(&p->rfft, p->work, out, 0);
	out[p->size] = out[1]; /* Nyquist last, the im of DC and Nyquist 0 */
	out[p->size+1] = 0;
	out[1] = 0;
}

const kernel k_cmsis = {"cmsis", can_do, make, execute, destroy, 0};
