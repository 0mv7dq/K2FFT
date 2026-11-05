// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#define _POSIX_C_SOURCE 199309L /* clock_gettime in clock.h */
/* usage: size_race [from] [to] */
/* each size stands for every n that rounds up to it */
#include "clock.h"
#include "api/fft.h"
#include "kernel/race.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
	int from = argc>1 ? atoi(argv[1]) : 64, to = argc>2 ? atoi(argv[2]) : 16384;
	printf("# clock step %.4f us\n# n chosen ms (sizes raced by fft_new in an earlier line are kept, so later lines can be quicker than a fresh process)\n", tick_us());
	for (int n = fft_next_size(from); n>0 && n<=to; n = fft_next_size(n+1)){
		double t0 = now_us();
		int s = race_size(n);
		printf("%6d %6d %9.2f\n", n, s, (now_us()-t0)/1000);
		fflush(stdout);
	}
	return 0;
}
