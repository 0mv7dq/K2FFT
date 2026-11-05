// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#define _POSIX_C_SOURCE 200112L /* clock_gettime, pthreads */
#include <time.h>
#endif
#include "race.h"
#include <stdlib.h>

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
	fft *f;
	int path; /* for use(), unused by race_size */
	float *power;
	int reps;
	double idle; /* us per hop, best idle round */
	double busy[3*BUSY]; /* us per busy hop */
	int nbusy;
} contestant;

void (*race_hook)(fft *f, int size, int path);

static int paths[KEPT][2]; /* size, 1 + path */
static int sizes[KEPT][2]; /* smallest candidate, size kept */
static int oldest[2];
static double ramped; /* when the last race ended, in us */
#if defined(_WIN32)
static SRWLOCK planning = SRWLOCK_INIT;
#elif CLOCKED
static pthread_mutex_t planning = PTHREAD_MUTEX_INITIALIZER;
#endif

void race_lock(void){
#if defined(_WIN32)
	AcquireSRWLockExclusive(&planning);
#elif CLOCKED
	pthread_mutex_lock(&planning);
#endif
}

void race_unlock(void){
#if defined(_WIN32)
	ReleaseSRWLockExclusive(&planning);
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

/* value kept for key, or 0 */
static int lookup(int t[KEPT][2], int key){
	for (int i=0; i<KEPT && t[i][0]; i++){
		if (t[i][0]==key){
			return t[i][1];
		}
	}
	return 0;
}

/* replaces the oldest entry once t is full */
static void keep(int t[KEPT][2], int *old, int key, int value){
	int i = 0;
	while (i<KEPT && t[i][0]){
		i++;
	}
	if (i==KEPT){
		i = *old;
		*old = (*old+1)%KEPT;
	}
	t[i][0] = key;
	t[i][1] = value;
}

static void hop(contestant *c){
	fft_forward(c->f);
	fft_power(c->f, c->power, 1);
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

static contestant entrant(fft *f, int path, float *power){
	contestant c = {0};
	c.f = f;
	c.path = path;
	c.power = power;
	return c;
}

/* whether b beats a, timed idle (back to back) and busy (after other writes), as either alone misleads */
static int beats(contestant *a, contestant *b, void (*use)(fft *f, int path), volatile float *junk){
	contestant *c[2] = {a, b};
	if (now_us()-ramped>RAMP_US){
		if (use){
			use(a->f, a->path);
		}
		double t0 = now_us();
		while (now_us()-t0<RAMP_US){
			hop(a);
		}
	}
	for (int k=0; k<2; k++){
		if (use){
			use(c[k]->f, c[k]->path);
		}
		c[k]->reps = reps_for(c[k]);
		c[k]->idle = 1e30;
		c[k]->nbusy = 0; /* race_size reuses its best */
	}
	for (int round=-1; round<3; round++){
		for (int k=0; k<2; k++){
			contestant *x = c[((round+1)&1) ? 1-k : k]; /* alternates and ends on a, so a clock still ramping up favours a */
			if (use){
				use(x->f, x->path);
			}
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
				if (use){
					use(x->f, x->path);
				}
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

int race_paths(fft *f, int size, void (*use)(fft *f, int path)){
#ifdef RACE_PATH
	return RACE_PATH; /* fixed by a test build, tests/CMakeLists.txt */
#endif
	if (!CLOCKED){
		return 0;
	}
	int known = lookup(paths, size);
	if (known){
		return known-1;
	}
	float *power = malloc(((size_t)size/2+1)*sizeof(float));
	volatile float *junk = calloc(EVICTED/sizeof(float), sizeof(float));
	int faster = 0;
	if (power && junk){
		contestant a = entrant(f, 0, power);
		contestant b = entrant(f, 1, power);
		if (race_hook){
			for (int p=0; p<2; p++){
				use(f, p);
				race_hook(f, size, p);
			}
		}
		faster = beats(&a, &b, use, junk);
		keep(paths, &oldest[0], size, 1+faster);
	}
	free(power);
	free((void *)junk);
	return faster;
}

int race_size(int n){
	int first = fft_next_size(n);
	if (first==0 || n>RACED_UP_TO || !CLOCKED){
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
	int known = lookup(sizes, first);
	if (known){
		return known;
	}
	volatile float *junk = calloc(EVICTED/sizeof(float), sizeof(float));
	contestant best = entrant(fft_new(first), 0, malloc(((size_t)first/2+1)*sizeof(float)));
	int chosen = first;
	int timed = junk && best.f && best.power; /* keep only an answer from every candidate */
	if (race_hook && best.f){
		race_hook(best.f, first, -1);
	}
	if (timed){
		for (int s=next; s>0 && s<=top; s=fft_next_size(s+1)){
			if (s>NEARBY*first && s!=top){
				continue;
			}
			contestant c = entrant(fft_new(s), 0, malloc(((size_t)s/2+1)*sizeof(float)));
			if (race_hook && c.f){
				race_hook(c.f, s, -1);
			}
			if (!c.f || !c.power){
				timed = 0;
			} else if (beats(&best, &c, NULL, junk)){
				fft_free(best.f);
				free(best.power);
				best = c;
				chosen = s;
				continue;
			}
			fft_free(c.f);
			free(c.power);
		}
		if (timed){
			keep(sizes, &oldest[1], first, chosen);
		}
	}
	fft_free(best.f);
	free(best.power);
	free((void *)junk);
	return chosen;
}
