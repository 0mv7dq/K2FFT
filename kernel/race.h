// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#include "api/fft.h"

#ifdef __cplusplus
extern "C" {
#endif


/* whichever of use(f, 0) and use(f, 1) runs faster, timed once per size, allocates */
int race_paths(fft *f, int size, void (*use)(fft *f, int path));

/* fastest nearby backend size >= n, timed once, allocates */
int race_size(int n); /* the smallest if n > RACED_UP_TO, 0 if none */

/* serialises race_size, fft_new and fft_free, held by spectrum_init and spectrum_free */
void race_lock(void);
void race_unlock(void);

/* tests only, called on each contestant before it is timed (path -1 in race_size), must leave f's input as it was */
extern void (*race_hook)(fft *f, int size, int path);


#ifdef __cplusplus
}
#endif
