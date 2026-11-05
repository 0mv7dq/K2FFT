// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/fft.h"

#include <math.h>
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

int backend_tests(void){
	BACKEND_CHECK(fft_next_size(1000)==1024 && fft_next_size(2049)==2304 && fft_next_size(33)==64, "fft_next_size: %d %d %d (expect 1024 2304 64)", fft_next_size(1000), fft_next_size(2049), fft_next_size(33));
	int sizes[] = {32, 1024, 2048, 2304, 4096}; /* a size for each backend path */
	for (int i = 0; i<5; i++){
		backend_check_size(sizes[i]);
	}
	printf(backend_fails ? "backend: %d checks FAILED\n" : "backend: all checks passed\n", backend_fails);
	return backend_fails!=0;
}

int main(void){
	return backend_tests();
}
