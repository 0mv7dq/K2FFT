// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* rdft/trig.h's DCT-II/III/IV passes, as FFTW's TODO would have its little loops unified: whole vectors of the reorders (deinterleaved,
   reversed) and of the twiddles, Highway's targets chosen at run time as rdft/hwyfft.cpp's; the scalar ones' arguments (u in, w the
   twiddles, v out), their remainders */
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "rdft/trig_hwy.cpp"
#include <hwy/foreach_target.h> // IWYU pragma: keep
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();
namespace trighwy {
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

/* a[j] = x[2j], a[n-1-j] = +-x[2j+1] */
template <typename T>
HWY_INLINE void dct2_in(const T *HWY_RESTRICT x, const T *HWY_RESTRICT, T *HWY_RESTRICT a, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), m = (size_t)n;
	const auto sign = hn::Set(d, odd ? T(-1) : T(1)); /* no vector ?:, which SVE's sizeless types may not take */
	size_t j = 0;
	for (; 2*(j+N)<=m; j+=N){
		hn::Vec<decltype(d)> ev, od;
		hn::LoadInterleaved2(d, x+2*j, ev, od);
		hn::StoreU(ev, d, a+j);
		hn::StoreU(hn::Reverse(d, hn::Mul(od, sign)), d, a+m-j-N);
	}
	for (size_t i=j; 2*i<m; i++){
		a[i] = x[2*i];
	}
	for (size_t i=j; 2*i+1<m; i++){
		a[m-1-i] = odd ? -x[2*i+1] : x[2*i+1];
	}
}

/* lo_k = 2(re c + im s) into y[k], hi_k = 2(re s - im c) into y[n-k], 0 < k < n-k; odd: y[n-1-k] and y[k-1] */
template <typename T>
HWY_INLINE void dct2_out(const T *HWY_RESTRICT b, const T *HWY_RESTRICT w, T *HWY_RESTRICT y, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), m = (size_t)n;
	const auto two = hn::Set(d, T(2));
	size_t k = 1;
	for (; 2*(k+N-1)<m; k+=N){
		hn::Vec<decltype(d)> re, im, c, s;
		hn::LoadInterleaved2(d, b+2*k, re, im);
		hn::LoadInterleaved2(d, w+2*k, c, s);
		const auto lo = hn::Mul(two, hn::MulAdd(re, c, hn::Mul(im, s)));
		const auto hi = hn::Mul(two, hn::MulSub(re, s, hn::Mul(im, c)));
		if (odd){
			hn::StoreU(hn::Reverse(d, lo), d, y+m-k-N);
			hn::StoreU(hi, d, y+k-1);
		} else {
			hn::StoreU(lo, d, y+k);
			hn::StoreU(hn::Reverse(d, hi), d, y+m-k-N+1);
		}
	}
	for (size_t i=0; 2*i<=m; i=i==0 ? k : i+1){
		const T re = b[2*i], im = b[2*i+1], c = w[2*i], s = w[2*i+1];
		y[odd ? m-1-i : i] = 2*(re*c+im*s);
		if (i>0 && m-i!=i){
			y[odd ? i-1 : m-i] = 2*(re*s-im*c);
		}
	}
}

/* a[2k] = re c + im s, a[2k+1] = re s - im c, re x[k] and im x[n-k]; odd: x[n-1-k] and x[k-1]; a[1], and n even a[n+1], 0 */
template <typename T>
HWY_INLINE void dct3_in(const T *HWY_RESTRICT x, const T *HWY_RESTRICT w, T *HWY_RESTRICT a, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), m = (size_t)n;
	size_t k = 1;
	for (; 2*(k+N)<=m; k+=N){
		auto re = hn::LoadU(d, x+k), im = hn::Reverse(d, hn::LoadU(d, x+m-k-N+1));
		if (odd){
			re = hn::Reverse(d, hn::LoadU(d, x+m-k-N));
			im = hn::LoadU(d, x+k-1);
		}
		hn::Vec<decltype(d)> c, s;
		hn::LoadInterleaved2(d, w+2*k, c, s);
		hn::StoreInterleaved2(hn::MulAdd(re, c, hn::Mul(im, s)), hn::MulSub(re, s, hn::Mul(im, c)), d, a+2*k);
	}
	for (size_t i=0; 2*i<=m; i=i==0 ? k : i+1){
		const T re = odd ? x[m-1-i] : x[i], im = i==0 ? 0 : (odd ? x[i-1] : x[m-i]), c = w[2*i], s = w[2*i+1];
		a[2*i] = re*c+im*s;
		a[2*i+1] = re*s-im*c;
	}
	a[1] = 0;
	if (m%2==0){
		a[m+1] = 0;
	}
}

/* y[2j] = b[j], y[2j+1] = +-b[n-1-j] */
template <typename T>
HWY_INLINE void dct3_out(const T *HWY_RESTRICT b, const T *HWY_RESTRICT, T *HWY_RESTRICT y, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), m = (size_t)n;
	const auto sign = hn::Set(d, odd ? T(-1) : T(1));
	size_t j = 0;
	for (; 2*(j+N)<=m; j+=N){
		const auto od = hn::Reverse(d, hn::LoadU(d, b+m-j-N));
		hn::StoreInterleaved2(hn::LoadU(d, b+j), hn::Mul(od, sign), d, y+2*j);
	}
	for (size_t i=j; 2*i<m; i++){
		y[2*i] = b[i];
	}
	for (size_t i=j; 2*i+1<m; i++){
		y[2*i+1] = odd ? -b[m-1-i] : b[m-1-i];
	}
}

