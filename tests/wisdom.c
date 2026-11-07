// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* api/k2fft.h's wisdom: the races' picks come back after k2fftf_forget_wisdom, from its text and from a file, and the wisdom another
   version, kernel set or CPU wrote is refused */
#include "api/k2fft.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)){ fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static const char *plan(int n, float *in, float *out, unsigned flags){
	k2fftf_plan p = k2fftf_plan_dft_r2c_1d(n, in, (k2fftf_complex *)out, flags);
	const char *k = k2fftf_plan_kernel(p);
	k2fftf_destroy_plan(p);
	return k;
}

int main(void){
	static const int sizes[] = {256, 1000, 4096};
	const char *picked[3];
	char *room = malloc((2*4096+32)*sizeof(float)+64);
	if (!room){
		return 1;
	}
	float *in = (float *)(((uintptr_t)room+63)&~(uintptr_t)63), *out = in+4096+16; /* both 64-byte aligned, as libbench2's */
	for (int i=0; i<3; i++){
		picked[i] = plan(sizes[i], in, out, K2FFT_MEASURE);
		CHECK(picked[i]!=NULL, "%d: no plan", sizes[i]);
	}
	char *w = k2fftf_export_wisdom_to_string();
	CHECK(w!=NULL, "no wisdom");
	if (!w){
		return 1;
	}
	printf("%s", w);
	const int raced = strstr(w, "(pick ")!=NULL; /* none on bare metal, which has no clock */
	k2fftf_forget_wisdom();
	CHECK(k2fft_import_wisdom_from_string(w)==1, "its own wisdom refused");
	for (int i=0; raced && i<3; i++){
		const char *k = plan(sizes[i], in, out, K2FFT_ESTIMATE);
		CHECK(k && picked[i] && strcmp(k, picked[i])==0, "%d: %s after wisdom, %s raced", sizes[i], k ? k : "none", picked[i]);
	}
	char *other = malloc(strlen(w)+2);
	if (other){
		const char *at = strchr(w, ' ');
		snprintf(other, strlen(w)+2, "%.*sx%s", (int)(at-w), w, at); /* another version's */
		CHECK(k2fftf_import_wisdom_from_string(other)==0, "another version's wisdom taken");
		free(other);
	}
	const char *path = "k2fft-wisdom-check.txt";
	if (k2fftf_export_wisdom_to_filename(path)){ /* where there are files */
		k2fftf_forget_wisdom();
		CHECK(k2fftf_import_wisdom_from_filename(path)==1, "its own wisdom file refused");
		remove(path);
		for (int i=0; raced && i<3; i++){
			const char *k = plan(sizes[i], in, out, K2FFT_ESTIMATE);
			CHECK(k && picked[i] && strcmp(k, picked[i])==0, "%d: %s after the file, %s raced", sizes[i], k ? k : "none", picked[i]);
		}
	}
	free(w);
	free(room);
	printf(fails ? "wisdom: %d checks FAILED\n" : "wisdom: all checks passed\n", fails);
	return fails!=0;
}
