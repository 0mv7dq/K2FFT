// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/* fft_power from DC, Nyquist, then (re, im) of bins 1..size/2-1 */
float power_packed(const float *bins, int size, float *power, int from);
void bin_packed(const float *bins, int size, int k, float *re, float *im);

/* fft_power from size/2+1 (re, im) pairs */
float power_interleaved(const float *bins, int size, float *power, int from);
void bin_interleaved(const float *bins, int size, int k, float *re, float *im);


#ifdef __cplusplus
}
#endif
