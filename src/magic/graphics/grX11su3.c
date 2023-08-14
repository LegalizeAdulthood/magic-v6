/* grX11su3.c -
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
 * This file contains additional functions to manipulate an X window system
 * color display.  Included here are device-dependent routines to draw and 
 * erase text and draw a grid.
 *
 */

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "signals.h"
#include "utils.h"
#include "hash.h"
#include "grX11conf.h"
#include <X11/Xlib.h>
#include "grX11Int.h"

/* locals */

static XFontStruct *grXFonts[4];
#define	grSmallFont	grXFonts[0]
#define	grMediumFont	grXFonts[1]
#define	grLargeFont	grXFonts[2]
#define	grXLargeFont	grXFonts[3]



/*---------------------------------------------------------
 * grxDrawGrid:
 *	grxDrawGrid adds a grid to the grid layer, using the current
 *	write mask and color.
 *
 * Results:
 *	TRUE is returned normally.  However, if the grid gets too small
 *	to be useful, then nothing is drawn and FALSE is returned.
 *
 * Side Effects:    None.
 *---------------------------------------------------------
 */

bool
grx11DrawGrid (prect, outline, clip)
    Rect *prect;			/* A rectangle that forms the template
			         * for the grid.  Note:  in order to maintain
			         * precision for the grid, the rectangle
			         * coordinates are specified in units of
			         * screen coordinates multiplied by 4096.
			         */
    int outline;		/* the outline style */
    Rect *clip;			/* a clipping rectangle */
{
    int xsize, ysize;
    int x, y;
    int xstart, ystart;
    XSegment seg[GR_NUM_GRIDS];
    int snum, low, hi, shifted;

    xsize = prect->r_xtop - prect->r_xbot;
    ysize = prect->r_ytop - prect->r_ybot;
    if (GRID_TOO_SMALL(xsize, ysize, GR_NUM_GRIDS)) return FALSE;
    
    xstart = prect->r_xbot % xsize;
    while (xstart < clip->r_xbot<<12) xstart += xsize;
    ystart = prect->r_ybot % ysize;
    while (ystart < clip->r_ybot<<12) ystart += ysize;
    
    grx11SetLineStyle(outline);

    snum = 0;
    low = grMagicToX(clip->r_ybot);
    hi = grMagicToX(clip->r_ytop);
    for (x = xstart; x < (clip->r_xtop+1)<<12; x += xsize)
    {
        if (snum >= GR_NUM_GRIDS) break;
	shifted = x >> 12;
	seg[snum].x1 = shifted;
	seg[snum].y1 = low;
	seg[snum].x2 = shifted;
	seg[snum].y2 = hi;
	snum++;
    }
    XDrawSegments(grXdpy, grCurrent.window, grGCDraw, seg, snum);

    snum = 0;
    low = clip->r_xbot;
    hi = clip->r_xtop;
    for (y = ystart; y < (clip->r_ytop+1)<<12; y += ysize)
    {
        if (snum >= GR_NUM_GRIDS) break;
	shifted = grMagicToX(y >> 12);
	seg[snum].x1 = low;
	seg[snum].y1 = shifted;
	seg[snum].x2 = hi;
	seg[snum].y2 = shifted;
	snum++;
    }
    XDrawSegments(grXdpy, grCurrent.window, grGCDraw, seg, snum);
    return true;
}


/*---------------------------------------------------------
 * grxLoadFont
 *	This local routine loads the X fonts used by Magic.
 *
 * Results:	None.
 *
 * Side Effects:    None.
 *---------------------------------------------------------
 */

Void
grx11LoadFont()
{
    static char *fontnames[4] = {
      X_FONT_SMALL,
      X_FONT_MEDIUM,
      X_FONT_LARGE,
      X_FONT_XLARGE };
    static char *optionnames[4] = {
      "small",
      "medium",
      "large",
      "xlarge"};

    int i;
    char *unable = "Unable to load font";

    for (i=0; i!= 4; i++)
    {
    	 char	*s = XGetDefault(grXdpy,"magic",optionnames[i]);
	 if (s) fontnames[i] = s;
         if ((grXFonts[i] = XLoadQueryFont(grXdpy, fontnames[i])) == NULL) 
         {
	      TxError("%s %s\n",unable,fontnames[i]);
              if ((grXFonts[i]= XLoadQueryFont(grXdpy,GR_DEFAULT_FONT))==NULL) 
	      {
	           TxError("%s %s\n",unable,GR_DEFAULT_FONT);
		   return false;
	      }
         }
    }
    return true;
}


/*---------------------------------------------------------
 * grxSetCharSize:
 *	This local routine sets the character size in the display,
 *	if necessary.
 *
 * Results:	None.
 *
 * Side Effects:    None.
 *---------------------------------------------------------
 */

