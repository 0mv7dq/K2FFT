// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"
#include "api/k2fft.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int backend_fails = 0;
#define BACKEND_CHECK(c, ...) do { if (!(c)){ backend_fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void backend_check_size(int n){
	const double pi = 3.141592653589793;
	fft *f = fft_new(n);
	BACKEND_CHECK(f!=NULL, "fft_new(%d) failed", n);
	if (!f){
		return;
	}
	float *in = fft_input(f);
	for (int i = 0; i<n; i++){
		in[i] = (float)((i*7919)%1000)/1000.0f-0.5f+0.25f*(float)cos(2*pi*37.3*i/n);
	}
	float *copy = malloc((size_t)n*sizeof(float));
	double *c = malloc((size_t)n*sizeof(double));
	double *s = malloc((size_t)n*sizeof(double));
	float *power = malloc(((size_t)n/2+1)*sizeof(float));
	memcpy(copy, in, (size_t)n*sizeof(float));
	for (int i = 0; i<n; i++){
		c[i] = cos(2*pi*i/n);
		s[i] = sin(2*pi*i/n);
	}
	fft_forward(f);
	fft_forward(f); /* the second call must see the same input */
	BACKEND_CHECK(memcmp(copy, in, (size_t)n*sizeof(float))==0, "%d: the transform changed its input", n);
	float total = fft_power(f, power, 1);
	double worst = 0, peak = 0, worst_power = 0, sum = 0;
	for (int k = 0; k<=n/2; k++){
		double re = 0, im = 0;
		int idx = 0;
		for (int i = 0; i<n; i++){
			re += copy[i]*c[idx];
			im -= copy[i]*s[idx];
			idx += k;
			if (idx>=n){
				idx -= n;
			}
		}
		float fr, fi;
		fft_bin(f, k, &fr, &fi);
		double e = hypot(fr-re, fi-im), m = hypot(re, im), p = (double)fr*fr+(double)fi*fi;
		worst = e>worst ? e : worst;
		peak = m>peak ? m : peak;
		worst_power = fabs(power[k]-p)>worst_power ? fabs(power[k]-p) : worst_power;
		sum += k>=1 ? power[k] : 0;
	}
	BACKEND_CHECK(worst<1e-5*peak, "%d: worst bin error %.2e of the largest bin (limit 1e-5)", n, worst/peak);
	BACKEND_CHECK(worst_power<1e-5*peak*peak, "%d: power[k] differs from |bin k|^2 by %.2e of the peak power", n, worst_power/(peak*peak));
	BACKEND_CHECK(fabs(total-sum)<1e-5*sum, "%d: fft_power's sum %.6g vs %.6g", n, total, sum);
	printf("size %4d: worst bin error %.1e of the largest bin, power sum %.6g\n", n, worst/peak, total);
	free(copy);
	free(c);
	free(s);
	free(power);
	fft_free(f);
}

/* in and out 8 bytes past 64-byte boundaries, as some mallocs give: every kernel the race times takes them or declines, and the bins
   are the DFT's */
static void backend_check_unaligned(int n){
	const double pi = 3.141592653589793;
	char *room = malloc((size_t)(2*n+32)*sizeof(float)+64);
	BACKEND_CHECK(room!=NULL, "%d: no room", n);
	if (!room){
		return;
	}
	float *a = (float *)(((uintptr_t)room+63)&~(uintptr_t)63), *in = a+2, *out = a+n+18;
	k2fftf_plan p = k2fftf_plan_dft_r2c_1d(n, in, (k2fftf_complex *)out, K2FFT_MEASURE);
	if (!p){ /* none here takes them, as bare metal's PFFFT alone */
		printf("size %4d unaligned: declined\n", n);
	} else {
		for (int i = 0; i<n; i++){
			in[i] = (float)((i*7919)%1000)/1000.0f-0.5f;
		}
		k2fftf_execute(p);
		double worst = 0, peak = 0;
		for (int k = 0; k<=n/2; k++){
			double re = 0, im = 0;
			for (int i = 0; i<n; i++){
				re += in[i]*cos(2*pi*(double)i*k/n);
				im -= in[i]*sin(2*pi*(double)i*k/n);
			}
			const double e = hypot(out[2*k]-re, out[2*k+1]-im), m = hypot(re, im);
			worst = e>worst ? e : worst;
			peak = m>peak ? m : peak;
		}
		BACKEND_CHECK(worst<1e-5*peak, "%d unaligned, %s: worst bin error %.2e of the largest", n, k2fftf_plan_kernel(p), worst/peak);
		printf("size %4d unaligned: %s, worst bin error %.1e of the largest bin\n", n, k2fftf_plan_kernel(p), worst/peak);
		k2fftf_destroy_plan(p);
	}
	free(room);
}

/* K2FFT_HALF_ZERO: a zero-padded block's plan (rdft/hwyfft.cpp's split4h), its bins the DFT's */
static void backend_check_half(int n){
	const double pi = 3.141592653589793;
	char *room = malloc((size_t)(2*n+32)*sizeof(float)+64);
	BACKEND_CHECK(room!=NULL, "%d: no room", n);
	if (!room){
		return;
	}
	float *in = (float *)(((uintptr_t)room+63)&~(uintptr_t)63), *out = in+n+16;
	k2fftf_plan p = k2fftf_plan_dft_r2c_1d(n, in, (k2fftf_complex *)out, K2FFT_MEASURE|K2FFT_HALF_ZERO);
	BACKEND_CHECK(p!=NULL, "%d: no K2FFT_HALF_ZERO plan", n);
	if (p){
		for (int i = 0; i<n; i++){
			in[i] = 2*i<n ? (float)((i*7919)%1000)/1000.0f-0.5f : 0.0f;
		}
		k2fftf_execute(p);
		double worst = 0, peak = 0;
		for (int k = 0; k<=n/2; k++){
			double re = 0, im = 0;
			for (int i = 0; 2*i<n; i++){
				re += in[i]*cos(2*pi*(double)i*k/n);
				im -= in[i]*sin(2*pi*(double)i*k/n);
			}
			const double e = hypot(out[2*k]-re, out[2*k+1]-im), m = hypot(re, im);
			worst = e>worst ? e : worst;
			peak = m>peak ? m : peak;
		}
		BACKEND_CHECK(worst<1e-5*peak, "%d half zero, %s: worst bin error %.2e of the largest", n, k2fftf_plan_kernel(p), worst/peak);
		printf("size %4d half zero: %s, worst bin error %.1e of the largest bin\n", n, k2fftf_plan_kernel(p), worst/peak);
		k2fftf_destroy_plan(p);
	}
	free(room);
}

int backend_tests(void){
	BACKEND_CHECK(fft_next_size(1000)==1024 && fft_next_size(2049)==2304 && fft_next_size(33)==64, "fft_next_size: %d %d %d (expect 1024 2304 64)", fft_next_size(1000), fft_next_size(2049), fft_next_size(33));
	int sizes[] = {32, 1024, 2048, 2304, 4096}; /* the smallest, powers of two and a 3-smooth size, so every kernel's sizes */
	for (int i = 0; i<5; i++){
		backend_check_size(sizes[i]);
	}
	for (int i = 0; i<3; i++){
		backend_check_unaligned(sizes[i+1]);
		backend_check_half(sizes[i+1]);
	}
	printf(backend_fails ? "backend: %d checks FAILED\n" : "backend: all checks passed\n", backend_fails);
	return backend_fails!=0;
}

int main(void){
	return backend_tests();
}
