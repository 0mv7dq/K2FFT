// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/* one library's real forward transform, as FFTW's solvers: api/fft.c's planner picks among them by kernel/table.h and a race */
typedef struct kernel {
	const char *name; /* its benchee's, as benchmark/ and kernel/table.h name it */
	int (*can_do)(int size); /* here, after the CPU's run-time checks */
	void *(*plan)(int size, float *in, float *out); /* allocates, may overwrite in and out; NULL if it can't */
	void (*forward)(void *plan, float *in, float *out); /* never allocates, leaves in as it was */
	void (*destroy)(void *plan);
	float (*power)(const float *out, int size, float *power, int from); /* fft_power and fft_bin of out's layout */
	void (*bin)(const float *out, int size, int k, float *re, float *im);
} kernel; /* in: size floats, out: size+2, both 64-byte aligned */

/* the kernels built here, NULL-terminated, in the order the planner falls back on (kernel/kernels.c) */
extern const kernel *const kernels[];


#ifdef __cplusplus
}
#endif
