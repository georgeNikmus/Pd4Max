#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *czero_rev_class;

typedef struct sigczero_rev
{
	t_pxobject x_obj;
	double x_lastre;
	double x_lastim;

	double x_inim1;
	double x_inre2;
	double x_inim2;

	short x_in2_connected;
	short x_in3_connected;
	short x_in4_connected;
} t_sigczero_rev;

void *sigczero_rev_new(t_symbol *s, short argc, t_atom *argv);

void czero_rev_perform(t_sigczero_rev *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigczero_rev_perform(t_int *w);

void czero_rev_dsp64(t_sigczero_rev *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigczero_rev_dsp(t_sigczero_rev *x, t_signal **sp, short *count);

void czero_rev_ft1(t_sigczero_rev *x, double f);
void czero_rev_int1(t_sigczero_rev *x, long f);

void czero_rev_clear(t_sigczero_rev *x);
void czero_rev_set(t_sigczero_rev *x, double re, double im);
void czero_rev_free(t_sigczero_rev *x);

void czero_rev_assist(t_sigczero_rev *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	czero_rev_class = class_new("czero_rev~", (method)sigczero_rev_new, (method)czero_rev_free,
		sizeof(t_sigczero_rev), 0, A_GIMME, 0);
	class_addmethod(czero_rev_class, (method)czero_rev_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(czero_rev_class, (method)sigczero_rev_dsp, "dsp", A_CANT, 0);

	class_addmethod(czero_rev_class, (method)czero_rev_set, "set", A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(czero_rev_class, (method)czero_rev_clear, "clear",A_GIMME, 0);

	class_addmethod(czero_rev_class, (method)czero_rev_ft1, "float", A_FLOAT, 0);
	class_addmethod(czero_rev_class, (method)czero_rev_int1, "int", A_LONG, 0);

	class_addmethod(czero_rev_class, (method)czero_rev_assist, "assist", A_CANT, 0);

	class_dspinit(czero_rev_class);
	class_register(CLASS_BOX, czero_rev_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's czero_rev~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sigczero_rev_new(t_symbol *s, short argc, t_atom *argv)
{

	t_sigczero_rev *x = (t_sigczero_rev *)object_alloc(czero_rev_class);

	x->x_lastre = x->x_lastim = 0;

	dsp_setup((t_pxobject *)x, 4);
	outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	atom_arg_getdouble(&x->x_inre2, 0, argc, argv);
	atom_arg_getdouble(&x->x_inim2, 1, argc, argv);

	return(x);
}

void czero_rev_perform(t_sigczero_rev *x, t_object *dsp64,
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
	double nextre;

	while (n--)
	{
		nextre = *inre1++;

		*outre++ = lastre - nextre * coefre - nextim * coefim;
		*outim++ = lastim - nextre * coefim + nextim * coefre;
		lastre = nextre;
		lastim = nextim;
	}
	x->x_lastre = lastre;
	x->x_lastim = lastim;
}

void czero_rev_dsp64(t_sigczero_rev *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->x_in2_connected = count[1];
	x->x_in3_connected = count[2];
	x->x_in4_connected = count[3];

	object_method(dsp64, gensym("dsp_add64"), x,
		czero_rev_perform, 0, NULL);
}

void czero_rev_free(t_sigczero_rev *x)
{
	dsp_free((t_pxobject *)x);
}

void czero_rev_clear(t_sigczero_rev *x){
	x->x_lastre = x->x_lastim = 0;
}

void czero_rev_set(t_sigczero_rev *x, double re, double im){
	x->x_lastre = re;
	x->x_lastim = im;
}


void czero_rev_assist(t_sigczero_rev *x, void *b, long msg, long arg, char *dst){
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

void czero_rev_ft1(t_sigczero_rev *x, double f)
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

void czero_rev_int1(t_sigczero_rev *x, long f)
{
	czero_rev_ft1(x, (double)f);
}

t_int *sigczero_rev_perform(t_int *w)
{
	t_sigczero_rev *x = (t_sigczero_rev *)(w[1]);
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
	float nextre;

	while (n--)
	{
		nextre = *inre1++;

		*outre++ = lastre - nextre * coefre - nextim * coefim;
		*outim++ = lastim - nextre * coefim + nextim * coefre;
		lastre = nextre;
		lastim = nextim;
	}
	x->x_lastre = lastre;
	x->x_lastim = lastim;

	return (w + 9);
}

void sigczero_rev_dsp(t_sigczero_rev *x, t_signal **sp, short *count)
{
	x->x_in2_connected = count[1];
	x->x_in3_connected = count[2];
	x->x_in4_connected = count[3];

	dsp_add(sigczero_rev_perform, 8,
		x,sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
		sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);
}