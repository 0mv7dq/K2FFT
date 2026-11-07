// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* api/k2fft.h: FFTW's planners, all as its guru one, for kernel/planner.c */
#include "api/k2fft.h"
#include "kernel/planner.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* p's plan from the guru's arguments; batch dimensions of 1 dropped, as FFTW does */
static struct k2fft_plan_s *guru(problem_kind kind, int single, int sign, int rank, const k2fft_iodim *dims, int howmany_rank,
								 const k2fft_iodim *howmany_dims, void *in, void *out, const k2fft_r2r_kind *r2r, unsigned flags){
	problem p = {0};
	if (rank<1 || rank>RANKS || howmany_rank<0 || howmany_rank>RANKS || !dims || (howmany_rank && !howmany_dims) || !in || !out){
		return NULL;
	}
	p.kind = kind;
	p.sign = sign;
	p.single = single;
	p.preserve = (flags&K2FFT_PRESERVE_INPUT)!=0;
	p.half = (flags&K2FFT_HALF_ZERO)!=0 && rank==1 && sign==K2FFT_FORWARD && (kind==KIND_R2C || kind==KIND_DFT);
	p.rank = rank;
	for (int d=0; d<rank; d++){
		if (dims[d].n<1){
			return NULL;
		}
		p.dims[d] = dims[d];
		p.r2r[d] = kind==KIND_R2R ? r2r[d] : K2FFT_R2HC;
	}
	for (int v=0; v<howmany_rank; v++){
		if (howmany_dims[v].n<1){
			return NULL;
		}
		if (howmany_dims[v].n>1){
			p.vdims[p.vrank++] = howmany_dims[v];
		}
	}
	p.in = in;
	p.out = out;
	return planner_plan(&p, flags);
}

/* rank dimensions n of a contiguous row-major array: a real's last padded to 2(n/2+1) in place, a complex one's n/2+1 */
static void strides(problem_kind kind, int rank, const int *n, const int *inembed, int istride, const int *onembed, int ostride,
					int inplace, k2fft_iodim *dims){
	long is = istride, os = ostride;
	for (int d=rank-1; d>=0; d--){
		dims[d].n = n[d];
		dims[d].is = (int)is;
		dims[d].os = (int)os;
		long ni = n[d], no = n[d];
		if (d==rank-1 && (kind==KIND_R2C || kind==KIND_C2R)){
			const long half = n[d]/2+1, real = inplace ? 2*half : n[d];
			ni = kind==KIND_R2C ? real : half;
			no = kind==KIND_R2C ? half : real;
		}
		if (d>0){ /* nembed[0] unread, as FFTW */
			is *= inembed ? inembed[d] : ni;
			os *= onembed ? onembed[d] : no;
		}
	}
}

static struct k2fft_plan_s *many(problem_kind kind, int single, int sign, int rank, const int *n, int howmany, void *in,
								 const int *inembed, int istride, int idist, void *out, const int *onembed, int ostride, int odist,
								 const k2fft_r2r_kind *r2r, unsigned flags){
	k2fft_iodim dims[RANKS];
	if (rank<1 || rank>RANKS || !n){
		return NULL;
	}
	strides(kind, rank, n, inembed, istride, onembed, ostride, in==out, dims);
	const k2fft_iodim batch = {howmany, idist, odist};
	return guru(kind, single, sign, rank, dims, 1, &batch, in, out, r2r, flags);
}

static struct k2fft_plan_s *basic(problem_kind kind, int single, int sign, int rank, const int *n, void *in, void *out,
								  const k2fft_r2r_kind *r2r, unsigned flags){
	return many(kind, single, sign, rank, n, 1, in, NULL, 1, 0, out, NULL, 1, 0, r2r, flags);
}

static FILE *opened(const char *path, const char *mode){
#ifdef _MSC_VER
	FILE *f = NULL;
	return fopen_s(&f, path, mode)==0 ? f : NULL;
#else
	return fopen(path, mode);
#endif
}

static int wisdom_to(const char *path){
	char *s = planner_wisdom();
	FILE *f = s ? opened(path, "w") : NULL;
	int ok = f && fputs(s, f)>=0;
	if (f && fclose(f)!=0){
		ok = 0;
	}
	free(s);
	return ok;
}

