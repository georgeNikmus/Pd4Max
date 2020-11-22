#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include <math.h>

static t_class *moog_class;

typedef struct _moog
{
	t_pxobject x_obj;
	
	double x_1, x_2, x_3, x_4;
	double y_1, y_2, y_3, y_4;
	//double freq, qual;
}t_moog;

void moog_dsp(t_moog *x, t_signal **sp, short *count);
void moog_dsp64(t_moog *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);

//void moog_perform64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam);
//void NoCon_moog_perform64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam);
//t_int *moog_perform(t_int *w);
//t_int *NoCon_moog_perform(t_int *w);

void moog_8perf64(t_moog *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
//void NoCon_moog_8perf64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam);
t_int *moog_perf8(t_int *w);
//t_int *NoCon_moog_perf8(t_int *w);

//void moog_int(t_moog *x, long n);
//void moog_float(t_moog *x, double f);
void moog_reset(t_moog *x);
double calc_k(double f, double k);
void moog_assist(t_moog *x, void *b, long msg, long arg, char *dst);
void *moog_new(double f, double q);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("moog~", (method)moog_new, (method)dsp_free,
		sizeof(t_moog), 0L, A_DEFFLOAT, A_DEFFLOAT, 0);
	
	class_addmethod(c, (method)moog_dsp, "dsp", A_CANT, 0);
	class_addmethod(c, (method)moog_dsp64, "dsp64", A_CANT, 0);

	//class_addmethod(c, (method)moog_float, "float", A_FLOAT, 0);
	//class_addmethod(c, (method)moog_int, "int", A_LONG, 0);

	class_addmethod(c, (method)moog_reset, "reset", 0);
	class_addmethod(c, (method)moog_assist, "assist", A_CANT, 0);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	moog_class = c;

	post("--------------------------------------------------------------------------");
	post("This is Guenter Geiger's moog~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2016 -");
	post("--------------------------------------------------------------------------");
	
	return 0;
}

//void moog_dsp(t_moog *x, t_signal **sp, short *count)
//{
//	
//	if (count[1] & count[2]){
//		if (sp[0]->s_n & 7){
//			dsp_add(moog_perform, 6,
//				x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
//		}
//		else{
//			dsp_add(moog_perf8, 6,
//				x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
//		}
//	}
//	else{
//		if (sp[0]->s_n & 7){
//			dsp_add(NoCon_moog_perform, 6,
//				x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
//		}
//		else{
//			dsp_add(NoCon_moog_perf8, 6,
//				x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
//		}
//	}
//}

