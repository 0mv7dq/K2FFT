// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#define _POSIX_C_SOURCE 200112L
#include "api/spectrum.h"
#include "api/fft.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 8
#define ROUNDS 4
#define ASKS 8
static const int asks[ASKS] = {700, 1000, 1500, 2049, 2500, 3000, 4100, 5000};
static int got[THREADS][ROUNDS][ASKS];
static float win[1024];

static void *worker(void *arg){
	int t = (int)(size_t)arg;
	float frame[1024];
	for (int i = 0; i<1024; i++){
		frame[i] = (float)(((i+t)*7919)%1000)/1000.0f-0.5f;
	}
	for (int r = 0; r<ROUNDS; r++){
		for (int k = 0; k<ASKS; k++){
			int a = (k+t)%ASKS; /* each thread asks in a different order */
			spectrum s;
			if (spectrum_init(&s, win, 1024, asks[a])!=0){
				got[t][r][a] = -1;
			} else {
				spectrum_transform(&s, frame, 1);
				got[t][r][a] = s.size;
			}
			spectrum_free(&s);
		}
	}
	return NULL;
}

int main(void){
	for (int i = 0; i<1024; i++){
		win[i] = 1;
	}
	pthread_t th[THREADS];
	for (int t = 0; t<THREADS; t++){
		pthread_create(&th[t], NULL, worker, (void *)(size_t)t);
	}
	for (int t = 0; t<THREADS; t++){
		pthread_join(th[t], NULL);
	}
	int fails = 0;
	for (int a = 0; a<ASKS; a++){
		int n = asks[a]>1024 ? asks[a] : 1024, v = got[0][0][a];
		for (int t = 0; t<THREADS; t++){
			for (int r = 0; r<ROUNDS; r++){
				int s = got[t][r][a];
				if (s!=v || s<n || fft_next_size(s)!=s){
					fails++;
					printf("FAIL: ask %d, thread %d round %d -> %d (thread 0 round 0: %d)\n", asks[a], t, r, s, v);
				}
			}
		}
		printf("ask %5d -> %5d in all %d threads x %d rounds\n", asks[a], v, THREADS, ROUNDS);
	}
	printf(fails ? "threads: %d checks FAILED\n" : "threads: all checks passed\n", fails);
	return fails!=0;
}
