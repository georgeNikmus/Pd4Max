#include "ext.h"			
#include "ext_obex.h"

static t_class *moses_class;

typedef struct _moses
{
	t_object x_ob;
	void *x_out0;
	void *x_out1;
	double x_y;
	double x_x;
} t_moses;

void *moses_new(t_symbol *s, short argc, t_atom *argv);

void moses_float(t_moses *x,double n);
void moses_int(t_moses *x, long n);
void moses_float2(t_moses *x, double n);
void moses_int2(t_moses *x, long n);

void moses_assist(t_moses *x, void *b, long m, long a, char *s);

int C74_EXPORT main(void)
{
	moses_class = class_new("moses", (method)moses_new, 0,
		sizeof(t_moses), 0, A_GIMME, 0);

	class_addmethod(moses_class, (method)moses_float, "float", A_CANT, 0);
	class_addmethod(moses_class, (method)moses_int, "int", A_CANT, 0);
	class_addmethod(moses_class, (method)moses_float2, "ft1", A_CANT, 0);
	class_addmethod(moses_class, (method)moses_int2, "int1", A_CANT, 0);

	class_addmethod(moses_class, (method)moses_assist, "assist", A_CANT, 0);
	
	class_register(CLASS_BOX, moses_class);

	post("--------------------------------------------------------------------------");
	post("This is Miller Puckette's moses object for Pure data.");
	post("Ported to Max/MSP: George Nikolopoulos - July 2015 -");

	return 0;
}

void *moses_new(t_symbol *s, short argc, t_atom *argv)
{
	t_moses *x = (t_moses *)object_alloc(moses_class);

	floatin(x, 1);
	x->x_out1 = outlet_new(x, NULL);
	x->x_out0 = outlet_new(x, NULL);
	
	atom_arg_getdouble(&x->x_y, 0, argc, argv); //first argument(0)

	return (x);
}

void moses_float(t_moses *x,double n)
{
	x->x_x = n;
	if (n < x->x_y) outlet_float(x->x_out0, n);
	else outlet_float(x->x_out1, n);
}

void moses_int(t_moses *x, long n)
{
	moses_float(x, n);
}

void moses_float2(t_moses *x, double n)
{
	x->x_y = n;
	if (x->x_x < n) outlet_float(x->x_out0, x->x_x);
	else outlet_float(x->x_out1, x->x_x);
}

void moses_int2(t_moses *x, long n)
{
	moses_float2(x, n);
}

void moses_assist(t_moses *x, void *b, long m, long a, char *s) // 4 final arguments are always the same for the assistance method
{
	if (m == ASSIST_OUTLET){
		switch (a){
		case 0:
			sprintf(s, "Incoming values (to the left inlet) will pass at the left outlet if they are less than the control value.");
			break;
		case 1:
			sprintf(s, "Incoming values (to the left inlet) will pass at the right outlet if they are greater than or equal to the control value.");
			break;
		}
	}
	else {
		switch (a) {
		case 0:
			sprintf(s, "If the float the left inlet is less than the control value, it passes at the left inlet. If it is greater than or equal to the control value it passes at the right inlet.");
			break;
		case 1:
			sprintf(s, "A float to the right inlet sets the control value");
			break;
		}
	}
}