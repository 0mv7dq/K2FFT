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


#ifdef __cplusplus
}
#endif
