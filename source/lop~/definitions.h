#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include <math.h>

#ifndef MAXFLOAT
#define MAXFLOAT  1e18;
#endif
#define LOGTEN 2.302585092994

#ifndef MAX_FLOAT_PRECISION
#define MAX_FLOAT_PRECISION 32
#endif

#if MAX_FLOAT_PRECISION == 64
#define sinf sin
#define cosf cos
#define atanf atan
#define atan2f atan2
#define sqrtf sqrt
#define logf log
#define expf exp
#define fabsf fabs
#define powf pow
#endif
