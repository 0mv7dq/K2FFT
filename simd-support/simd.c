// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute


#define _DEFAULT_SOURCE // syscall()
#include "simd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__x86_64__) || defined(_M_X64)
#if defined(_MSC_VER) && !defined(__clang__)
#include <immintrin.h>
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif
#if defined(__APPLE__)
#include <sys/sysctl.h>
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

void simd_cpu(char *name, size_t room){
	if (room==0){
		return;
	}
	name[0] = '\0';
#if defined(__x86_64__) || defined(_M_X64)
	unsigned r[12];
	cpuid(0x80000000u, 0, r);
	if (r[0]>=0x80000004u && room>1){
		for (unsigned i=0; i<3; i++){
			cpuid(0x80000002u+i, 0, r+4*i);
		}
		size_t n = room-1<sizeof(r) ? room-1 : sizeof(r);
		memcpy(name, r, n);
		name[n] = '\0';
	}
#elif defined(__APPLE__)
	size_t n = room;
	if (sysctlbyname("machdep.cpu.brand_string", name, &n, NULL, 0)!=0){
		name[0] = '\0';
	}
#elif defined(__linux__)
	/* /proc/cpuinfo's first model fields, as "CPU implementer 0x41 CPU part 0xd49" where it names no model */
	static const char *const keys[] = {"model name", "CPU implementer", "CPU part", "uarch", "cpu"};
	int seen[sizeof(keys)/sizeof(keys[0])] = {0};
	char line[256];
	FILE *f = fopen("/proc/cpuinfo", "r");
	while (f && fgets(line, sizeof(line), f)){
		char *colon = strchr(line, ':');
		for (size_t k=0; colon && k<sizeof(keys)/sizeof(keys[0]); k++){
			const size_t len = strlen(keys[k]), used = strlen(name);
			if (seen[k] || strncmp(line, keys[k], len)!=0 || strspn(line+len, " \t")!=(size_t)(colon-line-len)){
				continue; /* not the key, as "cpu MHz" isn't "cpu" */
			}
			seen[k] = 1;
			char *v = colon+1+strspn(colon+1, " \t");
			v[strcspn(v, "\n")] = '\0';
			snprintf(name+used, room-used, "%s%s%s%s", used ? " " : "", k ? keys[k] : "", k ? " " : "", v);
		}
	}
	if (f){
		fclose(f);
	}
#endif
	for (size_t i=0; name[i]; i++){ /* one line, for wisdom's */
		name[i] = name[i]=='\n' || name[i]=='\t' ? ' ' : name[i];
	}
	size_t len = strlen(name), lead = strspn(name, " "); /* x86's brand strings are padded w/ spaces */
	while (len>lead && name[len-1]==' '){
		len--;
	}
	memmove(name, name+lead, len-lead);
	name[len-lead] = '\0';
}
