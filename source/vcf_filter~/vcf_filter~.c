/*
lp2
wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
*out++ = rcp*(wn0 + 2.0f*wn1 + wn2);
wn2 = wn1;
wn1 = wn0;

bp2
wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
*out++ = rcp*al*(wn0 - wn2);
wn2 = wn1;
wn1 = wn0;

rbp2
wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
*out++ = rcp*l*(wn0 - wn2);
wn2 = wn1;
wn1 = wn0;

hp2
wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
*out++ = rcp*(wn0 - 2.0f*wn1 + wn2);
wn2 = wn1;
wn1 = wn0;

*/
#define __x86_64__
#define MAX_FLOAT_PRECISION 64

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "bigsmalllib.h"

#include <string.h>
#include <math.h>

static t_class *vcf_class;

typedef struct vcf
{
	t_pxobject x_obj;
	double  x_wn1;
	double  x_wn2;
	double  x_msi;

	double    x_freq;
	double    x_q;

	t_symbol  *x_filtname;

	short freq_connected;
	short q_connected;
} t_vcf;

void *vcf_filter_tilde_new(t_symbol *s, short argc, t_atom *argv);

void vcf_filter_perform_hp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);

void vcf_filter_perform_lp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);

void vcf_filter_perform_rbp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);

void vcf_filter_perform_bp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);

void vcf_dsp64(t_vcf *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);

void vcf_filt_assist(t_vcf *x, void *b, long msg, long arg, char *dst);
void vcf_int(t_vcf *x, long f);
void vcf_float(t_vcf *x, double f);
void vcf_free(t_vcf *x);

