// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "api/fft.h"
#include "kernel/kernel.h"

#ifdef __cplusplus
extern "C" {
#endif


/* a contestant: a kernel's plan and the problem it's for, which it runs on p->in and p->out */
typedef struct racer {
	const kernel *k;
	void *plan;
	const problem *p;
} racer;

/* whether plans may time transforms: a clock, and K2FFT_NORACE unset, as an emulator's time isn't the target's */
int race_timing(void);

/* whether the target has a clock, so races run there (the candidates K2FFT_NORACE indexes, under an emulator), not bare metal */
int race_clocked(void);

/* where plans don't time transforms, the candidate of n to plan alone: K2FFT_NORACE's k-th (-1 if fewer), so scripts/benchmark can
   count each, else the first */
int race_untimed(int n);

/* the fastest of n racers, timed idle and busy, the first on a tie or w/o timing; allocates */
int race_fastest(const racer *r, int n);

/* fastest nearby fft_new size >= n, timed once, allocates */
int race_size(int n); /* the smallest if n > RACED_UP_TO, 0 if none */

/* serialises planning (recursive), held by the planner and by spectrum_init and spectrum_free */
void race_lock(void);
void race_unlock(void);

/* api/fft.c's racer for f */
racer fft_racer(fft *f);

/* tests only, called on each racer before it is timed (its index in race_fastest, -1 in race_size), must leave its input as it was */
extern void (*race_hook)(const racer *r, int path);


#ifdef __cplusplus
}
#endif
