// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#define _XOPEN_SOURCE 700 /* clock_gettime, pthreads' recursive mutexes */
#include <time.h>
#endif
#include "race.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(__APPLE__) || defined(CLOCK_MONOTONIC)
#define CLOCKED 1
#else
#define CLOCKED 0 /* no monotonic clock, as on bare metal */
#endif
#if CLOCKED && !defined(_WIN32)
#include <pthread.h>
#endif

#define KEPT 64
#define ROUND_US 100 /* shortest idle round, above clock resolution */
#define RAMP_US 1000 /* time an idle CPU takes to come up to speed */
#define ROUNDS 5 /* timed, after a warm-up */
#define BUSY 8 /* hops per busy round, at most */
#define RACED_UP_TO 16384
#define NEARBY 1.2 /* race_size tries sizes up to this times the smallest, and the next power of two */
#define MARGIN 1.02 /* so noise keeps path 0 and the smaller size */
#define STRIDE 8 /* 32 bytes, so every cache line is written */
#ifdef ESP_PLATFORM
#define EVICTED (32<<10) /* bytes, the ESP32-S3's data cache */
#else
#define EVICTED (256<<10) /* bytes, standing in for the rest of a loaded Pd tick */
#endif

typedef struct contestant {
	racer r;
	int reps;
	double idle; /* us per hop, best idle round */
	double busy[ROUNDS*BUSY]; /* us per busy hop */
	int nbusy;
} contestant;

void (*race_hook)(const racer *r, int path);

static int sizes[KEPT][2]; /* smallest candidate, size kept */
static int oldest;
static double ramped; /* when the last race ended, in us */
#if defined(_WIN32)
static CRITICAL_SECTION planning; /* recursive */
static INIT_ONCE planning_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK planning_init(PINIT_ONCE once, PVOID parameter, PVOID *context){
	(void)once;
	(void)parameter;
	(void)context;
	InitializeCriticalSection(&planning);
	return TRUE;
}
#elif CLOCKED
static pthread_mutex_t planning;
static pthread_once_t planning_once = PTHREAD_ONCE_INIT;

static void planning_init(void){
	pthread_mutexattr_t a;
	pthread_mutexattr_init(&a);
	pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE); /* spectrum_init holds it around the planner, which takes it too */
	pthread_mutex_init(&planning, &a);
	pthread_mutexattr_destroy(&a);
}
#endif

void race_lock(void){
#if defined(_WIN32)
	InitOnceExecuteOnce(&planning_once, planning_init, NULL, NULL);
	EnterCriticalSection(&planning);
#elif CLOCKED
	pthread_once(&planning_once, planning_init);
	pthread_mutex_lock(&planning);
#endif
}

void race_unlock(void){
#if defined(_WIN32)
	LeaveCriticalSection(&planning);
#elif CLOCKED
	pthread_mutex_unlock(&planning);
#endif
}

static double now_us(void){
#if defined(_WIN32)
	LARGE_INTEGER t;
	LARGE_INTEGER hz;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&hz);
	return (double)t.QuadPart*1e6/(double)hz.QuadPart;
#elif defined(__APPLE__)
	mach_timebase_info_data_t tb;
	mach_timebase_info(&tb);
	return (double)mach_absolute_time()*tb.numer/tb.denom*1e-3;
#elif CLOCKED
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec*1e6+(double)t.tv_nsec*1e-3;
#else
	return 0;
#endif
}

/* K2FFT_NORACE, 0 if unset, as simd-support/simd.c reads K2FFT_NOSIMD */
static int norace(void){
#if defined(_MSC_VER)
	char *v = NULL;
	size_t n = 0;
	int k = _dupenv_s(&v, &n, "K2FFT_NORACE")==0 && v ? (int)strtol(v, NULL, 10) : 0;
	free(v);
	return k;
#else
	const char *v = getenv("K2FFT_NORACE");
	return v ? (int)strtol(v, NULL, 10) : 0;
#endif
}

int race_timing(void){
	return CLOCKED && norace()<=0;
}

int race_clocked(void){
	return CLOCKED;
}

int race_untimed(int n){
	int k = norace();
	return k>0 ? (k<=n ? k-1 : -1) : 0;
}

/* value kept for key, or 0 */
static int lookup(int key){
	for (int i=0; i<KEPT && sizes[i][0]; i++){
		if (sizes[i][0]==key){
			return sizes[i][1];
		}
	}
	return 0;
}

/* replaces the oldest entry once sizes is full */
static void keep(int key, int value){
	int i = 0;
	while (i<KEPT && sizes[i][0]){
		i++;
	}
	if (i==KEPT){
		i = oldest;
		oldest = (oldest+1)%KEPT;
	}
	sizes[i][0] = key;
	sizes[i][1] = value;
}

/* the transform alone, as libbench2 and tools/cost.c time it */
static void hop(contestant *c){
	c->r.k->execute(c->r.plan, c->r.p->in, c->r.p->out);
}

/* hops for an idle round of ROUND_US */
static int reps_for(contestant *c){
	hop(c);
	double t0 = now_us();
	for (int i=0; i<4; i++){
		hop(c);
	}
	double t = (now_us()-t0)/4;
	int reps = (int)(ROUND_US/(t>0.01 ? t : 0.01))+1;
	return reps<2 ? 2 : reps;
}

/* mean of the faster three quarters, so an interrupt doesn't count */
static double busy_time(contestant *c){
	double *s = c->busy;
	int n = c->nbusy;
	for (int i=1; i<n; i++){
		double x = s[i];
		int j = i-1;
		while (j>=0 && s[j]>x){
			s[j+1] = s[j];
			j--;
		}
		s[j+1] = x;
	}
	int fast = n*3/4;
	double sum = 0;
	for (int i=0; i<fast; i++){
		sum += s[i];
	}
	return sum/fast;
}

