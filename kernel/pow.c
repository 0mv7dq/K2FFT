// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "pow.h"

/* eight partial sums, so each add doesn't wait for the one before */
static float sum_from(const float *power, int half, int from){
	float part[8] = {0};
	int k = from<0 ? 0 : from;
	for (; k+8<=half+1; k+=8){
		for (int j=0; j<8; j++){
			part[j] += power[k+j];
		}
	}
	float total = 0;
	for (; k<=half; k++){
		total += power[k];
	}
	for (int j=0; j<8; j++){
		total += part[j];
	}
	return total;
}

float power_interleaved(const float *bins, int size, float *power, int from){
	int half = size/2;
	for (int k=0; k<=half; k++){
		float re = bins[2*k];
		float im = bins[2*k+1];
		power[k] = re*re+im*im;
	}
	return sum_from(power, half, from);
}

void bin_interleaved(const float *bins, int size, int k, float *re, float *im){
	(void)size;
	*re = bins[2*k];
	*im = bins[2*k+1];
}
