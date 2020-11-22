#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *rzero_rev_class;

typedef struct sigrzero_rev
{
	t_pxobject x_obj;
	double x_last;

	double x_f_coef;
	short x_coef_connected;
} t_sigrzero_rev;

void *sigrzero_rev_new(t_symbol *s, short argc, t_atom *argv);

void sigrzero_rev_ft1(t_sigrzero_rev *x, double f);
void sigrzero_rev_int1(t_sigrzero_rev *x, long f);

void rzero_rev_perform(t_sigrzero_rev *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigrzero_rev_perform(t_int *w);


void rzero_rev_dsp64(t_sigrzero_rev *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigrzero_rev_dsp(t_sigrzero_rev *x, t_signal **sp, short *count);


void rzero_rev_clear(t_sigrzero_rev *x);
void rzero_rev_set(t_sigrzero_rev *x, double f);
void rzero_rev_free(t_sigrzero_rev *x);

void rzero_rev_assist(t_sigrzero_rev *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	rzero_rev_class = class_new("rzero_rev~", (method)sigrzero_rev_new, (method)rzero_rev_free,
		sizeof(t_sigrzero_rev), 0, A_GIMME, 0);
	class_addmethod(rzero_rev_class, (method)rzero_rev_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(rzero_rev_class, (method)sigrzero_rev_dsp, "dsp", A_CANT, 0);


	class_addmethod(rzero_rev_class, (method)sigrzero_rev_ft1, "float", A_FLOAT, 0);
	class_addmethod(rzero_rev_class, (method)sigrzero_rev_int1, "int", A_LONG, 0);

	class_addmethod(rzero_rev_class, (method)rzero_rev_set, "set", A_DEFFLOAT, 0);
	class_addmethod(rzero_rev_class, (method)rzero_rev_clear, "clear", 0);

	class_addmethod(rzero_rev_class, (method)rzero_rev_assist, "assist", A_CANT, 0);

	class_dspinit(rzero_rev_class);
	class_register(CLASS_BOX, rzero_rev_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's rzero_rev~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sigrzero_rev_new(t_symbol *s, short argc, t_atom *argv)
{
	double fc;
	t_sigrzero_rev *x = (t_sigrzero_rev *)object_alloc(rzero_rev_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	atom_arg_getdouble(&fc, 0, argc, argv);

	x->x_f_coef = fc;

	x->x_last = 0;

	return(x);
}

void sigrzero_rev_ft1(t_sigrzero_rev *x, double f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}

void sigrzero_rev_int1(t_sigrzero_rev *x, long f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}


void rzero_rev_perform(t_sigrzero_rev *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in1 = ins[0];
	//t_double *in2 = ins[1];
	t_double	coef = x->x_coef_connected ? *ins[1] : x->x_f_coef;
	t_double *out = outs[0];

	int n = vectorsize;

	double last = x->x_last;
	double next;

	while (n--)
	{
		next = *in1++;
		*out++ = last - coef * next;
		last = next;
	}
	x->x_last = last;
}

void rzero_rev_dsp64(t_sigrzero_rev *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	object_method(dsp64, gensym("dsp_add64"), x,
		rzero_rev_perform, 0, NULL);
}

void rzero_rev_free(t_sigrzero_rev *x)
{
	dsp_free((t_pxobject *)x);
}

void rzero_rev_clear(t_sigrzero_rev *x){
	x->x_last = 0;
}

void rzero_rev_set(t_sigrzero_rev *x, double f){
	x->x_last = f;
}


void rzero_rev_assist(t_sigrzero_rev *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered"
			"(clear) Clear internal state to zero"
			"(set) set internal state"); break;
		case 1: sprintf(dst, "(signal/float) Filter coefficient"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Filtered signal");
	}
}

t_int *sigrzero_rev_perform(t_int *w)
{
	t_sigrzero_rev *x = (t_sigrzero_rev *)(w[1]);
	t_float *in1 = (t_float *)(w[2]);
	//float *in2 = (t_float *)(w[3]);
	t_float	coef = x->x_coef_connected ? *(t_float *)(w[3]) : (float)x->x_f_coef;
	t_float *out = (t_float *)(w[4]);

	int n = w[5];

	float last = (float)x->x_last;
	float next;

	while (n--)
	{
		next = *in1++;
		*out++ = last - coef * next;
		last = next;
	}
	x->x_last = last;

	return (w + 6);
}

void sigrzero_rev_dsp(t_sigrzero_rev *x, t_signal **sp, short *count)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	dsp_add(sigrzero_rev_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}