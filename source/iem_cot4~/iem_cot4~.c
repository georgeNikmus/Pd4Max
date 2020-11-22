/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2006 */

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "iemlib.h"
#include <math.h>

/* ------------------------ iem_cot4~ ----------------------------- */


t_class *iem_cot4_tilde_class;

typedef struct _iem_cot4_tilde
{
	t_pxobject  x_obj;
	double   x_sr;
	double   x_msi;

	double *iem_cot4_tilde_table_cos;
	double *iem_cot4_tilde_table_sin;

} t_iem_cot4_tilde;

void *iem_cot4_tilde_new(t_symbol *s, short argc, t_atom *argv);

void iem_cot4_tilde_maketable(t_iem_cot4_tilde *x);

void iem_cot4_tilde_perform(t_iem_cot4_tilde *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);

void iem_cot4_tilde_dsp(t_iem_cot4_tilde *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);

void iem_free(t_iem_cot4_tilde *x);

int C74_EXPORT main(void)
{
	iem_cot4_tilde_class = class_new("iem_cot4~", (method)iem_cot4_tilde_new, (method)iem_free,
		sizeof(t_iem_cot4_tilde), 0, 0);
	class_addmethod(iem_cot4_tilde_class, (method)iem_cot4_tilde_dsp, "dsp64", A_CANT, 0);
	
	class_dspinit(iem_cot4_tilde_class);
	class_register(CLASS_BOX, iem_cot4_tilde_class);

	post("--------------------------------------------------------------------------");
	post("From Thomas Musil's IEM library for Pure Data\n");
	return 0;
}

void *iem_cot4_tilde_new(t_symbol *s, short argc, t_atom *argv)
{
	t_iem_cot4_tilde *x = (t_iem_cot4_tilde *)object_alloc(iem_cot4_tilde_class);

	dsp_setup((t_pxobject *)x, 1);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	x->x_sr = 2.0f / sys_getsr();//determines the sample rate
	
	x->iem_cot4_tilde_table_cos = (double *)0L;
	x->iem_cot4_tilde_table_sin = (double *)0L;

	iem_cot4_tilde_maketable(x);
	//  class_sethelpsymbol(iem_cot4_tilde_class, gensym("iemhelp/help-iem_cot4~"));

	x->x_msi = 0;
	return (x);
}

void iem_cot4_tilde_perform(t_iem_cot4_tilde *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];
	t_double *out = outs[0];
	double norm_freq;
	double hout;

	double sr = x->x_sr;
	int n = vectorsize;
	double *ctab = x->iem_cot4_tilde_table_cos, *stab = x->iem_cot4_tilde_table_sin;
	double *caddr, *saddr, cf1, cf2, sf1, sf2, frac;
	double dphase;
	int normhipart;
	int32 mytfi;
	union tabfudge_d tf;

	tf.tf_d = UNITBIT32;
	normhipart = tf.tf_i[HIOFFSET];

#if 0     /* this is the readable version of the code. */
	while (n--)
	{
		norm_freq = *in * sr;
		if (norm_freq < 0.0001f)
			norm_freq = 0.0001f;
		else if (norm_freq > 0.9f)
			norm_freq = 0.9f;
		dphase = (double)(norm_freq * (double)(COSTABSIZE)) + UNITBIT32;
		tf.tf_d = dphase;
		mytfi = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		saddr = stab + (mytfi);
		caddr = ctab + (mytfi);
		tf.tf_i[HIOFFSET] = normhipart;
		frac = tf.tf_d - UNITBIT32;
		sf1 = saddr[0];
		sf2 = saddr[1];
		cf1 = caddr[0];
		cf2 = caddr[1];
		in++;
		*out++ = (cf1 + frac * (cf2 - cf1)) / (sf1 + frac * (sf2 - sf1));
	}
#endif
#if 1     /* this is the same, unwrapped by hand. prolog beg*/
	n /= 4;
	norm_freq = *in * sr;
	if (norm_freq < 0.0001f)
		norm_freq = 0.0001f;
	else if (norm_freq > 0.9f)
		norm_freq = 0.9f;
	dphase = (double)(norm_freq * (double)(COSTABSIZE)) + UNITBIT32;
	tf.tf_d = dphase;
	mytfi = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
	saddr = stab + (mytfi);
	caddr = ctab + (mytfi);
	tf.tf_i[HIOFFSET] = normhipart;
	in += 4;                 /*prolog end*/
	while (--n)
	{
		norm_freq = *in * sr;
		if (norm_freq < 0.0001f)
			norm_freq = 0.0001f;
		else if (norm_freq > 0.9f)
			norm_freq = 0.9f;
		dphase = (double)(norm_freq * (double)(COSTABSIZE)) + UNITBIT32;
		frac = tf.tf_d - UNITBIT32;
		tf.tf_d = dphase;
		sf1 = saddr[0];
		sf2 = saddr[1];
		cf1 = caddr[0];
		cf2 = caddr[1];
		mytfi = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		saddr = stab + (mytfi);
		caddr = ctab + (mytfi);
		hout = (cf1 + frac * (cf2 - cf1)) / (sf1 + frac * (sf2 - sf1));
		*out++ = hout;
		*out++ = hout;
		*out++ = hout;
		*out++ = hout;
		in += 4;
		tf.tf_i[HIOFFSET] = normhipart;
	}/*epilog beg*/
	frac = tf.tf_d - UNITBIT32;
	sf1 = saddr[0];
	sf2 = saddr[1];
	cf1 = caddr[0];
	cf2 = caddr[1];
	hout = (cf1 + frac * (cf2 - cf1)) / (sf1 + frac * (sf2 - sf1));
	*out++ = hout;
	*out++ = hout;
	*out++ = hout;
	*out++ = hout;
	/*epilog end*/
#endif
}

void iem_cot4_tilde_dsp(t_iem_cot4_tilde *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	
	x->x_sr = 2.0f / samplerate;

	object_method(dsp64, gensym("dsp_add64"), x,
		iem_cot4_tilde_perform, 0, NULL);
	
}

void iem_cot4_tilde_maketable(t_iem_cot4_tilde *x)
{
	int i;
	double *fp, phase, phsinc = 0.5*3.141592653 / ((double)COSTABSIZE);
	union tabfudge_d tf;

	if (!x->iem_cot4_tilde_table_sin)
	{   
		x->iem_cot4_tilde_table_sin = (double *)sysmem_newptr(sizeof(double) * (COSTABSIZE + 1));
		for (i = COSTABSIZE + 1, fp = x->iem_cot4_tilde_table_sin, phase = 0; i--; fp++, phase += phsinc)
			*fp = sin(phase);
	}
	if (!x->iem_cot4_tilde_table_cos)
	{
		x->iem_cot4_tilde_table_cos = (double *)sysmem_newptr(sizeof(double) * (COSTABSIZE + 1));
		for (i = COSTABSIZE + 1, fp = x->iem_cot4_tilde_table_cos, phase = 0; i--; fp++, phase += phsinc)
			*fp = cos(phase);
	}
	tf.tf_d = UNITBIT32 + 0.5;
	if ((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
		post("iem_cot4~: unexpected machine alignment");
}

void iem_free(t_iem_cot4_tilde *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->iem_cot4_tilde_table_cos);
	sysmem_freeptr(x->iem_cot4_tilde_table_sin);
	

}