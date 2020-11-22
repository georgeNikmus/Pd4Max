/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#ifndef __IEMLIB_H__
#define __IEMLIB_H__

#define LOGCOSTABSIZE 9
#define COSTABSIZE (1<<LOGCOSTABSIZE)

#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)
#define IS_A_DOLLAR(atom,index) ((atom+index)->a_type == A_DOLLAR)
#define IS_A_DOLLSYM(atom,index) ((atom+index)->a_type == A_DOLLSYM)
#define IS_A_SEMI(atom,index) ((atom+index)->a_type == A_SEMI)
#define IS_A_COMMA(atom,index) ((atom+index)->a_type == A_COMMA)

/* now miller's code starts : 
     for 4 point interpolation
     for lookup tables
     for denormal floats
 */


int sys_noloadbang;
//t_symbol *iemgui_key_sym=0;
#include <io.h>


#define DEFDELVS 64
#define XTRASAMPS 4
#define SAMPBLK 4

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

    /* machine-dependent definitions.  These ifdefs really
    should have been by CPU type and not by operating system! */

    /* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long


#if defined(__unix__) || defined(__APPLE__)
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif                                                                          

#if BYTE_ORDER == LITTLE_ENDIAN                                             
#define HIOFFSET 1                                                              
#define LOWOFFSET 0                                                             
#else                                                                           
#define HIOFFSET 0    /* word offset to find MSB */                             
#define LOWOFFSET 1    /* word offset to find LSB */                            
#endif /* __BYTE_ORDER */                                                       
#include <sys/types.h>
#define int32 int32_t
#endif /* __unix__ or __APPLE__*/

union tabfudge_d
{
  double tf_d;
  int32 tf_i[2];
};

union tabfudge_f
{
  float tf_f;
  long  tf_l;
};

//#if defined __i386__ || defined __x86_64__
//#define IEM_DENORMAL(f) ((((*(unsigned int*)&(f))&0x60000000)==0) || \
//(((*(unsigned int*)&(f))&0x60000000)==0x60000000))
///* more stringent test: anything not between 1e-19 and 1e19 in absolute val */
//#else
//
//#define IEM_DENORMAL(f) 0

//#endif

#endif