/* n even: a[2j] = re c + im s, a[2j+1] = im c - re s, re x[2j] and im x[n-1-2j] (odd: swapped), c and s w[4j], w[4j+1] */
template <typename T>
HWY_INLINE void dct4_in(const T *HWY_RESTRICT x, const T *HWY_RESTRICT w, T *HWY_RESTRICT a, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), h = (size_t)n/2;
	size_t j = 0;
	for (; j+N<=h; j+=N){
		hn::Vec<decltype(d)> ev, od, top, c, s, c2, s2;
		hn::LoadInterleaved2(d, x+2*j, ev, od);
		hn::LoadInterleaved2(d, x+(size_t)n-2*j-2*N, od, top); /* x[n-2j-2N+2i+1], reversed: x[n-1-2j-2i] */
		top = hn::Reverse(d, top);
		hn::LoadInterleaved4(d, w+4*j, c, s, c2, s2);
		if (odd){
			hn::StoreInterleaved2(hn::MulAdd(top, c, hn::Mul(ev, s)), hn::MulSub(ev, c, hn::Mul(top, s)), d, a+2*j);
		} else {
			hn::StoreInterleaved2(hn::MulAdd(ev, c, hn::Mul(top, s)), hn::MulSub(top, c, hn::Mul(ev, s)), d, a+2*j);
		}
	}
	for (size_t i=j; i<h; i++){
		const T re = odd ? x[(size_t)n-1-2*i] : x[2*i], im = odd ? x[2*i] : x[(size_t)n-1-2*i], c = w[4*i], s = w[4*i+1];
		a[2*i] = re*c+im*s;
		a[2*i+1] = im*c-re*s;
	}
}

/* y[2k] = 2(re c + im s) of b's k, y[2k+1] = 2(re s - im c) of b's n/2-1-k (odd: negated), c and s w[4k+2], w[4k+3] */
template <typename T>
HWY_INLINE void dct4_out(const T *HWY_RESTRICT b, const T *HWY_RESTRICT w, T *HWY_RESTRICT y, int n, int odd){
	const hn::ScalableTag<T> d;
	const size_t N = hn::Lanes(d), h = (size_t)n/2;
	const auto two = hn::Set(d, T(2)), twice = hn::Set(d, odd ? T(-2) : T(2));
	size_t k = 0;
	for (; k+N<=h; k+=N){
		const size_t q = h-k-N;
		hn::Vec<decltype(d)> re, im, re2, im2, c, s, c2, s2;
		hn::LoadInterleaved2(d, b+2*k, re, im);
		hn::LoadInterleaved4(d, w+4*k, c2, s2, c, s);
		const auto lo = hn::Mul(two, hn::MulAdd(re, c, hn::Mul(im, s)));
		hn::LoadInterleaved2(d, b+2*q, re2, im2);
		hn::LoadInterleaved4(d, w+4*q, c2, s2, c, s);
		const auto hi = hn::Mul(twice, hn::MulSub(re2, s, hn::Mul(im2, c)));
		hn::StoreInterleaved2(lo, hn::Reverse(d, hi), d, y+2*k);
	}
	for (size_t i=k; i<h; i++){
		const size_t q = h-1-i;
		const T re = b[2*i], im = b[2*i+1], c = w[4*i+2], s = w[4*i+3];
		const T re2 = b[2*q], im2 = b[2*q+1], c2 = w[4*q+2], s2 = w[4*q+3];
		y[2*i] = 2*(re*c+im*s);
		y[2*i+1] = (odd ? -2 : 2)*(re2*s2-im2*c2);
	}
}

#define TRIG_HWY_PASS(name) \
	void name##_f(const float *u, const float *w, float *v, int n, int odd){ \
		name(u, w, v, n, odd); \
	} \
	void name##_d(const double *u, const double *w, double *v, int n, int odd){ \
		name(u, w, v, n, odd); \
	}
TRIG_HWY_PASS(dct2_in)
TRIG_HWY_PASS(dct2_out)
TRIG_HWY_PASS(dct3_in)
TRIG_HWY_PASS(dct3_out)
TRIG_HWY_PASS(dct4_in)
TRIG_HWY_PASS(dct4_out)
#undef TRIG_HWY_PASS

} // namespace HWY_NAMESPACE
} // namespace trighwy
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace trighwy {
#define TRIG_HWY_EXPORT(name) \
	HWY_EXPORT(name##_f); \
	HWY_EXPORT(name##_d); \
	extern "C" void trig_hwy_##name##_f(const float *u, const float *w, float *v, int n, int odd){ \
		HWY_DYNAMIC_DISPATCH(name##_f)(u, w, v, n, odd); \
	} \
	extern "C" void trig_hwy_##name##_d(const double *u, const double *w, double *v, int n, int odd){ \
		HWY_DYNAMIC_DISPATCH(name##_d)(u, w, v, n, odd); \
	}
TRIG_HWY_EXPORT(dct2_in)
TRIG_HWY_EXPORT(dct2_out)
TRIG_HWY_EXPORT(dct3_in)
TRIG_HWY_EXPORT(dct3_out)
TRIG_HWY_EXPORT(dct4_in)
TRIG_HWY_EXPORT(dct4_out)
} // namespace trighwy
#endif
