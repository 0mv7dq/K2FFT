// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once
/* finer than gettimeofday, to time one transform */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now_us(void){
	static double scale;
	LARGE_INTEGER t;
	if (!scale){
		LARGE_INTEGER hz;
		QueryPerformanceFrequency(&hz);
		scale = 1e6/(double)hz.QuadPart;
	}
	QueryPerformanceCounter(&t);
	return (double)t.QuadPart*scale;
}
#elif defined(__APPLE__)
#include <mach/mach_time.h>
static double now_us(void){
	static double scale;
	if (!scale){
		mach_timebase_info_data_t tb;
		mach_timebase_info(&tb);
		scale = (double)tb.numer/(double)tb.denom*1e-3;
	}
	return (double)mach_absolute_time()*scale;
}
#else
#include <time.h>
static double now_us(void){
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec*1e6+(double)t.tv_nsec*1e-3;
}
#endif

/* the clock's smallest observed step */
static double tick_us(void){
	double best = 1e30;
	for (int i = 0; i<2000; i++){
		double a = now_us(), b;
		do {
			b = now_us();
		} while (b==a);
		if (b-a<best){
			best = b-a;
		}
	}
	return best;
}
