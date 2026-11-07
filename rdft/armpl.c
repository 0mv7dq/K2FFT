// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* Arm Performance Libraries' FFTW interface, float and double, as benchees/armpl: renamed armplf_ and armpld_ (cmake/armpl.cmake);
   a transform at planning, as ArmPL allocates on a plan's first: there, not in Pd's DSP */
#define F(name) armplf_##name
#define D(name) armpld_##name
#define WARM
#include "rdft/fftw_like.h"

const kernel k_armpl = {"armpl", can_do, make, execute, destroy};
