// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "spectrum.h"
#include "fft.h"
#include "kernel/race.h"
#include <stdlib.h>
#include <string.h>

int spectrum_init(spectrum *s, const float *win, int npts, int size){
	memset(s, 0, sizeof(*s));
	int n = size>npts ? size : npts;
	race_lock();
	s->size = race_size(n);
	if (s->size>=n){ /* 0 if no size fits */
		s->fft = fft_new(s->size);
	}
	race_unlock();
	if (!s->fft){
		return -1;
	}
	s->npts = npts;
	s->nbins = s->size/2+1;
	s->win = malloc((size_t)npts*sizeof(float));
	s->power = malloc((size_t)s->nbins*sizeof(float));
	if (!s->win || !s->power){
		return -1;
	}
	memcpy(s->win, win, (size_t)npts*sizeof(float));
	return 0;
}

void spectrum_free(spectrum *s){
	race_lock();
	fft_free(s->fft);
	race_unlock();
	free(s->win);
	free(s->power);
	memset(s, 0, sizeof(*s));
}

float spectrum_transform(spectrum *s, const float *frame, int from){
	float *in = fft_input(s->fft);
	int half = s->npts/2;
	for (int i=half; i<s->npts; i++){
		in[i-half] = s->win[i]*frame[i];
	}
	for (int i=0; i<half; i++){
		in[s->size-half+i] = s->win[i]*frame[i];
	}
	fft_forward(s->fft);
	return fft_power(s->fft, s->power, from);
}

void spectrum_bin(const spectrum *s, int k, float *re, float *im){
	fft_bin(s->fft, k, re, im);
}
