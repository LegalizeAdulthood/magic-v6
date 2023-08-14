/* Sun2.c -
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
 * This file contains additional functions to manipulate an Sun
 * color display.  Included here are rectangle drawing and color map
 * loading.
 */


#ifndef lint
static char rcsid[]="$Header: grSun2.c,v 6.0 90/08/28 18:40:56 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunInt.h"

/* Library imports: */
extern int sscanf();

/* Imports from other graphics modules: */


/* imports from other SUN modules */
extern int sunColor;
extern void sunSetWMandC(), sunSetLineStyle();
extern void sunSetStipple();



/*---------------------------------------------------------
 * SunSetCMap:
 *	SunSetCMap outputs new values to the Sun color map.
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
SunSetCMap(pmap)
    char *pmap;			/* A pointer to 256*3 bytes containing the
				 * new values for the color map.  The first
				 * three values are red, green, and blue
				 * intensities for color 0, and so on.
				 */
{
    unsigned char red[256], green[256], blue[256];
    char *p;
    int i;

    p = pmap;
    for (i = 0; i < 256; i++)
    {
	red[i] = *p++;
	green[i] = *p++;
	blue[i] = *p++;
    }
    /* a terrible hack -- the sun always writes the cursor in color 255! */
/*  red[255] = 0; green[255] = 0; blue[255] = 0;	*/
    pr_putcolormap(grSunCpr, 0, 256, red, green, blue);
}


/*---------------------------------------------------------
 * sunDrawLine:
 *	This routine draws a line.  Dotted lines are drawn as solid.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A line is drawn from (x1, y1) to (x2, y2), using the current
 *	color, style, and write mask.  This is an internal routine
 *	used by GrFastBox.
 *---------------------------------------------------------
 */

Void
sunDrawLine(x1, y1, x2, y2)
    int x1, y1;			/* Screen coordinates of first point. */
    int x2, y2;			/* Screen coordinates of second point. */
{
    int y1a, y2a, op;
    y1a = GrScreenRect.r_ytop - y1;
    y2a = GrScreenRect.r_ytop - y2;
    op = PIX_SRC | PIX_DONTCLIP;

    pr_vector(grSunCpr, x1, y1a, x2, y2a, op, sunColor);

    /**** OLD DEVICE DRIVER ROUTINES
    if (sunWMask == 255)
    {
	*GR_freg = GR_Mask;
	*GR_creg = sunColor;
	COPvector(x1, y1a, x2, y2a);
    }
    else
    {
	int zeros, ones;

	ones = sunWMask & sunColor;
	zeros = sunWMask & ~sunColor;
	if (ones != 0)
	{
	    *GR_creg = ones;
	    *GR_freg = GR_Set_Mask_1;
	    COPvector(x1, y1a, x2, y2a);
	}
	if (zeros != 0)
	{
	    *GR_creg = zeros;
	    *GR_freg = GR_Set_Mask_0;
	    COPvector(x1, y1a, x2, y2a);
	}
    }    
    ****/
}



/*---------------------------------------------------------
 * sunFillFast:
 *	This routine draws a solid rectangle, but using any
 *	available hacks for speed.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A solid rectangle is drawn in the current color and
 *	using the current write mask.  This is an internal
 *	routine used by GrFastBox.
 *---------------------------------------------------------
 */

Void
sunFillFast(prect)
    Rect *prect;		/* Address of a rectangle in screen
				 * coordinates.
				 */
{
    int x, y, height, width, op;

    x = prect->r_xbot;
    y = GrScreenRect.r_ytop - prect->r_ytop;
    width = prect->r_xtop - prect->r_xbot + 1;
    height = prect->r_ytop - prect->r_ybot + 1;

    TxError("FillFast not implemented\n");


    /***** OLD DEVICE DRIVER LEVEL
    if (sunStipple == 0) 
    {
	if (sunWMask == 255)
	{
	    *GR_freg = GR_Mask;
	    *GR_creg = sunColor;
	    COPds(x, y, width, height);
	}
	else
	{
	    int zeros, ones;

	    ones = sunWMask & sunColor;
	    zeros = sunWMask & ~sunColor;
	    if (ones != 0)
	    {
		*GR_creg = ones;
		*GR_freg = GR_Set_Mask_1;
		COPds(x, y, width, height);
	    }
	    if (zeros != 0)
	    {
		*GR_creg = zeros;
		*GR_freg = GR_Set_Mask_0;
		COPds(x, y, width, height);
	    }
	}    
    }
    else 
    {
    }
    *****/
}



Point grSunStippleOrigin = {0, 0};

/*---------------------------------------------------------
 * sunFillRect:
 *	This routine draws a solid rectangle.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A solid rectangle is drawn in the current color and
 *	using the current write mask.  This is an internal
 *	routine used by GrFastBox.
 *---------------------------------------------------------
 */

Void
sunFillRect(prect)
    Rect *prect;		/* Address of a rectangle in screen
				 * coordinates.
				 */
{
    int miny, maxy, height, width, op;

    miny = GrScreenRect.r_ytop - prect->r_ytop;
    width = prect->r_xtop - prect->r_xbot + 1;
    height = prect->r_ytop - prect->r_ybot + 1;

    if (sunStipple == NULL) 
    {
	pr_rop(grSunCpr, prect->r_xbot, miny, width, height, 
	    PIX_SRC | PIX_DONTCLIP | PIX_COLOR(sunColor),
	    NULL, 0, 0);
    }
    else
    {
	int stipx, stipy, screenx, screeny, height;

	maxy = GrScreenRect.r_ytop - prect->r_ybot;

	screeny = stipy = miny;
	while (stipy - grSunStippleOrigin.p_y < 0) stipy += 65536;
	/* Stipple pattern repeats with period 8 */
	stipy = (stipy - grSunStippleOrigin.p_y) & 07;
	while (screeny <= maxy)
	{
	    screenx = stipx = prect->r_xbot;
	    while (stipx - grSunStippleOrigin.p_x < 0) stipx += 65536;
	    /* Stipple pattern repeats with period 8 */
	    stipx = (stipx - grSunStippleOrigin.p_x) & 07;
	    height = MIN(STIPMASKSIZE - stipy, maxy - screeny + 1);
	    while (screenx <= prect->r_xtop)
	    {
		pr_stencil(grSunCpr, screenx, screeny,
		    MIN(STIPMASKSIZE - stipx, prect->r_xtop - screenx + 1), 
		    height,
		    PIX_SRC | PIX_DONTCLIP | PIX_COLOR(sunColor),
		    sunStipple, stipx, stipy, NULL, 0, 0);
		screenx += (STIPMASKSIZE - stipx);
		stipx = 0;
	    }
	    screeny += (STIPMASKSIZE - stipy);
	    stipy = 0;
	}
    }
}

#endif	sun

