// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* every problem libbench2 poses, through api/k2fft.h as FFTW's tests/bench.c through fftw3.h: the basic, advanced or guru planner by
   the problem's shape; a FORCED_KERNEL build does only what its kernel does */
#include "benchee.h"
#include "libbench2/bench.h" /* problem_parse */
#include "api/k2fft.h"
#include "kernel/race.h"
#include <stdio.h>
#include <string.h>

#if defined(BENCHFFT_LDOUBLE) || defined(BENCHFFT_QUAD)
#error "K2FFT plans floats and doubles"
#elif defined(BENCHFFT_SINGLE)
#define K(name) k2fftf_##name
#else
#define K(name) k2fft_##name
#endif


BEGIN_BENCH_DOC
    BENCH_DOC("name", BENCHEE)
BENCH_DOC("package", "K2FFT")
END_BENCH_DOC


static K(plan) plan;
const racer *contestant; /* tests/hook.c's, run on the problem's arrays in plan's place */
static int paranoid;
void hook(const racer *r, int path);

static int *sizes(const bench_tensor *t){
	int *n = bench_malloc(sizeof(int)*(size_t)t->rnk);
	for (int i=0; i<t->rnk; i++){
		n[i] = t->dims[i].n;
	}
	return n;
}

static k2fft_iodim *iodims(const bench_tensor *t){
	if (t->rnk==0){
		return NULL;
	}
	k2fft_iodim *d = bench_malloc(sizeof(*d)*(size_t)t->rnk);
	for (int i=0; i<t->rnk; i++){
		d[i] = (k2fft_iodim){t->dims[i].n, t->dims[i].is, t->dims[i].os};
	}
	return d;
}

/* whether t's strides are its last's times the embeds, as the advanced interface's */
static int embedded(const bench_tensor *t){
	for (int i=t->rnk-2; i>=0; i--){
		const bench_iodim *d = &t->dims[i];
		if (!d[1].is || !d[1].os || d[0].is%d[1].is || d[0].os%d[1].os){
			return 0;
		}
	}
	return 1;
}

static void embeds(const bench_tensor *t, int **inembed, int **onembed){
	*inembed = bench_malloc(sizeof(int)*(size_t)t->rnk);
	*onembed = bench_malloc(sizeof(int)*(size_t)t->rnk);
	(*inembed)[0] = (*onembed)[0] = t->dims[0].n; /* unread */
	for (int i=t->rnk-2; i>=0; i--){
		(*inembed)[i+1] = t->dims[i].is/t->dims[i+1].is;
		(*onembed)[i+1] = t->dims[i].os/t->dims[i+1].os;
	}
}

static K(plan) basic(bench_problem *p, const k2fft_r2r_kind *k, unsigned flags){
	const bench_tensor *sz = p->sz;
	const int r2c = p->sign<0;
	if (sz->rnk==1){
		const int n = sz->dims[0].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return K(plan_dft_1d)(n, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? K(plan_dft_r2c_1d)(n, p->in, p->out, flags) : K(plan_dft_c2r_1d)(n, p->in, p->out, flags);
		default:
			return K(plan_r2r_1d)(n, p->in, p->out, k[0], flags);
		}
	}
	if (sz->rnk==2){
		const int n0 = sz->dims[0].n, n1 = sz->dims[1].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return K(plan_dft_2d)(n0, n1, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? K(plan_dft_r2c_2d)(n0, n1, p->in, p->out, flags) : K(plan_dft_c2r_2d)(n0, n1, p->in, p->out, flags);
		default:
			return K(plan_r2r_2d)(n0, n1, p->in, p->out, k[0], k[1], flags);
		}
	}
	if (sz->rnk==3){
		const int n0 = sz->dims[0].n, n1 = sz->dims[1].n, n2 = sz->dims[2].n;
		switch (p->kind){
		case PROBLEM_COMPLEX:
			return K(plan_dft_3d)(n0, n1, n2, p->in, p->out, p->sign, flags);
		case PROBLEM_REAL:
			return r2c ? K(plan_dft_r2c_3d)(n0, n1, n2, p->in, p->out, flags) : K(plan_dft_c2r_3d)(n0, n1, n2, p->in, p->out, flags);
		default:
			return K(plan_r2r_3d)(n0, n1, n2, p->in, p->out, k[0], k[1], k[2], flags);
		}
	}
	int *n = sizes(sz);
	K(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = K(plan_dft)(sz->rnk, n, p->in, p->out, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = r2c ? K(plan_dft_r2c)(sz->rnk, n, p->in, p->out, flags) : K(plan_dft_c2r)(sz->rnk, n, p->in, p->out, flags);
		break;
	default:
		x = K(plan_r2r)(sz->rnk, n, p->in, p->out, k, flags);
		break;
	}
	bench_free(n);
	return x;
}

static K(plan) advanced(bench_problem *p, const k2fft_r2r_kind *k, unsigned flags){
	const bench_tensor *sz = p->sz;
	const bench_iodim *v = &p->vecsz->dims[0], *last = &sz->dims[sz->rnk-1];
	int *n = sizes(sz), *ie, *oe;
	embeds(sz, &ie, &oe);
	K(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = K(plan_many_dft)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = p->sign<0 ? K(plan_many_dft_r2c)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, flags)
					  : K(plan_many_dft_c2r)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, flags);
		break;
	default:
		x = K(plan_many_r2r)(sz->rnk, n, v->n, p->in, ie, last->is, v->is, p->out, oe, last->os, v->os, k, flags);
		break;
	}
	bench_free(n);
	bench_free(ie);
	bench_free(oe);
	return x;
}

