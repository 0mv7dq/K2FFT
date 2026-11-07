// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* rdft/trig.c's transforms in R (float or double), T(name) its names: O(n) before and after the planned FFT, on a, b and w (the
   plan's buffers and twiddles); FFTW's definitions (its manual's "1d Real-even DFTs" and "Real-odd DFTs"), unnormalized */

/* cos and sin of pi k/(2n), k < n, for the DCT-II/III; of pi (4k+1)/(4n) then pi k/n, k < n/2, for the DCT-IV */
static void T(twiddles)(R *w, int n, k2fft_r2r_kind kind){
	if (kind==K2FFT_REDFT11 || kind==K2FFT_RODFT11){
		for (int k=0; k<n/2; k++){
			const double a = M_PI*(4*k+1)/(4.0*n), b = M_PI*k/n;
			w[4*k] = (R)cos(a);
			w[4*k+1] = (R)sin(a);
			w[4*k+2] = (R)cos(b);
			w[4*k+3] = (R)sin(b);
		}
		return;
	}
	for (int k=0; k<n; k++){
		w[2*k] = (R)cos(M_PI*k/(2.0*n));
		w[2*k+1] = (R)sin(M_PI*k/(2.0*n));
	}
}

/* DCT-II, Makhoul's: x reordered (evens, then odds backwards) into a, a's DFT into b's n/2+1 (re, im), turned by e^-i pi k/2n; odd,
   the DST-II's: x's odd elements negated, y reversed */
static void T(dct2_in)(const R *x, const R *w, R *a, int n, int odd){
	(void)w;
	for (int j=0; 2*j<n; j++){
		a[j] = x[2*j];
	}
	for (int j=0; 2*j+1<n; j++){
		a[n-1-j] = odd ? -x[2*j+1] : x[2*j+1];
	}
}

static void T(dct2_out)(const R *b, const R *w, R *y, int n, int odd){
	for (int k=0; 2*k<=n; k++){
		const R re = b[2*k], im = b[2*k+1], c = w[2*k], s = w[2*k+1];
		const R lo = 2*(re*c+im*s), hi = 2*(re*s-im*c);
		y[odd ? n-1-k : k] = lo;
		if (k>0 && n-k!=k){
			y[odd ? k-1 : n-k] = hi;
		}
	}
}

/* DCT-III, the DCT-II's inverse doubled: (x_k - i x_n-k) turned by e^i pi k/2n into a's n/2+1 (re, im), its real inverse into b, b
   un-reordered; odd, the DST-III's: x reversed, y's odd elements negated */
static void T(dct3_in)(const R *x, const R *w, R *a, int n, int odd){
	for (int k=0; 2*k<=n; k++){
		const R re = odd ? x[n-1-k] : x[k], im = k==0 ? 0 : (odd ? x[k-1] : x[n-k]), c = w[2*k], s = w[2*k+1];
		a[2*k] = re*c+im*s;
		a[2*k+1] = re*s-im*c;
	}
	a[1] = 0;
	if (n%2==0){
		a[n+1] = 0;
	}
}

static void T(dct3_out)(const R *b, const R *w, R *y, int n, int odd){
	(void)w;
	for (int j=0; 2*j<n; j++){
		y[2*j] = b[j];
	}
	for (int j=0; 2*j+1<n; j++){
		y[2*j+1] = odd ? -b[n-1-j] : b[n-1-j];
	}
}

/* DCT-IV, n even: (x_2j + i x_n-1-2j) e^-i pi (4j+1)/4n into a's n/2 (re, im), their DFT into b, turned by e^-i pi k/n into y_2k
   and -y_n-1-2k; odd, the DST-IV's: x reversed, y's odd elements negated */
static void T(dct4_in)(const R *x, const R *w, R *a, int n, int odd){
	for (int j=0; j<n/2; j++){
		const R re = odd ? x[n-1-2*j] : x[2*j], im = odd ? x[2*j] : x[n-1-2*j], c = w[4*j], s = w[4*j+1];
		a[2*j] = re*c+im*s;
		a[2*j+1] = im*c-re*s;
	}
}

static void T(dct4_out)(const R *b, const R *w, R *y, int n, int odd){
	for (int k=0; k<n/2; k++){
		const R re = b[2*k], im = b[2*k+1], c = w[4*k+2], s = w[4*k+3];
		const R lo = 2*(re*c+im*s), hi = -2*(im*c-re*s);
		y[2*k] = lo;
		y[n-1-2*k] = odd ? -hi : hi; /* n-1-2k odd: the DST-IV's sign */
	}
}

/* DCT-I: x evenly extended to 2(n-1) into a, y the real parts of its DFT; DST-I: oddly to 2(n+1), y minus the imaginary parts */
static void T(dct1_in)(const R *x, R *a, int n){
	for (int j=0; j<n; j++){
		a[j] = x[j];
	}
	for (int j=1; j<n-1; j++){
		a[2*(n-1)-j] = x[j];
	}
}

static void T(dct1_out)(const R *b, R *y, int n){
	for (int k=0; k<n; k++){
		y[k] = b[2*k];
	}
}

static void T(dst1_in)(const R *x, R *a, int n){
	a[0] = a[n+1] = 0;
	for (int j=0; j<n; j++){
		a[j+1] = x[j];
		a[2*(n+1)-1-j] = -x[j];
	}
}

static void T(dst1_out)(const R *b, R *y, int n){
	for (int k=0; k<n; k++){
		y[k] = -b[2*(k+1)+1];
	}
}

/* R2HC: r0 .. r_n/2, then i_(n+1)/2-1 .. i1, from the DFT's (re, im); HC2R back; DHT: re - im, the upper half's re + im */
static void T(r2hc_out)(const R *b, R *y, int n){
	for (int k=0; 2*k<=n; k++){
		y[k] = b[2*k];
	}
	for (int k=1; 2*k<n; k++){
		y[n-k] = b[2*k+1];
	}
}

static void T(hc2r_in)(const R *x, R *a, int n){
	a[0] = x[0];
	a[1] = 0;
	for (int k=1; 2*k<n; k++){
		a[2*k] = x[k];
		a[2*k+1] = x[n-k];
	}
	if (n%2==0){
		a[n] = x[n/2];
		a[n+1] = 0;
	}
}

static void T(dht_out)(const R *b, R *y, int n){
	for (int k=0; 2*k<=n; k++){
		y[k] = b[2*k]-b[2*k+1];
		if (k>0 && n-k!=k){
			y[n-k] = b[2*k]+b[2*k+1];
		}
	}
}

/* x's transform into y through the plan's FFT, on its buffers */
static void T(run)(const trig *p, const R *x, R *y){
	R *a = p->a, *b = p->b;
	const R *w = p->w;
	const int n = p->n, odd = p->kind==K2FFT_RODFT10 || p->kind==K2FFT_RODFT01 || p->kind==K2FFT_RODFT11;
	switch (p->kind){ /* the DCT-II/III/IV's passes rdft/trig_hwy.cpp's where Highway's here (H) */
	case K2FFT_REDFT10: case K2FFT_RODFT10: (p->hwy ? H(dct2_in) : T(dct2_in))(x, w, a, n, odd); break;
	case K2FFT_REDFT01: case K2FFT_RODFT01: (p->hwy ? H(dct3_in) : T(dct3_in))(x, w, a, n, odd); break;
	case K2FFT_REDFT11: case K2FFT_RODFT11: (p->hwy ? H(dct4_in) : T(dct4_in))(x, w, a, n, odd); break;
	case K2FFT_REDFT00: T(dct1_in)(x, a, n); break;
	case K2FFT_RODFT00: T(dst1_in)(x, a, n); break;
	case K2FFT_HC2R: T(hc2r_in)(x, a, n); break;
	default: memcpy(a, x, (size_t)n*sizeof(R)); break; /* R2HC, DHT */
	}
	p->fft->k->execute(p->fft->plan, a, b);
	switch (p->kind){
	case K2FFT_REDFT10: case K2FFT_RODFT10: (p->hwy ? H(dct2_out) : T(dct2_out))(b, w, y, n, odd); break;
	case K2FFT_REDFT01: case K2FFT_RODFT01: (p->hwy ? H(dct3_out) : T(dct3_out))(b, w, y, n, odd); break;
	case K2FFT_REDFT11: case K2FFT_RODFT11: (p->hwy ? H(dct4_out) : T(dct4_out))(b, w, y, n, odd); break;
	case K2FFT_REDFT00: T(dct1_out)(b, y, n); break;
	case K2FFT_RODFT00: T(dst1_out)(b, y, n); break;
	case K2FFT_R2HC: T(r2hc_out)(b, y, n); break;
	case K2FFT_DHT: T(dht_out)(b, y, n); break;
	default: memcpy(y, b, (size_t)n*sizeof(R)); break; /* HC2R */
	}
}
