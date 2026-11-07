// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* rdft/vdsp.c's steps in R (float or double), T(name) its names, V(name) vDSP's (name, or name##D), CPLX and SPLIT its complex types */

/* in to vDSP's split arrays, its DFT, back to FFTW's layout */
static void T(run)(const plan *p, const R *in, R *out){
	const int n = p->size, half = p->kind==KIND_DFT ? n : n/2;
	R *s = p->split, *b = p->bins;
	SPLIT x = {s, s+half};
	const SPLIT y = {b, b+half};
	V(vDSP_ctoz)((const CPLX *)in, 2, &x, 1, (vDSP_Length)half); /* a real one's even and odd samples */
	if (p->kind==KIND_C2R){ /* vDSP's packed bins: Nyquist's re in DC's im */
		x.imagp[0] = in[n];
	}
	V(vDSP_DFT_Execute)(p->dft, x.realp, x.imagp, y.realp, y.imagp);
	V(vDSP_ztoc)(&y, 1, (CPLX *)out, 2, (vDSP_Length)half);
	if (p->kind==KIND_DFT){
		return;
	}
	const R scale = (R)p->scale;
	V(vDSP_vsmul)(out, 1, &scale, out, 1, (vDSP_Length)n);
	if (p->kind==KIND_R2C){
		out[n] = out[1];
		out[n+1] = 0;
		out[1] = 0;
	}
}

static void *T(setup)(int size, int complex, vDSP_DFT_Direction way){
	return complex ? (void *)V(vDSP_DFT_zop_CreateSetup)(NULL, (vDSP_Length)size, way) : (void *)V(vDSP_DFT_zrop_CreateSetup)(NULL, (vDSP_Length)size, way);
}

static void T(unsetup)(void *dft){
	V(vDSP_DFT_DestroySetup)(dft);
}

/* a c2r's output at 0 for DC alone, whose inverse is 1 everywhere, unscaled */
static double T(dc)(plan *p, void *in_, void *out_){
	R *in = in_, *out = out_;
	memset(in, 0, (size_t)(p->size+2)*sizeof(R));
	in[0] = 1;
	p->scale = 1;
	T(run)(p, in, out);
	return out[0];
}
