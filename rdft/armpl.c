// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Arm Performance Libraries' FFTW interface, float and double, as benchees/armpl: renamed armplf_ and armpld_ (cmake/armpl.cmake);
   a transform at planning, as ArmPL allocates on a plan's first: there, not in Pd's DSP */
#include "kernel/kernel.h"

/* w/o its DCT-IV and DST-IV, which malloc each transform */
static int fourth(const problem *pb){
	for (int d=0; pb->kind==KIND_R2R && d<pb->rank; d++){
		if (pb->r2r[d]==K2FFT_REDFT11 || pb->r2r[d]==K2FFT_RODFT11){
			return 1;
		}
	}
	return 0;
}

#define F(name) armplf_##name
#define D(name) armpld_##name
#define CAN(pb) !fourth(pb)
#define WARM
#include "rdft/fftw_like.h"

const kernel k_armpl = {"armpl", can_do, make, execute, destroy};
