#define __x86_64__ 
#define MAX_FLOAT_PRECISION 64 //for Max 6

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include <math.h>

#include "bigsmalllib.h"

//typedef union _sampleint_union {
//	t_sample f;
//	unsigned int i;
//} t_sampleint_union;
//
//static int  BIGORSMALL(t_sample f) {
//	t_sampleint_union u;
//	u.f = f;
//	return ((u.i & 0x60000000) == 0) || ((u.i & 0x60000000) == 0x60000000);
//}

//#define BIGORSMALL(f) ((((*(unsigned int*)&(f))&0x60000000)==0) || \
//(((*(unsigned int*)&(f))&0x60000000)==0x60000000))

static t_class *hip_class;

typedef struct hipctl
{
	double c_x;
	double c_coef;
}t_hipctl;

typedef struct hip
{
	t_pxobject x_obj;
	double sr;
	double x_hz;
	t_hipctl x_cspace;
	t_hipctl *x_ctl;
}t_hip;

void *hip_new(t_symbol *s, short argc, t_atom *argv);

void hip_perform(t_hip *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void hip_perform2(t_hip *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sighip_perform1(t_int *w);
t_int *sighip_perform2(t_int *w);


void hip_dsp64(t_hip *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sighip_dsp(t_hip *x, t_signal **sp, short *count);

void hip_ft1(t_hip *x, double f);

void hip_int(t_hip *x, long f);
void hip_clear(t_hip *x, double q);
void hip_float(t_hip *x, double f);
void hip_free(t_hip *x);
void hip_assist(t_hip *x, void *b, long msg, long arg, char *dst);

//void  ext_main(void *r)
int C74_EXPORT main(void)
{
	hip_class = class_new("hip~", (method)hip_new, (method)hip_free,
		sizeof(t_hip), 0, A_GIMME, 0);
	class_addmethod(hip_class, (method)hip_dsp64, "dsp64", A_CANT, 0); 
	class_addmethod(hip_class, (method)sighip_dsp, "dsp", A_CANT, 0);

	class_addmethod(hip_class, (method)hip_float, "float", A_FLOAT, 0);
	class_addmethod(hip_class, (method)hip_int, "int", A_LONG, 0);
	class_addmethod(hip_class, (method)hip_clear, "clear", 0);

	class_addmethod(hip_class, (method)hip_assist, "assist", A_CANT, 0);

	class_dspinit(hip_class);
	class_register(CLASS_BOX, hip_class);
	
	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's hip~ object for Pure Data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *hip_new(t_symbol *s, short argc, t_atom *argv)
{
	double f;
	t_hip *x = (t_hip *)object_alloc(hip_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	x->sr = sys_getsr();//determines the sample rate

	x->x_ctl = &x->x_cspace;
	x->x_cspace.c_x = 0;

	atom_arg_getdouble(&f, 0, argc, argv);

	hip_ft1(x, f);

	return (x);
}

void hip_ft1(t_hip *x, double f)
{
	if (f < 0) f = 0;
	x->x_hz = f;
	x->x_ctl->c_coef = 1 - f * (2 * 3.141592653589793) / x->sr;
	if (x->x_ctl->c_coef < 0)
		x->x_ctl->c_coef = 0;
	else if (x->x_ctl->c_coef > 1)
		x->x_ctl->c_coef = 1;
}

void hip_perform1(t_hip *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *freq = ins[1];

	t_double *out = outs[0];

	int n = vectorsize;
	
	t_hipctl *c = x->x_ctl;
	//double new;
	int i;

	double last = c->c_x;
	double coef = c->c_coef;

	if (coef < 1)
	{
		for (i = 0; i < n; i++)
		{
			x->x_hz = *freq++;
			hip_ft1(x, x->x_hz);
			t_sample new = *in++ + coef * last;
			*out++ = new - last;
			last = new;
		}
		if (MAX_BIGORSMALL(last))
			last = 0;
		c->c_x = last;
	}
	else
	{
		for (i = 0; i < n; i++)
			*out++ = *in++;
		c->c_x = 0;
	}
}

void hip_perform2(t_hip *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *out = outs[0];

	int n = vectorsize;

	t_hipctl *c = x->x_ctl;
	int i;

	double last = c->c_x;
	double coef = c->c_coef;

	if (coef < 1)
	{
		for (i = 0; i < n; i++)
		{
			t_sample new = *in++ + coef * last;
			*out++ = new - last;
			last = new;
		}
		if (MAX_BIGORSMALL(last))
			last = 0;
		c->c_x = last;
	}
	else
	{
		for (i = 0; i < n; i++)
			*out++ = *in++;
		c->c_x = 0;
	}
}

void hip_dsp64(t_hip *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	if (x->sr != samplerate){//compare the sample rate of the object to signal's vectors sample rate 
		x->sr = samplerate;
	}
	hip_ft1(x, x->x_hz);
	if (count[1]){
		object_method(dsp64, gensym("dsp_add64"), x,
			hip_perform1, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			hip_perform2, 0, NULL);
	}
}

void hip_float(t_hip *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_hz = f;
		hip_ft1(x, x->x_hz);
	}
}

void hip_int(t_hip *x, long f)
{
	hip_float(x, (double)f);
}

void hip_clear(t_hip *x, double q)
{
	x->x_cspace.c_x = 0;
}

void hip_free(t_hip *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->x_ctl);
}

void hip_assist(t_hip *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered"); break;
		case 1: sprintf(dst, "(signal/float) Cutoff Frequency"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Filtered signal");
	}
}

t_int *sighip_perform1(t_int *w)
{
	t_hip *x = (t_hip *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *out = (t_float *)(w[4]);
	
	int n = w[5];

	t_hipctl *c = x->x_ctl;

	float last = (float)c->c_x;
	float coef = (float)c->c_coef;
	//float new;
	int i;

	if (coef < 1)
	{
		for (i = 0; i < n; i++)
		{
			x->x_hz = *freq++;
			hip_ft1(x, x->x_hz);
			float new = *in++ + coef * last;
			*out++ = new - last;
			last = new;
		}
		if (MAX_BIGORSMALL(last))
			last = 0;
		c->c_x = last;
	}
	else
	{
		for (i = 0; i < n; i++)
			*out++ = *in++;
		c->c_x = 0;
	}
	return (w + 6);
}

t_int *sighip_perform2(t_int *w)
{
	t_hip *x = (t_hip *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *out = (t_float *)(w[4]);

	int n = w[5];

	t_hipctl *c = x->x_ctl;

	float last = (float)c->c_x;
	float coef = (float)c->c_coef;
	int i;

	if (coef < 1)
	{
		for (i = 0; i < n; i++)
		{
			float new = *in++ + coef * last;
			*out++ = new - last;
			last = new;
		}
		if (MAX_BIGORSMALL(last))
			last = 0;
		c->c_x = last;
	}
	else
	{
		for (i = 0; i < n; i++)
			*out++ = *in++;
		c->c_x = 0;
	}
	return (w + 6);
}

void sighip_dsp(t_hip *x, t_signal **sp, short *count)
{
	
	if (x->sr != sp[0]->s_sr) x->sr = sp[0]->s_sr;
	hip_ft1(x, x->x_hz);

	if (count[1]){
		dsp_add(sighip_perform1, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
	}
	else{
		dsp_add(sighip_perform2, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);

	}
}