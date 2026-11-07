// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* pocketfft: one 1D transform of floats, complex either way, real forward or back, as benchees/pocketfft; the one malloc of its
   transform (pass_all's, rfftp_forward's or rfftp_backward's) gets the plan's scratch */
#include "kernel/kernel.h"
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
	cffts_plan cfft;
	rffts_plan rfft;
	int size;
	problem_kind kind;
	int sign;
	float *scratch;
} plan;

/* sizes w/ pocketfft's direct passes alone, so a transform mallocs once: w/o Bluestein's, and, complex, w/o its generic pass's */
static int direct(size_t n, int complex){
	if (n>=50 && largest_prime_factor(n)>sqrt((double)n)){
		return 0;
	}
	while (complex && n%2==0){
		n /= 2;
	}
	for (size_t f=3; complex && f<=11; f+=2){
		while (n%f==0){
			n /= f;
		}
	}
	return !complex || n==1;
}

static int can_do(const problem *pb){
	const int size = problem_1d(pb);
	return pb->single && size>0 && pb->kind!=KIND_R2R && direct((size_t)size, pb->kind==KIND_DFT);
}

static void destroy(void *p_){
	plan *p = p_;
	if (p->cfft){
		destroy_cffts_plan(p->cfft);
	}
	if (p->rfft){
		destroy_rffts_plan(p->rfft);
	}
	free(p->scratch);
	free(p);
}

static void *make(const problem *pb){
	const int size = pb->dims[0].n, complex = pb->kind==KIND_DFT;
	plan *p = calloc(1, sizeof(*p));
	if (!p){
		return NULL;
	}
	p->size = size;
	p->kind = pb->kind;
	p->sign = pb->sign;
	if (complex){
		p->cfft = make_cffts_plan((size_t)size);
	} else {
		p->rfft = make_rffts_plan((size_t)size);
	}
	p->scratch = malloc((size_t)size*(complex ? 2 : 1)*sizeof(float));
	if (!(p->cfft || p->rfft) || !p->scratch){
		destroy(p);
		return NULL;
	}
	return p;
}

/* its halfcomplex r0 r1 i1 ... (r(n/2)) in place, from or to FFTW's (re, im) pairs; failing only if a malloc did, which the scratch
   can't */
static void execute(const void *p_, void *in_, void *out_){
	float *in = in_, *out = out_;
	const plan *p = p_;
	const int n = p->size;
	scratch = p->scratch;
	room = (size_t)n*(p->kind==KIND_DFT ? 2 : 1)*sizeof(float);
	int failed = 0;
	if (p->kind==KIND_DFT){
		if (out!=in){
			memcpy(out, in, 2*(size_t)n*sizeof(float));
		}
		failed = p->sign<0 ? cffts_forward(p->cfft, out, 1.0f) : cffts_backward(p->cfft, out, 1.0f);
	} else if (p->kind==KIND_R2C){
		memmove(out+1, in, (size_t)n*sizeof(float));
		failed = rffts_forward(p->rfft, out+1, 1.0f);
		out[0] = out[1];
		out[1] = 0;
		if (n%2==0){
			out[n+1] = 0;
		}
	} else {
		const float dc = in[0];
		memmove(out+1, in+2, (size_t)(n-1)*sizeof(float));
		out[0] = dc;
		failed = rffts_backward(p->rfft, out, 1.0f);
	}
	(void)failed;
	scratch = NULL;
}

const kernel k_pocketfft = {"pocketfft", can_do, make, execute, destroy};
