// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/* real forward FFT of a fixed size */
typedef struct fft fft;


/* smallest size >= n the backend can transform, 0 if none */
int fft_next_size(int n);

/* allocates and may time transforms, main thread only */
fft *fft_new(int size); /* size from fft_next_size, NULL if out of memory */
void fft_free(fft *f); /* main thread only */

/* size floats, zeroed by fft_new, unchanged by fft_forward */
float *fft_input(fft *f);

/* never allocates */
void fft_forward(fft *f);

/* power[0..size/2] = |bin|^2, returns the sum of power[from..size/2] */
float fft_power(const fft *f, float *power, int from);

/* 0 <= k <= size/2 */
void fft_bin(const fft *f, int k, float *re, float *im);


#ifdef __cplusplus
}
#endif
