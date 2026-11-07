// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* a transform, as FFTW's guru problems: what kernels can do and the planner plans */
#pragma once

#include "api/k2fft.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


enum { RANKS = 8 }; /* most transform dimensions, and batch dimensions */

typedef enum problem_kind {
	KIND_DFT, /* complex to complex, sign's way */
	KIND_R2C, /* real to the n/2+1 complex of the last dimension, forward */
	KIND_C2R, /* back, unnormalized */
	KIND_R2R /* r2r[d] along dimension d */
} problem_kind;

typedef struct problem {
	problem_kind kind;
	int sign; /* DFT's, K2FFT_FORWARD or K2FFT_BACKWARD */
	int single; /* floats, else doubles */
	int preserve; /* a C2R leaves in as it was */
	int measure; /* kernels may time while planning, as FFTW_MEASURE (the planner's set it: a race, w/ a clock) */
	int rank, vrank;
	k2fft_iodim dims[RANKS]; /* strides in the arrays' elements: reals, or complex pairs */
	k2fft_iodim vdims[RANKS]; /* the batch */
	k2fft_r2r_kind r2r[RANKS];
	void *in, *out; /* the arrays it's planned on */
} problem;

/* n if p is what the real forward kernels do (one size-n R2C of floats, contiguous, out of place), else 0 */
int problem_rof(const problem *p);

/* n if p is one contiguous 1D transform of its kind w/o a batch (r2r's kind in *r2r), else 0 */
int problem_1d(const problem *p);

/* benchFFT's name for p, as benchmark/ and kernel/table.h key it: class ("srof", "dcib", "skof") and tensor ("1024", "64x64",
   "1024e10", "1024*16"); 0 if they don't fit */
int problem_name(const problem *p, char *cls, size_t cls_room, char *tensor, size_t tensor_room);

/* zeroes the elements of p's input, so a race's C2R, which overwrites them, keeps running on zeros */
void problem_zero_input(const problem *p);


#ifdef __cplusplus
}
#endif
