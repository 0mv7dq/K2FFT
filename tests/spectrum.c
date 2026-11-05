// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#include "api/spectrum.h"
#include "api/fft.h"
#include "kernel/race.h"

#include <math.h>
#include <stdio.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)){ fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* gaussian window, lobe bins wide, return its sum */
static double gaussian(float *win, int npts, double lobe){
	double sigma = npts/(2*3.141592653589793*lobe), sum = 0;
	for (int i = 0; i<npts; i++){
		double x = (i-npts/2)/sigma;
		win[i] = (float)exp(-0.5*x*x);
		sum += (double)win[i];
	}
	return sum;
}

int main(void){
	const double pi = 3.141592653589793;
	static float win[1024], frame[1024];
	spectrum s;

	/* the sizes spectrum_init picks */
	CHECK(spectrum_init(&s, win, 1000, 0)==0 && s.size==1024, "npts 1000, size 0 -> %d (expect 1024, the only candidate)", s.size);
	spectrum_free(&s);
	CHECK(spectrum_init(&s, win, 1024, 2049)==0 && s.size>=2304 && s.size<=4096 && fft_next_size(s.size)==s.size, "npts 1024, size 2049 -> %d (expect a backend size in [2304, 4096])", s.size);
	int chosen = s.size;
	spectrum_free(&s);
	CHECK(spectrum_init(&s, win, 1024, 2049)==0 && s.size==chosen, "npts 1024, size 2049 again -> %d (expect %d, kept)", s.size, chosen);
	spectrum_free(&s);
	CHECK(race_size(20000)==fft_next_size(20000), "race_size(20000) -> %d (expect %d: above RACED_UP_TO, not timed)", race_size(20000), fft_next_size(20000)); /* allocates nothing, so safe on a microcontroller */

	/* a cosine on bin 100, phase 0.7 at the frame centre */
	const int npts = 1024;
	double sum = gaussian(win, npts, 1.27);
	CHECK(spectrum_init(&s, win, npts, 2048)==0 && s.size==2048 && s.nbins==1025, "init 1024/2048");
	const double phase = 0.7;
	for (int i = 0; i<npts; i++){
		frame[i] = (float)cos(2*pi*100*(i-npts/2)/2048+phase);
	}
	float total = spectrum_transform(&s, frame, 1);
	float re, im;
	spectrum_bin(&s, 100, &re, &im);
	double got = atan2(im, re), mag = sqrt((double)re*re+(double)im*im);
	CHECK(fabs(got-phase)<1e-4, "phase at the frame centre %.6f (expect %.6f)", got, phase);
	CHECK(fabs(mag-sum/2)<1e-3*sum, "bin 100 magnitude %.3f (expect sum(win)/2 = %.3f)", mag, sum/2);
	CHECK(fabs(s.power[100]-mag*mag)<1e-3*mag*mag, "power[100] %.1f vs |bin|^2 %.1f", s.power[100], mag*mag);
	double exact = 0;
	for (int k = 1; k<s.nbins; k++){
		exact += s.power[k];
	}
	CHECK(fabs(total-exact)<1e-5*exact, "sum %.3f vs %.3f", total, exact);

	/* the padding must still be zero */
	float again = spectrum_transform(&s, frame, 1);
	CHECK(again==total, "same frame twice gives the same sum (%.6f vs %.6f)", again, total);
	printf("size for 2049: %d; phase %.6f (expect 0.7), |bin 100| %.2f (expect %.2f), sum %.1f\n", chosen, got, mag, sum/2, total);

	spectrum_free(&s);
	printf(fails ? "%d checks FAILED\n" : "all checks passed\n", fails);
	return fails!=0;
}
