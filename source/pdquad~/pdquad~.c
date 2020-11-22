#define __x86_64__
#define MAX_FLOAT_PRECISION 64

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "bigsmalllib.h"

static t_class *pdquad_class;

typedef struct pdquadctl
{
	double c_x1;
	double c_x2;
	double c_fb1;
	double c_fb2;
	double c_ff1;
	double c_ff2;
	double c_ff3;
} t_pdquadctl;

typedef struct sigpdquad
{
	t_pxobject x_obj;
	double x_f;
	t_pdquadctl x_cspace;
	t_pdquadctl *x_ctl;
} t_sigpdquad;

void sigpdquad_list(t_sigpdquad *x, t_symbol *msg, short argc, t_atom *argv);

void *pdquad_new(t_symbol *s, short argc, t_atom *argv);

void pdquad_set(t_sigpdquad *x, t_symbol *s, int argc, t_atom *argv);

void pdq_dsp64(t_sigpdquad *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigpdquad_dsp(t_sigpdquad *x, t_signal **sp);

void pdq_perform(t_sigpdquad *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigpdquad_perform(t_int *w);


void pdq_free(t_sigpdquad *x);
void pdq_assist(t_sigpdquad *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	pdquad_class = class_new("pdquad~", (method)pdquad_new, (method)pdq_free,
		sizeof(t_sigpdquad), 0, A_GIMME, 0);
	class_addmethod(pdquad_class, (method)pdq_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(pdquad_class, (method)sigpdquad_dsp, "dsp", A_CANT, 0);
	
	class_addmethod(pdquad_class, (method)sigpdquad_list, "list", A_GIMME, 0);
	class_addmethod(pdquad_class, (method)pdquad_set, "set",A_GIMME, 0);
	class_addmethod(pdquad_class, (method)pdquad_set, "clear",A_GIMME, 0);

	class_addmethod(pdquad_class, (method)pdq_assist, "assist", A_CANT, 0);

	class_dspinit(pdquad_class);
	class_register(CLASS_BOX, pdquad_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's biquad~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *pdquad_new(t_symbol *s, short argc, t_atom *argv)
{
	//double f, q;
	t_sigpdquad *x = (t_sigpdquad *)object_alloc(pdquad_class);

	dsp_setup((t_pxobject *)x, 1);
	outlet_new((t_pxobject *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	x->x_ctl = &x->x_cspace;
	x->x_cspace.c_x1 = x->x_cspace.c_x2 = 0;
	sigpdquad_list(x, s, argc, argv);
	x->x_f = 0;

	return (x);
}

void sigpdquad_list(t_sigpdquad *x, t_symbol *msg, short argc, t_atom *argv)
{
	double fb1, fb2, ff1, ff2, ff3;
	
	fb1 = atom_getfloat(argv + 0);
	fb2 = atom_getfloat(argv + 1);
	ff1 = atom_getfloat(argv + 2);
	ff2 = atom_getfloat(argv + 3);
	ff3 = atom_getfloat(argv + 4);

	double discriminant = fb1 * fb1 + 4 * fb2;
	t_pdquadctl *c = x->x_ctl;
	
	if (discriminant < 0) /* imaginary roots -- resonant filter */
	{
		/* they're conjugates so we just check that the product
		is less than one */
		if (fb2 >= -1.0f) goto stable;
	}
	else    /* real roots */
	{
		/* check that the parabola 1 - fb1 x - fb2 x^2 has a
		vertex between -1 and 1, and that it's nonnegative
		at both ends, which implies both roots are in [1-,1]. */
		if (fb1 <= 2.0f && fb1 >= -2.0f &&
			1.0f - fb1 - fb2 >= 0 && 1.0f + fb1 - fb2 >= 0)
			goto stable;
	}
	/* if unstable, just bash to zero */
	fb1 = fb2 = ff1 = ff2 = ff3 = 0;
stable:
	c->c_fb1 = fb1;
	c->c_fb2 = fb2;
	c->c_ff1 = ff1;
	c->c_ff2 = ff2;
	c->c_ff3 = ff3;
}

void pdquad_set(t_sigpdquad *x, t_symbol *s, int argc, t_atom *argv)
{
	t_pdquadctl *c = x->x_ctl;
	c->c_x1 = atom_getfloatarg(0, argc, argv);
	c->c_x2 = atom_getfloatarg(1, argc, argv);
	
}

void pdq_perform(t_sigpdquad *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *out = outs[0];

	int n = vectorsize;
	t_pdquadctl *c = x->x_ctl;
	double last = c->c_x1;
	double prev = c->c_x2;
	double fb1 = c->c_fb1;
	double fb2 = c->c_fb2;
	double ff1 = c->c_ff1;
	double ff2 = c->c_ff2;
	double ff3 = c->c_ff3;
	double output;

	while (n--)
	{
		output = *in++ + fb1 * last + fb2 * prev;
		if (MAX_BIGORSMALL(output)) output = 0;
	
		*out++ = ff1 * output + ff2 * last + ff3 * prev;
		prev = last;
		last = output;
	}
	c->c_x1 = last;
	c->c_x2 = prev;
}

void pdq_dsp64(t_sigpdquad *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	object_method(dsp64, gensym("dsp_add64"), x,
		pdq_perform, 0, NULL);
}


void pdq_free(t_sigpdquad *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->x_ctl);
}

void pdq_assist(t_sigpdquad *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		sprintf(dst, "\n(signal) Incoming signal\n "
			"(list) A list of 5 floats is used to set the filter parameters\n "
			"(clear)clear internal state");
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Outcoming signal");
	}
}

t_int *sigpdquad_perform(t_int *w)
{
	t_sigpdquad *x = (t_sigpdquad *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *out = (t_float *)(w[3]);

	int n = w[4];

	t_pdquadctl *c = x->x_ctl;
	float last = (float)c->c_x1;
	float prev = (float)c->c_x2;
	float fb1 = (float)c->c_fb1;
	float fb2 = (float)c->c_fb2;
	float ff1 = (float)c->c_ff1;
	float ff2 = (float)c->c_ff2;
	float ff3 = (float)c->c_ff3;
	float output;

	while (n--)
	{
		output = *in++ + fb1 * last + fb2 * prev;
		if (MAX_BIGORSMALL(output)) output = 0;

		*out++ = ff1 * output + ff2 * last + ff3 * prev;
		prev = last;
		last = output;
	}
	c->c_x1 = last;
	c->c_x2 = prev;

	return (w + 5);
}

void sigpdquad_dsp(t_sigpdquad *x, t_signal **sp)
{
	dsp_add(sigpdquad_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}