// a Pd object on PUREfft, for checking that a Pd external links and loads it: [fftcheck] plans a spectrum when created (the race), and on
// bang transforms a cosine at bin 8 and outputs the strongest bin
#include "m_pd.h"
#include "fft.h"
#include "spectrum.h"
#include <math.h>

static t_class *fftcheck_class;

typedef struct fftcheck {
	t_object obj;
	t_outlet *out;
	spectrum s;
	int ok;
} t_fftcheck;

static void fftcheck_bang(t_fftcheck *x){
	static float frame[1000];
	if (!x->ok){
		pd_error(x, "fftcheck: no spectrum");
		return;
	}
	for (int i = 0; i<1000; i++){
		frame[i] = cosf(6.2831853f*8*(float)(i-500)/(float)x->s.size);
	}
	spectrum_transform(&x->s, frame, 0);
	int peak = 0;
	for (int k = 1; k<x->s.nbins; k++){
		if (x->s.power[k]>x->s.power[peak]){
			peak = k;
		}
	}
	post("fftcheck: size %d, peak at bin %d", x->s.size, peak);
	outlet_float(x->out, (t_float)peak);
}

static void *fftcheck_new(void){
	static float win[1000];
	t_fftcheck *x = (t_fftcheck *)pd_new(fftcheck_class);
	for (int i = 0; i<1000; i++){
		win[i] = 1;
	}
	x->ok = spectrum_init(&x->s, win, 1000, 0)==0;
	x->out = outlet_new(&x->obj, &s_float);
	return x;
}

static void fftcheck_free(t_fftcheck *x){
	spectrum_free(&x->s);
}

void fftcheck_setup(void){
	fftcheck_class = class_new(gensym("fftcheck"), (t_newmethod)fftcheck_new, (t_method)fftcheck_free, sizeof(t_fftcheck), CLASS_DEFAULT, 0);
	class_addbang(fftcheck_class, fftcheck_bang);
}
