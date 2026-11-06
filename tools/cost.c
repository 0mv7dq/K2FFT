// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* hot_us back to back, busy_us after 256 KB of other writes */
#include "clock.h"
#include "tools.h"

static volatile float *junk;

static void evict(size_t bytes){
	for (size_t i = 0; i<bytes/sizeof(float); i += 8){
		junk[i] += 1.0f;
	}
}

static int cmpd(const void *a, const void *b){
	double x = *(const double *)a, y = *(const double *)b;
	return x<y ? -1 : x>y;
}

int main(int argc, char **argv){
	main_init(&argc, &argv);
	static int sizes[4096];
	int count = argc>1 ? read_sizes(argv[1], sizes, 4096) : 0;
	junk = calloc((256u<<10)/sizeof(float), sizeof(float));
	if (count==0 || !junk){
		fprintf(stderr, "usage: cost <sizes file>\n");
		return 2;
	}
	printf("# clock step %.4f us\n# size hot_us busy_us\n", tick_us());
	for (int s = 0; s<count; s++){
		bench_problem *p = rof_problem(sizes[s]);
		if (!p){
			continue;
		}
		setup(p);
		rof_input(p);
		doit(1, p);
		double t0 = now_us(), t[32];
		doit(4, p);
		double one = (now_us()-t0)/4;
		int reps = (int)(250.0/(one>0.01 ? one : 0.01))+1;
		double hot = 1e30;
		for (int r = 0; r<15; r++){
			t0 = now_us();
			doit(reps, p);
			double us = (now_us()-t0)/reps;
			hot = us<hot ? us : hot;
		}
		for (int k = 0; k<32; k++){
			evict(256u<<10);
			t0 = now_us();
			doit(1, p);
			t[k] = now_us()-t0;
		}
		qsort(t, 32, sizeof(double), cmpd);
		double busy = 0;
		for (int k = 0; k<24; k++){
			busy += t[k]/24;
		}
		printf("%6d %10.4f %10.4f\n", sizes[s], hot, busy);
		fflush(stdout);
		done(p);
		problem_destroy(p);
	}
	return 0;
}