int C74_EXPORT main(void)
{
	vcf_class = class_new("vcf_filter~", (method)vcf_filter_tilde_new, (method)vcf_free,
		sizeof(t_vcf), 0, A_GIMME, 0);
	class_addmethod(vcf_class, (method)vcf_dsp64, "dsp64", A_CANT, 0);

	class_addmethod(vcf_class, (method)vcf_float, "float", A_FLOAT, 0);
	class_addmethod(vcf_class, (method)vcf_int, "int", A_LONG, 0);

	class_addmethod(vcf_class, (method)vcf_filt_assist, "assist", A_CANT, 0);

	class_dspinit(vcf_class);
	class_register(CLASS_BOX, vcf_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's/Thomas Musil's vcf_filter~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *vcf_filter_tilde_new(t_symbol *s, short argc, t_atom *argv)
{
	t_vcf *x = (t_vcf *)object_alloc(vcf_class);

	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");

	x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate
	
	x->x_msi = 0;
	x->x_wn1 = 0.0f;
	x->x_wn2 = 0.0f;
	x->x_filtname = gensym("bp2");

	atom_arg_getsym(&x->x_filtname, 0, argc, argv);
	post("Initial-arguments: <sym> kind: bp2, rbp2, lp2, hp2!");
	
	return (x);
}

void vcf_filter_perform_hp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *lp = ins[1];
	t_double *q = ins[2];
	t_double *out = outs[0];
	//t_vcf *x = outs[1];

	double freq = x->x_freq;
	double qiu = x->x_q;
	int n = vectorsize;
	int i;

	double wn0, wn1 = x->x_wn1, wn2 = x->x_wn2;
	double l, al, l2, rcp, forw;

	for (i = 0; i<n; i += 4)
	{
		if (x->freq_connected)	l = lp[i];
		else l = freq;

		if (l < 0) l = fabs(l);

		if (x->q_connected){
			if (q[i] < 0.000001f)
				al = 1000000.0f*l;
			else if (q[i] > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / q[i];
		}
		else{
			if (qiu < 0.000001f)
				al = 1000000.0f*l;
			else if (qiu > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / qiu;
		}
		l2 = l*l + 1.0f;
		rcp = 1.0f / (al + l2);
		forw = rcp * (l2 - 1.0f);

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = forw*(wn0 - 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = forw*(wn0 - 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = forw*(wn0 - 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = forw*(wn0 - 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(wn2))
		wn2 = 0.0f;
	if (MAX_BIGORSMALL(wn1))
		wn1 = 0.0f;

	x->x_wn1 = wn1;
	x->x_wn2 = wn2;
}

void vcf_filter_perform_lp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *lp = ins[1];
	t_double *q = ins[2];
	t_double *out = outs[0];
	//t_vcf *x = outs[1];
	double freq = x->x_freq;
	double qiu = x->x_q;

	int n = vectorsize;
	int i;

	double wn0, wn1 = x->x_wn1, wn2 = x->x_wn2;
	double al, l, l2, rcp;

	for (i = 0; i<n; i += 4)
	{
		if (x->freq_connected)	l = lp[i];
		else l = freq;

		if (l < 0) l = fabs(l);

		if (x->q_connected){
			if (q[i] < 0.000001f)
				al = 1000000.0f*l;
			else if (q[i] > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / q[i];
		}
		else{
			if (qiu < 0.000001f)
				al = 1000000.0f*l;
			else if (qiu > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / qiu;
		}

		l2 = l*l + 1.0f;
		rcp = 1.0f / (al + l2);

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*(wn0 + 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*(wn0 + 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*(wn0 + 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*(wn0 + 2.0f*wn1 + wn2);
		wn2 = wn1;
		wn1 = wn0;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(wn2))
		wn2 = 0.0f;
	if (MAX_BIGORSMALL(wn1))
		wn1 = 0.0f;

	x->x_wn1 = wn1;
	x->x_wn2 = wn2;
}

void vcf_filter_perform_rbp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *lp = ins[1];
	t_double *q = ins[2];
	t_double *out = outs[0];
	//t_vcf *x = outs[1];

	double freq = x->x_freq;
	double qiu = x->x_q;
	int n = vectorsize;
	int i;

	double wn0, wn1 = x->x_wn1, wn2 = x->x_wn2;
	double al, l, l2, rcp;

	for (i = 0; i<n; i += 4)
	{
		if (x->freq_connected) l = lp[i];
		else l = freq;

		if (l < 0) l = fabs(l);

		if (x->q_connected){
			if (q[i] < 0.000001f)
				al = 1000000.0f*l;
			else if (q[i] > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / q[i];
		}
		else{
			if (qiu < 0.000001f)
				al = 1000000.0f*l;
			else if (qiu > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / qiu;
		}
		l2 = l*l + 1.0f;
		rcp = 1.0f / (al + l2);


		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*l*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*l*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*l*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*l*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(wn2))
		wn2 = 0.0f;
	if (MAX_BIGORSMALL(wn1))
		wn1 = 0.0f;

	x->x_wn1 = wn1;
	x->x_wn2 = wn2;
}

void vcf_filter_perform_bp2(t_vcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *lp = ins[1];
	t_double *q = ins[2];
	t_double *out = outs[0];
	//t_vcf *x = outs[1];

	double freq = x->x_freq;
	double qiu = x->x_q;
	int n = vectorsize;
	int i;

	double wn0, wn1 = x->x_wn1, wn2 = x->x_wn2;
	double l, al, l2, rcp;

	for (i = 0; i<n; i += 4)
	{
		if (x->freq_connected) l = lp[i];
		else l = freq;

		if (l < 0) l = fabs(l);

		if (x->q_connected){
			if (q[i] < 0.000001f)
				al = 1000000.0f*l;
			else if (q[i] > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / q[i];
		}
		else{
			if (qiu < 0.000001f)
				al = 1000000.0f*l;
			else if (qiu > 1000000.0f)
				al = 0.000001f*l;
			else
				al = l / qiu;
		}
		l2 = l*l + 1.0f;
		rcp = 1.0f / (al + l2);

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*al*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*al*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*al*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;

		wn0 = *in++ - rcp*(2.0f*(2.0f - l2)*wn1 + (l2 - al)*wn2);
		*out++ = rcp*al*(wn0 - wn2);
		wn2 = wn1;
		wn1 = wn0;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(wn2))
		wn2 = 0.0f;
	if (MAX_BIGORSMALL(wn1))
		wn1 = 0.0f;

	x->x_wn1 = wn1;
	x->x_wn2 = wn2;
}

void vcf_dsp64(t_vcf *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->freq_connected = count[1];//count contains info about signal connections
	x->q_connected = count[2];

	//if (x->sr != samplerate){//compare the sample rate of the object to signal's vectors sample rate 
	//	x->sr = samplerate;
	//}
	if (x->x_filtname == gensym("bp2")){
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_filter_perform_bp2, 0, NULL);
	}
	else if (x->x_filtname == gensym("rbp2")){
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_filter_perform_rbp2, 0, NULL);
	}
	else if (x->x_filtname == gensym("lp2")){
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_filter_perform_lp2, 0, NULL);
	}
	else if (x->x_filtname == gensym("hp2")){
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_filter_perform_hp2, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_filter_perform_bp2, 0, NULL);
		post("vcf_filter~ Default: bp2");
		//post("Initial-arguments: <sym> kind: bp2, rbp2, lp2, hp2!");
	}
}

void vcf_float(t_vcf *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_freq = f;
	}
	else if (in == 2) {
		x->x_q = f;
	}
}

void vcf_int(t_vcf *x, long f)
{
	vcf_float(x, (double)f);
}

void vcf_free(t_vcf *x)
{
	dsp_free((t_pxobject *)x);
}

void vcf_filt_assist(t_vcf *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered"); break;
		case 1: sprintf(dst, "(signal/float) Center Frequency"); break;
		case 2: sprintf(dst, "(signal/float) Q/filter sharpness"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Filtered signal");
	}
}