void moog_dsp(t_moog *x, t_signal **sp, short *count)
{
	dsp_add(moog_perf8, 6,
		x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

//void moog_dsp64(t_moog *x, t_object *dsp64, short *count,
//	double samplerate, long maxvectorsize, long flags)
//{
//	if (count[1] & count[2]){
//		if (maxvectorsize & 7){
//			object_method(dsp64, gensym("dsp_add64"), x,
//				moog_perform64, 0, NULL);
//		}
//		else{
//			object_method(dsp64, gensym("dsp_add64"), x,
//				moog_8perf64, 0, NULL);
//		}
//	}
//	else{
//		if (maxvectorsize & 7){
//			object_method(dsp64, gensym("dsp_add64"), x,
//				NoCon_moog_perform64, 0, NULL);
//		}
//		else{
//			object_method(dsp64, gensym("dsp_add64"), x,
//				NoCon_moog_8perf64, 0, NULL);
//		}
//	}
//}

void moog_dsp64(t_moog *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	object_method(dsp64, gensym("dsp_add64"), x,
		moog_8perf64, 0, NULL);
}

//void moog_perform64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam)
//{
//	t_double *in1 = ins[0];
//	t_double *out = outs[0];
//	t_double *p = ins[1];
//	t_double *k = ins[2];
//
//	int n = vectorsize;
//
//	double in;
//	double pt, pt1;
//
//	double x1 = x->x_1;
//	double x2 = x->x_2;
//	double x3 = x->x_3;
//	double x4 = x->x_4;
//	double ys1 = x->y_1;
//	double ys2 = x->y_2;
//	double ys3 = x->y_3;
//	double ys4 = x->y_4;
//
//	while (n--) {
//		if (*p > 8140) *p = 8140.;
//		*k = calc_k(*p, *k);
//		pt = *p;
//		pt1 = (pt + 1)*0.76923077;
//		in = *in1++ - *k*ys4;
//		ys1 = (pt1)*in + 0.3*x1 - pt*ys1;
//		x1 = in;
//		ys2 = (pt1)*ys1 + 0.3*x2 - pt*ys2;
//		x2 = ys1;
//		ys3 = (pt1)*ys2 + 0.3 *x3 - pt*ys3;
//		x3 = ys2;
//		ys4 = (pt1)*ys3 + 0.3*x4 - pt*ys4;
//		x4 = ys3;
//		*out++ = ys4;
//	}
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//}
//
//void NoCon_moog_perform64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam)
//{
//	t_double *in1 = ins[0];
//	t_double *out = outs[0];
//
//	double p = x->freq;
//	double k = x->qual;
//
//	int n = vectorsize;
//
//	double in;
//	double pt, pt1;
//
//	double x1 = x->x_1;
//	double x2 = x->x_2;
//	double x3 = x->x_3;
//	double x4 = x->x_4;
//	double ys1 = x->y_1;
//	double ys2 = x->y_2;
//	double ys3 = x->y_3;
//	double ys4 = x->y_4;
//
//	while (n--) {
//		if (p > 8140) p = 8140.;
//		k = calc_k(p, k);
//		pt = p;
//		pt1 = (pt + 1)*0.76923077;
//		in = *in1++ - k*ys4;
//		ys1 = (pt1)*in + 0.3*x1 - pt*ys1;
//		x1 = in;
//		ys2 = (pt1)*ys1 + 0.3*x2 - pt*ys2;
//		x2 = ys1;
//		ys3 = (pt1)*ys2 + 0.3 *x3 - pt*ys3;
//		x3 = ys2;
//		ys4 = (pt1)*ys3 + 0.3*x4 - pt*ys4;
//		x4 = ys3;
//		*out++ = ys4;
//	}
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//}
//
//t_int *moog_perform(t_int *w)
//{	
//	t_moog* x = (t_moog*)(w[1]);
//	t_float *in1 = (t_float *)(w[2]);
//	t_float *p = (t_float *)(w[3]);
//	t_float *k = (t_float *)(w[4]);
//
//	t_float *out = (t_float *)(w[5]);
//	int n = (int)(w[6]);
//	float in;
//	float pt, pt1;
//
//	float x1 = (float)x->x_1;
//	float x2 = (float)x->x_2;
//	float x3 = (float)x->x_3;
//	float x4 = (float)x->x_4;
//	float ys1 = (float)x->y_1;
//	float ys2 = (float)x->y_2;
//	float ys3 = (float)x->y_3;
//	float ys4 = (float)x->y_4;
//
//
//	while (n--) {
//		if (*p > 8140) *p = 8140.;
//		*k = (float)(calc_k(*p, *k));
//		pt = *p;
//		pt1 = (float)((pt + 1)*0.76923077);
//		in = *in1++ - *k*ys4;
//		ys1 = (float)((pt1)*in + 0.3*x1 - pt*ys1);
//		x1 = in;
//		ys2 = (float)((pt1)*ys1 + 0.3*x2 - pt*ys2);
//		x2 = ys1;
//		ys3 = (float)((pt1)*ys2 + 0.3 *x3 - pt*ys3);
//		x3 = ys2;
//		ys4 = (float)((pt1)*ys3 + 0.3*x4 - pt*ys4);
//		x4 = ys3;
//		*out++ = ys4;
//	}
//
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//
//	return (w + 7);
//}
//
//t_int *NoCon_moog_perform(t_int *w)
//{
//	t_moog* x = (t_moog*)(w[1]);
//	t_float *in1 = (t_float *)(w[2]);
//	//t_float *p = (t_float *)(w[3]);
//	//t_float *k = (t_float *)(w[4]);
//
//	t_float *out = (t_float *)(w[5]);
//	int n = (int)(w[6]);
//	
//	float p = (float)x->freq;
//	float k = (float)x->qual;
//
//	float in;
//	float pt, pt1;
//
//	float x1 = (float)x->x_1;
//	float x2 = (float)x->x_2;
//	float x3 = (float)x->x_3;
//	float x4 = (float)x->x_4;
//	float ys1 = (float)x->y_1;
//	float ys2 = (float)x->y_2;
//	float ys3 = (float)x->y_3;
//	float ys4 = (float)x->y_4;
//
//
//	while (n--) {
//		if (p > 8140) p = 8140.;
//		k = (float)(calc_k(p, k));
//		pt = p;
//		pt1 = (float)((pt + 1)*0.76923077);
//		in = *in1++ - k*ys4;
//		ys1 = (float)((pt1)*in + 0.3*x1 - pt*ys1);
//		x1 = in;
//		ys2 = (float)((pt1)*ys1 + 0.3*x2 - pt*ys2);
//		x2 = ys1;
//		ys3 = (float)((pt1)*ys2 + 0.3 *x3 - pt*ys3);
//		x3 = ys2;
//		ys4 = (float)((pt1)*ys3 + 0.3*x4 - pt*ys4);
//		x4 = ys3;
//		*out++ = ys4;
//	}
//
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//
//	return (w + 7);
//}

void moog_8perf64(t_moog *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in1 = ins[0];
	t_double *out = outs[0];
	t_double *p = ins[1];
	t_double *k = ins[2];

	int n = vectorsize;

	double x1 = x->x_1;
	double x2 = x->x_2;
	double x3 = x->x_3;
	double x4 = x->x_4;
	double ys1 = x->y_1;
	double ys2 = x->y_2;
	double ys3 = x->y_3;
	double ys4 = x->y_4;
	//double temp, temp2;
	double pt, pt1;
	double in;

	while (n--) {
		if (*p > 8140.) *p = 8140.;
		*k = calc_k(*p, *k);
		pt = *p* 0.01*0.0140845 - 0.9999999f;
		pt1 = (pt + 1.0)*0.76923077;
		in = *in1++ - *k*ys4;
		
		ys1 = pt1*(in + 0.3*x1) - pt*ys1;
		x1 = in;
		ys2 = pt1*(ys1 + 0.3*x2) - pt*ys2;
		x2 = ys1;
		ys3 = pt1*(ys2 + 0.3*x3) - pt*ys3;
		x3 = ys2;
		ys4 = pt1*(ys3 + 0.3*x4) - pt*ys4;
		x4 = ys3;
		*out++ = ys4;

		p++; k++;
	}
	x->y_1 = ys1;
	x->y_2 = ys2;
	x->y_3 = ys3;
	x->y_4 = ys4;
	x->x_1 = x1;
	x->x_2 = x2;
	x->x_3 = x3;
	x->x_4 = x4;
}

//void NoCon_moog_8perf64(t_moog *x, t_object *dsp64,
//	double **ins, long numins, double **outs,
//	long numouts, long vectorsize, long flags,
//	void *userparam)
//{
//	t_double *in1 = ins[0];
//	t_double *out = outs[0];
//	
//	double p = x->freq;
//	double k = x->qual;
//
//	int n = vectorsize;
//
//	double x1 = x->x_1;
//	double x2 = x->x_2;
//	double x3 = x->x_3;
//	double x4 = x->x_4;
//	double ys1 = x->y_1;
//	double ys2 = x->y_2;
//	double ys3 = x->y_3;
//	double ys4 = x->y_4;
//	//double temp, temp2;
//	double pt, pt1;
//	double in;
//
//	while (n--) {
//		if (p > 8140.) p = 8140.;
//		k = calc_k(p, k);
//		pt = p* 0.01*0.0140845 - 0.9999999f;
//		pt1 = (pt + 1.0)*0.76923077;
//		in = *in1++ - k*ys4;
//		ys1 = pt1*(in + 0.3*x1) - pt*ys1;
//		x1 = in;
//		ys2 = pt1*(ys1 + 0.3*x2) - pt*ys2;
//		x2 = ys1;
//		ys3 = pt1*(ys2 + 0.3*x3) - pt*ys3;
//		x3 = ys2;
//		ys4 = pt1*(ys3 + 0.3*x4) - pt*ys4;
//		x4 = ys3;
//		*out++ = ys4;
//
//		p++; k++;
//	}
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//}

t_int *moog_perf8(t_int *w)
{
	t_moog* x = (t_moog*)(w[1]);
	t_float *in1 = (t_float *)(w[2]);
	t_float *p = (t_float *)(w[3]);
	t_float *k = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);

	t_float x1 = (float)x->x_1;
	t_float x2 = (float)x->x_2;
	t_float x3 = (float)x->x_3;
	t_float x4 = (float)x->x_4;
	t_float ys1 = (float)x->y_1;
	t_float ys2 = (float)x->y_2;
	t_float ys3 = (float)x->y_3;
	t_float ys4 = (float)x->y_4;
	//t_float temp, temp2;
	t_float pt, pt1;
	t_float in;

	while (n--) {
		if (*p > 8140.) *p = 8140.;
		*k = (float)(calc_k(*p, *k));

		pt = (float)(*p* 0.01*0.0140845 - 0.9999999f);
		pt1 = (float)((pt + 1.0)*0.76923077);
		in = *in1++ - *k*ys4;
		ys1 = (float)(pt1*(in + 0.3*x1) - pt*ys1);
		x1 = in;
		ys2 = (float)(pt1*(ys1 + 0.3*x2) - pt*ys2);
		x2 = ys1;
		ys3 = (float)(pt1*(ys2 + 0.3*x3) - pt*ys3);
		x3 = ys2;
		ys4 = (float)(pt1*(ys3 + 0.3*x4) - pt*ys4);
		x4 = ys3;
		*out++ = ys4;

		p++; k++;
	}

	x->y_1 = ys1;
	x->y_2 = ys2;
	x->y_3 = ys3;
	x->y_4 = ys4;
	x->x_1 = x1;
	x->x_2 = x2;
	x->x_3 = x3;
	x->x_4 = x4;

	return (w + 7);
}

//t_int *NoCon_moog_perf8(t_int *w)
//{
//	t_moog* x = (t_moog*)(w[1]);
//	t_float *in1 = (t_float *)(w[2]);
//	//t_float *p = (t_float *)(w[3]);
//	//t_float *k = (t_float *)(w[4]);
//	t_float *out = (t_float *)(w[5]);
//	int n = (int)(w[6]);
//
//	float p = (float)x->freq;
//	float k = (float)x->qual;
//	t_float x1 = (float)x->x_1;
//	t_float x2 = (float)x->x_2;
//	t_float x3 = (float)x->x_3;
//	t_float x4 = (float)x->x_4;
//	t_float ys1 = (float)x->y_1;
//	t_float ys2 = (float)x->y_2;
//	t_float ys3 = (float)x->y_3;
//	t_float ys4 = (float)x->y_4;
//	//t_float temp, temp2;
//	t_float pt, pt1;
//	t_float in;
//
//	while (n--) {
//		if (p > 8140.) p = 8140.;
//		k = (float)(calc_k(p, k));
//
//		pt = (float)(p* 0.01*0.0140845 - 0.9999999f);
//		pt1 = (float)((pt + 1.0)*0.76923077);
//		in = *in1++ - k*ys4;
//		ys1 = (float)(pt1*(in + 0.3*x1) - pt*ys1);
//		x1 = in;
//		ys2 = (float)(pt1*(ys1 + 0.3*x2) - pt*ys2);
//		x2 = ys1;
//		ys3 = (float)(pt1*(ys2 + 0.3*x3) - pt*ys3);
//		x3 = ys2;
//		ys4 = (float)(pt1*(ys3 + 0.3*x4) - pt*ys4);
//		x4 = ys3;
//		*out++ = ys4;
//
//		p++; k++;
//	}
//
//	x->y_1 = ys1;
//	x->y_2 = ys2;
//	x->y_3 = ys3;
//	x->y_4 = ys4;
//	x->x_1 = x1;
//	x->x_2 = x2;
//	x->x_3 = x3;
//	x->x_4 = x4;
//
//	return (w + 7);
//}


//void moog_int(t_moog *x, long n)
//{
//	moog_float(x, (double)n);
//}
//
//void moog_float(t_moog *x, double f)
//{
//	long in = proxy_getinlet((t_object *)x);
//
//	if (in == 1) {
//		x->freq = f;
//	}
//	else if (in == 2) {
//		x->qual = f;
//	}
//}

void moog_reset(t_moog *x){
	x->x_1 = x->x_2 = x->x_3 = x->x_4 = 0.0;
	x->y_1 = x->y_2 = x->y_3 = x->y_4 = 0.0;
}

double calc_k(double f, double k)
{
	if (k > 4.) k = 4.;
	if (k < 0.) k = 0.;
	if (f <= 3800) return k;
	k = k - 0.5*((f - 3800.f) / 4300.f);
	return k;
}

void moog_assist(t_moog *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered"); break;
		case 1: sprintf(dst, "(signal) Center Frequency"); break;
		case 2: sprintf(dst, "(signal) Q/filter sharpness"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Filtered signal");
	}
}

void *moog_new(double f, double q)
{
	t_moog *x = (t_moog *)object_alloc(moog_class);

	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");

	//x->freq = f;
	//x->qual = q;

	moog_reset(x);

	return (x);
}