Void
grx11SetCharSize (size)
    int size;		/* Width of characters, in pixels (6 or 8). */
{
    grCurrent.fontSize = size;
    switch (size)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:
	    grCurrent.font = grSmallFont;
	    break;
	case GR_TEXT_MEDIUM:
	    grCurrent.font = grMediumFont;
	    break;
	case GR_TEXT_LARGE:
	    grCurrent.font = grLargeFont;
	    break;
	case GR_TEXT_XLARGE:
	    grCurrent.font = grXLargeFont;
	    break;
	default:
	    TxError("%s%d\n", "grx11SetCharSize: Unknown character size ",
		size );
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 * GrXTextSize --
 *
 *	Determine the size of a text string. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle is filled in that is the size of the text in pixels.
 *	The origin (0, 0) of this rectangle is located on the baseline
 *	at the far left side of the string.
 * ----------------------------------------------------------------------------
 */

Void
GrX11TextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    XCharStruct overall;
    XFontStruct *font;
    int dir,fa,fd;
    
    switch (size) {
    case GR_TEXT_DEFAULT:
    case GR_TEXT_SMALL:
	font = grSmallFont;
	break;
    case GR_TEXT_MEDIUM:
	font = grMediumFont;
	break;
    case GR_TEXT_LARGE:
	font = grLargeFont;
	break;
    case GR_TEXT_XLARGE:
	font = grXLargeFont;
	break;
    default:
	TxError("%s%d\n", "GrX11TextSize: Unknown character size ",
		size );
	break;
    }
    if (font == NULL) return;
    XTextExtents(font, text, strlen(text), &dir, &fa, &fd, &overall);
    r->r_ytop = overall.ascent;
    r->r_ybot = -overall.descent;
    r->r_xtop = overall.width - overall.lbearing;
    r->r_xbot = -overall.lbearing - 1;
}


/*
 * ----------------------------------------------------------------------------
 * GrXReadPixel --
 *
 *	Read one pixel from the screen.
 *
 * Results:
 *	An integer containing the pixel's color.
 *
 * Side effects:
 *	none.
 *
 * ----------------------------------------------------------------------------
 */

int
GrX11ReadPixel (w, x, y)
    MagicWindow *w;
    int x,y;		/* the location of a pixel in screen coords */
{
    XImage *image;
    unsigned long value;
    XWindowAttributes	att;

    XGetWindowAttributes(grXdpy,grCurrent.window, &att);
    if ( x < 0 || x >= att.width || grMagicToX(y) < 0 || grMagicToX(y) >= att.height) return(0);
    image = XGetImage(grXdpy, grCurrent.window, x, grMagicToX(y), 1, 1,
		      ~0, ZPixmap);
    value = XGetPixel(image, 0, 0);
    return (value & (1 << grCurrent.depth) - 1);
}


/*
 * ----------------------------------------------------------------------------
 * GrXBitBlt --
 *
 *	Copy information from one part of the screen to the other.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	changes the screen.
 * ----------------------------------------------------------------------------
 */

Void
GrX11BitBlt(r, p)
    Rect *r;
    Point *p;
{
    XCopyArea(grXdpy, grCurrent.window, grCurrent.window, grGCCopy,
	      r->r_xbot, grMagicToX(r->r_ytop),
	      r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
	      p->p_x, grMagicToX(p->p_y));
}



/*---------------------------------------------------------
 * grxPutText:
 *      (modified on SunPutText)
 *
 *	This routine puts a chunk of text on the screen in the current
 *	color, size, etc.  The caller must ensure that it fits on
 *	the screen -- no clipping is done except to the obscuring rectangle
 *	list and the clip rectangle.
 *
 * Results:	
 *	none.
 *
 * Side Effects:
 *	The text is drawn on the screen.  
 *
 *---------------------------------------------------------
 */

Void
grx11PutText (text, pos, clip, obscure)
    char *text;			/* The text to be drawn. */
    Point *pos;			/* A point located at the leftmost point of
				 * the baseline for this string.
				 */
    Rect *clip;			/* A rectangle to clip against */
    LinkedRect *obscure;	/* A list of obscuring rectangles */

{
    Rect location;
    Rect overlap;
    Rect textrect;
    LinkedRect *ob;
    void grX11suGeoSub();

    if (grCurrent.font == NULL) return;

    GrX11TextSize(text, grCurrent.fontSize, &textrect);

    location.r_xbot = pos->p_x + textrect.r_xbot;
    location.r_xtop = pos->p_x + textrect.r_xtop;
    location.r_ybot = pos->p_y + textrect.r_ybot;
    location.r_ytop = pos->p_y + textrect.r_ytop;

    /* erase parts of the bitmap that are obscured */
    for (ob = obscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH(&ob->r_r, &location))
	{
	    overlap = location;
	    GeoClip(&overlap, &ob->r_r);
	    grX11suGeoSub(&location, &overlap);
	}
    }
 
    overlap = location;
    GeoClip(&overlap, clip);

    /* copy the text to the color screen */
    if ((overlap.r_xbot < overlap.r_xtop)&&(overlap.r_ybot <= overlap.r_ytop))
    {
	XRectangle xr;

	XSetFont(grXdpy, grGCText, grCurrent.font->fid);
	xr.x = overlap.r_xbot;
	xr.y = grMagicToX(overlap.r_ytop);
	xr.width = overlap.r_xtop - overlap.r_xbot + 1;
	xr.height = overlap.r_ytop - overlap.r_ybot + 1;
	XSetClipRectangles(grXdpy, grGCText, 0, 0, &xr, 1, Unsorted);
	XDrawString(grXdpy, grCurrent.window, grGCText,
		    pos->p_x, grMagicToX(pos->p_y),
		    text, strlen(text));
    }
}


/* grX11suGeoSub:
 *	return the tallest sub-rectangle of r not obscured by area
 *	area must be within r.
 */

void
grX11suGeoSub(r, area)
register Rect *r;		/* Rectangle to be subtracted from. */
register Rect *area;		/* Area to be subtracted. */

{
    if (r->r_xbot == area->r_xbot) r->r_xbot = area->r_xtop;
    else
    if (r->r_xtop == area->r_xtop) r->r_xtop = area->r_xbot;
    else
    if (r->r_ybot <= area->r_ybot) r->r_ybot = area->r_ytop;
    else
    if (r->r_ytop == area->r_ytop) r->r_ytop = area->r_ybot;
    else
    r->r_xtop = area->r_xbot;
}
