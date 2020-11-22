#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include <stdlib.h>
#include <string.h>

#define MAXOVERLAP 32
#define INITVSTAKEN 64
#define LOGTEN 2.302585092994

static t_class *env_tilde_class;

typedef struct sigenv
{
	t_pxobject x_obj;                 /* header */
	void *x_outlet;                 /* a "float" outlet */
	void *x_clock;                  /* a "clock" object */
	double *x_buf;                   /* a Hanning window */
	int x_phase;                    /* number of points since last output */
	int x_period;                   /* requested period of output */
	int x_realperiod;               /* period rounded up to vecsize multiple */
	int x_npoints;                  /* analysis window size in samples */
	double x_result;                 /* result to output */
	double x_sumbuf[MAXOVERLAP];     /* summing buffer */

	int x_allocforvs;               /* extra buffer for DSP vector size */
} t_sigenv;

double powtodb(double f);

void *resizebytes(void *old, size_t oldsize, size_t newsize);


void env_tilde_tick(t_sigenv *x);

void *env_tilde_new(t_symbol *s, short argc, t_atom *argv);

void env_tilde_perform(t_sigenv *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *env_t_perform(t_int *w);

void env_tilde_dsp(t_sigenv *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void env_t_dsp(t_sigenv *x, t_signal **sp);


void env_tilde_tick(t_sigenv *x);

void env_tilde_ff(t_sigenv *x);


int C74_EXPORT main(void)
{
	env_tilde_class = class_new("env~", (method)env_tilde_new,
		(method)env_tilde_ff, sizeof(t_sigenv), 0, A_GIMME, 0);

	class_addmethod(env_tilde_class, (method)env_tilde_dsp, "dsp64", A_CANT, 0);
	class_addmethod(env_tilde_class, (method)env_t_dsp, "dsp", A_CANT, 0);

	class_dspinit(env_tilde_class);
	class_register(CLASS_BOX, env_tilde_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's env~ object for Pure data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *env_tilde_new(t_symbol *s, short argc, t_atom *argv)
{
	t_atom_long npoints = 1024;
	t_atom_long period = npoints / 2;
	t_sigenv *x;
	double *buf;
	int i;

	x = (t_sigenv *)object_alloc(env_tilde_class);

	atom_arg_getlong(&npoints, 0, argc, argv);
	atom_arg_getlong(&period, 1, argc, argv);

	if (npoints < 1) npoints = 1024;
	if (period < 1) period = npoints / 2;
	if (period < npoints / MAXOVERLAP + 1) period = npoints / MAXOVERLAP + 1;
	if (!(buf = (double *)sysmem_newptr(sizeof(double) * (npoints + INITVSTAKEN))))
	{
		error("env: couldn't allocate buffer");
		return (0);
	}

	x->x_buf = buf;
	x->x_npoints = (int)npoints;
	x->x_phase = 0;
	x->x_period = (int)period;

	for (i = 0; i < MAXOVERLAP; i++) x->x_sumbuf[i] = 0;

	for (i = 0; i < npoints; i++){
		buf[i] = (1. - cos((2 * 3.141592654 * i) / npoints)) / npoints;
	}
	for (; i < (npoints + INITVSTAKEN); i++) buf[i] = 0;

	x->x_clock = clock_new((t_object *)x, (method)env_tilde_tick);

	dsp_setup((t_pxobject *)x, 1);

	x->x_outlet = outlet_new(x, "float");

	//x->x_obj.z_misc |= Z_NO_INPLACE;

	x->x_allocforvs = INITVSTAKEN;

	return (x);
}

void env_tilde_perform(t_sigenv *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in = ins[0];

	int n = vectorsize;
	int count;
	double *sump;
	in += n;

		for (count = x->x_phase, sump = x->x_sumbuf;
			count < x->x_npoints; count += x->x_realperiod, sump++)
		{
			double *hp = x->x_buf + count;
			double *fp = in;
			double sum = *sump;
			int i;

			for (i = 0; i < n; i++)
			{
				fp--;
				sum += *hp++ * (*fp * *fp);
			}
			*sump = sum;
		}

		sump[0] = 0;
		x->x_phase -= n;

		if (x->x_phase < 0)
		{
			x->x_result = x->x_sumbuf[0];
			for (count = x->x_realperiod, sump = x->x_sumbuf;
				count < x->x_npoints; count += x->x_realperiod, sump++)
				sump[0] = sump[1];
			sump[0] = 0;
			x->x_phase = x->x_realperiod - n;
			clock_delay(x->x_clock, 0L);
		}
}

t_int *env_t_perform(t_int *w)
{
	t_sigenv *x = (t_sigenv *)(w[1]);
	float *in = (t_float *)(w[2]);
	int n = (int)(w[3]);
	int count;
	double *sump;
	in += n;
	for (count = x->x_phase, sump = x->x_sumbuf;
		count < x->x_npoints; count += x->x_realperiod, sump++)
	{
		double *hp = x->x_buf + count;
		float *fp = in;
		float sum = *sump;
		int i;

		for (i = 0; i < n; i++)
		{
			fp--;
			sum += *hp++ * (*fp * *fp);
		}
		*sump = sum;
	}
	sump[0] = 0;
	x->x_phase -= n;
	if (x->x_phase < 0)
	{
		x->x_result = x->x_sumbuf[0];
		for (count = x->x_realperiod, sump = x->x_sumbuf;
			count < x->x_npoints; count += x->x_realperiod, sump++)
			sump[0] = sump[1];
		sump[0] = 0;
		x->x_phase = x->x_realperiod - n;
		clock_delay(x->x_clock, 0L);
	}
	return (w + 4);
}

void env_tilde_dsp(t_sigenv *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	if (x->x_period % (long)maxvectorsize) x->x_realperiod =
		x->x_period + (long)maxvectorsize - (x->x_period % (long)maxvectorsize);
	else x->x_realperiod = x->x_period;
	if (maxvectorsize > x->x_allocforvs)
	{
		void *xx = resizebytes(x->x_buf,
			(x->x_npoints + x->x_allocforvs) * sizeof(double),
			(x->x_npoints + maxvectorsize) * sizeof(double));
		if (!xx)
		{
			error("env~: out of memory");
			return;
		}
		x->x_buf = (double *)xx;
		x->x_allocforvs = (long)maxvectorsize;
	}
		object_method(dsp64, gensym("dsp_add64"), x,
			env_tilde_perform, 0, NULL);
}

void env_t_dsp(t_sigenv *x, t_signal **sp)
{
	if (x->x_period % sp[0]->s_n) x->x_realperiod =
		x->x_period + sp[0]->s_n - (x->x_period % sp[0]->s_n);
	else x->x_realperiod = x->x_period;
	if (sp[0]->s_n > x->x_allocforvs)
	{
		void *xx = resizebytes(x->x_buf,
			(x->x_npoints + x->x_allocforvs) * sizeof(t_sample),
			(x->x_npoints + sp[0]->s_n) * sizeof(t_sample));
		if (!xx)
		{
			error("env~: out of memory");
			return;
		}
		x->x_buf = (t_sample *)xx;
		x->x_allocforvs = sp[0]->s_n;
	}
	dsp_add(env_t_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void env_tilde_tick(t_sigenv *x) /* callback function for the clock */
{
	//clock_delay(x->x_clock, 1);
	outlet_float(x->x_outlet, powtodb(x->x_result));
}

double powtodb(double f)
{
	if (f <= 0) return (0);
	else
	{
		double val = 100 + 10. / LOGTEN * log(f);
		return (val < 0 ? 0 : val);
	}
}

void env_tilde_ff(t_sigenv *x)           /* cleanup on free */
{
	dsp_free((t_pxobject *)x);//removes object from DSP chain
	clock_free(x->x_clock);
	sysmem_freeptr(x->x_buf);
	sysmem_freeptr(x->x_outlet);
}

void *resizebytes(void *old, size_t oldsize, size_t newsize)
{
	void *ret;
	if (newsize < 1) newsize = 1;
	if (oldsize < 1) oldsize = 1;
	ret = (void *)sysmem_resizeptr((char *)old, newsize);
	if (newsize > oldsize && ret){
		memset(((char *)ret) + oldsize, 0, newsize - oldsize);
	}
#ifdef LOUD
	fprintf(stderr, "resize %lx %d --> %lx %d\n", (int)old, oldsize, (int)ret, newsize);
#endif /* LOUD */
#ifdef DEBUGMEM
	totalmem += (newsize - oldsize);
#endif
	if (!ret)
		post("max: resizebytes() failed -- out of memory");
	return (ret);
}