// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


struct fft;

/* sums over bins scale with size, a peak's |bin|^2 doesn't */
typedef struct spectrum {
	int npts; /* window length */
	int size; /* FFT size >= npts and the size asked for, can differ between machines and runs */
	int nbins; /* size/2+1 */
	float *win;
	float *power; /* |bin|^2 from spectrum_transform */
	struct fft *fft;
} spectrum;


/* copies win, allocates and may time transforms, thread safe */
int spectrum_init(spectrum *s, const float *win, int npts, int size); /* -1 on failure, spectrum_free either way */
void spectrum_free(spectrum *s); /* thread safe */

/* frame[npts/2] at the FFT's time zero, returns the sum of power[from..nbins), never allocates */
float spectrum_transform(spectrum *s, const float *frame, int from);

/* 0 <= k < nbins */
void spectrum_bin(const spectrum *s, int k, float *re, float *im);


#ifdef __cplusplus
}
#endif
