// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#define _POSIX_C_SOURCE 199309L /* clock_gettime in clock.h */
/* usage: init_cost [n ...]   (default 1000 1500 2049 3000 5000 9000 16000) */
/* the first spectrum_init races, the second reuses its answers */
#include "clock.h"
#include "api/spectrum.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
	int ns[32] = {1000, 1500, 2049, 3000, 5000, 9000, 16000}, count = 7;
	if (argc>1){
		count = 0;
		for (int a = 1; a<argc && count<32; a++){
			ns[count++] = atoi(argv[a]);
		}
	}
	static float win[16384];
	for (int i = 0; i<16384; i++){
		win[i] = 1;
	}
	for (int k = 0; k<count; k++){
		int n = ns[k], npts = n<1024 ? n : 1024;
		spectrum s;
		double t0 = now_us();
		int r1 = spectrum_init(&s, win, npts, n);
		double first = now_us()-t0;
		int size = s.size;
		spectrum_free(&s);
		t0 = now_us();
		int r2 = spectrum_init(&s, win, npts, n);
		double again = now_us()-t0;
		spectrum_free(&s);
		printf("n %5d -> size %5d: first spectrum_init %8.2f ms, again %6.3f ms%s\n", n, size, first/1000, again/1000, r1 || r2 ? " (FAILED)" : "");
	}
	return 0;
}
