#define __x86_64__
#define MAX_FLOAT_PRECISION  64

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "bigsmalllib.h"

static t_class *cpole_class;

typedef struct sigcpole
{
	t_pxobject x_obj;
	double x_f;
	double x_lastre;
	double x_lastim;

	double x_inim1;
	double x_inre2;
	double x_inim2;

	short x_in2_connected;
	short x_in3_connected;
	short x_in4_connected;
} t_sigcpole;

void *sigcpole_new(t_symbol *s, short argc, t_atom *argv);

void cpole_perform(t_sigcpole *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigcpole_perform(t_int *w);

void cpole_dsp64(t_sigcpole *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigcpole_dsp(t_sigcpole *x, t_signal **sp, short *count);


void sigcpole_ft1(t_sigcpole *x, double f);
void sigcpole_int1(t_sigcpole *x, long f);

void cpole_clear(t_sigcpole *x);
void cpole_set(t_sigcpole *x, double re, double im);
void cpole_free(t_sigcpole *x);

void cpole_assist(t_sigcpole *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	cpole_class = class_new("cpole~", (method)sigcpole_new, (method)cpole_free,
		sizeof(t_sigcpole), 0, A_GIMME, 0);
	class_addmethod(cpole_class, (method)cpole_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(cpole_class, (method)sigcpole_dsp, "dsp", A_CANT, 0);

	class_addmethod(cpole_class, (method)sigcpole_ft1, "float", A_FLOAT, 0);
	class_addmethod(cpole_class, (method)sigcpole_int1, "int", A_LONG, 0);

	class_addmethod(cpole_class, (method)cpole_set, "set", A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(cpole_class, (method)cpole_clear, "clear",A_GIMME, 0);

	class_addmethod(cpole_class, (method)cpole_assist, "assist", A_CANT, 0);

	class_dspinit(cpole_class);
	class_register(CLASS_BOX, cpole_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's cpole~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sigcpole_new(t_symbol *s, short argc, t_atom *argv)
{

	t_sigcpole *x = (t_sigcpole *)object_alloc(cpole_class);

	x->x_lastre = x->x_lastim = 0;
	x->x_f = 0;

	dsp_setup((t_pxobject *)x, 4);
	outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	atom_arg_getdouble(&x->x_inre2, 0, argc, argv);
	atom_arg_getdouble(&x->x_inim2, 1, argc, argv);

	return(x);
}

void cpole_perform(t_sigcpole *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *inre1 = ins[0];
	//t_double *inim1 = ins[1];
	t_double	nextim = x->x_in2_connected ? *ins[1] : x->x_inim1;
	//t_double *inre2 = ins[2];
	t_double	coefre = x->x_in3_connected ? *ins[2] : x->x_inre2;
	//t_double *inim2 = ins[3];
	t_double	coefim = x->x_in4_connected ? *ins[3] : x->x_inim2;

	t_double *outre = outs[0];
	t_double *outim = outs[1];

	int n = vectorsize;

	double lastre = x->x_lastre;
	double lastim = x->x_lastim;
	double nextre, tempre;

	while (n--)
	{
		nextre = *inre1++;

		tempre = *outre++ = nextre + lastre * coefre - lastim * coefim;
		lastim = *outim++ = nextim + lastre * coefim + lastim * coefre;
		lastre = tempre;
	}

	if (MAX_BIGORSMALL(lastre)) lastre = 0;
	if (MAX_BIGORSMALL(lastim)) lastim = 0;

	x->x_lastre = lastre;
	x->x_lastim = lastim;
}

void cpole_dsp64(t_sigcpole *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->x_in2_connected = count[1];
	x->x_in3_connected = count[2];
	x->x_in4_connected = count[3];

	object_method(dsp64, gensym("dsp_add64"), x,
		cpole_perform, 0, NULL);
}

void cpole_free(t_sigcpole *x)
{
	dsp_free((t_pxobject *)x);
}

void cpole_clear(t_sigcpole *x){
	x->x_lastre = x->x_lastim = 0;
}

void cpole_set(t_sigcpole *x, double re, double im){
	x->x_lastre = re;
	x->x_lastim = im;
}


void cpole_assist(t_sigcpole *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered-Real part"
			"(clear) Clear internal state to zero"
			"(set) set internal state-real and imaginary parts"); break;
		case 1: sprintf(dst, "(signal) Signal to filter-Imaginary part "); break;
		case 2: sprintf(dst, "(signal) Filter coefficient - Real part "); break;
		case 3: sprintf(dst, "(signal) Filter coefficient - Imaginary part "); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Filtered signal-re"); break;
		case 1: sprintf(dst, "(signal) Filtered signal-im"); break;
		}
	}
}

void sigcpole_ft1(t_sigcpole *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_inim1 = f;
	}
	else if (in == 2) {
		x->x_inre2 = f;
	}
	else if (in == 3) {
		x->x_inim2 = f;
	}
}

void sigcpole_int1(t_sigcpole *x, long f)
{
	sigcpole_ft1(x, (double)f);
}

t_int *sigcpole_perform(t_int *w)
{
	t_sigcpole *x = (t_sigcpole *)(w[1]);
	t_float *inre1 = (t_float *)(w[2]);
	//float *inim1 = (t_float *)(w[3]);
	t_float	nextim = x->x_in2_connected ? *(t_float *)(w[3]) : (float)x->x_inim1;
	//float *inre2 = (t_float *)(w[4]);
	t_float	coefre = x->x_in3_connected ? *(t_float *)(w[4]) : (float)x->x_inre2;
	//float *inim2 = (t_float *)(w[5]);
	t_float	coefim = x->x_in4_connected ? *(t_float *)(w[5]) : (float)x->x_inim2;

	t_float *outre = (t_float *)(w[6]);
	t_float *outim = (t_float *)(w[7]);
	int n = w[8];

	float lastre = (float)x->x_lastre;
	float lastim = (float)x->x_lastim;
	float nextre, tempre;

	while (n--)
	{
		nextre = *inre1++;

		tempre = *outre++ = nextre + lastre * coefre - lastim * coefim;
		lastim = *outim++ = nextim + lastre * coefim + lastim * coefre;
		lastre = tempre;
	}

	if (MAX_BIGORSMALL(lastre)) lastre = 0;
	if (MAX_BIGORSMALL(lastim)) lastim = 0;

	x->x_lastre = lastre;
	x->x_lastim = lastim;

	return (w + 9);
}

void sigcpole_dsp(t_sigcpole *x, t_signal **sp, short *count)
{
	x->x_in2_connected = count[1];
	x->x_in3_connected = count[2];
	x->x_in4_connected = count[3];

	dsp_add(sigcpole_perform, 8,
		x,sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
		sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);
}