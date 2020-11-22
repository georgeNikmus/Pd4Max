#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *rzero_class;

typedef struct sigrzero
{
	t_pxobject x_obj;
	double x_f;
	double x_last;

	double x_f_coef;
	short x_coef_connected;
} t_sigrzero;

void *sigrzero_new(t_symbol *s, short argc, t_atom *argv);

void sigrzero_ft1(t_sigrzero *x, double f);
void sigrzero_int1(t_sigrzero *x, long f);

void rzero_perform(t_sigrzero *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigrzero_perform(t_int *w);

void rzero_dsp64(t_sigrzero *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigrzero_dsp(t_sigrzero *x, t_signal **sp, short *count);


void rzero_clear(t_sigrzero *x);
void rzero_set(t_sigrzero *x, double f);
void rzero_free(t_sigrzero *x);

void rzero_assist(t_sigrzero *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	rzero_class = class_new("rzero~", (method)sigrzero_new, (method)rzero_free,
		sizeof(t_sigrzero), 0, A_GIMME, 0);
	class_addmethod(rzero_class, (method)rzero_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(rzero_class, (method)sigrzero_dsp, "dsp", A_CANT, 0);

	class_addmethod(rzero_class, (method)sigrzero_ft1, "float", A_FLOAT, 0);
	class_addmethod(rzero_class, (method)sigrzero_int1, "int", A_LONG, 0);

	class_addmethod(rzero_class, (method)rzero_set, "set", A_DEFFLOAT, 0);
	class_addmethod(rzero_class, (method)rzero_clear, "clear", 0);

	class_addmethod(rzero_class, (method)rzero_assist, "assist", A_CANT, 0);

	class_dspinit(rzero_class);
	class_register(CLASS_BOX, rzero_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's rzero~ object for Pure data.");
	post("Translated into Max/MSP and updated fo 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sigrzero_new(t_symbol *s, short argc, t_atom *argv)
{
	double fc;
	t_sigrzero *x = (t_sigrzero *)object_alloc(rzero_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	atom_arg_getdouble(&fc, 0, argc, argv);

	x->x_f_coef = fc;

	x->x_last = 0;

	return(x);
}

void sigrzero_ft1(t_sigrzero *x, double f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}

void sigrzero_int1(t_sigrzero *x, long f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}


void rzero_perform(t_sigrzero *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in1 = ins[0];
	//t_double *in2 = ins[1];
	t_double coef = x->x_coef_connected ? *ins[1] : x->x_f_coef;
	t_double *out = outs[0];

	int n = vectorsize;

	double last = x->x_last;
	double next;

	while (n--)
	{
		next = *in1++;
		*out++ = next - coef * last;
		last = next;
	}
	x->x_last = last;
}

void rzero_dsp64(t_sigrzero *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	object_method(dsp64, gensym("dsp_add64"), x,
		rzero_perform, 0, NULL);
}

void rzero_free(t_sigrzero *x)
{
	dsp_free((t_pxobject *)x);
}

void rzero_clear(t_sigrzero *x){
	x->x_last = 0;
}

void rzero_set(t_sigrzero *x, double f){
	x->x_last = f;
}

void rzero_assist(t_sigrzero *x, void *b, long msg, long arg, char *dst){
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

t_int *sigrzero_perform(t_int *w)
{
	t_sigrzero *x = (t_sigrzero *)(w[1]);
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
		*out++ = next - coef * last;
		last = next;
	}
	x->x_last = last;

	return (w + 6);
}

void sigrzero_dsp(t_sigrzero *x, t_signal **sp, short *count)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	dsp_add(sigrzero_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}