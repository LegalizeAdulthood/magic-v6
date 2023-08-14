/* windView.c -
 *
 *	Routines in this file control what is viewed in the contents
 *	of the windows.  This includes things like pan, zoom, and loading
 *	of windows.
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
 */

#ifndef lint
static char rcsid[]="$Header: windView.c,v 6.0 90/08/28 19:02:32 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "windows.h"
#include "glyphs.h"
#include "windInt.h"
#include "textio.h"

extern void windNewView();


/*
 * ----------------------------------------------------------------------------
 *
 * windFixSurfaceArea --
 *
 * 	When a window's surface or screen area has been changed,
 *	this procedure is usually called to fix up w->w_surfaceArea and
 *	w_origin.  Before calling this procedure, w->w_origin gives the
 *	screen location of w_surfaceArea.r_ll in 4096ths of a pixel and
 *	w->w_scale is correct, but w->w_surfaceArea may no longer be a
 *	slight overlap of w->w_screenArea as it should be.  This procedure
 *	regenerates w->w_surfaceArea to correspond to w->w_screenArea and
 *	changes w->w_origin to correspond to w->w_surfaceArea.r_ll again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	w->w_origin and w->w_surfaceArea are modified.
 *
 * ----------------------------------------------------------------------------
 */

void
windFixSurfaceArea(w)
    Window *w;			/* Window to fix up. */
{
    Rect newArea, tmp;

    GEO_EXPAND(&w->w_screenArea, 1, &tmp);
    WindScreenToSurface(w, &tmp, &newArea);
    w->w_origin.p_x += (newArea.r_xbot - w->w_surfaceArea.r_xbot) * w->w_scale;
    w->w_origin.p_y += (newArea.r_ybot - w->w_surfaceArea.r_ybot) * w->w_scale;
    w->w_surfaceArea = newArea;
}