/* whether b beats a, timed idle (back to back) and busy (after other writes), as either alone misleads */
static int beats(contestant *a, contestant *b, volatile float *junk){
	contestant *c[2] = {a, b};
	if (now_us()-ramped>RAMP_US){
		double t0 = now_us();
		while (now_us()-t0<RAMP_US){
			hop(a);
		}
	}
	for (int k=0; k<2; k++){
		c[k]->reps = reps_for(c[k]);
		c[k]->idle = 1e30;
		c[k]->nbusy = 0; /* race_size reuses its best */
	}
	for (int round=-1; round<ROUNDS; round++){
		for (int k=0; k<2; k++){
			contestant *x = c[((round+1)&1) ? 1-k : k]; /* alternates and ends on a, so a clock still ramping up favours a */
			double t0 = now_us();
			for (int i=0; i<x->reps; i++){
				hop(x);
			}
			double t = (now_us()-t0)/x->reps;
			if (round>=0 && t<x->idle){
				x->idle = t;
			}
		}
		double spent[2] = {0, 0};
		for (int i=0; i<BUSY && (i<2 || (spent[0]<ROUND_US && spent[1]<ROUND_US)); i++){
			for (int k=0; k<2; k++){
				int y = (i&1) ? 1-k : k;
				contestant *x = c[y];
				for (int j=0; j<EVICTED/(int)sizeof(float); j+=STRIDE){
					junk[j] += 1.0f;
				}
				double t0 = now_us();
				hop(x);
				double t1 = now_us()-t0;
				spent[y] += t1;
				if (round>=0){
					x->busy[x->nbusy++] = t1;
				}
			}
		}
	}
	ramped = now_us();
	double busy[2] = {busy_time(a), busy_time(b)};
	double idle1 = a->idle<b->idle ? a->idle : b->idle;
	double busy1 = busy[0]<busy[1] ? busy[0] : busy[1];
	double worse_a = a->idle/idle1>busy[0]/busy1 ? a->idle/idle1 : busy[0]/busy1;
	double worse_b = b->idle/idle1>busy[1]/busy1 ? b->idle/idle1 : busy[1]/busy1;
	return worse_b*MARGIN<worse_a; /* the one less slowed in its worse condition wins */
}

int race_fastest(const racer *r, int n){
	if (n<2 || !race_timing()){
		return 0;
	}
	volatile float *junk = calloc(EVICTED/sizeof(float), sizeof(float));
	int fastest = 0;
	if (junk){
		if (race_hook){
			for (int i=0; i<n; i++){
				race_hook(&r[i], i);
			}
		}
		contestant best = {.r = r[0]};
		for (int i=1; i<n; i++){ /* in the table's order, so MARGIN keeps the benchmark's pick on a tie */
			contestant c = {.r = r[i]};
			if (beats(&best, &c, junk)){
				best = c;
				fastest = i;
			}
		}
	}
	free((void *)junk);
	return fastest;
}

int race_size(int n){
	int first = fft_next_size(n);
	if (first==0 || n>RACED_UP_TO || !race_timing()){
		return first;
	}
	int top = 1;
	while (top<first){
		top *= 2;
	}
	int next = fft_next_size(first+1);
	if (next==0 || next>top){
		return first;
	}
	int known = lookup(first);
	if (known){
		return known;
	}
	volatile float *junk = calloc(EVICTED/sizeof(float), sizeof(float));
	fft *kept = fft_new(first);
	contestant best = {.r = kept ? fft_racer(kept) : (racer){0}};
	int chosen = first;
	int timed = junk && kept; /* keep only an answer from every candidate */
	if (race_hook && kept){
		race_hook(&best.r, -1);
	}
	if (timed){
		for (int s=next; s>0 && s<=top; s=fft_next_size(s+1)){
			if (s>NEARBY*first && s!=top){
				continue;
			}
			fft *f = fft_new(s);
			if (!f){
				timed = 0;
				continue;
			}
			contestant c = {.r = fft_racer(f)};
			if (race_hook){
				race_hook(&c.r, -1);
			}
			if (beats(&best, &c, junk)){
				fft_free(kept);
				kept = f;
				best = c;
				chosen = s;
				continue;
			}
			fft_free(f);
		}
		if (timed){
			keep(first, chosen);
		}
	}
	fft_free(kept);
	free((void *)junk);
	return chosen;
}

int race_known(int i, int *n, int *chosen){
	if (i<0 || i>=KEPT || !sizes[i][0]){
		return 0;
	}
	*n = sizes[i][0];
	*chosen = sizes[i][1];
	return 1;
}

void race_learn(int n, int chosen){
	if (n>0 && chosen>=n && !lookup(n)){
		keep(n, chosen);
	}
}

void race_forget(void){
	memset(sizes, 0, sizeof(sizes));
	oldest = 0;
}

double race_time(void (*hop)(void *ctx), void *ctx){
	if (!race_timing()){
		return 0;
	}
	hop(ctx);
	double t0 = now_us();
	hop(ctx);
	const double once = now_us()-t0;
	const int reps = (int)(ROUND_US/(once>0.01 ? once : 0.01))+1;
	double best = 1e30;
	for (int round=0; round<ROUNDS; round++){
		t0 = now_us();
		for (int i=0; i<reps; i++){
			hop(ctx);
		}
		const double t = (now_us()-t0)/reps;
		best = t<best ? t : best;
	}
	return best;
}
