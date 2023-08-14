/* grSunW2.c -
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
static char rcsid[]="$Header: grSunW2.c,v 6.0 90/08/28 18:41:13 mayo Exp $";
#endif  not lint

#ifdef  sun

#include <stdio.h>
#include <suntool/tool_hs.h>
#undef bool
#define	Rect	MagicRect	/* Sun-3 brain death */

#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunWInt.h"

/* Library imports: */
extern int sscanf();

/* Colored stipple patterns for Sun 3/110s */
struct pixrect **sunWColorStipples;

/* Color map converted to sun format */
static unsigned char red[256], green[256], blue[256];

/*
 * ----------------------------------------------------------------------------
 *
 * SunWSetCMap --
 *
 *	SunWSetCMap outputs new values to the Sun color map.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The values in the color map are set from the array indicated
 *	by pmap.
 *
 * Errors:		None.
 *
 * ----------------------------------------------------------------------------
 */

Void
SunWSetCMap(pmap)
    char *pmap;			/* A pointer to 256*3 bytes containing the
				 * new values for the color map.  The first
				 * three values are red, green, and blue
				 * intensities for color 0, and so on.
				 */
{
    char *p;
    int i;
    Window *w;
    struct screen screen;
    int cmapDepth;

    extern sunWCmapProc();
    extern FILE *grSunRootWindow;

    cmapDepth = (1 << grNumBitPlanes);
    p = pmap;
    ASSERT(cmapDepth <= 256, "SunWSetCMap");
    for (i = 0; i < cmapDepth; i++)
    {
	red[i] = *p++;
	green[i] = *p++;
	blue[i] = *p++;
    }

    /* By Sun convention, the first and last values of all colormaps are 
     * the same in all windows, and set to be the foreground and background 
     * colors of this screen.
     */
    /****
    win_screenget(fileno(grSunRootWindow), &screen);
    red[0] = screen.scr_background.red;
    green[0] = screen.scr_background.green;
    blue[0] = screen.scr_background.blue;
    red[cmapDepth-1] = screen.scr_foreground.red;
    green[cmapDepth-1] = screen.scr_foreground.green;
    blue[cmapDepth-1] = screen.scr_foreground.blue;
    *****/

    WindSearch(NULL, NULL, NULL, sunWCmapProc, NULL);
}

/* Proc for each window, load the new colormap into each */
int
sunWCmapProc(w, cdata)
    Window *w;
    ClientData cdata;
{
    grSunWRec *grdata;

    grdata = (grSunWRec *) w->w_grdata;
    ASSERT(grdata != NULL, "SunWSetCMap");

    pw_putcolormap(grdata->gr_pw, 0, (1 << grNumBitPlanes), red, green, blue);
    return 0;
}



/*
 * ----------------------------------------------------------------------------
 *
 * sunWDrawLine --
 *	This routine draws a line.  Dotted lines are drawn as solid.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A line is drawn from (x1, y1) to (x2, y2), using the current
 *	color, style, and write mask.  This is an internal routine
 *	used by GrFastBox.
 *
 * ----------------------------------------------------------------------------
 */

Void
sunWDrawLine(x1, y1, x2, y2)
    int x1, y1;			/* Screen coordinates of first point. */
    int x2, y2;			/* Screen coordinates of second point. */
{
    int y1a, y2a, op;

    y1a = INVERTWY(y1);
    y2a = INVERTWY(y2);
    op = PIX_SRC;

    ASSERT(grCurSunWData != NULL, "sunWDrawLine");
    DIDDLE_LOCK();
    pw_vector(grCurSunWData->gr_pw, x1, y1a, x2, y2a, op, sunWColor);
}

/*
 * ----------------------------------------------------------------------------
 *
 * sunWFillRect --
 *	This routine draws a solid rectangle.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A solid rectangle is drawn in the current color and
 *	using the current write mask.  This is an internal
 *	routine used by GrFastBox.
 *
 * ----------------------------------------------------------------------------
 */

