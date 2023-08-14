/*
 * grAedInt.h --
 *
 * 	INTERNAL definitions for the Aed graphics routines.
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
 *
 */
/* rcsid "$Header: grAedInt.h,v 6.0 90/08/28 18:40:36 mayo Exp $" */

#define _GRAEDINT

#define aedOutxy20(x,y)	{putc((((x)>>4)&060)+(((y)>>8)&03), grAedOutput);\
	putc((x)&0377, grAedOutput); putc((y)&0377, grAedOutput);}

#define aedEncode16(x)	{putc((((x)>>8)&0377), grAedOutput);\
	putc((x)&0377, grAedOutput);}

#define	aedSetPos(x,y)	{aedCurx = x; aedCury = y; \
	putc('Q', grAedOutput); aedOutxy20(x, y);}

extern int aedCurx, aedCury, aedColor;
extern bool grAedMouseIsDisabled;
extern bool grAedTabletOn;
extern int aedCursorRow;	/* First line of off-screen memory */
extern bool aedHasProgCursor;   /* TRUE if there is a programmable cursor */
#define BUT421	0		/* Button order for tablets. */
#define BUT248  1
#define BUT124  2
#define BUT842	3		/* Useful for GTCO tablets. */
extern int aedButtonOrder;	/* Tells what style of button encoding, 
				 * see above and grAed4.c.
				 */

extern FILE *grAedInput, *grAedOutput;

#define BLINKING	TRUE	/* Should we use the built-in blinking AED
				 * cursor for the default cursor?
				 */

