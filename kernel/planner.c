// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the planner, as FFTW's: kernel/table.h (the benchmark's wisdom) names each problem's candidates, a race times them here */
#include "kernel/planner.h"
#include "kernel/race.h"
#include "kernel/table.h"
#include <stdlib.h>
#include <string.h>

#define CANDIDATES 16 /* most a race times, more than a build's kernels: the table's 4 (scripts/table), then the rest */
#define KEPT 64

static struct {
	unsigned long hash; /* of the problem's name */
	const kernel *k; /* the race's winner */
} picked[KEPT];
static int oldest;

/* FNV-1a of a problem's class and tensor */
static unsigned long hash(const char *cls, const char *tensor){
	unsigned long h = 2166136261UL;
	for (const char *s=cls; *s; s++){
		h = (h^(unsigned char)*s)*16777619UL;
	}
	h = (h^'/')*16777619UL;
	for (const char *s=tensor; *s; s++){
		h = (h^(unsigned char)*s)*16777619UL;
	}
	return h ? h : 1;
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

/* the table's codes for p's name that run here, fastest first, then every other kernel that can if a code can't run on this CPU or none
   is listed, then those none of this architecture's machines ran p's class with (its "*" row), which only a race can rank, then where
   a race can run (a clock) the rest, as this CPU may not be one the benchmark measured (a Zen 3 where the table's is a Zen 4) */
static int candidates(const problem *p, const char *cls, const char *tensor, const kernel **c){
	int n = 0;
	int partial = 1;
	const char *ran = NULL;
#ifdef FORCED_KERNEL
	const kernel *forced = built(FORCED_KERNEL, strlen(FORCED_KERNEL)); /* a test build's, tests/CMakeLists.txt */
	if (forced && forced->can_do(p)){
		c[0] = forced;
		return 1;
	}
#endif
	for (int i=0; table[i].cls; i++){
		if (strcmp(table[i].cls, cls)!=0){
			continue;
		}
		if (strcmp(table[i].tensor, "*")==0){
			ran = table[i].codes;
			continue;
		}
		if (strcmp(table[i].tensor, tensor)!=0){
			continue;
		}
		partial = 0;
		for (const char *s=table[i].codes; *s && n<CANDIDATES;){
			size_t len = strcspn(s, " ");
			const kernel *k = built(s, len);
			if (k && k->can_do(p)){
				c[n++] = k;
			} else if (k){
				partial = 1; /* a CPU the benchmark didn't measure */
			}
			s += len+(s[len]==' ');
		}
	}
	for (int i=0; (partial || n==0) && kernels[i] && n<CANDIDATES; i++){
		if (!listed(c, n, kernels[i]) && kernels[i]->can_do(p)){
			c[n++] = kernels[i];
		}
	}
	for (int i=0; ran && kernels[i] && n<CANDIDATES; i++){
		if (!listed(c, n, kernels[i]) && !named(ran, kernels[i]->name) && kernels[i]->can_do(p)){
			c[n++] = kernels[i];
		}
	}
	for (int i=0, clocked=race_clocked(); clocked && kernels[i] && n<CANDIDATES; i++){
		if (!listed(c, n, kernels[i]) && kernels[i]->can_do(p)){
			c[n++] = kernels[i];
		}
	}
	return n;
}

/* k's plan for p, or NULL */
static struct k2fft_plan_s *planned(const kernel *k, const problem *p){
	struct k2fft_plan_s *x = calloc(1, sizeof(*x));
	if (x){
		x->k = k;
		x->p = *p;
		x->plan = k->plan(&x->p);
		if (!x->plan){
			free(x);
			x = NULL;
		}
	}
	return x;
}

static void keep(unsigned long h, const kernel *k){
	int i = 0;
	while (i<KEPT && picked[i].k){
		i++;
	}
	if (i==KEPT){
		i = oldest;
		oldest = (oldest+1)%KEPT;
	}
	picked[i].hash = h;
	picked[i].k = k;
}

struct k2fft_plan_s *planner_plan(const problem *p_, unsigned flags){
	problem p = *p_;
	char cls[8] = "", tensor[160] = "";
	if (!problem_name(&p, cls, sizeof(cls), tensor, sizeof(tensor))){
		tensor[0] = '\0'; /* no row matches: every kernel that can, raced */
	}
	const unsigned long h = hash(cls, tensor);
	const int timed = race_timing() && !(flags&K2FFT_ESTIMATE);
	p.measure = timed;
	race_lock();
	struct k2fft_plan_s *won = NULL;
	for (int i=0; i<KEPT && !won && picked[i].k; i++){ /* raced before: plans the winner alone */
		if (picked[i].hash==h && picked[i].k->can_do(&p)){
			won = planned(picked[i].k, &p);
		}
	}
	const kernel *c[CANDIDATES];
	const int m = won ? 0 : candidates(&p, cls, tensor, c);
	if (!won && !timed){ /* no race: the one K2FFT_NORACE names, else the first that plans; alone, as bare metal has little memory */
		const int forced = !(flags&K2FFT_ESTIMATE) && race_untimed(m)>0;
		for (int k=flags&K2FFT_ESTIMATE ? 0 : race_untimed(m); !won && k>=0 && k<m; k++){
			won = planned(c[k], &p);
			if (forced){
				break;
			}
		}
	} else if (!won){
		struct k2fft_plan_s *x[CANDIDATES];
		racer r[CANDIDATES];
		int n = 0;
		for (int i=0; i<m; i++){
			x[n] = planned(c[i], &p);
			if (x[n]){
				r[n] = (racer){x[n]->k, x[n]->plan, &x[n]->p};
				n++;
			}
		}
		if (n>0){
			problem_zero_input(&p);
			const int fastest = race_fastest(r, n);
			for (int i=0; i<n; i++){
				if (i!=fastest){
					planner_destroy(x[i]);
				}
			}
			won = x[fastest];
			keep(h, won->k);
		}
	}
	race_unlock();
	return won;
}

void planner_destroy(struct k2fft_plan_s *plan){
	if (plan){
		plan->k->destroy(plan->plan);
		free(plan);
	}
}
