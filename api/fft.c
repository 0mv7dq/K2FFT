// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* api/fft.h, a real forward plan of kernel/planner.c's on arrays of its own */
#include "api/fft.h"
#include "kernel/planner.h"
#include "kernel/pow.h"
#include "kernel/race.h"
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

#ifndef K2FFT_LARGEST
#define K2FFT_LARGEST 65536
#endif
#define LARGEST_SIZE K2FFT_LARGEST /* the largest size the benchmark covers here (CMakeLists.txt), so every size is proven */

struct fft {
	int size;
	struct k2fft_plan_s *plan;
	float *in; /* size floats */
	float *out; /* size/2+1 (re, im) pairs */
};

/* PFFFT's sizes, so they don't change with the kernels */
int fft_next_size(int n){
	if (n>LARGEST_SIZE){
		return 0; /* also keeps n+31 from overflowing */
	}
	for (int m=(n>32 ? n+31 : 32)/32*32; m<=LARGEST_SIZE; m+=32){
		int r = m/32;
		while (r%2==0){
			r /= 2;
		}
		while (r%3==0){
			r /= 3;
		}
		while (r%5==0){
			r /= 5;
		}
		if (r==1){
			return m;
		}
	}
	return 0;
}

fft *fft_new(int size){
	if (fft_next_size(size)!=size){
		return NULL;
	}
	fft *f = calloc(1, sizeof(*f));
	if (!f){
		return NULL;
	}
	f->size = size;
	size_t skew = (1536-(size_t)size%1024)%1024; /* out half a page past in, mod 4 KB, so loads from in don't wait on stores to out (4K aliasing) */
	f->in = pffft_aligned_malloc((2*(size_t)size+skew+2)*sizeof(float));
	if (f->in){
		f->out = f->in+size+skew;
		const problem p = {.kind = KIND_R2C, .sign = K2FFT_FORWARD, .single = 1, .rank = 1, .dims = {{size, 1, 1}}, .in = f->in,
						   .out = f->out};
		f->plan = planner_plan(&p, K2FFT_MEASURE);
	}
	if (!f->plan){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float)); /* planning overwrites it */
	return f;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	planner_destroy(f->plan);
	pffft_aligned_free(f->in); /* out's too */
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	f->plan->execute(f->plan->plan, f->in, f->out);
}

float fft_power(const fft *f, float *power, int from){
	return power_interleaved(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	bin_interleaved(f->out, f->size, k, re, im);
}

racer fft_racer(fft *f){
	return (racer){f->plan->k, f->plan->plan, &f->plan->p};
}
