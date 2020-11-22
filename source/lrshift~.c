#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *lrshift_class;

typedef struct siglrshift
{
	t_pxobject x_obj;
	//t_atom_long x_n;
	int x_n;
	int blck_size;
} t_siglrshift;

void *siglrshift_new(t_symbol *s, short argc, t_atom *argv);

void lrshift_tilde_dsp(t_siglrshift *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void siglrshift_dsp(t_siglrshift *x, t_signal **sp);


void rightshift_perform(t_siglrshift *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigrightshift_perform(t_int *w);

void leftshift_perform(t_siglrshift *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigleftshift_perform(t_int *w);

void lrshift_free(t_siglrshift *x);

int C74_EXPORT main(void)
{
	lrshift_class = class_new("lrshift~", (method)siglrshift_new, (method)lrshift_free,
		sizeof(t_siglrshift), 0, A_GIMME, 0);
	class_addmethod(lrshift_class, (method)lrshift_tilde_dsp, "dsp64", A_CANT, 0);
	class_addmethod(lrshift_class, (method)siglrshift_dsp, "dsp", A_CANT, 0);

	class_dspinit(lrshift_class);
	class_register(CLASS_BOX, lrshift_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's lrshift~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *siglrshift_new(t_symbol *s, short argc, t_atom *argv)
{	
	t_siglrshift *x = (t_siglrshift *)object_alloc(lrshift_class);

	dsp_setup((t_pxobject *)x, 1);
	outlet_new((t_object *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE; //force independent signal vectors

	x->blck_size = sys_getblksize();
	
	atom_arg_getlong(&x->x_n, 0, argc, argv);

	if (x->x_n >= x->blck_size)
		x->x_n = x->blck_size;
	if (x->x_n <= -x->blck_size)
		x->x_n = -x->blck_size;

	return(x);
}

void rightshift_perform(t_siglrshift *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *out = outs[0];
	int n = vectorsize;
	int shift = -1 * x->x_n;

	in += n;
	out += n;

	n -= shift;
	in -= shift;
	while (n--){
		*--out = *--in;
	}
	while (shift--){
		*--out = 0;
	}
}

void leftshift_perform(t_siglrshift *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *out = outs[0];
	int shift = x->x_n;
	int n = vectorsize;

	in += shift;
	n -= shift;
	while (n--){
		*out++ = *in++;
	}
	while (shift--){
		*out++ = 0.;
	}
}

void lrshift_tilde_dsp(t_siglrshift *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	/*int n = sp[0]->s_n;
	int shift = x->x_n;
	if (shift > n)
		shift = n;
	if (shift < -n)
		shift = -n;*/
	if (x->x_n < 0){
		object_method(dsp64, gensym("dsp_add64"), x,
			rightshift_perform, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			leftshift_perform, 0, NULL);
	}
}

void lrshift_free(t_siglrshift *x)
{
	dsp_free((t_pxobject *)x);
}

t_int *sigrightshift_perform(t_int *w)
{
	t_siglrshift *x = (t_siglrshift *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *out = (t_float *)(w[3]);
	int n = w[4];

	int shift = -1 * x->x_n;

	in += n;
	out += n;

	n -= shift;
	in -= shift;
	while (n--){
		*--out = *--in;
	}
	while (shift--){
		*--out = 0;
	}
	return (w + 5);
}

t_int *sigleftshift_perform(t_int *w)
{
	t_siglrshift *x = (t_siglrshift *)(w[1]);
	float *in = (t_float *)(w[2]);
	float *out = (t_float *)(w[3]);
	int n = w[4];
	int shift = x->x_n;

	in += shift;
	n -= shift;
	while (n--){
		*out++ = *in++;
	}
	while (shift--){
		*out++ = 0.;
	}
	return (w + 5);
}

void siglrshift_dsp(t_siglrshift *x, t_signal **sp)
{
	if (x->x_n < 0){
		dsp_add(sigrightshift_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
	}
	else{
		dsp_add(sigleftshift_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
	}
}