Void
sunWFillRect(prect)
    register Rect *prect;	/* Rectangle in screen coordinates */
{
    int miny, maxy, height, width, op, screenx, screeny;
    register int stipx, stipy, n;

    miny = INVERTWY(prect->r_ytop);
    maxy = INVERTWY(prect->r_ybot);
#define minx	prect->r_xbot
#define maxx	prect->r_xtop

    ASSERT(grCurSunWData != NULL, "sunWFillRect");
    DIDDLE_LOCK();

    /*
     * Non-stippled rectangles don't involve any fancy work, and
     * are the same for all types of displays.
     */
    if (sunWStipple == NULL) 
    {
	width = prect->r_xtop - prect->r_xbot + 1;
	height = prect->r_ytop - prect->r_ybot + 1;
	pw_rop(grCurSunWData->gr_pw, minx, miny, width, height, 
	    PIX_SRC | PIX_COLOR(sunWColor),
	    NULL, 0, 0);
	return;
    }

    if (sunWIntType == SUNTYPE_110
	&& (sunWColorStipples == NULL || sunWColorStipples[grCurDStyle] == NULL))
	sunWInitColorStipple(grCurDStyle);

    /* For B&W suns */
    op = sunWColor ? (PIX_SRC|PIX_DST) : (PIX_NOT(PIX_SRC) & PIX_DST);

    screeny = miny;

    /*
     * Stipple patterns repeat with period 8.  Since any starting
     * point mod 8 is equivalent to any other, we may as well get
     * as much out of a single stipple pixrect as possible, so we
     * start in the first 8 bits always, both for x and y.  The
     * while loop makes miny positive, enabling us to take it (mod 8)
     * easily.
     */
    while (miny < 0) miny += 65536;
    stipy = miny & 07;
    while (screeny <= maxy)
    {
	screenx = minx;

	while (minx < 0) minx += 65536;
	stipx = minx & 07;	/* Starting x-coordinate in stipple, mod 8 */
	height = maxy - screeny + 1;
	n = STIPMASKSIZE - stipy;
	if (n < height) height = n;
	while (screenx <= maxx)
	{
	    width = maxx - screenx + 1;
	    n = STIPMASKSIZE - stipx;
	    if (n < width) width = n;
	    switch (sunWIntType)
	    {
		/*
		 * Black&white Sun: since there's only a single bit plane,
		 * this either paints the stipple pattern or erases it.
		 */
		case SUNTYPE_BW:
		    pw_rop(grCurSunWData->gr_pw, screenx, screeny,
			width, height,
			op,
			sunWStipple, stipx, stipy);
		    break;
		/*
		 * 3/110C: erase bits covered by stipple, then paint (or-in)
		 * the full-color stipple pattern, i.e., the 8-bit deep pixrect
		 * whose value is sunWColor wherever the stipple is a 1,
		 * and 0 wherever the stipple is a 0.
		 */
		case SUNTYPE_110:
		    pw_rop(grCurSunWData->gr_pw, screenx, screeny, width, height,
			PIX_NOT(PIX_SRC) & PIX_DST,
			sunWStipple, stipx, stipy);
		    pw_rop(grCurSunWData->gr_pw, screenx, screeny, width, height,
			PIX_DST | PIX_SRC,
			sunWColorStipples[grCurDStyle], stipx, stipy);
		    break;
		/*
		 * 3/160C: generic full color stencil operation.  This has the
		 * same effect as the two pw_rop() calls in the SUNTYPE_110
		 * case above, but for many graphics devices (Sun 160's and
		 * 260's) it's noticeably more efficient.
		 */
		case SUNTYPE_160:
		    pw_stencil(grCurSunWData->gr_pw, screenx, screeny,
			width, height,
			PIX_SRC | PIX_COLOR(sunWColor),
			sunWStipple, stipx, stipy, 
			NULL, 0, 0);
		    break;
	    }
	    screenx += STIPMASKSIZE - stipx;
	    stipx = 0;
	}
	screeny += STIPMASKSIZE - stipy;
	stipy = 0;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * sunWInitColorStipple --
 *
 * For Sun 3/110's we want to avoid using stencils at all costs,
 * since their implementation is excessively slow (it involves
 * a whole bunch of mallocs of memory pixrects).  Instead, we
 * keep a cache of full-color (8-bit deep) pixrects, one for
 * each display style that uses stippling, allocated the first
 * time that display style is used.  This procedure initializes
 * the full-color pixrect for the display style 'style', which
 * uses stipple pattern sunWStipple and color sunWColor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory; ensures that sunWColorStipples[style]
 *	points to a valid full-color pixrect.
 *
 * ----------------------------------------------------------------------------
 */

Void
sunWInitColorStipple(style)
    register int style;
{
    register struct pixrect *memPr;
    register int n;

    if (sunWColorStipples == NULL)
    {
	/* Initialize the cache of full-color stipple pixrects */
	sunWColorStipples = (struct pixrect **)
	    mallocMagic(sizeof (struct pixrect *) * GR_NUM_STYLES);
	for (n = 0; n < GR_NUM_STYLES; n++)
	    sunWColorStipples[n] = (struct pixrect *) NULL;
    }

    if (sunWColorStipples[style] == (struct pixrect *) NULL)
    {
	/* Create the current style's entry in the cache */
	memPr = mem_create(STIPMASKSIZE, STIPMASKSIZE, 8);
	pr_rop(memPr, 0, 0, STIPMASKSIZE, STIPMASKSIZE,
		PIX_SRC | PIX_COLOR(sunWColor),
		sunWStipple, 0, 0);
	sunWColorStipples[style] = memPr;
    }
}
#endif	sun
