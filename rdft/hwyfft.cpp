// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* K2FFT's own real FFT, and the complex one inside it (backward swapped), on Highway's run-time dispatch: N/2 complex points as 4
   rows of N/8, a radix-4 pass that also splits even/odd and stores each column's 4 outputs as one 4-lane element, then AutoFFT's
   Stockham network (Li et al., SC19) over the elements, whose lanes are independent, so no load or store has a stride (GFFT, ICPP
   2023), and the real split last; plans time a few element widths and radix orders and keep the fastest, as AutoFFT's evaluator */
#include <chrono>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef HWYFFT_SHARED
#define HWYFFT_SHARED
#include "kernel/kernel.h"
#include "simd-support/simd.h"
#include <pffft/pffft.h> /* aligned buffers */

namespace hwyfft {
enum { STAGES = 24, PAD = 16 }; /* PAD: the floats a padded buffer skips each 2^shift elements, a cache line */
static const unsigned NOPAD = 8*sizeof(size_t)-1; /* an unpadded buffer's shift: j >> it is 0 */

/* a stage's addresses: element j of its input at e j + PAD (j >> in) floats, of its output at e j + PAD (j >> to) */
struct geo {
	size_t s, m; /* its stride and span, in elements */
	size_t span, out; /* floats from its input t to t+1, from its output k to k+1 */
	unsigned in, to;
};

#if defined(__aarch64__) && !defined(_MSC_VER) /* rdft/stages_neon64.S, which CMake builds w/ this file, but under MSVC */
#define K2FFT_NEON_STAGES 1
#endif
#ifdef K2FFT_NEON_STAGES /* scripts/asmgen's, on 4-lane elements: x, y re and im, the geo, the twiddles (W^(p k), k = 1 .. r-1) */
extern "C" {
void k2fft_neon_stage4(const float *, const float *, float *, float *, const geo *, const float *);
void k2fft_neon_stage4_tw(const float *, const float *, float *, float *, const geo *, const float *);
void k2fft_neon_stage8(const float *, const float *, float *, float *, const geo *, const float *);
void k2fft_neon_stage8_tw(const float *, const float *, float *, float *, const geo *, const float *);
}

/* radix r's stage as scripts/asmgen wrote it, 0 if it wrote none */
static int generated(int r, const float *xr, const float *xi, float *yr, float *yi, const geo &g, const float *tw){
	if (r!=4 && r!=8){
		return 0;
	}
	if (r==4){
		(tw ? k2fft_neon_stage4_tw : k2fft_neon_stage4)(xr, xi, yr, yi, &g, tw);
	} else {
		(tw ? k2fft_neon_stage8_tw : k2fft_neon_stage8)(xr, xi, yr, yi, &g, tw);
	}
	return 1;
}
#endif

/* padded where l, the elements, is a large power of 2, as its stages' r inputs, l/r elements apart, would share a cache set */
struct plan {
	int n; /* real points */
	int w; /* an element's lanes, the first pass's radix: 4, or a whole vector's 8 or 16 */
	int zip; /* w 4: the first pass w/o structure loads and stores (split4z) */
	int cplx; /* a complex transform of n/2 points: their (re, im) pairs out, w/o the real split */
	int back; /* cplx, backward */
	int as; /* its radix 4 and 8 stages scripts/asmgen's (rdft/stages_neon64.S), where they're built */
	int zero; /* the input's second half zeros (K2FFT_HALF_ZERO): w 4's first pass skips it (split4h) */
	int fuse; /* the last stage and the real split as one pass (last_split): its radix 2 or 4, 4-lane elements, its s even */
	int stages;
	int radix[STAGES];
	geo g[STAGES];
	unsigned shift; /* NOPAD, or the padded buffers' (the first pass's output, each stage's but the last's) */
	size_t im, half; /* floats from a buffer's re to its im, from buf's first buffer to its second */
	float *tw[STAGES]; /* W_(radix m)^(p k), k = 1 .. radix-1, for each p: (re, im) */
	float *twa; /* the first pass's W_(n/2)^(p k): k = 1 .. w-1, re then im, n/(2w) each */
	float *wr; /* W_n^k/2, k = 0 .. n/4 */
	float *wi;
	float *buf; /* an n/2-point buffer, re then im, out the other; padded, two */
};

/* radices of l = 2^a 3^b 5^c, a >= 2: the 5s, the 3s, then 8s and 4s, a 2 last; -1 if l has another factor */
static int factor(int l, int *r){
	int n = 0;
	while (l%5==0){
		r[n++] = 5;
		l /= 5;
	}
	while (l%3==0){
		r[n++] = 3;
		l /= 3;
	}
	int a = 0;
	while (l%2==0){
		a++;
		l /= 2;
	}
	while (a>=3 && a!=4){
		r[n++] = 8;
		a -= 3;
	}
	while (a>=2){
		r[n++] = 4;
		a -= 2;
	}
	if (a==1){
		r[n++] = 2;
	}
	return l==1 && n<=STAGES ? n : -1;
}
}
#endif

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "rdft/hwyfft.cpp"
#include <hwy/foreach_target.h> // IWYU pragma: keep
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();
namespace hwyfft {
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET == HWY_SCALAR /* one lane: never chosen, as usable says */
int usable(void){
	return 0;
}

int widest(void){
	return 4;
}

void forward(const plan *, const float *, float *){
}
#else
using D4 = hn::FixedTag<float, 4>; /* one 4-lane element */
using DF = hn::ScalableTag<float>; /* the widest: Lanes/4 whole 4-lane elements */
#undef HWYFFT_WIDE /* vectors w/ a size, which arrays can hold: not SVE's or RVV's */
#define HWYFFT_WIDE (!HWY_HAVE_SCALABLE && !HWY_TARGET_IS_SVE && HWY_TARGET != HWY_EMU128)

/* the widest element: a whole vector's 8 or 16 lanes where vectors have a size, else 4 */
int widest(void){
#if !HWYFFT_WIDE
	return 4;
#else
	const size_t n = hn::Lanes(DF());
	return n>=16 ? 16 : (n>=8 ? 8 : 4);
#endif
}

/* a *= w: a multiply and a multiply-add each, as NEON's fmls (NegMulAdd) needs no negation, which MulSub would */
template <class V>
HWY_INLINE void cmul(V &ar, V &ai, V wr, V wi){
	const V r = hn::NegMulAdd(ai, wi, hn::Mul(ar, wr));
	ai = hn::MulAdd(ai, wr, hn::Mul(ar, wi));
	ar = r;
}

/* (a0 .. a3) = their DFT, in place */
template <class V>
HWY_INLINE void dft4(V &a0r, V &a0i, V &a1r, V &a1i, V &a2r, V &a2i, V &a3r, V &a3i){
	const V t0r = hn::Add(a0r, a2r), t0i = hn::Add(a0i, a2i);
	const V t1r = hn::Sub(a0r, a2r), t1i = hn::Sub(a0i, a2i);
	const V t2r = hn::Add(a1r, a3r), t2i = hn::Add(a1i, a3i);
	const V t3r = hn::Sub(a1i, a3i), t3i = hn::Sub(a3r, a1r); /* (a1 - a3)(-i) */
	a0r = hn::Add(t0r, t2r);
	a0i = hn::Add(t0i, t2i);
	a2r = hn::Sub(t0r, t2r);
	a2i = hn::Sub(t0i, t2i);
	a1r = hn::Add(t1r, t3r);
	a1i = hn::Add(t1i, t3i);
	a3r = hn::Sub(t1r, t3r);
	a3i = hn::Sub(t1i, t3i);
}

/* (re, im) pairs at x to re and im, S swapped: a backward transform's, as IDFT x = swap DFT swap x, swap (re, im) = (im, re) */
template <bool S, class D>
HWY_INLINE void load2(D d, const float *HWY_RESTRICT x, hn::Vec<D> &re, hn::Vec<D> &im){
	if (S){
		hn::LoadInterleaved2(d, x, im, re);
	} else {
		hn::LoadInterleaved2(d, x, re, im);
	}
}

/* the radix-4 pass: z[l t + p] = x[2(l t + p)] + i x[2(l t + p) + 1], t = 0 .. 3, to element p's 4 lanes, twiddled; l a
   multiple of d's lanes */
template <bool S, class D>
HWY_NOINLINE void split4(D d, const float *HWY_RESTRICT x, float *HWY_RESTRICT yr, float *HWY_RESTRICT yi, size_t l,
						 const float *HWY_RESTRICT tw, unsigned sh){
	using V = hn::Vec<D>;
	const size_t n = hn::Lanes(d);
	for (size_t p=0; p<l; p+=n){
		V a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i;
		load2<S>(d, x+2*p, a0r, a0i);
		load2<S>(d, x+2*(l+p), a1r, a1i);
		load2<S>(d, x+2*(2*l+p), a2r, a2i);
		load2<S>(d, x+2*(3*l+p), a3r, a3i);
		dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i);
		cmul(a1r, a1i, hn::Load(d, tw+p), hn::Load(d, tw+l+p));
		cmul(a2r, a2i, hn::Load(d, tw+2*l+p), hn::Load(d, tw+3*l+p));
		cmul(a3r, a3i, hn::Load(d, tw+4*l+p), hn::Load(d, tw+5*l+p));
		const size_t o = 4*p+PAD*(p>>sh);
		hn::StoreInterleaved4(a0r, a1r, a2r, a3r, d, yr+o);
		hn::StoreInterleaved4(a0i, a1i, a2i, a3i, d, yi+o);
	}
}

