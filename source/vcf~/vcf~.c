#define __x86_64__
#define MAX_FLOAT_PRECISION 64

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include "bigsmalllib.h"
#include "iemlib.h"


static t_class *vcf_class;

typedef struct vcfctl
{
	double c_re;
	double c_im;
	double c_q;
	double c_isr;
	double c_cf;
} t_vcfctl;

typedef struct sigvcf
{
	t_pxobject x_obj;
	t_vcfctl x_cspace;
	t_vcfctl *x_ctl;
	double sr;

	double *cos_table;

	short freq_connected;
	short q_connected;
} t_sigvcf;

void *vcf_new(t_symbol *s, short argc, t_atom *argv);

void sigvcf_ft1(t_sigvcf *x, double f);
void sigvcf_int1(t_sigvcf *x, long f);

void vcf_perform1(t_sigvcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
void vcf_perform2(t_sigvcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigvcf_perform1(t_int *w);
t_int *sigvcf_perform2(t_int *w);

void vcf_dsp64(t_sigvcf *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigvcf_dsp(t_sigvcf *x, t_signal **sp, short *count);

void vcf_free(t_sigvcf *x);
void cos_maketable(t_sigvcf *x);
void vcf_assist(t_vcfctl *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	vcf_class = class_new("vcf~", (method)vcf_new, (method)vcf_free,
		sizeof(t_sigvcf), 0, A_GIMME, 0);
	class_addmethod(vcf_class, (method)vcf_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(vcf_class, (method)sigvcf_dsp, "dsp", A_CANT, 0);

	class_addmethod(vcf_class, (method)sigvcf_ft1, "float", A_FLOAT, 0);
	class_addmethod(vcf_class, (method)sigvcf_int1, "int", A_LONG, 0);

	class_addmethod(vcf_class, (method)vcf_assist, "assist", A_CANT, 0);

	class_dspinit(vcf_class);
	class_register(CLASS_BOX, vcf_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's vcf~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *vcf_new(t_symbol *s, short argc, t_atom *argv)
{
	double q = 1;
	double cfreq = 0;
	t_sigvcf *x = (t_sigvcf *)object_alloc(vcf_class);

	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	x->sr = sys_getsr();//determines the sample rate

	x->x_ctl = &x->x_cspace;
	x->x_cspace.c_re = 0;
	x->x_cspace.c_im = 0;
	x->x_cspace.c_isr = 0;

	atom_arg_getdouble(&cfreq, 0, argc, argv);
	atom_arg_getdouble(&q, 1, argc, argv);

	x->x_cspace.c_q = (q > 0 ? q : 0);
	x->x_cspace.c_cf = cfreq;

	cos_maketable(x);

	return(x);
}

void sigvcf_ft1(t_sigvcf *x, double f)
{
	long in = proxy_getinlet((t_object *)x);

	if (in == 1) {
		x->x_ctl->c_cf = f;
	}
	else if (in == 2) {
		x->x_ctl->c_q = (f > 0 ? f : 0);
	}
}

void sigvcf_int1(t_sigvcf *x, long f)
{
	sigvcf_ft1(x, (double)f);
}

void vcf_perform1(t_sigvcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_vcfctl *c = x->x_ctl;
	t_double *in1 = ins[0];
	t_double *in2 = ins[1];
	//t_double *in3 = ins[2];
	t_double q = x->q_connected ? *ins[2] : c->c_q;
	
	t_double *out1 = outs[0];
	t_double *out2 = outs[1];

	int n = vectorsize;
	
	double re = c->c_re, re2;
	double im = c->c_im;
	//double q;
	//if (x->q_connected) q = *in3;
	//else  q = c->c_q;
	double centerf = c->c_cf;
	double qinv = (q > 0 ? 1.0 / q : 0);
	double ampcorrect = 2.0 - 2.0 / (q + 2.0);
	double isr = c->c_isr;
	double coefr, coefi;
	double *tab = x->cos_table, *addr, f1, f2, frac;
	double dphase;
	int normhipart, tabindex;
	union tabfudge_d tf;

	tf.tf_d = UNITBIT32;
	normhipart = tf.tf_i[HIOFFSET];

	while(n--)
	{
		double cf, cfindx, r, oneminusr;
		cf = *in2++ * isr;

		if (cf < 0) cf = 0;

		cfindx = cf * (double)(COSTABSIZE / 6.28318f);
		r = (qinv > 0 ? 1 - cf * qinv : 0);
		
		if (r < 0) r = 0;
		
		oneminusr = 1.0 - r;
		dphase = ((double)(cfindx)) + UNITBIT32;
		
		tf.tf_d = dphase;
		tabindex = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		addr = tab + tabindex;
		
		tf.tf_i[HIOFFSET] = normhipart;
		frac = tf.tf_d - UNITBIT32;
		f1 = addr[0];
		f2 = addr[1];
		coefr = r * (f1 + frac * (f2 - f1));

		addr = tab + ((tabindex - (COSTABSIZE / 4)) & (COSTABSIZE - 1));
		f1 = addr[0];
		f2 = addr[1];
		coefi = r * (f1 + frac * (f2 - f1));

		f1 = *in1++;
		re2 = re;
		
		re = ampcorrect * oneminusr * f1 + coefr * re2 - coefi * im;
		*out1++ = re;
		
		im = coefi * re2 + coefr * im;
		*out2++ = im;
	}
	if (MAX_BIGORSMALL(re))	re = 0;
	if (MAX_BIGORSMALL(im)) im = 0;
	c->c_re = re;
	c->c_im = im;
}

void vcf_perform2(t_sigvcf *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_vcfctl *c = x->x_ctl;
	t_double *in1 = ins[0];
	//t_double *in2 = ins[1];
	//t_double *in3 = ins[2];
	t_double q = x->q_connected ? *ins[2] : c->c_q;

	t_double *out1 = outs[0];
	t_double *out2 = outs[1];

	int n = vectorsize;

	double re = c->c_re, re2;
	double im = c->c_im;
	//double q;
	//if (x->q_connected) q = *in3;
	//else  q = c->c_q;
	double centerf = c->c_cf;
	double qinv = (q > 0 ? 1.0 / q : 0);
	double ampcorrect = 2.0 - 2.0 / (q + 2.0);
	double isr = c->c_isr;
	double coefr, coefi;
	double *tab = x->cos_table, *addr, f1, f2, frac;
	double dphase;
	int normhipart, tabindex;
	union tabfudge_d tf;

	tf.tf_d = UNITBIT32;
	normhipart = tf.tf_i[HIOFFSET];

	while (n--)
	{
		double cf, cfindx, r, oneminusr;
		cf = centerf * isr;

		if (cf < 0) cf = 0;

		cfindx = cf * (double)(COSTABSIZE / 6.28318f);
		r = (qinv > 0 ? 1 - cf * qinv : 0);

		if (r < 0) r = 0;

		oneminusr = 1.0 - r;
		dphase = ((double)(cfindx)) + UNITBIT32;

		tf.tf_d = dphase;
		tabindex = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		addr = tab + tabindex;

		tf.tf_i[HIOFFSET] = normhipart;
		frac = tf.tf_d - UNITBIT32;
		f1 = addr[0];
		f2 = addr[1];
		coefr = r * (f1 + frac * (f2 - f1));

		addr = tab + ((tabindex - (COSTABSIZE / 4)) & (COSTABSIZE - 1));
		f1 = addr[0];
		f2 = addr[1];
		coefi = r * (f1 + frac * (f2 - f1));

		f1 = *in1++;
		re2 = re;

		re = ampcorrect * oneminusr * f1 + coefr * re2 - coefi * im;
		*out1++ = re;

		im = coefi * re2 + coefr * im;
		*out2++ = im;
	}
	if (MAX_BIGORSMALL(re))	re = 0;
	if (MAX_BIGORSMALL(im)) im = 0;
	c->c_re = re;
	c->c_im = im;
}

void vcf_dsp64(t_sigvcf *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->freq_connected = count[1];//count contains info about signal connections
	x->q_connected = count[2];//count contains info about signal connections

	if (x->sr != samplerate){//compare the sample rate of the object to signal's vectors sample rate 
		x->sr = samplerate;
	}
	x->x_ctl->c_isr = 6.28318f / x->sr;

	if (x->freq_connected){
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_perform1, 0, NULL);
	}
	else{
		object_method(dsp64, gensym("dsp_add64"), x,
			vcf_perform2, 0, NULL);
	}
}

void vcf_free(t_sigvcf *x)
{
	dsp_free((t_pxobject *)x);
	sysmem_freeptr(x->x_ctl);
	sysmem_freeptr(x->cos_table);
}

void cos_maketable(t_sigvcf *x)
{
	int i;
	double *fp, phase, phsinc = (2. * 3.14159) / COSTABSIZE;
	union tabfudge_d tf;
	
	if (x->cos_table) return;
	x->cos_table = (double *)sysmem_newptr(sizeof(double) * (COSTABSIZE + 1));
	for (i = COSTABSIZE + 1, fp = x->cos_table, phase = 0; i--;
		fp++, phase += phsinc)
		*fp = cos(phase);

	/* here we check at startup whether the byte alignment
	is as we declared it.  If not, the code has to be
	recompiled the other way. */
	tf.tf_d = UNITBIT32 + 0.5;
	if ((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
		post("cos~: unexpected machine alignment");
}

void vcf_assist(t_vcfctl *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be filtered"); break;
		case 1: sprintf(dst, "(signal/float) Center Frequency"); break;
		case 2: sprintf(dst, "(float) Q/filter sharpness"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Filtered signal(Real Part)"); break;
		case 1: sprintf(dst, "(signal) Filtered signal(Imaginary Part)"); break;
		}
	}
}

t_int *sigvcf_perform1(t_int *w)
{
	
	t_sigvcf *x = (t_sigvcf *)(w[1]);
	t_vcfctl *c = x->x_ctl;
	t_float *in1 = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	//float *in3 = (t_float *)(w[4]);
	t_float	q = x->q_connected ? *(t_float *)(w[4]) : (float)c->c_q;

	t_float *out1 = (t_float *)(w[5]);
	t_float *out2 = (t_float *)(w[6]);

	int n = w[7];

	float re = (float)c->c_re, re2;
	float im = (float)c->c_im;
	//float q;
	//if (x->q_connected)  q = *in3;
	//else  q = (float)c->c_q;
	float centerf = (float)c->c_cf;
	float qinv = (float)(q > 0 ? 1.0 / q : 0);
	float ampcorrect = (float)(2.0 - 2.0 / (q + 2.0));
	float isr = (float)c->c_isr;
	float coefr, coefi;
	double *tab = x->cos_table, *addr, f1, f2, frac;
	float dphase;
	int normhipart, tabindex;
	union tabfudge_d tf;

	tf.tf_d = UNITBIT32;
	normhipart = tf.tf_i[HIOFFSET];

	while (n--)
	{
		float cf, cfindx, r, oneminusr;
		cf = *in2++ * isr;
		if (cf < 0) cf = 0;

		cfindx = cf * (float)(COSTABSIZE / 6.28318f);
		r = (qinv > 0 ? 1 - cf * qinv : 0);

		if (r < 0) r = 0;

		oneminusr = (float)(1.0 - r);
		dphase = ((double)(cfindx)) + UNITBIT32;

		tf.tf_d = dphase;
		tabindex = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		addr = tab + tabindex;

		tf.tf_i[HIOFFSET] = normhipart;
		frac = tf.tf_d - UNITBIT32;
		f1 = addr[0];
		f2 = addr[1];
		coefr = (float)(r * (f1 + frac * (f2 - f1)));

		addr = tab + ((tabindex - (COSTABSIZE / 4)) & (COSTABSIZE - 1));
		f1 = addr[0];
		f2 = addr[1];
		coefi = (float)(r * (f1 + frac * (f2 - f1)));

		f1 = *in1++;
		re2 = re;

		re = (float)(ampcorrect * oneminusr * f1 + coefr * re2 - coefi * im);
		*out1++ = re;

		im = coefi * re2 + coefr * im;
		*out2++ = im;
	}
	if (MAX_BIGORSMALL(re))	re = 0;
	if (MAX_BIGORSMALL(im)) im = 0;
	c->c_re = re;
	c->c_im = im;

	return (w + 8);
}

t_int *sigvcf_perform2(t_int *w)
{

	t_sigvcf *x = (t_sigvcf *)(w[1]);
	t_vcfctl *c = x->x_ctl;
	t_float *in1 = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	//float *in3 = (t_float *)(w[4]);
	t_float	q = x->q_connected ? *(t_float *)(w[4]) : (float)c->c_q;

	t_float *out1 = (t_float *)(w[5]);
	t_float *out2 = (t_float *)(w[6]);

	int n = w[7];

	float re = (float)c->c_re, re2;
	float im = (float)c->c_im;
	//float q;
	//if (x->q_connected)  q = *in3;
	//else  q = (float)c->c_q;
	float centerf = (float)c->c_cf;
	float qinv = (float)(q > 0 ? 1.0 / q : 0);
	float ampcorrect = (float)(2.0 - 2.0 / (q + 2.0));
	float isr = (float)c->c_isr;
	float coefr, coefi;
	double *tab = x->cos_table, *addr, f1, f2, frac;
	float dphase;
	int normhipart, tabindex;
	union tabfudge_d tf;

	tf.tf_d = UNITBIT32;
	normhipart = tf.tf_i[HIOFFSET];

	while (n--)
	{
		float cf, cfindx, r, oneminusr;
		cf = centerf * isr;
		if (cf < 0) cf = 0;

		cfindx = cf * (float)(COSTABSIZE / 6.28318f);
		r = (qinv > 0 ? 1 - cf * qinv : 0);

		if (r < 0) r = 0;

		oneminusr = (float)(1.0 - r);
		dphase = ((double)(cfindx)) + UNITBIT32;

		tf.tf_d = dphase;
		tabindex = tf.tf_i[HIOFFSET] & (COSTABSIZE - 1);
		addr = tab + tabindex;

		tf.tf_i[HIOFFSET] = normhipart;
		frac = tf.tf_d - UNITBIT32;
		f1 = addr[0];
		f2 = addr[1];
		coefr = (float)(r * (f1 + frac * (f2 - f1)));

		addr = tab + ((tabindex - (COSTABSIZE / 4)) & (COSTABSIZE - 1));
		f1 = addr[0];
		f2 = addr[1];
		coefi = (float)(r * (f1 + frac * (f2 - f1)));

		f1 = *in1++;
		re2 = re;

		re = (float)(ampcorrect * oneminusr * f1 + coefr * re2 - coefi * im);
		*out1++ = re;

		im = coefi * re2 + coefr * im;
		*out2++ = im;
	}
	if (MAX_BIGORSMALL(re))	re = 0;
	if (MAX_BIGORSMALL(im)) im = 0;
	c->c_re = re;
	c->c_im = im;

	return (w + 8);
}

void sigvcf_dsp(t_sigvcf *x, t_signal **sp, short *count)
{
	x->freq_connected = count[1];//count contains info about signal connections
	x->q_connected = count[2];//count contains info about signal connections

	if (x->sr != sp[0]->s_sr) x->sr = sp[0]->s_sr;

	x->x_ctl->c_isr = 6.28318f / x->sr;

	if (x->freq_connected){
		dsp_add(sigvcf_perform1, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
	}
	else{
		dsp_add(sigvcf_perform2, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
	}
}