/* Aed2.c -
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
 * This file contains additional functions to manipulate an Aed512
 * color display.  Included here are rectangle drawing and color map
 * loading.
 */


#ifndef lint
static char rcsid[]="$Header: grAed2.c,v 6.0 90/08/28 18:40:26 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grAedInt.h"

/* Library imports: */

extern int sscanf();


/* imports from other AED modules */
extern void aedSetWMandC(), aedSetLineStyle();
extern void aedSetStipple();



/*---------------------------------------------------------
 * AedSetCMap:
 *	AedSetCMap outputs new values to the Aed512 color map.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The values in the color map are set from the array indicated
 *	by pmap.
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

Void
AedSetCMap(pmap)
    char *pmap;			/* A pointer to 256*3 bytes containing the
				 * new values for the color map.  The first
				 * three values are red, green, and blue
				 * intensities for color 0, and so on.
				 */
{
    char *p;
    int i;

    /* The stuff must be put out in two batches because we can't indicate
     * the value 256 in two hex bytes.
     */
    p = pmap;
    putc('K', grAedOutput);
    putc(0, grAedOutput);
    putc(0200, grAedOutput);
    for (i = 256*3; i>0; i--)
    {
	if (i == 128*3) fputs("K\200\200", grAedOutput);
	putc(*p++&0377, grAedOutput);
    }
    (void) fflush(grAedOutput);
}


/*---------------------------------------------------------
 * aedDrawLine:
 *	This routine draws a line.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A line is drawn from (x1, y1) to (x2, y2), using the current
 *	color, style, and write mask.  This is an internal routine
 *	used by AedClipBox.
 *---------------------------------------------------------
 */

Void
aedDrawLine(x1, y1, x2, y2)
    int x1, y1;			/* Screen coordinates of first point. */
    int x2, y2;			/* Screen coordinates of second point. */
{
    aedSetPos(x1, y1);
    putc('A', grAedOutput);
    aedOutxy20(x2, y2);
    aedCurx = x2;
    aedCury = y2;
}



/*---------------------------------------------------------
 * aedFillRect:
 *	This routine draws a solid rectangle.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A solid rectangle is drawn in the current color and
 *	using the current write mask.  This is an internal
 *	routine used by grFastBox..
 *---------------------------------------------------------
 */

Void
aedFillRect(prect)
    register Rect *prect;	/* Address of a rectangle in screen
				 * coordinates.
				 */
{
    aedSetPos(prect->r_xbot, prect->r_ybot);
    putc('o', grAedOutput);
    aedCurx = prect->r_xtop;
    aedCury = prect->r_ytop;
    aedOutxy20(prect->r_xtop, prect->r_ytop);
}

