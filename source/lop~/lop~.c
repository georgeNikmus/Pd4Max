#define __x86_64__ 
#define MAX_FLOAT_PRECISION 64 //for Max 6

#include "definitions.h"
#include "bigsmalllib.h"

static t_class *lop_class;

typedef struct lopctl
{
	double c_x;
	double c_coef;
}t_lopctl;

typedef struct lop
{
	t_pxobject x_obj;
	double sr;
	double x_hz;
	t_lopctl x_cspace;
	t_lopctl *x_ctl;
}t_lop;

void lop_dsp64(t_lop *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void siglop_dsp(t_lop *x, t_signal **sp, short *count);

void lop_perform1(t_lop *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void lop_perform2(t_lop *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *siglop_perform1(t_int *w);
t_int *siglop_perform2(t_int *w);

void lop_ft1(t_lop *x);
void lop_float(t_lop *x, double f);
void lop_int(t_lop *x, long f);
void lop_clear(t_lop *x, double q);
void lop_free(t_lop *x);
void lop_assist(t_lop *x, void *b, long msg, long arg, char *dst);
void *lop_new(t_symbol *s, short argc, t_atom *argv);

int C74_EXPORT main(void)
{
	lop_class = class_new("lop~", (method)lop_new, (method)lop_free,
		sizeof(t_lop), 0, A_GIMME, 0);
	class_addmethod(lop_class, (method)lop_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(lop_class, (method)siglop_dsp, "dsp", A_CANT, 0);

	class_addmethod(lop_class, (method)lop_float, "float", A_FLOAT, 0);
	class_addmethod(lop_class, (method)lop_int, "int", A_LONG, 0);
	class_addmethod(lop_class, (method)lop_clear, "clear", 0);

	class_addmethod(lop_class, (method)lop_assist, "assist", A_CANT, 0);

	class_dspinit(lop_class);
	class_register(CLASS_BOX, lop_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's lop~ object for Pure Data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void lop_dsp64(t_lop *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	if (x->sr != samplerate){//compare the sample rate of the object to signal's vectors sample rate 
		x->sr = samplerate;
	}
	lop_ft1(x);

	if (count[1]){
		object_method(dsp64, gensym("dsp_add64"), x,
			lop_perform1, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			lop_perform2, 0, NULL);
	}
}

void siglop_dsp(t_lop *x, t_signal **sp, short *count)
{
	if (x->sr != sp[0]->s_sr) x->sr = sp[0]->s_sr;
	lop_ft1(x);
	if (count[1]){
		dsp_add(siglop_perform1, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
	}
	else{
		dsp_add(siglop_perform2, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
	}
}

void lop_perform1(t_lop *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *freq = ins[1];

	t_double *out = outs[0];

	int n = vectorsize;

	t_lopctl *c = x->x_ctl;
	double last = c->c_x;
	double coef = c->c_coef;
	double feedback = 1 - coef;;

	while (n--){
		x->x_hz = *freq++;
		lop_ft1(x);
		
		last = *out++ = coef * *in++ + feedback * last;
	}
	if (MAX_BIGORSMALL(last)) last = 0;
	c->c_x = last;
}

void lop_perform2(t_lop *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];

	t_double *out = outs[0];

	int n = vectorsize;

	t_lopctl *c = x->x_ctl;
	double last = c->c_x;
	double coef = c->c_coef;
	double feedback = 1 - coef;;

	while (n--){
		last = *out++ = coef * *in++ + feedback * last;
	}
	if (MAX_BIGORSMALL(last)) last = 0;
	c->c_x = last;
}


t_int *siglop_perform1(t_int *w)
{
	t_lop *x = (t_lop *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *out = (t_float *)(w[4]);

	int n = w[5];

	t_lopctl *c = x->x_ctl;
	float last = (float)c->c_x;
	float coef = (float)c->c_coef;
	float feedback = 1 - coef;;

	while (n--){
		x->x_hz = *freq++;
		lop_ft1(x);

		last = *out++ = coef * *in++ + feedback * last;
	}
	if (MAX_BIGORSMALL(last)) last = 0;
	c->c_x = last;

	return (w + 6);
}

t_int *siglop_perform2(t_int *w)
{
	t_lop *x = (t_lop *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *freq = (t_float *)(w[3]);
	float *out = (t_float *)(w[4]);

	int n = w[5];

	t_lopctl *c = x->x_ctl;
	float last = (float)c->c_x;
	float coef = (float)c->c_coef;
	float feedback = 1 - coef;;

	while (n--){
		last = *out++ = coef * *in++ + feedback * last;
	}
	if (MAX_BIGORSMALL(last)) last = 0;
	c->c_x = last;

	return (w + 6);
}

void lop_ft1(t_lop *x)
{
	double f = x->x_hz;
	if (f < 0) f = 0;

	x->x_ctl->c_coef = f * (2 * 3.141592653589793) / x->sr;
	if (x->x_ctl->c_coef < 0)
		x->x_ctl->c_coef = 0;
	else if (x->x_ctl->c_coef > 1)
		x->x_ctl->c_coef = 1;
}

void lop_float(t_lop *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_hz = f;
		lop_ft1(x);
	}
}

void lop_int(t_lop *x, long f)
{
	lop_float(x, (double)f);
}

void lop_clear(t_lop *x, double q)
{
	x->x_cspace.c_x = 0;
}

void lop_free(t_lop *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->x_ctl);
}

void lop_assist(t_lop *x, void *b, long msg, long arg, char *dst){
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

void *lop_new(t_symbol *s, short argc, t_atom *argv)
{
	double f;
	t_lop *x = (t_lop *)object_alloc(lop_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	x->sr = sys_getsr();//determines the sample rate

	x->x_ctl = &x->x_cspace;
	x->x_cspace.c_x = 0;

	atom_arg_getdouble(&f, 0, argc, argv);
	x->x_hz = f;
	lop_ft1(x);

	return (x);
}