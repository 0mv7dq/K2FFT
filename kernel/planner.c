// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the planner, as FFTW's: kernel/table.h (the benchmark's wisdom) names each problem's candidates, a race times them here */
#include "kernel/planner.h"
#include "kernel/race.h"
#include "kernel/table.h"
#include "simd-support/simd.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef K2FFT_VERSION
#define K2FFT_VERSION "0" /* CMake's project version */
#endif
#define CANDIDATES 16 /* most a race times, more than a build's kernels: the table's 4 (scripts/table), then the rest */
#define KEPT 64
#define NAME 48 /* a problem's class and tensor, as wisdom names them; longer ones are kept, not exported */
#define IDENTITY 640

static struct {
	unsigned long hash; /* of the problem's name */
	const kernel *k; /* the race's winner */
	char name[NAME]; /* "srof 1024", "" if too long */
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

/* list's codes that run here into c, fastest first; 0 if one built can't do p, as on a CPU the benchmark didn't measure */
static int add(const char *list, const problem *p, const kernel **c, int *n){
	int all = 1;
	for (const char *s=list; *s && *n<CANDIDATES;){
		size_t len = strcspn(s, " ");
		const kernel *k = built(s, len);
		if (k && k->can_do(p)){
			if (!listed(c, *n, k)){
				c[(*n)++] = k;
			}
		} else if (k){
			all = 0;
		}
		s += len+(s[len]==' ');
	}
	return all;
}

/* which of 3, 5 and larger primes divide n */
static int factors(long n){
	static const int q[] = {3, 5};
	int f = 0;
	while (n%2==0){
		n /= 2;
	}
	for (int i=0; i<2; i++){
		f |= n%q[i]==0 ? 1<<i : 0;
		while (n%q[i]==0){
			n /= q[i];
		}
	}
	return n>1 ? f|4 : f;
}

/* the codes of cls's row nearest tensor's size, of a tensor alike past it (its kind, batch, other sizes) and of the same factors if
   one is: the benchmark's order for a size it didn't measure, as FFTW's estimator stands in for timing; NULL if none */
static const char *nearest(const char *cls, const char *tensor){
	char *rest, *end;
	const long n = strtol(tensor, &rest, 10);
	const char *best = NULL;
	double far = 0;
	for (int i=0; n>0 && table[i].cls; i++){
		const long m = strtol(table[i].tensor, &end, 10);
		if (m<=0 || strcmp(end, rest)!=0 || strcmp(table[i].cls, cls)!=0){
			continue;
		}
		const double d = (m>n ? (double)m/n : (double)n/m)*(factors(m)==factors(n) ? 1 : 1e9);
		if (!best || d<far){
			best = table[i].codes;
			far = d;
		}
	}
	return best;
}

/* the table's codes for p's name that run here, fastest first, else its nearest measured size's, then every other kernel that can if
   a code can't run on this CPU or none is listed, then those none of this architecture's machines ran p's class with (its "*" row),
   which only a race can rank, then where a race can run (a clock) the rest, as this CPU may not be one the benchmark measured (a Zen 3
   where the table's is a Zen 4) */
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
		if (strcmp(table[i].tensor, tensor)==0){
			partial = !add(table[i].codes, p, c, &n);
		}
	}
	const char *near = partial && n==0 ? nearest(cls, tensor) : NULL;
	if (near){
		add(near, p, c, &n); /* partial still: the rest follow, for a race to rank */
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

static void keep(unsigned long h, const kernel *k, const char *cls, const char *tensor){
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
	if (snprintf(picked[i].name, NAME, "%s %s", cls, tensor)>=NAME){
		picked[i].name[0] = '\0';
	}
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
			keep(h, won->k, cls, tensor);
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

/* s's used bytes after fmt's, short of room */
static size_t put(char *s, size_t room, size_t used, const char *fmt, ...){
	if (used+1>=room){
		return used;
	}
	va_list a;
	va_start(a, fmt);
	const int w = vsnprintf(s+used, room-used, fmt, a);
	va_end(a);
	return w<0 ? used : (used+(size_t)w<room ? used+(size_t)w : room-1);
}

/* wisdom's first line: as FFTW's TODO warns, it's this version's alone, its kernels' and its CPU's (what SIMD it runs, its model) */
static void identity(char *s, size_t room){
	char cpu[128];
	simd_cpu(cpu, sizeof(cpu));
	size_t used = put(s, room, 0, "(k2fft-wisdom %s ", K2FFT_VERSION);
	for (int i=0; kernels[i]; i++){
		used = put(s, room, used, "%s%s", i ? "," : "", kernels[i]->name);
	}
	put(s, room, used, " %d%d%d %s", simd_avx2_fma(), simd_neon(), simd_rvv(), cpu);
}

char *planner_wisdom(void){
	const size_t room = IDENTITY+KEPT*(NAME+48)+KEPT*32+8;
	char *s = malloc(room);
	if (!s){
		return NULL;
	}
	race_lock();
	identity(s, room);
	size_t used = put(s, room, strlen(s), "\n");
	for (int i=0; i<KEPT && picked[i].k; i++){
		if (picked[i].name[0]){
			used = put(s, room, used, "(pick %s %s)\n", picked[i].name, picked[i].k->name);
		}
	}
	int n, chosen;
	for (int i=0; race_known(i, &n, &chosen); i++){
		used = put(s, room, used, "(size %d %d)\n", n, chosen);
	}
	put(s, room, used, ")\n");
	race_unlock();
	return s;
}

int planner_learn(const char *text){
	char id[IDENTITY];
	identity(id, sizeof(id));
	const size_t len = strlen(id);
	if (!text || strncmp(text, id, len)!=0 || (text[len]!='\n' && text[len]!='\r')){
		return 0;
	}
	race_lock();
	for (const char *s=strchr(text, '\n'); s; s=strchr(s+1, '\n')){
		char cls[8], tensor[NAME], name[32];
		int n, chosen;
		if (sscanf(s+1, "(pick %7s %47s %31[^)\r\n])", cls, tensor, name)==3){
			const kernel *k = built(name, strlen(name));
			const unsigned long h = hash(cls, tensor);
			int known = 0;
			for (int i=0; i<KEPT && picked[i].k && !known; i++){
				known = picked[i].hash==h;
			}
			if (k && !known){
				keep(h, k, cls, tensor);
			}
		} else if (sscanf(s+1, "(size %d %d)", &n, &chosen)==2){
			race_learn(n, chosen);
		}
	}
	race_unlock();
	return 1;
}

void planner_forget(void){
	race_lock();
	memset(picked, 0, sizeof(picked));
	oldest = 0;
	race_forget();
	race_unlock();
}
