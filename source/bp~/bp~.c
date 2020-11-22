#define __x86_64__ 
#define MAX_FLOAT_PRECISION 64 //for Max 6

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

#include "definitions.h"
#include "bigsmalllib.h"

static t_class *bp_class;

typedef struct bpctl
{
	double c_x1;
	double c_x2;
	double c_coef1;
	double c_coef2;
	double c_gain;
}t_bpctl;

typedef struct bp
{
	t_pxobject x_obj;
	double x_freq;
	double x_q;
	double sr;
	t_bpctl x_cspace;
	t_bpctl *x_ctl;
}t_bp;

void *bp_new(t_symbol *s, short argc, t_atom *argv);

void bp_docoef(t_bp *x, double f, double q);

void bp_perform1(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void bp_perform2(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void bp_perform3(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void bp_perform4(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigbp_perform1(t_int *w);
t_int *sigbp_perform2(t_int *w);
t_int *sigbp_perform3(t_int *w);
t_int *sigbp_perform4(t_int *w);

void bp_dsp64(t_bp *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigbp_dsp(t_bp *x, t_signal **sp, short *count);

double bp_qcos(double f);

void bp_ft1(t_bp *x, double f);
void bp_ft2(t_bp *x, double q);
void bp_clear(t_bp *x, double q);
void bp_int(t_bp *x, long f);
void bp_float(t_bp *x, double f);
void bp_free(t_bp *x);
void bp_assist(t_bp *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	bp_class = class_new("bp~", (method)bp_new, (method)bp_free,
		sizeof(t_bp), 0, A_GIMME, 0);
	class_addmethod(bp_class, (method)bp_dsp64,"dsp64",A_CANT,0);
	class_addmethod(bp_class, (method)sigbp_dsp, "dsp", A_CANT, 0);

	class_addmethod(bp_class, (method)bp_float, "float", A_FLOAT, 0);
	class_addmethod(bp_class, (method)bp_int, "int", A_LONG, 0);
	class_addmethod(bp_class, (method)bp_clear,"clear", 0);

	class_addmethod(bp_class, (method)bp_assist, "assist", A_CANT, 0);

	class_dspinit(bp_class);
	class_register(CLASS_BOX, bp_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's bp~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *bp_new(t_symbol *s, short argc, t_atom *argv)
{
	double f, q;
	t_bp *x = (t_bp *)object_alloc(bp_class);
	
	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	x->sr = sys_getsr();//determines the sample rate

	x->x_ctl = &x->x_cspace;
	x->x_cspace.c_x1 = 0;
	x->x_cspace.c_x2 = 0;

	atom_arg_getdouble(&f, 0, argc, argv);
	atom_arg_getdouble(&q, 1, argc, argv);

	bp_docoef(x, f, q);

	return (x);
}

void bp_docoef(t_bp *x, double f, double q)
{
	double r, oneminusr, omega;
	if (f < 0.001) f = 10;
	if (q < 0) q = 0;
	x->x_freq = f;
	x->x_q = q;
	omega = f * (2.0f * 3.141592653589793) / x->sr;
	if (q < 0.001) oneminusr = 1.0f;
	else oneminusr = omega / q;
	if (oneminusr > 1.0f) oneminusr = 1.0f;
	r = 1.0f - oneminusr;
	x->x_ctl->c_coef1 = 2.0f * bp_qcos(omega) * r; 
	x->x_ctl->c_coef2 = -r * r;
	x->x_ctl->c_gain = 2 * oneminusr * (oneminusr + r * omega);

	/* post("r %f, omega %f, coef1 %f, coef2 %f",
	r, omega, x->x_ctl->c_coef1, x->x_ctl->c_coef2); */
}

double bp_qcos(double f)
{
	if (f >= -(0.5f*3.141592653589793) && f <= 0.5f*3.141592653589793)
	{
		double g = f*f;
		return(((g*g*g * (-1.0f / 720.0f) + g*g*(1.0f / 24.0f)) - g*0.5) + 1);
	}
	else  return(0);
}
void bp_perform1(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *input = ins[0];
	t_double *freq = ins[1];
	t_double *q = ins[2];

	t_double *output = outs[0];

	int n = vectorsize;

	t_bpctl *c = x->x_ctl;
	double last = c->c_x1;
	double prev = c->c_x2;
	double coef1 = c->c_coef1;
	double coef2 = c->c_coef2;
	double gain = c->c_gain;
	double out;

	while (n--){
		x->x_freq = *freq++;
		x->x_q = *q++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;

}

void bp_perform2(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *input = ins[0];
	t_double *freq = ins[1];

	t_double *output = outs[0];

	int n = vectorsize;

	t_bpctl *c = x->x_ctl;
	double last = c->c_x1;
	double prev = c->c_x2;
	double coef1 = c->c_coef1;
	double coef2 = c->c_coef2;
	double gain = c->c_gain;
	double out;

	while (n--){
		x->x_freq = *freq++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;
}

void bp_perform3(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *input = ins[0];
	t_double *q = ins[2];

	t_double *output = outs[0];

	int n = vectorsize;

	t_bpctl *c = x->x_ctl;
	double last = c->c_x1;
	double prev = c->c_x2;
	double coef1 = c->c_coef1;
	double coef2 = c->c_coef2;
	double gain = c->c_gain;
	double out;

	while (n--){
		x->x_q = *q++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;
}

void bp_perform4(t_bp *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *input = ins[0];
	t_double *output = outs[0];

	int n = vectorsize;

	t_bpctl *c = x->x_ctl;
	double last = c->c_x1;
	double prev = c->c_x2;
	double coef1 = c->c_coef1;
	double coef2 = c->c_coef2;
	double gain = c->c_gain;
	double out;

	while (n--){
		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;
}

void bp_dsp64(t_bp *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{

	if (x->sr != samplerate){//compare the sample rate of the object to signal's vectors sample rate 
		x->sr = samplerate;
	}
	bp_docoef(x, x->x_freq, x->x_q);

	if (count[1] & count[2]){
		object_method(dsp64, gensym("dsp_add64"), x,
			bp_perform1, 0, NULL);
	}
	else if(count[1]){
		object_method(dsp64, gensym("dsp_add64"), x,
			bp_perform2, 0, NULL);
	}
	else if (count[2]){
		object_method(dsp64, gensym("dsp_add64"), x,
			bp_perform3, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			bp_perform4, 0, NULL);
	}
}

void bp_clear(t_bp *x, double q)
{
	x->x_ctl->c_x1 = x->x_ctl->c_x2 = 0;
}

void bp_float(t_bp *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_freq = f;
		bp_docoef(x, x->x_freq, x->x_q);
	}
	else if (in == 2) {
		x->x_q = f;
		bp_docoef(x, x->x_freq, x->x_q);
	}
}

void bp_int(t_bp *x, long f)
{
	bp_float(x, (double)f);
}

void bp_free(t_bp *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->x_ctl);
}

void bp_assist(t_bp *x, void *b, long msg, long arg, char *dst){
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

t_int *sigbp_perform1(t_int *w)
{
	t_bp *x = (t_bp *)(w[1]);
	float *input = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *q = (t_float *)(w[4]);
	float *output = (t_float *)(w[5]);

	int n = (t_int)w[6];

	t_bpctl *c = x->x_ctl;
	float last = (float)c->c_x1;
	float prev = (float)c->c_x2;
	float coef1 = (float)c->c_coef1;
	float coef2 = (float)c->c_coef2;
	float gain = (float)c->c_gain;
	float out;

	while (n--){
		x->x_freq = *freq++;
		x->x_q = *q++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;

	return (w + 7);
}

t_int *sigbp_perform2(t_int *w)
{
	t_bp *x = (t_bp *)(w[1]);
	float *input = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *q = (t_float *)(w[4]);
	float *output = (t_float *)(w[5]);

	int n = (t_int)w[6];

	t_bpctl *c = x->x_ctl;
	float last = (float)c->c_x1;
	float prev = (float)c->c_x2;
	float coef1 = (float)c->c_coef1;
	float coef2 = (float)c->c_coef2;
	float gain = (float)c->c_gain;
	float out;

	while (n--){
		x->x_freq = *freq++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;

	return (w + 7);
}

t_int *sigbp_perform3(t_int *w)
{
	t_bp *x = (t_bp *)(w[1]);
	float *input = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *q = (t_float *)(w[4]);
	float *output = (t_float *)(w[5]);

	int n = (t_int)w[6];

	t_bpctl *c = x->x_ctl;
	float last = (float)c->c_x1;
	float prev = (float)c->c_x2;
	float coef1 = (float)c->c_coef1;
	float coef2 = (float)c->c_coef2;
	float gain = (float)c->c_gain;
	float out;

	while (n--){
		x->x_q = *q++;
		bp_docoef(x, x->x_freq, x->x_q);

		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;

	return (w + 7);
}

t_int *sigbp_perform4(t_int *w)
{
	t_bp *x = (t_bp *)(w[1]);
	float *input = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *q = (t_float *)(w[4]);
	float *output = (t_float *)(w[5]);

	int n = (t_int)w[6];

	t_bpctl *c = x->x_ctl;
	float last = (float)c->c_x1;
	float prev = (float)c->c_x2;
	float coef1 = (float)c->c_coef1;
	float coef2 = (float)c->c_coef2;
	float gain = (float)c->c_gain;
	float out;

	while (n--){
		out = *input++ + coef1 * last + coef2 * prev;
		*output++ = gain * out;

		prev = last;
		last = out;
	}
	/* NAN protect */
	if (MAX_BIGORSMALL(last))
		last = 0;
	if (MAX_BIGORSMALL(prev))
		prev = 0;
	c->c_x1 = last;
	c->c_x2 = prev;

	return (w + 7);
}

void sigbp_dsp(t_bp *x, t_signal **sp, short *count)
{

	if (x->sr != sp[0]->s_sr) x->sr = sp[0]->s_sr;

	bp_docoef(x, x->x_freq, x->x_q);
	if (count[1] & count[2]){
		dsp_add(sigbp_perform1, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
	}
	else if (count[1]){
		dsp_add(sigbp_perform2, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
	}
	else if (count[2]){
		dsp_add(sigbp_perform3, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
	}
	else{
		dsp_add(sigbp_perform4, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
	}
}