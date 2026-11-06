// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* the kernels CMake built here (KERNEL_<name>), in the order the planner falls back on w/o kernel/table.h's row */
#include "kernel/kernel.h"
#include <stddef.h>

extern const kernel k_ipp, k_vdsp, k_kfr, k_ne10, k_cmsis, k_espdsp, k_fftw3, k_zldsp;
extern const kernel k_pffft_avx2, k_pffft_neon, k_pffft_rvv, k_pocketfft, k_pffft;

const kernel *const kernels[] = {
#ifdef KERNEL_ipp
	&k_ipp,
#endif
#ifdef KERNEL_vdsp
	&k_vdsp,
#endif
#ifdef KERNEL_kfr
	&k_kfr,
#endif
#ifdef KERNEL_ne10
	&k_ne10,
#endif
#ifdef KERNEL_cmsis
	&k_cmsis,
#endif
#ifdef KERNEL_espdsp
	&k_espdsp,
#endif
#ifdef KERNEL_fftw3
	&k_fftw3,
#endif
#ifdef KERNEL_zldsp
	&k_zldsp,
#endif
#ifdef KERNEL_pffft_avx2
	&k_pffft_avx2,
#endif
#ifdef KERNEL_pffft_neon
	&k_pffft_neon,
#endif
#ifdef KERNEL_pffft_rvv
	&k_pffft_rvv,
#endif
#ifdef KERNEL_pocketfft
	&k_pocketfft,
#endif
	&k_pffft, /* everywhere: PUREfft's sizes are its */
	NULL
};