static int wisdom_from(const char *path){
	FILE *f = opened(path, "r");
	char *s = NULL;
	size_t used = 0, room = 0, got = 1;
	while (f && got>0){
		if (used+1>=room){
			char *t = realloc(s, room = room ? 2*room : 4096);
			if (!t){
				break;
			}
			s = t;
		}
		got = fread(s+used, 1, room-1-used, f);
		used += got;
	}
	if (f){
		fclose(f);
	}
	int ok = 0;
	if (s && got==0){
		s[used] = '\0';
		ok = planner_learn(s);
	}
	free(s);
	return ok;
}

#define K2FFT_DEFINE(X, R, C, SINGLE) \
	X##plan X##plan_guru_dft(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, C *in, C *out, \
							 int sign, unsigned flags){ \
		return guru(KIND_DFT, SINGLE, sign, rank, dims, howmany_rank, howmany_dims, in, out, NULL, flags); \
	} \
	X##plan X##plan_guru_dft_r2c(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, R *in, C *out, \
								 unsigned flags){ \
		return guru(KIND_R2C, SINGLE, K2FFT_FORWARD, rank, dims, howmany_rank, howmany_dims, in, out, NULL, flags); \
	} \
	X##plan X##plan_guru_dft_c2r(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, C *in, R *out, \
								 unsigned flags){ \
		return guru(KIND_C2R, SINGLE, K2FFT_BACKWARD, rank, dims, howmany_rank, howmany_dims, in, out, NULL, flags); \
	} \
	X##plan X##plan_guru_r2r(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, R *in, R *out, \
							 const k2fft_r2r_kind *kind, unsigned flags){ \
		return kind ? guru(KIND_R2R, SINGLE, 0, rank, dims, howmany_rank, howmany_dims, in, out, kind, flags) : NULL; \
	} \
	X##plan X##plan_many_dft(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist, C *out, \
							 const int *onembed, int ostride, int odist, int sign, unsigned flags){ \
		return many(KIND_DFT, SINGLE, sign, rank, n, howmany, in, inembed, istride, idist, out, onembed, ostride, odist, NULL, \
					flags); \
	} \
	X##plan X##plan_many_dft_r2c(int rank, const int *n, int howmany, R *in, const int *inembed, int istride, int idist, C *out, \
								 const int *onembed, int ostride, int odist, unsigned flags){ \
		return many(KIND_R2C, SINGLE, K2FFT_FORWARD, rank, n, howmany, in, inembed, istride, idist, out, onembed, ostride, odist, \
					NULL, flags); \
	} \
	X##plan X##plan_many_dft_c2r(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist, R *out, \
								 const int *onembed, int ostride, int odist, unsigned flags){ \
		return many(KIND_C2R, SINGLE, K2FFT_BACKWARD, rank, n, howmany, in, inembed, istride, idist, out, onembed, ostride, odist, \
					NULL, flags); \
	} \
	X##plan X##plan_many_r2r(int rank, const int *n, int howmany, R *in, const int *inembed, int istride, int idist, R *out, \
							 const int *onembed, int ostride, int odist, const k2fft_r2r_kind *kind, unsigned flags){ \
		return kind ? many(KIND_R2R, SINGLE, 0, rank, n, howmany, in, inembed, istride, idist, out, onembed, ostride, odist, kind, \
						   flags) : NULL; \
	} \
	X##plan X##plan_dft(int rank, const int *n, C *in, C *out, int sign, unsigned flags){ \
		return basic(KIND_DFT, SINGLE, sign, rank, n, in, out, NULL, flags); \
	} \
	X##plan X##plan_dft_1d(int n, C *in, C *out, int sign, unsigned flags){ \
		return X##plan_dft(1, &n, in, out, sign, flags); \
	} \
	X##plan X##plan_dft_2d(int n0, int n1, C *in, C *out, int sign, unsigned flags){ \
		const int n[2] = {n0, n1}; \
		return X##plan_dft(2, n, in, out, sign, flags); \
	} \
	X##plan X##plan_dft_3d(int n0, int n1, int n2, C *in, C *out, int sign, unsigned flags){ \
		const int n[3] = {n0, n1, n2}; \
		return X##plan_dft(3, n, in, out, sign, flags); \
	} \
	X##plan X##plan_dft_r2c(int rank, const int *n, R *in, C *out, unsigned flags){ \
		return basic(KIND_R2C, SINGLE, K2FFT_FORWARD, rank, n, in, out, NULL, flags); \
	} \
	X##plan X##plan_dft_r2c_1d(int n, R *in, C *out, unsigned flags){ \
		return X##plan_dft_r2c(1, &n, in, out, flags); \
	} \
	X##plan X##plan_dft_r2c_2d(int n0, int n1, R *in, C *out, unsigned flags){ \
		const int n[2] = {n0, n1}; \
		return X##plan_dft_r2c(2, n, in, out, flags); \
	} \
	X##plan X##plan_dft_r2c_3d(int n0, int n1, int n2, R *in, C *out, unsigned flags){ \
		const int n[3] = {n0, n1, n2}; \
		return X##plan_dft_r2c(3, n, in, out, flags); \
	} \
	X##plan X##plan_dft_c2r(int rank, const int *n, C *in, R *out, unsigned flags){ \
		return basic(KIND_C2R, SINGLE, K2FFT_BACKWARD, rank, n, in, out, NULL, flags); \
	} \
	X##plan X##plan_dft_c2r_1d(int n, C *in, R *out, unsigned flags){ \
		return X##plan_dft_c2r(1, &n, in, out, flags); \
	} \
	X##plan X##plan_dft_c2r_2d(int n0, int n1, C *in, R *out, unsigned flags){ \
		const int n[2] = {n0, n1}; \
		return X##plan_dft_c2r(2, n, in, out, flags); \
	} \
	X##plan X##plan_dft_c2r_3d(int n0, int n1, int n2, C *in, R *out, unsigned flags){ \
		const int n[3] = {n0, n1, n2}; \
		return X##plan_dft_c2r(3, n, in, out, flags); \
	} \
	X##plan X##plan_r2r(int rank, const int *n, R *in, R *out, const k2fft_r2r_kind *kind, unsigned flags){ \
		return kind ? basic(KIND_R2R, SINGLE, 0, rank, n, in, out, kind, flags) : NULL; \
	} \
	X##plan X##plan_r2r_1d(int n, R *in, R *out, k2fft_r2r_kind kind, unsigned flags){ \
		return X##plan_r2r(1, &n, in, out, &kind, flags); \
	} \
	X##plan X##plan_r2r_2d(int n0, int n1, R *in, R *out, k2fft_r2r_kind kind0, k2fft_r2r_kind kind1, unsigned flags){ \
		const int n[2] = {n0, n1}; \
		const k2fft_r2r_kind k[2] = {kind0, kind1}; \
		return X##plan_r2r(2, n, in, out, k, flags); \
	} \
	X##plan X##plan_r2r_3d(int n0, int n1, int n2, R *in, R *out, k2fft_r2r_kind kind0, k2fft_r2r_kind kind1, k2fft_r2r_kind kind2, \
						   unsigned flags){ \
		const int n[3] = {n0, n1, n2}; \
		const k2fft_r2r_kind k[3] = {kind0, kind1, kind2}; \
		return X##plan_r2r(3, n, in, out, k, flags); \
	} \
	void X##execute(const X##plan p){ \
		p->k->execute(p->plan, p->p.in, p->p.out); \
	} \
	void X##execute_dft(const X##plan p, C *in, C *out){ \
		p->k->execute(p->plan, in, out); \
	} \
	void X##execute_dft_r2c(const X##plan p, R *in, C *out){ \
		p->k->execute(p->plan, in, out); \
	} \
	void X##execute_dft_c2r(const X##plan p, C *in, R *out){ \
		p->k->execute(p->plan, in, out); \
	} \
	void X##execute_r2r(const X##plan p, R *in, R *out){ \
		p->k->execute(p->plan, in, out); \
	} \
	void X##destroy_plan(X##plan p){ \
		planner_destroy(p); \
	} \
	const char *X##plan_kernel(const X##plan p){ \
		return p ? p->k->name : NULL; \
	} \
	char *X##export_wisdom_to_string(void){ \
		return planner_wisdom(); \
	} \
	int X##import_wisdom_from_string(const char *s){ \
		return planner_learn(s); \
	} \
	int X##export_wisdom_to_filename(const char *path){ \
		return wisdom_to(path); \
	} \
	int X##import_wisdom_from_filename(const char *path){ \
		return wisdom_from(path); \
	} \
	void X##forget_wisdom(void){ \
		planner_forget(); \
	}

K2FFT_DEFINE(k2fft_, double, k2fft_complex, 0)
K2FFT_DEFINE(k2fftf_, float, k2fftf_complex, 1)
