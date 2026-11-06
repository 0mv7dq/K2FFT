// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "api/fft.h"

#ifdef __cplusplus
extern "C" {
#endif


/* whether plans may time transforms: a clock, and K2FFT_NORACE unset, as an emulator's time isn't the target's */
int race_timing(void);

/* the fastest of n transforms of one size, the first on a tie or w/o a clock, timed once per size, allocates */
int race_ffts(fft **f, int n, int size); /* K2FFT_NORACE=k: the k-th, -1 if fewer, so scripts/benchmark can count each */

/* 1 + what race_ffts picked for size, 0 if it hasn't raced it */
int race_picked(int size);

/* fastest nearby backend size >= n, timed once, allocates */
int race_size(int n); /* the smallest if n > RACED_UP_TO, 0 if none */

/* serialises race_size, fft_new and fft_free, held by spectrum_init and spectrum_free */
void race_lock(void);
void race_unlock(void);

/* tests only, called on each contestant before it is timed (its index in race_ffts, -1 in race_size), must leave f's input as it was */
extern void (*race_hook)(fft *f, int size, int path);


#ifdef __cplusplus
}
#endif
