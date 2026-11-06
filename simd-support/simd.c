// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute


#define _DEFAULT_SOURCE // syscall()
#include "simd.h"
#include <stdlib.h>
#if defined(__x86_64__) || defined(_M_X64)
#if defined(_MSC_VER) && !defined(__clang__)
#include <immintrin.h>
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif
#if defined(__linux__) && defined(__arm__) && !defined(__ARM_NEON)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif
#if defined(__linux__) && defined(__riscv)
#include <sys/syscall.h>
#include <unistd.h>
#if __has_include(<asm/hwprobe.h>)
#include <asm/hwprobe.h>
#endif
#endif

static int off(void){
#ifdef _MSC_VER
	char *v = NULL;
	size_t n = 0;
	int set = _dupenv_s(&v, &n, "K2FFT_NOSIMD")==0 && v && *v && !(v[0]=='0' && v[1]==0);
	free(v);
	return set;
#else
	const char *v = getenv("K2FFT_NOSIMD");
	return v && *v && !(v[0]=='0' && v[1]==0);
#endif
}

#if defined(__x86_64__) || defined(_M_X64)
static void cpuid(unsigned leaf, unsigned sub, unsigned r[4]){
#if defined(_MSC_VER) && !defined(__clang__)
	int v[4];
	__cpuidex(v, (int)leaf, (int)sub);
	for (int i=0; i<4; i++){
		r[i] = (unsigned)v[i];
	}
#else
	__cpuid_count(leaf, sub, r[0], r[1], r[2], r[3]);
#endif
}

//! XCR0: register state the OS saves upon context switch
static unsigned long long xcr0(void){
#if defined(_MSC_VER) && !defined(__clang__)
	return _xgetbv(0);
#else
	unsigned lo, hi;
	__asm__ volatile ("xgetbv" : "=a" (lo), "=d" (hi) : "c" (0)); // inline asm needs no -mxsave
	return (unsigned long long)hi<<32|lo;
#endif
}
#endif

int simd_avx2_fma(void){
#if defined(__x86_64__) || defined(_M_X64)
	unsigned r[4];
	cpuid(0, 0, r);
	if (off() || r[0]<7){
		return 0;
	}
	cpuid(1, 0, r);
	unsigned fma = r[2]>>12&1u, osxsave = r[2]>>27&1u, avx = r[2]>>28&1u;
	if (!fma || !osxsave || !avx || (xcr0()&6)!=6){ // XMM & YMM state saved
		return 0;
	}
	cpuid(7, 0, r);
	return (r[1]>>5&1u)!=0; // AVX2
#else
	return 0;
#endif
}

int simd_neon(void){
	if (off()){
		return 0;
	}
#if defined(__ARM_NEON)
	return 1;
#elif defined(__linux__) && defined(__arm__)
	return (getauxval(AT_HWCAP)&HWCAP_NEON)!=0;
#else
	return 0;
#endif
}

int simd_rvv(void){
	if (off()){
		return 0;
	}
#if defined(__linux__) && defined(__riscv) && defined(RISCV_HWPROBE_IMA_V)
	struct riscv_hwprobe probe = {RISCV_HWPROBE_KEY_IMA_EXT_0, 0};
	return syscall(__NR_riscv_hwprobe, &probe, 1, 0, NULL, 0)==0 && (probe.value&RISCV_HWPROBE_IMA_V);
#else
	return 0;
#endif
}

int simd_any(void){
	return !off();
}
