// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the planner, as FFTW's: kernel/table.h (the benchmark's wisdom) names each size's candidates, a race times them here */
#include "api/fft.h"
#include "kernel/kernel.h"
#include "kernel/race.h"
#include "kernel/table.h"
#include <pffft/pffft.h> /* aligned buffers */
#include <stdlib.h>
#include <string.h>

#ifndef K2FFT_LARGEST
#define K2FFT_LARGEST 65536
#endif
#define LARGEST_SIZE K2FFT_LARGEST /* the largest size the benchmark covers here (CMakeLists.txt), so every size is proven */
#define CANDIDATES 8 /* most a race times: the table's 4 (scripts/table), then kernels the benchmark never ran here */

struct fft {
	int size;
	const kernel *k;
	void *plan;
	float *in;
	float *out;
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

static const kernel *built(const char *name, size_t len){
	for (int i=0; kernels[i]; i++){
		if (strlen(kernels[i]->name)==len && strncmp(kernels[i]->name, name, len)==0){
			return kernels[i];
		}
	}
	return NULL;
}

static int listed(const kernel **c, int n, const kernel *k){
	for (int i=0; i<n; i++){
		if (c[i]==k){
			return 1;
		}
	}
	return 0;
}

/* whether the space-separated list names name */
static int named(const char *list, const char *name){
	size_t len = strlen(name);
	for (const char *s=list; *s;){
		size_t l = strcspn(s, " ");
		if (l==len && strncmp(s, name, len)==0){
			return 1;
		}
		s += l+(s[l]==' ');
	}
	return 0;
}

/* the table's codes for size that run here, fastest first, then every other kernel that can if a code can't run on this CPU or none is listed,
   then the kernels none of this architecture's machines ran (its -1 row), which only a race can rank */
static int candidates(int size, const kernel **c){
	int n = 0;
	int partial = 1;
	const char *ran = NULL;
#ifdef FORCED_KERNEL
	const kernel *forced = built(FORCED_KERNEL, strlen(FORCED_KERNEL)); /* a test build's, tests/CMakeLists.txt */
	if (forced && forced->can_do(size)){
		c[0] = forced;
		return 1;
	}
#endif
	for (int i=0; table[i].size; i++){
		if (table[i].size<0){
			ran = table[i].codes;
		}
		if (table[i].size!=size){
			continue;
		}
		partial = 0;
		for (const char *s=table[i].codes; *s && n<CANDIDATES;){
			size_t len = strcspn(s, " ");
			const kernel *k = built(s, len);
			if (k && k->can_do(size)){
				c[n++] = k;
			} else if (k){
				partial = 1; /* a CPU the benchmark didn't measure */
			}
			s += len+(s[len]==' ');
		}
	}
	for (int i=0; (partial || n==0) && kernels[i] && n<CANDIDATES; i++){
		if (!listed(c, n, kernels[i]) && kernels[i]->can_do(size)){
			c[n++] = kernels[i];
		}
	}
	for (int i=0; ran && kernels[i] && n<CANDIDATES; i++){
		if (!listed(c, n, kernels[i]) && !named(ran, kernels[i]->name) && kernels[i]->can_do(size)){
			c[n++] = kernels[i];
		}
	}
	return n;
}

/* f with k's plan, or NULL */
static fft *planned(const kernel *k, int size){
	fft *f = calloc(1, sizeof(*f));
	if (!f){
		return NULL;
	}
	f->size = size;
	f->k = k;
	f->in = pffft_aligned_malloc((size_t)size*sizeof(float));
	f->out = pffft_aligned_malloc(((size_t)size+2)*sizeof(float));
	if (f->in && f->out){
		f->plan = k->plan(size, f->in, f->out);
	}
	if (!f->plan){
		fft_free(f);
		return NULL;
	}
	memset(f->in, 0, (size_t)size*sizeof(float)); /* FFTW_MEASURE plans on it */
	return f;
}

fft *fft_new(int size){
	if (fft_next_size(size)!=size){
		return NULL;
	}
	const kernel *c[CANDIDATES];
	int m = candidates(size, c);
	int known = race_picked(size);
	if (known>0 && known<=m){ /* raced before: plans the winner alone */
		fft *f = planned(c[known-1], size);
		if (f){
			return f;
		}
	}
	fft *f[CANDIDATES];
	int n = 0;
	for (int i=0; i<m; i++){
		f[n] = planned(c[i], size);
		n += f[n]!=NULL;
	}
	if (n==0){
		return NULL;
	}
	int fastest = race_ffts(f, n, size);
	for (int i=0; i<n; i++){
		if (i!=fastest){
			fft_free(f[i]);
		}
	}
	return fastest>=0 ? f[fastest] : NULL;
}

void fft_free(fft *f){
	if (!f){
		return;
	}
	if (f->plan){
		f->k->destroy(f->plan);
	}
	pffft_aligned_free(f->in);
	pffft_aligned_free(f->out);
	free(f);
}

float *fft_input(fft *f){
	return f->in;
}

void fft_forward(fft *f){
	f->k->forward(f->plan, f->in, f->out);
}

float fft_power(const fft *f, float *power, int from){
	return f->k->power(f->out, f->size, power, from);
}

void fft_bin(const fft *f, int k, float *re, float *im){
	f->k->bin(f->out, f->size, k, re, im);
}