/* split4 where z's second half (t 2 and 3) is zeros, a K2FFT_HALF_ZERO input's, as FFTW's TODO's pruned FFTs: the DFT4 of a0 and a1
   alone, half the loads */
template <bool S, class D>
HWY_NOINLINE void split4h(D d, const float *HWY_RESTRICT x, float *HWY_RESTRICT yr, float *HWY_RESTRICT yi, size_t l,
						  const float *HWY_RESTRICT tw, unsigned sh){
	using V = hn::Vec<D>;
	const size_t n = hn::Lanes(d);
	for (size_t p=0; p<l; p+=n){
		V a0r, a0i, a1r, a1i;
		load2<S>(d, x+2*p, a0r, a0i);
		load2<S>(d, x+2*(l+p), a1r, a1i);
		const V b0r = hn::Add(a0r, a1r), b0i = hn::Add(a0i, a1i);
		V b2r = hn::Sub(a0r, a1r), b2i = hn::Sub(a0i, a1i);
		V b1r = hn::Add(a0r, a1i), b1i = hn::Sub(a0i, a1r); /* a0 - i a1 */
		V b3r = hn::Sub(a0r, a1i), b3i = hn::Add(a0i, a1r); /* a0 + i a1 */
		cmul(b1r, b1i, hn::Load(d, tw+p), hn::Load(d, tw+l+p));
		cmul(b2r, b2i, hn::Load(d, tw+2*l+p), hn::Load(d, tw+3*l+p));
		cmul(b3r, b3i, hn::Load(d, tw+4*l+p), hn::Load(d, tw+5*l+p));
		const size_t o = 4*p+PAD*(p>>sh);
		hn::StoreInterleaved4(b0r, b1r, b2r, b3r, d, yr+o);
		hn::StoreInterleaved4(b0i, b1i, b2i, b3i, d, yi+o);
	}
}

/* lane j of a, b, c, e to vector j: zips of 32-bit lanes, then of 64-bit */
template <class D>
HWY_INLINE void transpose4(D d, hn::Vec<D> &a, hn::Vec<D> &b, hn::Vec<D> &c, hn::Vec<D> &e){
	const hn::Repartition<uint64_t, D> du;
	const auto t0 = hn::BitCast(du, hn::InterleaveLower(d, a, b)), t1 = hn::BitCast(du, hn::InterleaveUpper(d, a, b));
	const auto t2 = hn::BitCast(du, hn::InterleaveLower(d, c, e)), t3 = hn::BitCast(du, hn::InterleaveUpper(d, c, e));
	a = hn::BitCast(d, hn::InterleaveLower(du, t0, t2));
	b = hn::BitCast(d, hn::InterleaveUpper(du, t0, t2));
	c = hn::BitCast(d, hn::InterleaveLower(du, t1, t3));
	e = hn::BitCast(d, hn::InterleaveUpper(du, t1, t3));
}

/* 4 (re, im) pairs at x, unzipped w/o ld2 */
HWY_INLINE void unzip(const float *HWY_RESTRICT x, hn::Vec<D4> &re, hn::Vec<D4> &im){
	const D4 d;
	const hn::Vec<D4> lo = hn::LoadU(d, x), hi = hn::LoadU(d, x+4);
	re = hn::ConcatEven(d, hi, lo);
	im = hn::ConcatOdd(d, hi, lo);
}

/* split4 on 4 lanes w/o structure loads and stores (ld2, st4), slow on some cores (SVE's on Neoverse N2): unzips, then zips */
template <bool S>
HWY_NOINLINE void split4z(const float *HWY_RESTRICT x, float *HWY_RESTRICT yr, float *HWY_RESTRICT yi, size_t l,
						  const float *HWY_RESTRICT tw, unsigned sh){
	using V = hn::Vec<D4>;
	const D4 d;
	for (size_t p=0; p<l; p+=4){
		V a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i;
		unzip(x+2*p, S ? a0i : a0r, S ? a0r : a0i);
		unzip(x+2*(l+p), S ? a1i : a1r, S ? a1r : a1i);
		unzip(x+2*(2*l+p), S ? a2i : a2r, S ? a2r : a2i);
		unzip(x+2*(3*l+p), S ? a3i : a3r, S ? a3r : a3i);
		dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i);
		cmul(a1r, a1i, hn::Load(d, tw+p), hn::Load(d, tw+l+p));
		cmul(a2r, a2i, hn::Load(d, tw+2*l+p), hn::Load(d, tw+3*l+p));
		cmul(a3r, a3i, hn::Load(d, tw+4*l+p), hn::Load(d, tw+5*l+p));
		transpose4(d, a0r, a1r, a2r, a3r);
		transpose4(d, a0i, a1i, a2i, a3i);
		const size_t o = 4*p+PAD*(p>>sh);
		hn::StoreU(a0r, d, yr+o);
		hn::StoreU(a1r, d, yr+o+4);
		hn::StoreU(a2r, d, yr+o+8);
		hn::StoreU(a3r, d, yr+o+12);
		hn::StoreU(a0i, d, yi+o);
		hn::StoreU(a1i, d, yi+o+4);
		hn::StoreU(a2i, d, yi+o+8);
		hn::StoreU(a3i, d, yi+o+12);
	}
}

#if HWYFFT_WIDE
/* W_16^k = cos(2 pi k/16) - i sin(2 pi k/16) */
static const float cos16[16] = {1.0f, 0.923879532511286756f, 0.707106781186547524f, 0.382683432365089772f, 0.0f, -0.382683432365089772f,
								-0.707106781186547524f, -0.923879532511286756f, -1.0f, -0.923879532511286756f, -0.707106781186547524f,
								-0.382683432365089772f, 0.0f, 0.382683432365089772f, 0.707106781186547524f, 0.923879532511286756f};
