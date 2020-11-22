/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#ifndef __BIGSMALLLIB_H__
#define __BIGSMALLLIB_H__

#if defined(__i386__) || defined(__x86_64__) // Type punning code:

#if MAX_FLOAT_PRECISION == 32

typedef  union
{
	double f;
	unsigned int ui;
}t_bigorsmall32;

static int MAX_BADFLOAT(double f)    // test for NANs, infs and denormals
{
	t_bigorsmall32 pun;
	pun.f = f;
	pun.ui &= 0x7f800000;
	return((pun.ui == 0) | (pun.ui == 0x7f800000));
}

static int MAX_BIGORSMALL(double f)  // > abs(2^64) or < abs(2^-64)
{
	t_bigorsmall32 pun;
	pun.f = f;
	return((pun.ui & 0x20000000) == ((pun.ui >> 1) & 0x20000000));
}

#elif MAX_FLOAT_PRECISION == 64

typedef  union
{
	double f;
	unsigned int ui[2];
}t_bigorsmall64;

static int MAX_BADFLOAT(double f)    // test for NANs, infs and denormals
{
	t_bigorsmall64 pun;
	pun.f = f;
	pun.ui[1] &= 0x7ff00000;
	return((pun.ui[1] == 0) | (pun.ui[1] == 0x7ff00000));
}

static int MAX_BIGORSMALL(double f)  // > abs(2^512) or < abs(2^-512)
{
	t_bigorsmall64 pun;
	pun.f = f;
	return((pun.ui[1] & 0x20000000) == ((pun.ui[1] >> 1) & 0x20000000));
}

#endif // endif MAX_FLOAT_PRECISION
#else   // if not defined(__i386__) || defined(__x86_64__)
#define MAX_BADFLOAT(f) 0
#define MAX_BIGORSMALL(f) 0
#endif // end if defined(__i386__) || defined(__x86_64__)



#endif
