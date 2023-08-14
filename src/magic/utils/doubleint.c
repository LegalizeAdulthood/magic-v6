/* doubleint.c --
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
 * This file contains for double-precision integer operations.
 * WARNING: all double-precision integers are assumed to be non-negative!
 */

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "doubleint.h"
#ifdef SYSV
#include <string.h>
#endif

#ifndef lint
static char rcsid[] = "$Header: doubleint.c,v 6.0 90/08/28 19:00:27 mayo Exp $";
#endif lint

DoubleInt DIMaxInt = { CARRYMASK, CARRYMASK };
DoubleInt DIZero = { 0, 0 };


/*
 * -------------------------------------------------------------------
 *
 * DoubleAdd --
 *
 * Add two double-precision integers
 *
 * Results:
 *	Returns the sum.
 *
 * Side Effects:
 *	None.
 *
 *-------------------------------------------------------------------
 */

DoubleInt
DoubleAdd(n1, n2)
    DoubleInt n1, n2;
{
    register unsigned long r;
    DoubleInt result;

    r = n1.di_low + n2.di_low;
    result.di_low = (r & CARRYMASK);
    result.di_high = n1.di_high + n2.di_high + ((r & CARRYBIT) ? 1 : 0);
    return (result);
}


/*
 * -------------------------------------------------------------------
 *
 * DoubleMultI --
 *
 * Multiply a single-precision non-negative integer by a double-precision
 * one.
 *
 * Results:
 *	Returns the double-precision product.
 *
 * Side Effects:
 *	None.
 *
 *-------------------------------------------------------------------
 */

DoubleInt
DoubleMultI(d, s)
    DoubleInt d;
    unsigned long s;
{
    register unsigned short *dPtr, *sPtr;
    register unsigned long temp;
    DoubleInt result;

    dPtr = (unsigned short *) &d;	/* d3 d2 d1 d0 */
    sPtr = (unsigned short *) &s;	/*       s1 s0 */

    /* Terms d3*s0 d2*s0 d3*s1 d2*s1 */
    result.di_high =
	d.di_high * (sPtr[SHORT_1] << 16) + d.di_high * sPtr[SHORT_0];

    /* Term  d0*s0 */
    result.di_low = dPtr[SHORT_0] * sPtr[SHORT_0];
    if (result.di_low & CARRYBIT)
    {
	result.di_high++;
	result.di_low &= CARRYMASK;
    }

    /*
     * Terms d1*s0 d0*s1.
     * Since dPtr[SHORT_1] and sPtr[SHORT_1] are guaranteed to have
     * their high-order bit zero, both are effectively 15-bit numbers,
     * so the product of each with a 16-bit number is only 31 bits.
     * The sum of two 31-bit numbers can be a 32-bit number, but never
     * a 33-bit number.
     */

    /*	      15 bits	      16 bits	      16 bits	      15 bits	*/
    temp = dPtr[SHORT_1] * sPtr[SHORT_0] + dPtr[SHORT_0] * sPtr[SHORT_1];

    /*
     * Add the high-order 17 bits of temp into result.di_high,
     * ignoring carries.  Add the low-order 15 bits of temp
     * into result.di_low, propagating the carry if one exists.
     */
    result.di_high += (temp >> 15) & 0x1ffff;
    result.di_low += (temp & 0x7fff) << 16;
    if (result.di_low & CARRYBIT)
    {
	result.di_high++;
	result.di_low &= CARRYMASK;
    }

    /*
     * Term d1*s1.
     * Form the product, then shift left by 1 to reflect
     * the fact that result.di_high has 31 bits to its
     * right, not 32.
     */
    result.di_high += (dPtr[SHORT_1] * sPtr[SHORT_1]) << 1;

    return (result);
}


/*
 * -------------------------------------------------------------------
 *
 * DoubleMultII --
 *
 * Multiply two single-precision non-negative integers obtaining a
 * double precision result.
 *
 * Results:
 *	Returns the double-precision product.
 *
 * Side Effects:
 *	None.
 *
 *-------------------------------------------------------------------
 */

