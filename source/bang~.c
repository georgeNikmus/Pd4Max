#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *bang_tilde_class;

typedef struct sigbang
{
	t_pxobject x_obj;                 /* header */
	void *x_outlet;                 /* a "float" outlet */
	void *x_clock;                  /* a "clock" object */
} t_sigbang;

void *bang_tilde_new(t_symbol *s, short argc, t_atom *argv);

void bang_tilde_perform(t_sigbang *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *bang_t_perform(t_int *w);

void bang_tilde_dsp(t_sigbang *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void bang_t_dsp(t_sigbang *x, t_signal **sp);

void bang_tilde_tick(t_sigbang *x);

void bang_tilde_free(t_sigbang *x);

int C74_EXPORT main(void)
{
	bang_tilde_class = class_new("bang~", (method)bang_tilde_new,
		(method)bang_tilde_free, sizeof(t_sigbang), 0, 0);

	class_addmethod(bang_tilde_class, (method)bang_tilde_dsp, "dsp64", A_CANT, 0);
	class_addmethod(bang_tilde_class, (method)bang_t_dsp, "dsp", A_CANT, 0);


	class_dspinit(bang_tilde_class);
	class_register(CLASS_BOX, bang_tilde_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's bang~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *bang_tilde_new(t_symbol *s, short argc, t_atom *argv)
{
	t_sigbang *x;

	x = (t_sigbang *)object_alloc(bang_tilde_class);

	x->x_outlet = bangout((t_object *)x);

	x->x_clock = clock_new((t_object *)x, (method)bang_tilde_tick);

	return (x);
}

void bang_tilde_perform(t_sigbang *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	clock_delay(x->x_clock, 0);
}

void bang_tilde_dsp(t_sigbang *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
		object_method(dsp64, gensym("dsp_add64"), x,
			bang_tilde_perform, 0, NULL);
}

void bang_tilde_tick(t_sigbang *x) /* callback function for the clock */
{
	outlet_bang(x->x_outlet);
}

void bang_tilde_free(t_sigbang *x)
{
	clock_free(x->x_clock);
}

t_int *bang_t_perform(t_int *w)
{
	t_sigbang *x = (t_sigbang *)(w[1]);
	clock_delay(x->x_clock, 0);
	return (w + 2);
}

void bang_t_dsp(t_sigbang *x, t_signal **sp)
{
	dsp_add(bang_t_perform, 1, x);
}