/*
 * ----------------------------------------------------------------------------
 * WindLoad --
 *
 *	Load a new client surface into a window.  An initial surface area
 *	must be specified -- this is the area that will be visible in
 *	the window initially.
 *
 * Results:
 *	True if everything went well, false if the client does not
 *	own the window.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

bool
WindLoad(w, client, surfaceID, surfaceArea)
    Window *w;
    WindClient client;		/* The unique identifier of the client */
    ClientData surfaceID;	/* A unique ID for this surface */
    Rect *surfaceArea;		/* The area that should appear in the window */
{
   if (client != w->w_client) return FALSE;

   w->w_surfaceID = surfaceID;
   WindMove(w, surfaceArea);
   return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 * WindMove --
 *
 *	Move the surface under the window so that the given area is visible
 *	and as large as possible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window will be now view a different portion of the clients area.  
 *	The client may be called to redisplay the areas that moved.
 * ----------------------------------------------------------------------------
 */

void
WindMove(w, surfaceArea)
    Window *w;			/* the window to be panned */
    Rect *surfaceArea;		/* The area to be viewed */
{
    int size, xscale, yscale, halfSizePixels, halfSizeUnits;

    /* Compute the scale factor from world coordinates to 1/4096ths
     * of a pixel.  To be sure that surfaceArea will all fit in the
     * window, compute the scale twice, once using the y-dimension
     * alone, and once using x alone.  Then use the smaller scale factor.
     */
    
    size = surfaceArea->r_xtop - surfaceArea->r_xbot + 1;
    xscale = ((w->w_screenArea.r_xtop - 
	    w->w_screenArea.r_xbot + 1) * 4096)/size;

    size = surfaceArea->r_ytop - surfaceArea->r_ybot + 1;
    yscale = ((w->w_screenArea.r_ytop - 
	    w->w_screenArea.r_ybot + 1) * 4096)/size;

    w->w_scale = MIN(xscale, yscale);
    if (w->w_scale < 1) w->w_scale = 1;

    /* Recompute w->surfaceArea and w->w_origin, which determine the
     * screen-surface mapping along with w->w_scale.  In order to
     * center surfaceArea in the window, compute the windows's half-size
     * in units of 4096ths of a pixel and in units.  Be sure to round
     * things up so that w->w_surfaceArea actually overlaps the window
     * slightly.
     */

    halfSizePixels = (w->w_screenArea.r_xtop - w->w_screenArea.r_xbot) * 2048;
    halfSizeUnits = (halfSizePixels/w->w_scale) + 1;
    w->w_surfaceArea.r_xbot = (surfaceArea->r_xbot + surfaceArea->r_xtop)/2
	- halfSizeUnits;
    w->w_surfaceArea.r_xtop = w->w_surfaceArea.r_xbot + 2*halfSizeUnits + 1;
    w->w_origin.p_x =
	((w->w_screenArea.r_xtop + w->w_screenArea.r_xbot) * 2048) -
	(halfSizeUnits * w->w_scale);

    halfSizePixels = (w->w_screenArea.r_ytop - w->w_screenArea.r_ybot) * 2048;
    halfSizeUnits = (halfSizePixels/w->w_scale) + 1;
    w->w_surfaceArea.r_ybot = (surfaceArea->r_ybot + surfaceArea->r_ytop)/2
	- halfSizeUnits;
    w->w_surfaceArea.r_ytop = w->w_surfaceArea.r_ybot + 2*halfSizeUnits + 1;
    w->w_origin.p_y =
	((w->w_screenArea.r_ytop + w->w_screenArea.r_ybot) * 2048) -
	(halfSizeUnits * w->w_scale);

    WindAreaChanged(w, &(w->w_screenArea));
    windNewView(w);
}


/*
 * ----------------------------------------------------------------------------
 * WindZoom --
 *
 *	Zoom a window.  The window will stay centered about the same point
 *	as it is currently.  A factor greater than 1 increases the scale
 *	of the window (higher magnification), while a factor smaller than 1
 *	results in a lower scale (lower magnification).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window will be now view a different portion of the clients area.  
 *	The client may be called to redisplay part of the screen.
 * ----------------------------------------------------------------------------
 */

void
WindZoom(w, factor)
    Window *w;			/* the window to be zoomed */
    float factor;		/* The amount to zoom by (1 is no change),
				 * greater than 1 is a larger magnification
				 * (zoom in), and less than 1 is less mag.
				 * (zoom out) )
				 */
{
    int centerx, centery;
    Rect newArea;

    centerx = (w->w_surfaceArea.r_xbot + w->w_surfaceArea.r_xtop) / 2;
    centery = (w->w_surfaceArea.r_ybot + w->w_surfaceArea.r_ytop) / 2;

    newArea.r_xbot = centerx - (centerx - w->w_surfaceArea.r_xbot) * factor;
    newArea.r_xtop = centerx + (w->w_surfaceArea.r_xtop - centerx) * factor;
    newArea.r_ybot = centery - (centery - w->w_surfaceArea.r_ybot) * factor;
    newArea.r_ytop = centery + (w->w_surfaceArea.r_ytop - centery) * factor;

    WindMove(w, &newArea);
}


/*
 * ----------------------------------------------------------------------------
 *
 * WindView --
 *
 * Change the view in the selected window to just contain the
 * bounding box for that window.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is changed.
 *
 * ----------------------------------------------------------------------------
 */
    
    /* ARGSUSED */

void
WindView(w)
    Window *w;
{
    Rect bbox;
#define SLOP	10	/* Amount of border (in fraction of a screenfull) 
			 * to add.
			 */
    if (w == NULL)
	return;

    if (w->w_bbox == NULL) {
	TxError("Can't do 'view' because w_bbox is NULL.\n");
	TxError("Report this to a magic implementer.\n");
	return;
    };

    bbox = *(w->w_bbox);
    bbox.r_xbot -= (bbox.r_xtop - bbox.r_xbot + 1) / SLOP;
    bbox.r_xtop += (bbox.r_xtop - bbox.r_xbot + 1) / SLOP;
    bbox.r_ybot -= (bbox.r_ytop - bbox.r_ybot + 1) / SLOP;
    bbox.r_ytop += (bbox.r_ytop - bbox.r_ybot + 1) / SLOP;

    WindMove(w, &bbox);
}

/*
 * ----------------------------------------------------------------------------
 *
 * WindScroll --
 *
 *	Scroll the view around.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is changed.  The offset can
 *	be specified either in screen coordinates or surface coordinates
 *	or both.
 *
 * ----------------------------------------------------------------------------
 */

void
WindScroll(w, surfaceOffset, screenOffset)
    Window *w;
    Point *surfaceOffset;	/* An offset in surface coordinates.  The
				 * screen point that used to display surface
				 * point (0,0) will now display surface point
				 * surfaceOffset.  Can be NULL to indicate
				 * no offset.
				 */
    Point *screenOffset;	/* An additional offset in screen coordinates.
				 * Can be NULL to indicate no offset.  If
				 * non-NULL, then after scrolling according
				 * to surfaceOffset, the view is adjusted again
				 * so that the surface unit that used to be
				 * displayed at (0,0) will now be displayed
				 * at screenOffset.  Be careful to make sure
				 * the coordinates in here are relatively
				 * small (e.g. the same order as the screen
				 * size), or else there may be arithmetic
				 * overflow and unexpected results.  Use only
				 * surfaceOffset if you're going to be
				 * scrolling a long distance.
				 */
{
    if (surfaceOffset != NULL)
    {
	w->w_surfaceArea.r_xbot += surfaceOffset->p_x;
	w->w_surfaceArea.r_ybot += surfaceOffset->p_y;
	w->w_surfaceArea.r_xtop += surfaceOffset->p_x;
	w->w_surfaceArea.r_ytop += surfaceOffset->p_y;
    }

    /* Screen offsets are trickier.  Divide up into a whole-unit part
     * (which is applied to w->w_surfaceArea) and a fractional-unit
     * part (which is applied to w->w_origin.  Then readjust both to
     * make sure that w->w_surfaceArea still overlaps the window area
     * on all sides.
     */
    
    if (screenOffset != NULL)
    {
	int units, pixels;

	pixels = screenOffset->p_x * 4096;
	units = pixels/w->w_scale;
	w->w_surfaceArea.r_xbot -= units;
	w->w_surfaceArea.r_xtop -= units;
	w->w_origin.p_x += pixels - (w->w_scale*units);

	pixels = screenOffset->p_y * 4096;
	units = pixels/w->w_scale;
	w->w_surfaceArea.r_ybot -= units;
	w->w_surfaceArea.r_ytop -= units;
	w->w_origin.p_y += pixels - (w->w_scale*units);
    }

    windFixSurfaceArea(w);

    WindAreaChanged(w, &(w->w_screenArea));
    windNewView(w);
}
