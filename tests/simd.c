// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* What simd-support/ finds on this CPU. With the argument "off", run with PUREFFT_NOSIMD=1, every answer must be 0. */
#include "simd-support/simd.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv){
	int a = simd_avx2_fma(), n = simd_neon(), r = simd_rvv();
	printf("avx2_fma %d neon %d rvv %d\n", a, n, r);
	if (argc>1 && strcmp(argv[1], "off")==0){
		return a || n || r;
	}
	return 0;
}
