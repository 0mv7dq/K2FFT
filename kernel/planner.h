// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "kernel/kernel.h"

#ifdef __cplusplus
extern "C" {
#endif


/* api/k2fft.h's plans */
struct k2fft_plan_s {
	const kernel *k;
	void *plan;
	problem p;
};

/* p's plan by the fastest kernel that can do it: kernel/table.h's candidates for p's name, then the kernels the benchmark didn't rank,
   then where a race can run the rest, raced (K2FFT_MEASURE) or the first (K2FFT_ESTIMATE, or w/o timing); NULL if none can */
struct k2fft_plan_s *planner_plan(const problem *p, unsigned flags);

void planner_destroy(struct k2fft_plan_s *plan);
/* wisdom, as FFTW's: the races' picks and race_size's answers as text (malloc'd), read back where this K2FFT, its kernels and CPU are
   the ones that wrote it (1, else 0 and nothing learnt), forgotten */
char *planner_wisdom(void);
int planner_learn(const char *text);
void planner_forget(void);

/* api/k2fft.h's convolutions: a kernel's own spectra (k, plan, its conv), else the planner's real forward and backward plans of n w/
   mac by bin; whichever runs forward, mac and backward faster */
struct k2fft_conv_s {
	const kernel *k;
	void *plan;
	problem p;
	struct k2fft_plan_s *r2c, *c2r;
	int n;
};
struct k2fft_conv_s *planner_conv(int n, float *in, float *spectrum, float *out, unsigned flags);
void planner_conv_destroy(struct k2fft_conv_s *c);
void planner_conv_forward(const struct k2fft_conv_s *c, const float *in, float *spectrum);
void planner_conv_mac(const struct k2fft_conv_s *c, const float *a, const float *b, float *sum, float scale);
void planner_conv_backward(const struct k2fft_conv_s *c, float *spectrum, float *out);


#ifdef __cplusplus
}
#endif