static K(plan) guru(bench_problem *p, const k2fft_r2r_kind *k, unsigned flags){
	const int rank = p->sz->rnk, vrank = p->vecsz->rnk;
	k2fft_iodim *d = iodims(p->sz), *v = iodims(p->vecsz);
	K(plan) x;
	switch (p->kind){
	case PROBLEM_COMPLEX:
		x = K(plan_guru_dft)(rank, d, vrank, v, p->in, p->out, p->sign, flags);
		break;
	case PROBLEM_REAL:
		x = p->sign<0 ? K(plan_guru_dft_r2c)(rank, d, vrank, v, p->in, p->out, flags)
					  : K(plan_guru_dft_c2r)(rank, d, vrank, v, p->in, p->out, flags);
		break;
	default:
		x = K(plan_guru_r2r)(rank, d, vrank, v, p->in, p->out, k, flags);
		break;
	}
	bench_free0(d);
	bench_free0(v);
	return x;
}

/* as FFTW's tests: a multidimensional c2r out of place may destroy its input */
static unsigned preserving(bench_problem *p){
	if (p->kind==PROBLEM_REAL && p->sign>0 && !p->in_place && p->sz->rnk>1){
		p->destroy_input = 1;
	}
	return p->destroy_input ? 0 : K2FFT_PRESERVE_INPUT;
}

/* the planner FFTW's tests/bench.c picks: the basic for a contiguous row-major transform, the advanced for one batch dimension */
static K(plan) mkplan(bench_problem *p, unsigned flags){
	const bench_tensor *sz = p->sz, *v = p->vecsz;
	if (p->split || !BENCH_FINITE_RNK(sz->rnk) || !BENCH_FINITE_RNK(v->rnk) || sz->rnk<1){
		return NULL; /* K2FFT's arrays are interleaved; a rank 0 one is a copy */
	}
	k2fft_r2r_kind *k = NULL;
	if (p->kind==PROBLEM_R2R){
		k = bench_malloc(sizeof(*k)*(size_t)sz->rnk);
		for (int i=0; i<sz->rnk; i++){
			k[i] = (k2fft_r2r_kind)p->k[i]; /* r2r_kind_t's FFTW's order too */
		}
	}
	const int rowmajor = p->kind==PROBLEM_REAL ? tensor_real_rowmajorp(p->sz, p->sign, p->in_place) : tensor_rowmajorp(p->sz);
	K(plan) x;
	if (v->rnk==0 && tensor_unitstridep(p->sz) && rowmajor){
		x = basic(p, k, flags);
	} else if (v->rnk==1 && embedded(sz)){
		x = advanced(p, k, flags);
	} else {
		x = guru(p, k, flags);
	}
	bench_free0(k);
	return x;
}

/* untimed, on a copy of p's own: libbench2 asks before it allocates p's arrays, which planning may write */
int can_do(bench_problem *p){
	bench_problem *q = problem_parse(p->pstring);
	problem_alloc(q);
	K(plan) x = mkplan(q, preserving(q)|K2FFT_ESTIMATE);
	int ok = x!=NULL;
#ifdef FORCED_KERNEL
	ok = ok && strcmp(K(plan_kernel)(x), FORCED_KERNEL)==0;
#endif
	K(destroy_plan)(x);
	problem_destroy(q);
	return ok;
}

void setup(bench_problem *p){
	if (paranoid && rof_size(p)){
		race_size(rof_size(p)); /* so the hook also checks the sizes spectrum_init races */
	}
	plan = mkplan(p, preserving(p)|K2FFT_MEASURE);
	BENCH_ASSERT(plan);
}

void doit(int iter, bench_problem *p){
	if (contestant){ /* on p's arrays, as k2fft_execute_dft and the others allow */
		for (int i=0; i<iter; i++){
			contestant->k->execute(contestant->plan, p->in, p->out);
		}
		return;
	}
	for (int i=0; i<iter; i++){
		K(execute)(plan);
	}
}

void done(bench_problem *p){
	UNUSED(p);
	K(destroy_plan)(plan);
	plan = NULL;
}

void useropt(const char *arg){
	if (strcmp(arg, "paranoid")==0){
		paranoid = 1;
		race_hook = hook;
	} else if (strcmp(arg, "estimate")!=0){ /* scripts/benchmark's, for FFTW under an emulator, where K2FFT_NORACE plans untimed */
		fprintf(stderr, "unknown user option: %s.  Ignoring.\n", arg);
	}
}

void main_init(int *argc, char ***argv){
	UNUSED(argc);
	UNUSED(argv);
}

void cleanup(void){}
