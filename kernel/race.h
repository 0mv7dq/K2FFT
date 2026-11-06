// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "api/fft.h"

#ifdef __cplusplus
extern "C" {
#endif


/* the fastest of n transforms of one size, the first on a tie, timed once per size, allocates */
int race_ffts(fft **f, int n, int size); /* the first w/o a clock or with PUREFFT_NORACE set, as emulators' time isn't the target's */

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
