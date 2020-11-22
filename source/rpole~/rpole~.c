#define __x86_64__ 
#define MAX_FLOAT_PRECISION 64 //for Max 6

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "bigsmalllib.h"

static t_class *rpole_class;

typedef struct sigrpole
{
	t_pxobject x_obj;
	double x_f;
	double x_last;

	double x_f_coef;
	short x_coef_connected;
} t_sigrpole;

void *sigrpole_new(t_symbol *s, short argc, t_atom *argv);

void sigrpole_ft1(t_sigrpole *x, double f);
void sigrpole_int1(t_sigrpole *x, long f);

void rpole_perform(t_sigrpole *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigrpole_perform(t_int *w);

void rpole_dsp64(t_sigrpole *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigrpole_dsp(t_sigrpole *x, t_signal **sp, short *count);

void rpole_clear(t_sigrpole *x);
void rpole_set(t_sigrpole *x, double f);
void rpole_free(t_sigrpole *x);

void rpole_assist(t_sigrpole *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	rpole_class = class_new("rpole~", (method)sigrpole_new, (method)rpole_free,
		sizeof(t_sigrpole), 0, A_GIMME, 0);
	class_addmethod(rpole_class, (method)rpole_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(rpole_class, (method)sigrpole_dsp, "dsp", A_CANT, 0);
	
	class_addmethod(rpole_class, (method)sigrpole_ft1, "float", A_FLOAT, 0);
	class_addmethod(rpole_class, (method)sigrpole_int1, "int", A_LONG, 0);

	class_addmethod(rpole_class, (method)rpole_set, "set", A_DEFFLOAT, 0);
	class_addmethod(rpole_class, (method)rpole_clear, "clear", 0);

	class_addmethod(rpole_class, (method)rpole_assist, "assist", A_CANT, 0);

	class_dspinit(rpole_class);
	class_register(CLASS_BOX, rpole_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's rpole~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sigrpole_new(t_symbol *s, short argc, t_atom *argv)
{
	double fc;
	t_sigrpole *x = (t_sigrpole *)object_alloc(rpole_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	atom_arg_getdouble(&fc, 0, argc, argv);

	x->x_f_coef = fc;

	x->x_last = 0;

	return(x);
}

void sigrpole_ft1(t_sigrpole *x, double f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}

void sigrpole_int1(t_sigrpole *x, long f)
{
	int inlet = ((t_pxobject*)x)->z_in;

	switch (inlet){
	case 1:
		x->x_f_coef = f;
		break;
	}
}

void rpole_perform(t_sigrpole *x, t_object *dsp64,
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
		*out++ = last = coef * last + next;
	}
	if (MAX_BIGORSMALL(last)) last = 0;

	x->x_last = last;
}

void rpole_dsp64(t_sigrpole *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	object_method(dsp64, gensym("dsp_add64"), x,
		rpole_perform, 0, NULL);
}

void rpole_free(t_sigrpole *x)
{
	dsp_free((t_pxobject *)x);
}

void rpole_clear(t_sigrpole *x){
	x->x_last = 0;
}

void rpole_set(t_sigrpole *x, double f){
	x->x_last = f;
}


void rpole_assist(t_sigrpole *x, void *b, long msg, long arg, char *dst){
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

t_int *sigrpole_perform(t_int *w)
{
	t_sigrpole *x = (t_sigrpole *)(w[1]);
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
		*out++ = last = coef * last + next;
	}
	if (MAX_BIGORSMALL(last))
		last = 0;
	x->x_last = last;

	return (w + 6);
}

void sigrpole_dsp(t_sigrpole *x, t_signal **sp, short *count)
{
	x->x_coef_connected = count[1];//count contains info about signal connections

	dsp_add(sigrpole_perform, 5,x,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[0]->s_n);
}