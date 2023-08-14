/* doubleint.h --
 *
 * rcsid = "$Header: doubleint.h,v 6.0 90/08/28 19:00:29 mayo Exp $"
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 *		      Lawrence Livermore National Laboratory
 *		      All rights reserved.
 *
 * This file contains definitions for double-precision integer operations.
 */

#define _DOUBLEINT

#ifndef _MAGIC
    err0 = Need_To_Include_Magic_Header;
#endif  _MAGIC


typedef struct
{
    unsigned long	di_low;
    unsigned long	di_high;
} DoubleInt;

#define	CARRYBIT	0x80000000
#define	CARRYMASK	0x7fffffff
#define	CARRYFLOAT	(((double) (0x40000000)) * 2.0)
/* WORDSIZE = #of bits in unsigned long */
#define WORDSIZE	(8 * sizeof(long))

extern DoubleInt DIMaxInt;
extern DoubleInt DIZero;

/* Machine-specific word-order dependencies */

#ifdef	LITTLE_ENDIAN
#	define	SHORT_0		0
#	define	SHORT_1		1
#	define	SHORT_2		2
#	define	SHORT_3		3
#endif

#ifdef	BIG_ENDIAN
#	define	SHORT_0		1
#	define	SHORT_1		0
#	define	SHORT_2		3
#	define	SHORT_3		2
#endif

/* Error checking to maintain consistency of this file. */
#ifndef SHORT_0
    Error_1 = You_need_to_define_the_ENDIAN_constants_in_magic.h;
#endif

extern void DoubleInit();	/* Initialize this package. */
DoubleInt DoubleMultI();	/* multiply double by unsigned long */
DoubleInt DoubleMultII();	/* multiply unsigned long by unsigned long */
DoubleInt DoubleAdd();		/* add two doubles */
Void DoubleString();		/* convert double int to string */
double DoubleToDFloat();	/* convert double int  to double float */

/*
 * DOUBLE_CREATE --
 *
 * Set 'result' to be the double-precision integer corresponding
 * to 'n'
 */
#define	DOUBLE_CREATE(result, n) \
    if (1) { \
	(result).di_low = (n) & CARRYMASK; \
	(result).di_high = ((n) & CARRYBIT) ? 1 : 0; \
    } else

/*
 * DOUBLE_ADD --
 *
 * Set 'result' to be the sum of the two DoubleInts n1 and n2.
 */
#define	DOUBLE_ADD(result, n1, n2) \
    if (1) { \
	register unsigned long __r = (n1).di_low + (n2).di_low; \
	(result).di_low = (__r & CARRYMASK); \
	(result).di_high = (n1).di_high + (n2).di_high + ((__r&CARRYBIT) ?1 :0); \
    } else

/*
 * DOUBLE_ADDI --
 *
 * Set 'result' to be the sum of the DoubleInts n1 and the unsigned long i.
 */
#define	DOUBLE_ADDI(result, n1, i) \
    if (1) { \
	register unsigned long __r = (n1).di_low + (i); \
	(result).di_low = (__r & CARRYMASK); \
	(result).di_high = (n1).di_high + ((__r&CARRYBIT) ?1 :0); \
    } else
  
/*
 * DOUBLE_SUB --
 *
 * Set 'result' to be the difference of the two DoubleInts n1 and n2.
 * (NOTE: it is assumed that n2<=n1, since we are only doing positive 
 * arithmetic).
 */
#define	DOUBLE_SUB(result, n1, n2) \
    if (1) { \
	register unsigned long __r = ((n1).di_low | CARRYBIT) - (n2).di_low; \
	(result).di_low = (__r & CARRYMASK); \
	(result).di_high = (n1).di_high - (n2).di_high - ((__r&CARRYBIT) ?0 :1); \
    } else

/*
 * DOUBLE_SUBI --
 *
 * Set 'result' to be the difference of the DoubleInt n1 and the 
 * unsigned long i.
 * (NOTE: it is assumed that i<=n1, since we are only doing positive 
 * arithmetic).
 */
#define	DOUBLE_SUBI(result, n1, i) \
    if (1) { \
	register unsigned long __r = ((n1).di_low | CARRYBIT) - (i); \
	(result).di_low = (__r & CARRYMASK); \
	(result).di_high = (n1).di_high - ((__r&CARRYBIT) ?0 :1); \
    } else

/* comparisons */

/*
 * DOUBLE_GREATER --
 */
#define	DOUBLE_GREATER(n1, n2)	\
	((n1).di_high > (n2).di_high \
		    ? 1 : ((n1).di_high < (n2).di_high \
			    ? 0 : (n1).di_low > (n2).di_low))

/* 
 * DOUBLE_GE --
 */
#define	DOUBLE_GE(n1, n2)	\
	((n1).di_high > (n2).di_high \
		    ? 1 : ((n1).di_high < (n2).di_high \
			    ? 0 : (n1).di_low >= (n2).di_low))

/* 
 * DOUBLE_LESS --
 */
#define	DOUBLE_LESS(n1, n2)	\
	((n1).di_high < (n2).di_high \
		    ? 1 : ((n1).di_high > (n2).di_high \
			    ? 0 : (n1).di_low < (n2).di_low))

/* 
 * DOUBLE_LE --
 */
#define	DOUBLE_LE(n1, n2)	\
	((n1).di_high < (n2).di_high \
		    ? 1 : ((n1).di_high > (n2).di_high \
			    ? 0 : (n1).di_low <= (n2).di_low))
/*
 * DOUBLE_EQUAL --
 */
#define	DOUBLE_EQUAL(n1, n2)	\
	((n1).di_high == (n2).di_high && (n1).di_low == (n2).di_low)

/*
 * DOUBLE_SHIFTRIGHT --
 *
 * Set 'result' to be doubleint n1/(2**e).
 * (NOTE:  e must be: 0<=e<=WORDSIZE-1)
 */
#define	DOUBLE_SHIFTRIGHT(result, n1, e) \
    if (1) { \
	(result).di_low = \
		((n1).di_low>>(e) | (n1).di_high<<(WORDSIZE-(e)-1)) & \
		~CARRYBIT; \
	(result).di_high = (n1).di_high>>(e); \
    } else

/*
 * DOUBLE_SHIFTLEFT --
 *
 * Set 'result' to be doubleint n1*(2**e).
 * (NOTE:  e must be: 0<=e<=WORDSIZE-1)
 */
#define	DOUBLE_SHIFTLEFT(result, n1, e) \
    if (1) { \
	(result).di_high = \
		((n1).di_high<<(e) | (n1).di_low>>(WORDSIZE-(e)-1)); \
	(result).di_low = (n1).di_low<<(e) & ~CARRYBIT; \
    } else