DoubleInt
DoubleMultII(s, t)
    unsigned long s;
    unsigned long t;
{
    register unsigned short *sPtr, *tPtr;
    register unsigned long temp;
    DoubleInt result;

    sPtr = (unsigned short *) &s;	/* s1 s0 */
    tPtr = (unsigned short *) &t;	/* t1 t0 */

    /*
     * Term s1*t1.
     * Form the product, then shift left by 1 to reflect
     * the fact that result.di_high has 31 bits to its
     * right, not 32.
     */
    result.di_high = (sPtr[SHORT_1] * tPtr[SHORT_1]) << 1;

    /* term s0*t0 */
    result.di_low = sPtr[SHORT_0] * tPtr[SHORT_0];
    if (result.di_low & CARRYBIT)
    {
	result.di_high++;
	result.di_low &= CARRYMASK;
    }

    /*
     * Terms s0*t1, s1*t0
     * Since sPtr[SHORT_1] and tPtr[SHORT_1] are guaranteed to have
     * their high-order bit zero, both are effectively 15-bit numbers,
     * so the product of each with a 16-bit number is only 31 bits.
     * The sum of two 31-bit numbers can be a 32-bit number, but never
     * a 33-bit number.
     */

    /*	      15 bits	      16 bits	      16 bits	      15 bits	*/
    temp = sPtr[SHORT_1] * tPtr[SHORT_0] + sPtr[SHORT_0] * tPtr[SHORT_1];

    /*
     * Add the high-order 17 bits of temp into result.di_high,
     * ignoring carries.  Add the low-order 15 bits of temp
     * into result.di_low, propagating the carry if one exists.
     */
    result.di_high += (temp >> 15) & 0x1ffff;
    result.di_low += (temp & 0x7fff) << 16;
    if (result.di_low & CARRYBIT)
    {
	result.di_high++;
	result.di_low &= CARRYMASK;
    }

    return (result);
}
    

/*
 * -------------------------------------------------------------------
 *
 * DoubleString --
 *
 * Convert a double-precision integer into a string.
 * Here we cheat and use floating-point, since it's not
 * speed critical.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Stores the result in the string 'str'.
 *
 *-------------------------------------------------------------------
 */

Void
DoubleString(d, str)
    DoubleInt d;
    register char *str;
{
    double dfloat, dtemp;

    dfloat = d.di_low;
    dtemp = CARRYFLOAT;
    dfloat += ((double) d.di_high) * dtemp;
    (void) sprintf(str, "%.0f", dfloat);
    if (str = index(str, '.'))
	*str = '\0';
}
    

/*
 * -------------------------------------------------------------------
 *
 * DoubleToDFloat --
 *
 * Convert a double-precision integer into an extended precision floating
 * point ("double").
 *
 * Results:
 *	Floating point equivalent of doubleint.
 *
 * Side Effects:
 *	None.
 *
 *-------------------------------------------------------------------
 */

double
DoubleToDFloat(d)
    DoubleInt d;
{
    double dfloat, dtemp;

    dfloat = d.di_low;
    dtemp = CARRYFLOAT;
    dfloat += ((double) d.di_high) * dtemp;
    return dfloat;
}

/*
 * -------------------------------------------------------------------
 *
 * DoubleInit --
 *
 *      Initialize this package.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Just does some assertion checking.
 *
 *-------------------------------------------------------------------
 */

void
DoubleInit()
{
    unsigned short *sArray;
    unsigned long sInt;

    /* Test to make sure the SHORT_X definitions match the byte-order on
     * this machine.
     */
    sInt = 1 + 65536*2;
    sArray = (unsigned short *) &sInt;
    if ( (sArray[SHORT_0] != 1) || (sArray[SHORT_1] != 2) )
    {
	TxError("The SHORT_X definitions in utils/doubleint.h are wrong!\n");
	TxError("   sArray = { %d, %d} != { 1, 2}\n", 
	    sArray[SHORT_0], sArray[SHORT_1]);
	TxError("Report this to whomever compiled Magic for your machine.\n");
	ASSERT(FALSE, "DoubleInit");
    }
    ASSERT(sizeof(short) == 2, "DoubleInit");
    ASSERT(sizeof(long) >= 4, "DoubleInit");
}