static const float sin16[16] = {0.0f, 0.382683432365089772f, 0.707106781186547524f, 0.923879532511286756f, 1.0f, 0.923879532511286756f,
								0.707106781186547524f, 0.382683432365089772f, 0.0f, -0.382683432365089772f, -0.707106781186547524f,
								-0.923879532511286756f, -1.0f, -0.923879532511286756f, -0.707106781186547524f, -0.382683432365089772f};

/* v's W x W transpose, in place: 8 x 8 as transpose4 in each 128-bit block, then the blocks (24 shuffles; whole-vector interleaves cost 48
   on AVX2); 16 x 16 as log2(16) rounds of whole-vector interleaves, one vpermt2ps each on AVX-512 */
template <size_t W, class D>
HWY_INLINE void transpose(D d, hn::Vec<D> *v){
	if (W==8){
		transpose4(d, v[0], v[1], v[2], v[3]);
		transpose4(d, v[4], v[5], v[6], v[7]);
		HWY_UNROLL(4)
		for (size_t j=0; j<4; j++){
			const hn::Vec<D> lo = hn::ConcatLowerLower(d, v[4+j], v[j]);
			v[4+j] = hn::ConcatUpperUpper(d, v[4+j], v[j]);
			v[j] = lo;
		}
		return;
	}
	HWY_UNROLL(4)
	for (size_t h=W/2; h>=1; h/=2){
		HWY_UNROLL(16)
		for (size_t b=0; b<W; b+=2*h){
			HWY_UNROLL(16)
			for (size_t i=0; i<h; i++){
				const hn::Vec<D> lo = hn::InterleaveWholeLower(d, v[b+i], v[b+i+h]);
				v[b+i+h] = hn::InterleaveWholeUpper(d, v[b+i], v[b+i+h]);
				v[b+i] = lo;
			}
		}
	}
}

/* the radix-W pass, W a whole vector's lanes: rows t = 0 .. W-1 as the radix-4 pass's, a radix-2 DIF network (its outputs
   bit-reversed), twiddled, then transposed into W elements of W lanes; l a multiple of W. Unrolled, so the arrays are registers */
template <size_t W, bool S>
HWY_NOINLINE void splitw(const float *HWY_RESTRICT x, float *HWY_RESTRICT yr, float *HWY_RESTRICT yi, size_t l,
						 const float *HWY_RESTRICT tw, unsigned sh){
	const hn::CappedTag<float, W> d;
	using V = hn::Vec<decltype(d)>;
	for (size_t p=0; p<l; p+=W){
		V ar[W], ai[W], tr[W], ti[W];
		HWY_UNROLL(16)
		for (size_t t=0; t<W; t++){
			load2<S>(d, x+2*(t*l+p), ar[t], ai[t]);
		}
		HWY_UNROLL(4)
		for (size_t len=W; len>=2; len/=2){
			const size_t h = len/2;
			HWY_UNROLL(16)
			for (size_t b=0; b<W; b+=len){
				HWY_UNROLL(16)
				for (size_t i=0; i<h; i++){
					V dr = hn::Sub(ar[b+i], ar[b+i+h]), di = hn::Sub(ai[b+i], ai[b+i+h]);
					ar[b+i] = hn::Add(ar[b+i], ar[b+i+h]);
					ai[b+i] = hn::Add(ai[b+i], ai[b+i+h]);
					const size_t k = i*(16/len); /* W_len^i = W_16^k */
					if (k==4){ /* -i */
						const V t = dr;
						dr = di;
						di = hn::Neg(t);
					} else if (k){
						cmul(dr, di, hn::Set(d, cos16[k]), hn::Set(d, -sin16[k]));
					}
					ar[b+i+h] = dr;
					ai[b+i+h] = di;
				}
			}
		}
		HWY_UNROLL(16)
		for (size_t k=0; k<W; k++){
			size_t r = 0; /* k's bits reversed */
			HWY_UNROLL(4)
			for (size_t j=1; j<W; j*=2){
				r = 2*r+((k/j)&1);
			}
			tr[k] = ar[r];
			ti[k] = ai[r];
			if (k){
				cmul(tr[k], ti[k], hn::Load(d, tw+2*(k-1)*l+p), hn::Load(d, tw+(2*(k-1)+1)*l+p));
			}
		}
		transpose<W>(d, tr);
		transpose<W>(d, ti);
		HWY_UNROLL(16)
		for (size_t j=0; j<W; j++){
			hn::Store(tr[j], d, yr+W*(p+j)+PAD*(p>>sh));
			hn::Store(ti[j], d, yi+W*(p+j)+PAD*(p>>sh));
		}
	}
}
#endif

/* one Stockham stage (OTFFT's DIF form): y[q + s(r p + k)] = W_(r m)^(p k) sum_t x[q + s(p + t m)] W_r^(t k), on elements of e
   lanes, d's Lanes/e of them at once: s a multiple of that; addresses as g says */
#define STAGE_BEGIN(R) \
	const size_t s = g.s, m = g.m, span = g.span, out = g.out; \
	const size_t step = hn::Lanes(d)/e; \
	for (size_t p=0; p<m; p++){ \
		const float *w = TW ? tw+2*((R)-1)*p : NULL; \
		for (size_t q=0; q<s; q+=step){ \
			const size_t ix = q+s*p, ox = q+s*(R)*p; /* the first input's element, the first output's */ \
			const float *ar = xr+e*ix+PAD*(ix>>g.in); \
			const float *ai = xi+e*ix+PAD*(ix>>g.in); \
			float *br = yr+e*ox+PAD*(ox>>g.to); \
			float *bi = IL ? br : yi+e*ox+PAD*(ox>>g.to); \
			float *bo = yr+2*e*ox; /* IL's, the last stage's (no pad): (re, im) pairs, as pairs makes them */
#define STAGE_END \
		} \
	}
