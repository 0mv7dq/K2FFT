// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/*
    SIMD detection ... each returns 1 if the CPU and OS can run the SIMD code a given backend picks @ runtime, else 0.
*/

int simd_avx2_fma(void); // x86-64 				... AVX2 + FMA, w/ OS saving YMM state (cpuid...xgetbv)
int simd_neon(void);     // ARM	   				... NEON, always where compiler targets it, else from Linux's AT_HWCAP
int simd_rvv(void);      // 64-bit RISC-V Linux ... RVV 1.0 from riscv_hwprobe (Linux 6.4+)
int simd_any(void);      // any CPU                ... 0 under K2FFT_NOSIMD, for kernels that pick their own (rdft/hwyfft.cpp, Highway's dispatch)



#ifdef __cplusplus
}
#endif
