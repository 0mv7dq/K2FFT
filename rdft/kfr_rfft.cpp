// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* KFR's DFTs, as benchees/kfr, built alone (cmake/kfr.cmake's kfr_rfft): KFR's GCC flags (-flax-vector-conversions) stay here; how,
   rdft/kfr.c's: 0 a complex forward, 1 back, 2 a real forward, 3 back, + 4 in double */
#include <cstdlib>
#include <kfr/dft.hpp>

namespace {
struct plan {
	kfr::u8 *temp = nullptr; /* so a transform never allocates */
	virtual ~plan(){
		std::free(temp);
	}
	virtual void run(void *in, void *out) = 0;
};

template<typename T>
struct complex_plan : plan {
	kfr::dft_plan<T> dft;
	bool inverse;
	complex_plan(size_t size, bool back) : dft(size), inverse(back){
	}
	void run(void *in, void *out) override {
		dft.execute(static_cast<kfr::complex<T> *>(out), static_cast<const kfr::complex<T> *>(in), temp, inverse);
	}
};

/* size/2+1 (re, im) pairs */
template<typename T>
struct real_plan : plan {
	kfr::dft_plan_real<T> dft;
	bool inverse;
	real_plan(size_t size, bool back) : dft(size), inverse(back){
	}
	void run(void *in, void *out) override {
		if (inverse){
			dft.execute(static_cast<T *>(out), static_cast<const kfr::complex<T> *>(in), temp);
		} else {
			dft.execute(static_cast<kfr::complex<T> *>(out), static_cast<const T *>(in), temp);
		}
	}
};

template<typename P>
plan *made(size_t size, bool back){
	P *p = new P(size, back);
	p->temp = static_cast<kfr::u8 *>(std::malloc(p->dft.temp_size+1));
	if (!p->temp){
		delete p;
		return nullptr;
	}
	return p;
}
}

extern "C" void *kfr_plan(int how, int size){
	try {
		const size_t n = static_cast<size_t>(size);
		const bool back = how%2==1;
		switch (how){
		case 0: case 1: return made<complex_plan<float> >(n, back);
		case 2: case 3: return made<real_plan<float> >(n, back);
		case 4: case 5: return made<complex_plan<double> >(n, back);
		default: return made<real_plan<double> >(n, back);
		}
	} catch (...){
		return nullptr; /* an exception must not reach C */
	}
}

extern "C" void kfr_execute(void *p, void *in, void *out){
	static_cast<plan *>(p)->run(in, out);
}

extern "C" void kfr_destroy(void *p){
	delete static_cast<plan *>(p);
}
