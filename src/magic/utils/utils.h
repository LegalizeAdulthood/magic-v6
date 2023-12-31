/* utils.h --
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
 *
 * This file just defines all the features available from the
 * Magic utility routines.
 */

/* rcsid "$Header: utils.h,v 6.0 90/08/28 19:01:30 mayo Exp $" */

#define _UTILS 1

/*
 * Cast second argument to LookupStruct() to (LookupTable *) to
 * make lint very happy.
 */
typedef struct
{
    char *d_str;
} LookupTable;

/* The following stuff just defines the global routines provided
 * by files other than hash and stack and geometry.
 */

extern int Lookup();
extern int LookupAny();
extern int LookupFull();
extern int PaConvertTilde();
extern FILE *PaOpen();
extern char *StrDup();
extern int Match();
extern char *ArgStr();

/* The following macro takes an integer and returns another integer that
 * is the same as the first except that all the '1' bits are turned off,
 * except for the rightmost '1' bit.
 *
 * Examples:	01010100 --> 00000100
 *		1111 --> 0001
 *		0111011100 --> 0000000100
 */
#define	LAST_BIT_OF(x)	((x) & ~((x) - 1))