#define LOAD(t) V a##t##r = hn::Load(d, ar+(t)*span), a##t##i = hn::Load(d, ai+(t)*span)
#define TWIDDLE(t, k) \
	if (TW){ \
		cmul(a##t##r, a##t##i, hn::Set(d, w[2*((k)-1)]), hn::Set(d, w[2*((k)-1)+1])); \
	}
#define STORE(t, k) \
	if (IL){ \
		hn::StoreInterleaved2(a##t##r, a##t##i, d, bo+2*(k)*out); \
	} else { \
		hn::Store(a##t##r, d, br+(k)*out); \
		hn::Store(a##t##i, d, bi+(k)*out); \
	}
#define STAGE_ARGS \
	D d, size_t e, const float *HWY_RESTRICT xr, const float *HWY_RESTRICT xi, float *HWY_RESTRICT yr, float *HWY_RESTRICT yi, \
	const geo &g, const float *HWY_RESTRICT tw

template <class D, bool TW, bool IL>
HWY_NOINLINE void stage2(STAGE_ARGS){
	using V = hn::Vec<D>;
	STAGE_BEGIN(2)
	LOAD(0);
	LOAD(1);
	const V tr = hn::Sub(a0r, a1r), ti = hn::Sub(a0i, a1i);
	a0r = hn::Add(a0r, a1r);
	a0i = hn::Add(a0i, a1i);
	a1r = tr;
	a1i = ti;
	TWIDDLE(1, 1)
	STORE(0, 0);
	STORE(1, 1);
	STAGE_END
}

template <class D, bool TW, bool IL>
HWY_NOINLINE void stage3(STAGE_ARGS){
	using V = hn::Vec<D>;
	const V c = hn::Set(d, -0.5f), h = hn::Set(d, 0.866025403784438647f); /* cos, sin 2 pi/3 */
	STAGE_BEGIN(3)
	LOAD(0);
	LOAD(1);
	LOAD(2);
	const V sr = hn::Add(a1r, a2r), si = hn::Add(a1i, a2i);
	const V dr = hn::Sub(a1r, a2r), di = hn::Sub(a1i, a2i);
	const V mr = hn::MulAdd(sr, c, a0r), mi = hn::MulAdd(si, c, a0i);
	a0r = hn::Add(a0r, sr);
	a0i = hn::Add(a0i, si);
	a1r = hn::MulAdd(di, h, mr); /* m - i h d */
	a1i = hn::NegMulAdd(dr, h, mi);
	a2r = hn::NegMulAdd(di, h, mr);
	a2i = hn::MulAdd(dr, h, mi);
	TWIDDLE(1, 1)
	TWIDDLE(2, 2)
	STORE(0, 0);
	STORE(1, 1);
	STORE(2, 2);
	STAGE_END
}

template <class D, bool TW, bool IL>
HWY_NOINLINE void stage4(STAGE_ARGS){
	using V = hn::Vec<D>;
	STAGE_BEGIN(4)
	LOAD(0);
	LOAD(1);
	LOAD(2);
	LOAD(3);
	dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i);
	TWIDDLE(1, 1)
	TWIDDLE(2, 2)
	TWIDDLE(3, 3)
	STORE(0, 0);
	STORE(1, 1);
	STORE(2, 2);
	STORE(3, 3);
	STAGE_END
}

template <class D, bool TW, bool IL>
HWY_NOINLINE void stage5(STAGE_ARGS){
	using V = hn::Vec<D>;
	const V c1 = hn::Set(d, 0.309016994374947424f), c2 = hn::Set(d, -0.809016994374947424f); /* cos 2 pi/5, 4 pi/5 */
	const V s1 = hn::Set(d, 0.951056516295153572f), s2 = hn::Set(d, 0.587785252292473129f); /* sin */
	STAGE_BEGIN(5)
	LOAD(0);
	LOAD(1);
	LOAD(2);
	LOAD(3);
	LOAD(4);
	const V p14r = hn::Add(a1r, a4r), p14i = hn::Add(a1i, a4i), m14r = hn::Sub(a1r, a4r), m14i = hn::Sub(a1i, a4i);
	const V p23r = hn::Add(a2r, a3r), p23i = hn::Add(a2i, a3i), m23r = hn::Sub(a2r, a3r), m23i = hn::Sub(a2i, a3i);
	const V r1r = hn::MulAdd(p23r, c2, hn::MulAdd(p14r, c1, a0r)), r1i = hn::MulAdd(p23i, c2, hn::MulAdd(p14i, c1, a0i));
	const V r2r = hn::MulAdd(p23r, c1, hn::MulAdd(p14r, c2, a0r)), r2i = hn::MulAdd(p23i, c1, hn::MulAdd(p14i, c2, a0i));
	const V i1r = hn::MulAdd(m23r, s2, hn::Mul(m14r, s1)), i1i = hn::MulAdd(m23i, s2, hn::Mul(m14i, s1));
	const V i2r = hn::NegMulAdd(m23r, s1, hn::Mul(m14r, s2)), i2i = hn::NegMulAdd(m23i, s1, hn::Mul(m14i, s2));
	a0r = hn::Add(a0r, hn::Add(p14r, p23r));
	a0i = hn::Add(a0i, hn::Add(p14i, p23i));
	a1r = hn::Add(r1r, i1i); /* r1 - i i1 */
	a1i = hn::Sub(r1i, i1r);
	a4r = hn::Sub(r1r, i1i);
	a4i = hn::Add(r1i, i1r);
	a2r = hn::Add(r2r, i2i);
	a2i = hn::Sub(r2i, i2r);
	a3r = hn::Sub(r2r, i2i);
	a3i = hn::Add(r2i, i2r);
	TWIDDLE(1, 1)
	TWIDDLE(2, 2)
	TWIDDLE(3, 3)
	TWIDDLE(4, 4)
	STORE(0, 0);
	STORE(1, 1);
	STORE(2, 2);
	STORE(3, 3);
	STORE(4, 4);
	STAGE_END
}

/* radix 8 as 2 x 4: e_t = a_t + a_(t+4), o_t = (a_t - a_(t+4)) W_8^t, then b_2k = DFT4(e)_k, b_(2k+1) = DFT4(o)_k */
template <class D, bool TW, bool IL>
HWY_NOINLINE void stage8(STAGE_ARGS){
	using V = hn::Vec<D>;
	const V h = hn::Set(d, 0.707106781186547524f);
	STAGE_BEGIN(8)
	LOAD(0);
	LOAD(1);
	LOAD(2);
	LOAD(3);
	LOAD(4);
	LOAD(5);
	LOAD(6);
	LOAD(7);
	V o0r = hn::Sub(a0r, a4r), o0i = hn::Sub(a0i, a4i);
	V o1r = hn::Sub(a1r, a5r), o1i = hn::Sub(a1i, a5i);
	V o2r = hn::Sub(a2i, a6i), o2i = hn::Sub(a6r, a2r); /* (a2 - a6)(-i) */
	V o3r = hn::Sub(a3r, a7r), o3i = hn::Sub(a3i, a7i);
	a0r = hn::Add(a0r, a4r);
	a0i = hn::Add(a0i, a4i);
	a1r = hn::Add(a1r, a5r);
	a1i = hn::Add(a1i, a5i);
	a2r = hn::Add(a2r, a6r);
	a2i = hn::Add(a2i, a6i);
	a3r = hn::Add(a3r, a7r);
	a3i = hn::Add(a3i, a7i);
	const V t1r = hn::Mul(hn::Add(o1r, o1i), h), t1i = hn::Mul(hn::Sub(o1i, o1r), h); /* (1 - i)/sqrt 2 */
	const V t3r = hn::Mul(hn::Sub(o3i, o3r), h), t3i = hn::Neg(hn::Mul(hn::Add(o3r, o3i), h)); /* (-1 - i)/sqrt 2 */
	o1r = t1r;
	o1i = t1i;
	o3r = t3r;
	o3i = t3i;
	dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i); /* b0 b2 b4 b6 */
	dft4(o0r, o0i, o1r, o1i, o2r, o2i, o3r, o3i); /* b1 b3 b5 b7 */
	a4r = o0r;
	a4i = o0i;
	a5r = o1r;
	a5i = o1i;
	a6r = o2r;
	a6i = o2i;
	a7r = o3r;
	a7i = o3i;
	TWIDDLE(1, 2)
	TWIDDLE(2, 4)
	TWIDDLE(3, 6)
	TWIDDLE(4, 1)
	TWIDDLE(5, 3)
	TWIDDLE(6, 5)
	TWIDDLE(7, 7)
	STORE(0, 0);
	STORE(1, 2);
	STORE(2, 4);
	STORE(3, 6);
	STORE(4, 1);
	STORE(5, 3);
	STORE(6, 5);
	STORE(7, 7);
	STAGE_END
}

/* radix 16 as 4 x 4: c_(t, k) = DFT4(a_t, a_(4+t), a_(8+t), a_(12+t))_k W_16^(t k), in a_(4k+t), then b_(k+4j) = DFT4(c_(0..3, k))_j,
   in a_(4k+j) */
template <class D, bool TW, bool IL>
HWY_NOINLINE void stage16(STAGE_ARGS){
	using V = hn::Vec<D>;
	const V cs = hn::Set(d, 0.923879532511286756f), sn = hn::Set(d, 0.382683432365089772f), h = hn::Set(d, 0.707106781186547524f);
	STAGE_BEGIN(16)
	LOAD(0);
	LOAD(1);
	LOAD(2);
	LOAD(3);
	LOAD(4);
	LOAD(5);
	LOAD(6);
	LOAD(7);
	LOAD(8);
	LOAD(9);
	LOAD(10);
	LOAD(11);
	LOAD(12);
	LOAD(13);
	LOAD(14);
	LOAD(15);
	dft4(a0r, a0i, a4r, a4i, a8r, a8i, a12r, a12i);
	dft4(a1r, a1i, a5r, a5i, a9r, a9i, a13r, a13i);
	dft4(a2r, a2i, a6r, a6i, a10r, a10i, a14r, a14i);
	dft4(a3r, a3i, a7r, a7i, a11r, a11i, a15r, a15i);
	cmul(a5r, a5i, cs, hn::Neg(sn)); /* W_16^1 */
	cmul(a13r, a13i, sn, hn::Neg(cs)); /* W_16^3 */
	cmul(a7r, a7i, sn, hn::Neg(cs));
	cmul(a15r, a15i, hn::Neg(cs), sn); /* W_16^9 */
	V t = a9r; /* W_16^2 = (1 - i)/sqrt 2 */
	a9r = hn::Mul(hn::Add(t, a9i), h);
	a9i = hn::Mul(hn::Sub(a9i, t), h);
	t = a6r;
	a6r = hn::Mul(hn::Add(t, a6i), h);
	a6i = hn::Mul(hn::Sub(a6i, t), h);
	t = a14r; /* W_16^6 = (-1 - i)/sqrt 2 */
	a14r = hn::Mul(hn::Sub(a14i, t), h);
	a14i = hn::Neg(hn::Mul(hn::Add(t, a14i), h));
	t = a11r;
	a11r = hn::Mul(hn::Sub(a11i, t), h);
	a11i = hn::Neg(hn::Mul(hn::Add(t, a11i), h));
	t = a10r; /* W_16^4 = -i */
	a10r = a10i;
	a10i = hn::Neg(t);
	dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i);
	dft4(a4r, a4i, a5r, a5i, a6r, a6i, a7r, a7i);
	dft4(a8r, a8i, a9r, a9i, a10r, a10i, a11r, a11i);
	dft4(a12r, a12i, a13r, a13i, a14r, a14i, a15r, a15i);
	TWIDDLE(1, 4)
	TWIDDLE(2, 8)
	TWIDDLE(3, 12)
	TWIDDLE(4, 1)
	TWIDDLE(5, 5)
	TWIDDLE(6, 9)
	TWIDDLE(7, 13)
	TWIDDLE(8, 2)
	TWIDDLE(9, 6)
	TWIDDLE(10, 10)
	TWIDDLE(11, 14)
	TWIDDLE(12, 3)
	TWIDDLE(13, 7)
	TWIDDLE(14, 11)
	TWIDDLE(15, 15)
	STORE(0, 0);
	STORE(1, 4);
	STORE(2, 8);
	STORE(3, 12);
	STORE(4, 1);
	STORE(5, 5);
	STORE(6, 9);
	STORE(7, 13);
	STORE(8, 2);
	STORE(9, 6);
	STORE(10, 10);
	STORE(11, 14);
	STORE(12, 3);
	STORE(13, 7);
	STORE(14, 11);
	STORE(15, 15);
	STAGE_END
}

/* X_k = E + W_n^k O, X_(n/2-k) = conj(E - W_n^k O), E = (Z_k + conj Z_(n/2-k))/2, O = (Z_k - conj Z_(n/2-k))/2i, d's
   lanes of k at a time from k, as (re, im) pairs (FFTW's layout); where it stops */
template <class D>
HWY_INLINE size_t split_lanes(D d, const float *HWY_RESTRICT zr, const float *HWY_RESTRICT zi, float *HWY_RESTRICT x, size_t h,
							  size_t k, const float *HWY_RESTRICT wr, const float *HWY_RESTRICT wi){
	using V = hn::Vec<D>;
	const size_t n = hn::Lanes(d);
	const V half = hn::Set(d, 0.5f);
	const hn::RebindToSigned<D> di;
	const auto rev = hn::IndicesFromVec(d, hn::Sub(hn::Set(di, (int32_t)n-1), hn::Iota(di, 0))); /* one tbl on NEON; Reverse is two */
	for (; k+n<=h/2; k+=n){
		const V ar = hn::LoadU(d, zr+k), ai = hn::LoadU(d, zi+k);
		const V br = hn::TableLookupLanes(hn::LoadU(d, zr+h-k-(n-1)), rev), bi = hn::TableLookupLanes(hn::LoadU(d, zi+h-k-(n-1)), rev);
		const V er = hn::Mul(hn::Add(ar, br), half), ei = hn::Mul(hn::Sub(ai, bi), half);
		V tr = hn::Add(ai, bi), ti = hn::Sub(br, ar); /* 2 O, the twiddles halved */
		cmul(tr, ti, hn::LoadU(d, wr+k), hn::LoadU(d, wi+k));
		hn::StoreInterleaved2(hn::Add(er, tr), hn::Add(ei, ti), d, x+2*k);
		hn::StoreInterleaved2(hn::TableLookupLanes(hn::Sub(er, tr), rev), hn::TableLookupLanes(hn::Sub(ti, ei), rev), d,
							  x+2*(h-k-(n-1)));
	}
	return k;
}

/* the real split: the widest vectors, then 4 lanes, then one */
template <class D>
HWY_NOINLINE void split(D d, const float *HWY_RESTRICT zr, const float *HWY_RESTRICT zi, float *HWY_RESTRICT x, size_t h,
						const float *HWY_RESTRICT wr, const float *HWY_RESTRICT wi){
	x[0] = zr[0]+zi[0];
	x[1] = 0;
	x[2*h] = zr[0]-zi[0];
	x[2*h+1] = 0;
	size_t k = split_lanes(d, zr, zi, x, h, 1, wr, wi);
	k = split_lanes(D4(), zr, zi, x, h, k, wr, wi); /* the small sizes' few pairs */
	for (; k<=h/2; k++){
		const float ar = zr[k], ai = zi[k], br = zr[h-k], bi = zi[h-k];
		const float er = 0.5f*(ar+br), ei = 0.5f*(ai-bi), or_ = ai+bi, oi = br-ar;
		const float tr = or_*wr[k]-oi*wi[k], ti = or_*wi[k]+oi*wr[k];
		x[2*k] = er+tr;
		x[2*k+1] = ei+ti;
		x[2*(h-k)] = er-tr;
		x[2*(h-k)+1] = ti-ei;
	}
}

/* X_f .. X_f+3 and X_h-f-3 .. X_h-f from a = Z_f .. Z_f+3 and b = Z_h-f-3 .. Z_h-f, as split_lanes */
HWY_INLINE void pair_split(D4 d, hn::Vec<D4> ar, hn::Vec<D4> ai, hn::Vec<D4> br, hn::Vec<D4> bi, size_t f, size_t h,
						   const float *HWY_RESTRICT wr, const float *HWY_RESTRICT wi, float *HWY_RESTRICT x){
	using V = hn::Vec<D4>;
	const hn::RebindToSigned<D4> di;
	const auto rev = hn::IndicesFromVec(d, hn::Sub(hn::Set(di, 3), hn::Iota(di, 0)));
	br = hn::TableLookupLanes(br, rev);
	bi = hn::TableLookupLanes(bi, rev);
	const V half = hn::Set(d, 0.5f);
	const V er = hn::Mul(hn::Add(ar, br), half), ei = hn::Mul(hn::Sub(ai, bi), half);
	V tr = hn::Add(ai, bi), ti = hn::Sub(br, ar);
	cmul(tr, ti, hn::LoadU(d, wr+f), hn::LoadU(d, wi+f));
	hn::StoreInterleaved2(hn::Add(er, tr), hn::Add(ei, ti), d, x+2*f);
	hn::StoreInterleaved2(hn::TableLookupLanes(hn::Sub(er, tr), rev), hn::TableLookupLanes(hn::Sub(ti, ei), rev), d, x+2*(h-f-3));
}

/* the last stage's radix-R butterfly (R 2 or 4; m 1, no twiddles) on the 4 lanes at x, its inputs span floats apart */
template <int R, class V>
HWY_INLINE void last_butterfly(D4 d, const float *xr, const float *xi, size_t span, V &a0r, V &a0i, V &a1r, V &a1i, V &a2r, V &a2i,
							   V &a3r, V &a3i){
	a0r = hn::LoadU(d, xr);
	a0i = hn::LoadU(d, xi);
	a1r = hn::LoadU(d, xr+span);
	a1i = hn::LoadU(d, xi+span);
	if constexpr (R==2){
		const V tr = hn::Sub(a0r, a1r), ti = hn::Sub(a0i, a1i);
		a0r = hn::Add(a0r, a1r);
		a0i = hn::Add(a0i, a1i);
		a1r = tr;
		a1i = ti;
	} else {
		a2r = hn::LoadU(d, xr+2*span);
		a2i = hn::LoadU(d, xi+2*span);
		a3r = hn::LoadU(d, xr+3*span);
		a3i = hn::LoadU(d, xi+3*span);
		dft4(a0r, a0i, a1r, a1i, a2r, a2i, a3r, a3i);
	}
}

/* the last stage (radix R, m 1, S = g.s even, unpadded) and the real split as one pass, as Ne10's real FFT has none of its own:
   element j's lane L is Z_4j+L, so Z_h-f for f = 4j+1 .. 4j+4 is element l-1-j's, reversed, and loads a float on make a butterfly's
   outputs those lanes (j's 1..3, j+1's 0); q's (a float on) pair w/ S-1-q's, S-1-q's first R/2 w/ q's last; element 0's lane 0s
   (Z_4Sk) last */
template <int R>
HWY_NOINLINE void last_split(const float *HWY_RESTRICT xr, const float *HWY_RESTRICT xi, float *HWY_RESTRICT x, size_t h, const geo &g,
							 const float *HWY_RESTRICT wr, const float *HWY_RESTRICT wi){
	using V = hn::Vec<D4>;
	const D4 d;
	const size_t S = g.s, sp = g.span, o = 4*S; /* o: f from a butterfly's output k to k+1 */
	for (size_t q=0; q<S/2; q++){
		const size_t a = 4*q+1, b = 4*(S-1-q);
		V g0r, g0i, g1r, g1i, g2r, g2i, g3r, g3i, e0r, e0i, e1r, e1i, e2r, e2i, e3r, e3i;
		last_butterfly<R>(d, xr+a, xi+a, sp, g0r, g0i, g1r, g1i, g2r, g2i, g3r, g3i);
		last_butterfly<R>(d, xr+b, xi+b, sp, e0r, e0i, e1r, e1i, e2r, e2i, e3r, e3i);
		if constexpr (R==2){
			pair_split(d, g0r, g0i, e1r, e1i, a, h, wr, wi, x);
			pair_split(d, e0r, e0i, g1r, g1i, b, h, wr, wi, x);
		} else {
			pair_split(d, g0r, g0i, e3r, e3i, a, h, wr, wi, x);
			pair_split(d, g1r, g1i, e2r, e2i, a+o, h, wr, wi, x);
			pair_split(d, e0r, e0i, g3r, g3i, b, h, wr, wi, x);
			pair_split(d, e1r, e1i, g2r, g2i, b+o, h, wr, wi, x);
		}
	}
	V z0r, z0i, z1r, z1i, z2r, z2i, z3r, z3i;
	last_butterfly<R>(d, xr, xi, sp, z0r, z0i, z1r, z1i, z2r, z2i, z3r, z3i);
	const float r0 = hn::GetLane(z0r), i0 = hn::GetLane(z0i);
	x[0] = r0+i0;
	x[1] = 0;
	x[2*h] = r0-i0;
	x[2*h+1] = 0;
	if constexpr (R==2){ /* X_h/2 = conj Z_h/2 */
		x[h] = hn::GetLane(z1r);
		x[h+1] = -hn::GetLane(z1i);
	} else {
		x[h] = hn::GetLane(z2r);
		x[h+1] = -hn::GetLane(z2i);
		const float ar = hn::GetLane(z1r), ai = hn::GetLane(z1i), br = hn::GetLane(z3r), bi = hn::GetLane(z3i); /* Z_h/4, Z_3h/4 */
		const float er = 0.5f*(ar+br), ei = 0.5f*(ai-bi), or_ = ai+bi, oi = br-ar;
		const float tr = or_*wr[o]-oi*wi[o], ti = or_*wi[o]+oi*wr[o];
		x[2*o] = er+tr;
		x[2*o+1] = ei+ti;
		x[2*(h-o)] = er-tr;
		x[2*(h-o)+1] = ti-ei;
	}
}

/* a complex transform's: z's re and im as (re, im) pairs */
template <class D>
HWY_NOINLINE void pairs(D d, const float *HWY_RESTRICT zr, const float *HWY_RESTRICT zi, float *HWY_RESTRICT x, size_t h){
	const size_t n = hn::Lanes(d);
	size_t k = 0;
	for (; k+n<=h; k+=n){
		hn::StoreInterleaved2(hn::LoadU(d, zr+k), hn::LoadU(d, zi+k), d, x+2*k);
	}
	for (; k<h; k++){
		x[2*k] = zr[k];
		x[2*k+1] = zi[k];
	}
}

int usable(void){
	return 1;
}

/* a stage w/ radix r on d's vectors, elements of e lanes, TW if it has twiddles */
template <class D, bool TW, bool IL>
HWY_INLINE void radix(D d, size_t e, int r, const float *xr, const float *xi, float *yr, float *yi, const geo &g, const float *tw){
	switch (r){
	case 2:
		stage2<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	case 3:
		stage3<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	case 4:
		stage4<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	case 5:
		stage5<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	case 8:
		stage8<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	default:
		stage16<D, TW, IL>(d, e, xr, xi, yr, yi, g, tw);
		break;
	}
}

/* il, the last stage of a forward complex transform: (re, im) pairs into yr, not pairs' pass after */
template <class D>
HWY_INLINE void stage(D d, size_t e, int r, const float *xr, const float *xi, float *yr, float *yi, const geo &g, const float *tw,
					  bool il = false){
	if (il){
		if (tw){
			radix<D, true, true>(d, e, r, xr, xi, yr, yi, g, tw);
		} else {
			radix<D, false, true>(d, e, r, xr, xi, yr, yi, g, tw);
		}
	} else if (tw){
		radix<D, true, false>(d, e, r, xr, xi, yr, yi, g, tw);
	} else {
		radix<D, false, false>(d, e, r, xr, xi, yr, yi, g, tw);
	}
}

/* the first pass, S swapping its input (a backward transform's) */
template <bool S>
HWY_INLINE void first(const plan *p, const float *in, float *cr, float *ci, size_t l, unsigned sh){
	const DF df;
	const D4 d4;
#if HWYFFT_WIDE
	if (p->w==16){
		splitw<16, S>(in, cr, ci, l, p->twa, sh);
		return;
	}
	if (p->w==8){
		splitw<8, S>(in, cr, ci, l, p->twa, sh);
		return;
	}
#endif
	if (p->zero){
		if (l%hn::Lanes(df)==0){
			split4h<S>(df, in, cr, ci, l, p->twa, sh);
		} else {
			split4h<S>(d4, in, cr, ci, l, p->twa, sh);
		}
	} else if (p->zip){
		split4z<S>(in, cr, ci, l, p->twa, sh);
	} else if (l%hn::Lanes(df)==0){
		split4<S>(df, in, cr, ci, l, p->twa, sh);
	} else {
		split4<S>(d4, in, cr, ci, l, p->twa, sh);
	}
}

/* elements of a vector's lanes where the plan has them (every stage a vector an element), else 4-lane elements on the widest
   vectors where a stage's elements are whole and contiguous, one element's elsewhere */
void forward(const plan *p, const float *in, float *out){
	const DF df;
	const D4 d4;
	const size_t h = (size_t)p->n/2, e = (size_t)p->w, l = h/e, step = hn::Lanes(df)/4;
	const unsigned sh = p->shift;
	const int il = p->cplx && !p->back && p->stages>0; /* the last stage's pairs into out, as pairs' pass would */
	/* unpadded, out is the other buffer (n floats of its n+2), so the last stage writes the plan's for split to read, or (il, fuse)
	   reads the plan's while it writes out; padded, buf holds both */
	float *cr = (p->stages%2!=0)!=(il || p->fuse) ? out : p->buf, *nr = cr==out ? p->buf : out;
	size_t im = h;
	if (sh!=NOPAD){
		cr = p->buf;
		nr = p->buf+p->half;
		im = p->im;
	}
	float *ci = cr+im, *ni = nr+im;
	if (p->back){
		first<true>(p, in, cr, ci, l, sh);
	} else {
		first<false>(p, in, cr, ci, l, sh);
	}
	for (int i=0; i<p->stages; i++){
		const geo &g = p->g[i];
		const float *tw = g.m>1 ? p->tw[i] : NULL; /* p = 0 alone: no twiddles */
		const bool last = il && i==p->stages-1;
		float *yr = last ? out : nr;
		if (p->fuse && i==p->stages-1){
			if (p->radix[i]==2){
				last_split<2>(cr, ci, out, h, g, p->wr, p->wi);
			} else {
				last_split<4>(cr, ci, out, h, g, p->wr, p->wi);
			}
			return;
		}
#if HWYFFT_WIDE
		if (e==16){
			stage(hn::CappedTag<float, 16>(), 16, p->radix[i], cr, ci, yr, ni, g, tw, last);
		} else if (e==8){
			stage(hn::CappedTag<float, 8>(), 8, p->radix[i], cr, ci, yr, ni, g, tw, last);
		} else
#endif
#ifdef K2FFT_NEON_STAGES
		if (p->as && !last && generated(p->radix[i], cr, ci, nr, ni, g, tw)){
		} else
#endif
		if (step>1 && g.s%step==0){
			stage(df, 4, p->radix[i], cr, ci, yr, ni, g, tw, last);
		} else {
			stage(d4, 4, p->radix[i], cr, ci, yr, ni, g, tw, last);
		}
		float *t = cr;
		cr = nr;
		nr = t;
		t = ci;
		ci = ni;
		ni = t;
	}
	if (il){
		return;
	}
	if (p->cplx){
		pairs(df, p->back ? ci : cr, p->back ? cr : ci, out, h); /* swapped back */
	} else {
		split(df, cr, ci, out, h, p->wr, p->wi);
	}
}
#endif
}
}
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace hwyfft {
HWY_EXPORT(usable);
HWY_EXPORT(widest);
HWY_EXPORT(forward);

/* an element's lanes for size: the CPU's widest whose square divides size/2, so the first pass's vectors are whole */
static int width(int size){
	int w = HWY_DYNAMIC_DISPATCH(widest)();
	while (w>4 && (size/2)%(w*w)!=0){
		w /= 2;
	}
	return w;
}

/* its real points: a real forward's n, a complex one's 2n (the real one's n/2-point transform, w/o the split; backward, swapped) */
static int reals(const problem *pb){
	if (pb->kind==KIND_DFT && pb->single && pb->in!=pb->out){
		return 2*problem_1d(pb);
	}
	return problem_rof(pb);
}

static int can_do(const problem *pb){
	const int size = reals(pb);
	int r[STAGES];
	/* in and out aligned to the widest vector's 64 bytes: unpadded, out is a stage's buffer, whose loads and stores are aligned */
	return simd_any() && size>=32 && size%32==0 && ((uintptr_t)pb->in|(uintptr_t)pb->out)%64==0 && factor(size/8, r)>=0 &&
		   HWY_DYNAMIC_DISPATCH(usable)();
}

static void destroy(void *p_){
	plan *p = static_cast<plan *>(p_);
	for (int i=0; i<p->stages; i++){
		pffft_aligned_free(p->tw[i]);
	}
	pffft_aligned_free(p->twa);
	pffft_aligned_free(p->wr);
	pffft_aligned_free(p->wi);
	pffft_aligned_free(p->buf);
	free(p);
}

static float *floats(size_t n){
	return static_cast<float *>(pffft_aligned_malloc((n+4)*sizeof(float)));
}

/* W_n^j = exp(-2 pi i j/n), in double */
static void twiddle(long j, long n, float *re, float *im){
	const double a = -2*3.14159265358979323846*(double)(j%n)/(double)n;
	*re = (float)cos(a);
	*im = (float)sin(a);
}

/* the plan for size w/ elements of w lanes, these radices in order, its buffers padded each 2^shift elements (NOPAD: not) */
static plan *build(int size, int w, const int *radix, int stages, unsigned shift){
	plan *p = static_cast<plan *>(calloc(1, sizeof(plan)));
	if (!p){
		return NULL;
	}
	const int h = size/2, l = h/w;
	p->n = size;
	p->w = w;
	p->stages = stages;
	p->shift = shift;
	memcpy(p->radix, radix, (size_t)stages*sizeof(int));
	p->twa = floats(2*(size_t)(w-1)*(size_t)l);
	p->wr = floats((size_t)h/2+1);
	p->wi = floats((size_t)h/2+1);
	size_t room = 2*(size_t)h;
	if (shift!=NOPAD){
		p->im = (size_t)h+PAD*((size_t)l>>shift)+8*PAD; /* 8 lines past re's end, so re's and im's same elements don't share a set */
		p->half = 2*p->im;
		room = 2*p->half;
	}
	p->buf = floats(room);
	int ok = p->twa && p->wr && p->wi && p->buf;
	for (int k=1; ok && k<w; k++){
		for (int j=0; j<l; j++){
			twiddle((long)j*k, h, p->twa+2*(k-1)*l+j, p->twa+(2*(k-1)+1)*l+j);
		}
	}
	for (int k=0; ok && k<=h/2; k++){
		twiddle(k, size, p->wr+k, p->wi+k);
		p->wr[k] *= 0.5f; /* the real split's 1/2 */
		p->wi[k] *= 0.5f;
	}
	int s = 1;
	for (int i=0; ok && i<p->stages; i++){
		const int r = p->radix[i], m = l/(s*r);
		geo &g = p->g[i];
		g.s = (size_t)s;
		g.m = (size_t)m;
		g.in = shift;
		g.to = i+1<stages ? shift : NOPAD; /* the last's, for split, unpadded */
		g.span = (size_t)w*(size_t)(l/r)+PAD*((size_t)(l/r)>>g.in);
		g.out = (size_t)w*(size_t)s+PAD*((size_t)s>>g.to);
		p->tw[i] = floats(2*(size_t)(r-1)*(size_t)m);
		ok = p->tw[i]!=NULL;
		for (int j=0; ok && j<m; j++){
			for (int k=1; k<r; k++){
				twiddle((long)j*k, (long)r*m, p->tw[i]+2*((r-1)*j+k-1), p->tw[i]+2*((r-1)*j+k-1)+1);
			}
		}
		s *= r;
	}
	if (!ok){
		destroy(p);
		return NULL;
	}
	return p;
}

/* the radix orders AutoFFT's evaluator would time (Li et al., SC19): the powers of 2 as factor's (8s, 4s, a 2), reversed, w/ the
   smallest first, as the first stage (stride 1) runs one element a vector, or in 16s (GFFT's fewer, larger radices) first or last;
   each w/ the 3s and 5s first or last, so the radix w/o twiddles (the last) differs too; n's radices in r[i], k[i] of them, distinct */
enum { ORDERS = 10 };
static int orders(int l, int (*r)[STAGES], int *k){
	int f[STAGES], odd[STAGES], two[STAGES], seq[5][STAGES], len[5], no = 0, nt = 0, bits = 0;
	const int n = factor(l, f);
	if (n<0){
		return 0;
	}
	for (int i=0; i<n; i++){
		if (f[i]%2){
			odd[no++] = f[i];
		} else {
			two[nt++] = f[i];
			bits += f[i]==2 ? 1 : (f[i]==4 ? 2 : 3);
		}
	}
	for (int i=0; i<nt; i++){
		seq[0][i] = two[i];
		seq[1][i] = two[nt-1-i];
		seq[2][i] = i==0 ? two[nt-1] : two[i-1]; /* factor's ends w/ the smallest */
	}
	len[0] = len[1] = len[2] = nt;
	len[3] = 0;
	for (int b=bits; b>=4 && b!=5; b-=4){ /* 16s, then an 8 and a 4 for 5 bits, a 4 or an 8 for 2 or 3, a 2 for 1 */
		seq[3][len[3]++] = 16;
	}
	const int rest = bits-4*len[3];
	if (rest==5){
		seq[3][len[3]++] = 8;
		seq[3][len[3]++] = 4;
	} else if (rest>0){
		seq[3][len[3]++] = 1<<rest;
	}
	len[4] = len[3];
	for (int i=0; i<len[3]; i++){
		seq[4][i] = seq[3][len[3]-1-i];
	}
	int c = 0;
	for (int v=0; v<ORDERS; v++){
		int *o = r[c];
		int j = 0;
		if (v%2==0){
			for (int i=0; i<no; i++){
				o[j++] = odd[i];
			}
		}
		for (int i=0; i<len[v/2]; i++){
			o[j++] = seq[v/2][i];
		}
		if (v%2){
			for (int i=0; i<no; i++){
				o[j++] = odd[i];
			}
		}
		k[c] = j;
		int seen = 0;
		for (int i=0; i<c; i++){
			seen |= k[i]==j && memcmp(r[i], o, (size_t)j*sizeof(int))==0;
		}
		c += !seen;
	}
	return c;
}

/* p's best time per transform of a few runs, each long enough to time */
static double seconds(const plan *p, float *in, float *out){
	typedef std::chrono::steady_clock clock;
	double best = 0;
	long reps = 1;
	for (int run=0; run<4;){
		const clock::time_point t0 = clock::now();
		for (long i=0; i<reps; i++){
			HWY_DYNAMIC_DISPATCH(forward)(p, in, out);
		}
		const double t = std::chrono::duration<double>(clock::now()-t0).count();
		if (t<2e-5 && reps<(1L<<20)){
			reps *= 4;
			continue;
		}
		if (run++==0 || t/(double)reps<best){
			best = t/(double)reps;
		}
	}
	return best;
}

/* the shifts to time for l elements of w lanes in these radices: NOPAD, then where l is a power of 2 of 32 KB or more, each stage
   boundary G = 2^shift w/ 16 <= G <= l/16, so the first pass's vectors and every stage's loads and stores stay in one padded block
   (geo); their count */
static int shifts(int l, int w, const int *radix, int stages, unsigned *sh){
	int n = 0;
	sh[n++] = NOPAD;
	if ((l&(l-1))!=0 || w*l<8192){
		return n;
	}
	int s = 1;
	for (int i=0; i+1<stages; i++){
		s *= radix[i];
		if (s>=16 && s<=l/16){
			unsigned b = 0;
			while ((1<<b)<s){
				b++;
			}
			sh[n++] = b;
		}
	}
	return n;
}

/* the fastest width, order and padding where plans may time transforms (problem's measure), else the widest, factor's and none:
   narrower elements can win, as where 512-bit vectors run at half rate (Zen 4) or the 16 x 16 transposes spill */
static void *make(const problem *pb){
	const int size = reals(pb), cplx = pb->kind==KIND_DFT;
	float *in = static_cast<float *>(pb->in), *out = static_cast<float *>(pb->out);
	int r[ORDERS][STAGES], k[ORDERS];
	const int top = width(size), n = orders(size/2/top, r, k);
	if (n<=0){
		return NULL;
	}
	if (!pb->measure){
		plan *p = build(size, top, r[0], k[0], NOPAD);
		if (p){
			p->cplx = cplx;
			p->back = cplx && pb->sign>0;
			p->zero = pb->half;
		}
		return p;
	}
	memset(in, 0, (size_t)size*sizeof(float)); /* no denormals while timing */
	plan *best = NULL;
	double fastest = 0;
	for (int w=top; w>=4; w/=2){
		const int c = orders(size/2/w, r, k);
		for (int i=0; i<c; i++){
			unsigned sh[STAGES];
			const int ns = shifts(size/2/w, w, r[i], k[i], sh);
			for (int j=0; j<ns; j++){
				plan *p = build(size, w, r[i], k[i], sh[j]);
				if (!p){
					continue;
				}
				p->cplx = cplx;
				p->back = cplx && pb->sign>0;
				p->zero = pb->half;
				double t = seconds(p, in, out);
				if (w==4 && !p->zero){ /* the first pass's two forms; split4h has one */
					p->zip = 1;
					const double z = seconds(p, in, out);
					if (z<t){
						t = z;
					} else {
						p->zip = 0;
					}
				}
#ifdef K2FFT_NEON_STAGES
				if (w==4){ /* scripts/asmgen's stages, against Highway's; K2FFT_HWYFFT_AS=0 or 1 forces them, as perf.yml compares */
					const char *v = getenv("K2FFT_HWYFFT_AS");
					p->as = 1;
					const double a = v && *v=='0' ? 1e30 : seconds(p, in, out); /* forced, still timed: orders and pads compete */
					if (a<t || (v && *v=='1')){
						t = a;
					} else {
						p->as = 0;
					}
				}
#endif
				const int lr = p->radix[p->stages-1];
				if (w==4 && !cplx && sh[j]==NOPAD && (lr==2 || lr==4) && p->g[p->stages-1].s%2==0){ /* last_split; K2FFT_HWYFFT_FUSE as AS */
					const char *v = getenv("K2FFT_HWYFFT_FUSE");
					p->fuse = 1;
					const double f = v && *v=='0' ? 1e30 : seconds(p, in, out);
					if (f<t || (v && *v=='1')){
						t = f;
					} else {
						p->fuse = 0;
					}
				}
				if (!best || t<fastest){
					if (best){
						destroy(best);
					}
					best = p;
					fastest = t;
				} else {
					destroy(p);
				}
			}
		}
	}
	return best;
}

/* FFTW's layouts: a real one's size/2+1 (re, im) pairs, a complex one's size */
static void execute(const void *p, void *in, void *out){
	HWY_DYNAMIC_DISPATCH(forward)(static_cast<const plan *>(p), static_cast<const float *>(in), static_cast<float *>(out));
}
}

extern "C" const kernel k_hwyfft = {"hwyfft", hwyfft::can_do, hwyfft::make, hwyfft::execute, hwyfft::destroy, 0};
#endif
