// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* pocketfft's real FFT, as benchees/pocketfft; the one malloc of its transform (rfftp_forward's) gets the plan's scratch */
#include "kernel/kernel.h"
#include "kernel/pow.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) && (defined(__unix__) || defined(__APPLE__) || defined(_WIN32) || defined(ESP_PLATFORM))
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL /* bare metal: one thread */
#endif

static THREAD_LOCAL float *scratch; /* the running transform's */
static THREAD_LOCAL size_t room;

static void *scratch_malloc(size_t bytes){
	if (scratch && bytes<=room){
		room = 0; /* lent once */
		return scratch;
	}
	return malloc(bytes);
}

static void scratch_free(void *p){
	if (p!=scratch){
		free(p);
	}
}

#define malloc scratch_malloc
#define free scratch_free
#include "pocketfft_single.c"
#undef malloc
#undef free

typedef struct plan {
	rffts_plan rfft;
	int size;
	float *scratch;
} plan;

static int can_do(int size){
	return size>0 && size%2==0;
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->rfft){
		destroy_rffts_plan(p->rfft);
	}
	free(p->scratch);
	free(p);
}

static void *make(int size, float *in, float *out){
	(void)in;
	(void)out;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->rfft = make_rffts_plan((size_t)size);
	p->scratch = malloc((size_t)size*sizeof(float));
	if (!p->rfft || !p->scratch){
		destroy(p);
		return NULL;
	}
	return p;
}

/* in place at out+1, r0 r1 i1 ... r(n/2), so moving r0 and zeroing two makes size/2+1 (re, im) pairs */
static void forward(void *p_, float *in, float *out){
	const plan *p = p_;
	int n = p->size;
	memcpy(out+1, in, (size_t)n*sizeof(float));
	scratch = p->scratch;
	room = (size_t)n*sizeof(float);
	int failed = rffts_forward(p->rfft, out+1, 1.0f); /* only if its malloc fails, which the scratch can't */
	(void)failed;
	scratch = NULL;
	out[0] = out[1];
	out[1] = 0;
	out[n+1] = 0;
}

const kernel k_pocketfft = {"pocketfft", can_do, make, forward, destroy, power_interleaved, bin_interleaved};
