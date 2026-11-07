// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* any transform FFTW plans, through its interface (fftw3.h's: k2fft_ for double, k2fftf_ for float, FFTW's layouts, unnormalized),
   each by the fastest kernel this CPU has: kernel/table.h's candidates (the benchmark's), raced as FFTW's planner times its solvers */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#define K2FFT_FORWARD (-1)
#define K2FFT_BACKWARD (+1)

#define K2FFT_MEASURE 0U /* races the candidates, overwriting in and out, as FFTW_MEASURE */
#define K2FFT_PRESERVE_INPUT (1U<<4) /* a c2r leaves in as it was, as FFTW's flag */
#define K2FFT_ESTIMATE (1U<<6) /* the table's first, untimed; planning still writes in and out, so the input comes after */

typedef enum k2fft_r2r_kind { /* fftw_r2r_kind's */
	K2FFT_R2HC, K2FFT_HC2R, K2FFT_DHT,
	K2FFT_REDFT00, K2FFT_REDFT01, K2FFT_REDFT10, K2FFT_REDFT11, /* DCT-I..IV */
	K2FFT_RODFT00, K2FFT_RODFT01, K2FFT_RODFT10, K2FFT_RODFT11 /* DST-I..IV */
} k2fft_r2r_kind;

typedef struct k2fft_iodim {
	int n, is, os; /* a dimension's size, its input and output strides in the arrays' elements */
} k2fft_iodim;

typedef struct k2fft_plan_s *k2fft_plan;
typedef struct k2fft_plan_s *k2fftf_plan;
typedef double k2fft_complex[2];
typedef float k2fftf_complex[2];

/* X the prefix, R the real type, C the complex: FFTW's basic, advanced (many) and guru planners, execute and destroy */
#define K2FFT_DEFINE_API(X, R, C) \
	X##plan X##plan_dft(int rank, const int *n, C *in, C *out, int sign, unsigned flags); \
	X##plan X##plan_dft_1d(int n, C *in, C *out, int sign, unsigned flags); \
	X##plan X##plan_dft_2d(int n0, int n1, C *in, C *out, int sign, unsigned flags); \
	X##plan X##plan_dft_3d(int n0, int n1, int n2, C *in, C *out, int sign, unsigned flags); \
	X##plan X##plan_many_dft(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist, C *out, \
							 const int *onembed, int ostride, int odist, int sign, unsigned flags); \
	X##plan X##plan_guru_dft(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, C *in, C *out, \
							 int sign, unsigned flags); \
	X##plan X##plan_dft_r2c(int rank, const int *n, R *in, C *out, unsigned flags); \
	X##plan X##plan_dft_r2c_1d(int n, R *in, C *out, unsigned flags); \
	X##plan X##plan_dft_r2c_2d(int n0, int n1, R *in, C *out, unsigned flags); \
	X##plan X##plan_dft_r2c_3d(int n0, int n1, int n2, R *in, C *out, unsigned flags); \
	X##plan X##plan_many_dft_r2c(int rank, const int *n, int howmany, R *in, const int *inembed, int istride, int idist, C *out, \
								 const int *onembed, int ostride, int odist, unsigned flags); \
	X##plan X##plan_guru_dft_r2c(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, R *in, C *out, \
								 unsigned flags); \
	X##plan X##plan_dft_c2r(int rank, const int *n, C *in, R *out, unsigned flags); \
	X##plan X##plan_dft_c2r_1d(int n, C *in, R *out, unsigned flags); \
	X##plan X##plan_dft_c2r_2d(int n0, int n1, C *in, R *out, unsigned flags); \
	X##plan X##plan_dft_c2r_3d(int n0, int n1, int n2, C *in, R *out, unsigned flags); \
	X##plan X##plan_many_dft_c2r(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist, R *out, \
								 const int *onembed, int ostride, int odist, unsigned flags); \
	X##plan X##plan_guru_dft_c2r(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, C *in, R *out, \
								 unsigned flags); \
	X##plan X##plan_r2r(int rank, const int *n, R *in, R *out, const k2fft_r2r_kind *kind, unsigned flags); \
	X##plan X##plan_r2r_1d(int n, R *in, R *out, k2fft_r2r_kind kind, unsigned flags); \
	X##plan X##plan_r2r_2d(int n0, int n1, R *in, R *out, k2fft_r2r_kind kind0, k2fft_r2r_kind kind1, unsigned flags); \
	X##plan X##plan_r2r_3d(int n0, int n1, int n2, R *in, R *out, k2fft_r2r_kind kind0, k2fft_r2r_kind kind1, \
						   k2fft_r2r_kind kind2, unsigned flags); \
	X##plan X##plan_many_r2r(int rank, const int *n, int howmany, R *in, const int *inembed, int istride, int idist, R *out, \
							 const int *onembed, int ostride, int odist, const k2fft_r2r_kind *kind, unsigned flags); \
	X##plan X##plan_guru_r2r(int rank, const k2fft_iodim *dims, int howmany_rank, const k2fft_iodim *howmany_dims, R *in, R *out, \
							 const k2fft_r2r_kind *kind, unsigned flags); \
	void X##execute(const X##plan p); /* never allocates */ \
	void X##execute_dft(const X##plan p, C *in, C *out); /* other arrays, laid out and aligned as the planned ones */ \
	void X##execute_dft_r2c(const X##plan p, R *in, C *out); \
	void X##execute_dft_c2r(const X##plan p, C *in, R *out); \
	void X##execute_r2r(const X##plan p, R *in, R *out); \
	void X##destroy_plan(X##plan p); \
	const char *X##plan_kernel(const X##plan p); /* the library it runs, as its benchee's name */

K2FFT_DEFINE_API(k2fft_, double, k2fft_complex)
K2FFT_DEFINE_API(k2fftf_, float, k2fftf_complex)


#ifdef __cplusplus
}
#endif
