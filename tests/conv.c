// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* api/k2fft.h's convolutions: x and h, n/2 samples each zero-padded to n, through k2fftf_conv_forward, k2fftf_conv_mac and
   k2fftf_conv_backward, against their direct linear convolution; PFFFT's own spectra where it can (256, 4096), the real forward and
   backward plans' where not (1000) */
#include "api/k2fft.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)){ fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void check(int n){
	const int b = n/2;
	char *room = malloc((size_t)(5*(n+16))*sizeof(float)+64);
	if (!room){
		fails++;
		return;
	}
	float *x = (float *)(((uintptr_t)room+63)&~(uintptr_t)63), *h = x+n+16, *xs = h+n+16, *hs = xs+n+16, *y = hs+n+16;
	k2fftf_conv c = k2fftf_plan_conv(n, x, xs, y, K2FFT_MEASURE);
	CHECK(c!=NULL, "%d: no plan", n);
	if (c){
		for (int i = 0; i<n; i++){
			x[i] = i<b ? (float)((i*7919)%1000)/1000.0f-0.5f : 0.0f;
			h[i] = i<b ? (float)((i*104729)%997)/997.0f-0.5f : 0.0f;
		}
		k2fftf_conv_forward(c, x, xs);
		k2fftf_conv_forward(c, h, hs);
		float *acc = malloc((size_t)(n+2)*sizeof(float)+64), *a = acc ? (float *)(((uintptr_t)acc+63)&~(uintptr_t)63) : NULL;
		CHECK(a!=NULL, "%d: no room", n);
		if (a){
			for (int i = 0; i<n+2; i++){
				a[i] = 0;
			}
			k2fftf_conv_mac(c, xs, hs, a, 1.0f/(float)n);
			k2fftf_conv_backward(c, a, y);
			double worst = 0, peak = 0;
			for (int i = 0; i<n-1; i++){
				double d = 0;
				for (int j = 0; j<=i; j++){
					d += (j<b && i-j<b) ? (double)x[j]*h[i-j] : 0;
				}
				worst = fabs(y[i]-d)>worst ? fabs(y[i]-d) : worst;
				peak = fabs(d)>peak ? fabs(d) : peak;
			}
			CHECK(worst<1e-4*peak, "%d, %s: worst error %.2e of the largest", n, k2fftf_conv_kernel(c), worst/peak);
			printf("size %4d: %s, worst error %.1e of the largest\n", n, k2fftf_conv_kernel(c), worst/peak);
			free(acc);
		}
		k2fftf_destroy_conv(c);
	}
	free(room);
}

int main(void){
	check(256);
	check(1000);
	check(4096);
	printf(fails ? "conv: %d checks FAILED\n" : "conv: all checks passed\n", fails);
	return fails!=0;
}
