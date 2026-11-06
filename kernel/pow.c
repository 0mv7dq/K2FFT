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

float power_packed(const float *bins, int size, float *power, int from){
	int half = size/2;
	power[0] = bins[0]*bins[0];
	power[half] = bins[1]*bins[1];
	for (int k=1; k<half; k++){
		float re = bins[2*k];
		float im = bins[2*k+1];
		power[k] = re*re+im*im;
	}
	return sum_from(power, half, from);
}

void bin_packed(const float *bins, int size, int k, float *re, float *im){
	if (k==0 || k==size/2){
		*re = bins[k==0 ? 0 : 1];
		*im = 0;
	} else {
		*re = bins[2*k];
		*im = bins[2*k+1];
	}
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

float power_split(const float *bins, int size, float *power, int from){
	int half = size/2;
	const float *im = bins+half+1;
	for (int k=0; k<=half; k++){
		power[k] = bins[k]*bins[k]+im[k]*im[k];
	}
	return sum_from(power, half, from);
}

void bin_split(const float *bins, int size, int k, float *re, float *im){
	*re = bins[k];
	*im = bins[size/2+1+k];
}
