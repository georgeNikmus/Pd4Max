#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

static t_class *sah_class;

typedef struct sah
{
	t_pxobject x_obj;
	
	double x_lastin;
	double x_lastout;
	double x_floatin1;
	double x_floatin2;

	short firstIn_connected;
	short secondIn_connected;
}t_sah;

void *sah_new(t_symbol *s, short argc, t_atom *argv);

void sah_perform(t_sah *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam);
t_int *sigsah_perform(t_int *w);

void sah_dsp64(t_sah *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags);
void sigsah_dsp(t_sah *x, t_signal **sp, short *count);


void sah_free(t_sah *x);
void sah_set(t_sah *x, double f);
void sah_reset(t_sah *x, t_symbol *s, int argc, t_atom *argv);

void samphold_float(t_sah *x, double f);

void sah_assist(t_sah *x, void *b, long msg, long arg, char *dst);

int C74_EXPORT main(void)
{
	sah_class = class_new("samphold~", (method)sah_new, (method)sah_free,
		sizeof(t_sah),0, A_GIMME, 0);
	class_addmethod(sah_class, (method)sah_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(sah_class, (method)sigsah_dsp, "dsp", A_CANT, 0);

	class_addmethod(sah_class, (method)sah_set, "set",A_DEFFLOAT, 0);
	class_addmethod(sah_class, (method)sah_reset, "reset", A_GIMME, 0);

	class_addmethod(sah_class, (method)samphold_float, "float", A_FLOAT, 0);

	class_addmethod(sah_class, (method)sah_assist, "assist", A_CANT, 0);

	class_dspinit(sah_class);
	class_register(CLASS_BOX, sah_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's samphold~ object for Pure Data.");
	post("Translated into Max/MSP and updated for 64-bit perform routine(Max6): George Nikolopoulos - July 2015 -");
	return 0;
}

void *sah_new(t_symbol *s, short argc, t_atom *argv)
{
	t_sah *x = (t_sah *)object_alloc(sah_class);

	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");

	//x->x_obj.z_misc |= Z_NO_INPLACE;//inlets outlets DO NOT share the same memory
	//x->sr = sys_getsr();//determines the sample rate

	x->x_lastin = 0;
	x->x_lastout = 0;

	return (x);
}


void sah_perform(t_sah *x, t_object *dsp64,
	double **ins, long numins, double **outs,
	long numouts, long vectorsize, long flags,
	void *userparam)
{
	t_double *in1 = ins[0];
	t_double *in2 = ins[1];

	t_double *out = outs[0];

	int n = vectorsize;
	int i;

	double lastin = x->x_lastin;
	double lastout = x->x_lastout;
	double float_in1 = x->x_floatin1;
	double float_in2 = x->x_floatin2;
	double next;

	for (i = 0; i < n; i++,*in1++){
		if (x->secondIn_connected) next = *in2++;
		else next = float_in2++;

		if (next < lastin){
			if (x->firstIn_connected) lastout = *in1;
			else lastout = float_in1;
		}
		*out++ = lastout;
		lastin = next;
	}
	x->x_lastin = lastin;
	x->x_lastout = lastout;
}

void sah_dsp64(t_sah *x, t_object *dsp64, short *count,
	double samplerate, long maxvectorsize, long flags)
{
	x->firstIn_connected = count[0];//count contains info about signal connections
	x->secondIn_connected = count[1];

	object_method(dsp64, gensym("dsp_add64"), x,
		sah_perform, 0, NULL);
}

void sah_reset(t_sah *x, t_symbol *s, int argc, t_atom *argv)
{
	x->x_lastin = ((argc > 0 && (argv[0].a_type == A_FLOAT)) ?
		argv[0].a_w.w_float : 1e20);
}

void sah_set(t_sah *x, double f)
{
	x->x_lastout = f;
}

void sah_free(t_sah *x)
{
	dsp_free((t_pxobject *)x);
}

void samphold_float(t_sah *x, double f){
	int inlet = ((t_pxobject *)x)->z_in;
	switch (inlet){
	case 0:
		x->x_floatin1 = f;
		break;
	case 1:
		x->x_floatin2 = f;
		break;
	}
}

void sah_assist(t_sah *x, void *b, long msg, long arg, char *dst){
	if (msg == ASSIST_INLET){
		switch (arg){
		case 0: sprintf(dst, "(signal) Input to be sampled\n"
			"(set) sets the output value\n"
			"(reset) specifies teh most value"); break;
		case 1: sprintf(dst, "(signal) Control signal"); break;
		}
	}
	else if (msg == ASSIST_OUTLET){
		sprintf(dst, "(signal) Outgoing signal");
	}
}

t_int *sigsah_perform(t_int *w)
{
	t_sah *x = (t_sah *)(w[1]);
	float *in1 = (t_float *)(w[2]);
	float *in2 = (t_float *)(w[3]);
	float *out = (t_float *)(w[4]);

	int n = w[5];

	int i;

	double lastin = x->x_lastin;
	double lastout = x->x_lastout;
	double float_in1 = x->x_floatin1;
	double float_in2 = x->x_floatin2;
	double next;

	for (i = 0; i < n; i++, *in1++){
		if (x->secondIn_connected) next = *in2++;
		else next = float_in2++;

		if (next < lastin){
			if (x->firstIn_connected) lastout = *in1;
			else lastout = float_in1;
		}
		*out++ = (float)lastout;
		lastin = next;
	}
	x->x_lastin = lastin;
	x->x_lastout = lastout;

	return (w + 6);
}

void sigsah_dsp(t_sah *x, t_signal **sp, short *count)
{
	x->firstIn_connected = count[0];
	x->secondIn_connected = count[1];
	
	dsp_add(sigsah_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}