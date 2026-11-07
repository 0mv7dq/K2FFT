// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the arena a K2FFT plan of FFTW's lends while it runs (rdft/fftw3.c), and, included by FFTW's kernel/kalloc.c (cmake/fftw.cmake),
   FFTW's heap through it: the buffers FFTW's solvers take per transform (its reodft's, rdft2-rdft's, ...) come from the lent arena, so
   a transform never reaches the C library's heap; w/o one, the C library's, as before */
#pragma once
#include <stddef.h>

typedef struct k2fft_arena {
	char *base;
	size_t size; /* bytes at base */
	size_t used; /* the running transform's */
	size_t want; /* what it took, room or not */
	int over; /* it took more than the room, from the C library */
} k2fft_arena;

#ifdef X /* FFTW's, fftwf_ or fftw_ */
#include <stdlib.h>

#if defined(_MSC_VER)
#define K2FFT_THREAD __declspec(thread)
#else
#define K2FFT_THREAD __thread
#endif

static K2FFT_THREAD k2fft_arena *k2fft_lent;

/* lends a (NULL: none) to this thread's transforms, its count reset; the arena lent before */
k2fft_arena *X(k2fft_lend)(k2fft_arena *a){
	k2fft_arena *was = k2fft_lent;
	k2fft_lent = a;
	if (a){
		a->used = a->want = 0;
		a->over = 0;
	}
	return was;
}

static void *k2fft_malloc(size_t n){
	k2fft_arena *a = k2fft_lent;
	if (a){
		const size_t at = (a->want+63)&~(size_t)63;
		a->want = at+n;
		if (a->base && at<=a->size && n<=a->size-at){
			a->used = at+n;
			return a->base+at;
		}
		a->over = 1;
	}
	return malloc(n);
}

static void k2fft_free(void *p){
	const k2fft_arena *a = k2fft_lent;
	if (!(a && a->base && (char *)p>=a->base && (char *)p<a->base+a->size)){
		free(p);
	}
}

#define malloc k2fft_malloc
#define free k2fft_free
#endif
