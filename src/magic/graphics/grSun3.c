/* grSun3.c -
 *
 * This file contains additional functions to manipulate an Sun
 * color display.  Included here are device-dependent routines to draw and 
 * erase text and draw a grid.
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
static char rcsid[]="$Header: grSun3.c,v 6.0 90/08/28 18:40:57 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "grSunInt.h"
#include "signals.h"

/* Imports from other SUN modules */
extern Void sunSetWMandC(), sunSetLineStyle();

/* used for text clipping */
static struct pixrect *grSunTextPR = NULL;

/* our fonts */
struct pixfont *grSunFontSmall, *grSunFontMedium;
struct pixfont *grSunFontLarge, *grSunFontXLarge;
	


int sunCurCharSize = -1;		/* Current character size */


struct pixfont *
grSunFont(fontname)
    char *fontname;
{
    char filename1[100], filename2[100];
    struct pixfont *font;

    sprintf(filename1, "/usr/lib/fonts/fixedwidthfonts/%.30s", fontname);
    font = pf_open(filename1);
    if (font == NULL)
    {
	sprintf(filename2, "/usr/suntool/fixedwidthfonts/%.30s", fontname);
	font = pf_open(filename2);
	if (font == NULL)
	{
	    TxError("Could not open font file '%s'\nnor '%s'\n", 
		filename1, filename2);
	    MainExit(1);
	}
    }
    return font;
}

/*---------------------------------------------------------
 * grSunTextInit:
 *	Initialize the text stuff.
 *
 * Results:
 *	none.
 *
 * Side Effects:
 *	variables are initialized.
 *---------------------------------------------------------
 */

void
grSunTextInit()
{
    if (grSunTextPR == NULL)
	grSunTextPR = mem_create(2, 2, 1);
    grSunFontSmall = grSunFont("screen.r.7");
    grSunFontMedium = grSunFont("screen.r.11");
    grSunFontLarge = grSunFont("screen.b.14");
    grSunFontXLarge = grSunFont("gallant.r.19");
}


/*---------------------------------------------------------
 * sunDrawGrid:
 *	sunDrawGrid adds a grid to the grid layer, using the current
 *	write mask and color.
 *
 * Results:
 *	TRUE is returned normally.  However, if the grid gets too small
 *	to be useful, then nothing is drawn and FALSE is returned.
 *
 * Side Effects:
 *	Pixels are added to the grid layer, to draw a dot at each grid point.
 *	Intervals are given by
 *	the dimensions of prect such that a line runs through each side of
 *	the given rectangle.  
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

bool
sunDrawGrid(prect, outline, clip)
    Rect *prect;		/* A rectangle that forms the template
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
    bool useLines;

    xsize = prect->r_xtop - prect->r_xbot;
    ysize = prect->r_ytop - prect->r_ybot;
    if (GRID_TOO_SMALL(xsize, ysize, GR_NUM_GRIDS)) return FALSE;
    useLines = FALSE;	/* We don't care for the lines, we prefer dots */

    xstart = prect->r_xbot % xsize;
    while (xstart < clip->r_xbot<<12) xstart += xsize;
    ystart = prect->r_ybot % ysize;
    while (ystart < clip->r_ybot<<12) ystart += ysize;

    if (useLines) {
	/* Draw a grid using lines (solid lines, since the Sun has
	 * trouble drawing dashed ones).
	 */
	for (x = xstart; x < (clip->r_xtop+1)<<12; x += xsize)
	{
	    if (SigInterruptPending) return;
	    (*grDrawLinePtr)(x>>12, clip->r_ybot, x>>12, clip->r_ytop);
	}
	for (y = ystart; y < (clip->r_ytop+1)<<12; y += ysize)
	{
	    if (SigInterruptPending) return;
	    (*grDrawLinePtr)(clip->r_xbot, y>>12, clip->r_xtop, y>>12);
	}
    }
    else {
	/* Draw a grid using dots at the grid points */
	for (x = xstart; x < (clip->r_xtop+1)<<12; x += xsize)
	{
	    if (SigInterruptPending) return;
	    for (y = ystart; y < (clip->r_ytop+1)<<12; y += ysize)
	    {
		pr_put(grSunCpr, x >> 12, GrScreenRect.r_ytop - (y >> 12), 
		    sunColor);
	    }
	}
    }

    return TRUE;
}



/*---------------------------------------------------------
 * sunSetCharSize:
 *	This local routine sets the character size in the display,
 *	if necessary.
 *
 * Results:	None.
 *
 * Side Effects:
 *	If the current display character size isn't already equal to size,
 *	then it is made so.
 *---------------------------------------------------------
 */

Void
sunSetCharSize(size)
    int size;		/* Width of characters, in pixels (6 or 8). */
{
    sunCurCharSize = size;
}



/*
 * ----------------------------------------------------------------------------
 * SunTextSize --
 *
 *	Determine the size of a text string. 
 *
 * Results:
 *	None 
 *
 * Side effects:
 *	A rectangle is filled in that is the size of the text in pixels.
 *	The origin (0, 0) of this rectangle is located on the baseline
 *	at the far left side of the string.
 * ----------------------------------------------------------------------------
 */

Void
SunTextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    struct pixfont *font;
    struct pr_size prsize;

    switch (size)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:	/* a 5x8 font, 1 pixel space between */
	    font = grSunFontSmall;
	    break;
	case GR_TEXT_MEDIUM:	/* an 7x12 font, 1 pixel space between */
	    font = grSunFontMedium;
	    break;
	case GR_TEXT_LARGE:	/* a 10x16 font, 2 pixel space between */
	    font = grSunFontLarge;
	    break;
	case GR_TEXT_XLARGE:	/* a 14x24 font, 2 pixel space between */
	    font = grSunFontXLarge;
	    break;
	default:
	    TxError("Unusual text size (internal error)\n");
	    font = grSunFontSmall;
	    break;
    }
    prsize = pf_textwidth(strlen(text), font, text);
    r->r_xbot = 0;
    r->r_ybot = 0;
    r->r_xtop = prsize.x - 1;
    r->r_ytop = prsize.y - 1;
    if (text[0] != '\0')
    {
	int xorg, yorg;
	/* find the baseline origin */
	xorg = -font->pf_char[text[0]].pc_home.x;
	r->r_xbot += xorg; r->r_xtop += xorg;
	yorg = (-font->pf_char[text[0]].pc_home.y) - 
		(font->pf_char[text[0]].pc_pr->pr_height - 1);
	r->r_ybot += yorg; r->r_ytop += yorg;
    }
}


/*
 * ----------------------------------------------------------------------------
 * SunReadPixel --
 *
 *	Read one pixel.
 *
 * Results:
 *	An integer containing the pixel, or -1 if an error occurred.
 *
 * Side effects:
 *	none.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

int
SunReadPixel(w, x, y)
    Window *w;
    int x, y;		/* a pixel on the screen */
{
    return pr_get(grSunCpr, x, GrScreenRect.r_ytop - y);
}


/*
 * ----------------------------------------------------------------------------
 * SunBitBlt --
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

void
SunBitBlt(r, p)
    Rect *r;		/* the source of pixels */
    Point *p;		/* the destination */
{
    pr_rop(grSunCpr, r->r_xbot, GrScreenRect.r_ytop - r->r_ybot,
	r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
	PIX_SRC | PIX_DONTCLIP,
	grSunCpr, p->p_x, GrScreenRect.r_ytop - p->p_y);
}



/*---------------------------------------------------------
 * sunPutText:
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
 * Design:
 *	This routine clips the characters on pixel boundaries, which
 *	tricky because the SUN can only draw whole characters.  The
 *	routine sunDrawPartial does the pixel clipping, this routine
 *	does the overall string processing.  The array grCharDisplay
 *	is used to classify each character in the array according to
 *	whether or not it is completely visible, completely invisible,
 *	or partially visible.  Then this routine draws all the completely
 *	visible characters and calls sunDrawPartial to draw the partially
 *	visible ones.
 *---------------------------------------------------------
 */

Void
sunPutText(text, pos, clip, obscure)
    char *text;			/* The text to be drawn. */
    Point *pos;			/* A point located at the leftmost point of
				 * the baseline for this string.
				 */
    Rect *clip;			/* A rectangle to clip against */
    LinkedRect *obscure;	/* A list of obscuring rectangles */
{
    Rect size, location;
    Rect overlap;
    LinkedRect *ob;
    struct pr_prpos where;
    struct pixfont *font;
    int width, height;

    SunTextSize(text, sunCurCharSize, &size);
    width = size.r_xtop - size.r_xbot + 1;
    height = size.r_ytop - size.r_ybot + 1;
    if ((width > grSunTextPR->pr_width) || (height > grSunTextPR->pr_height))
    {
	mem_destroy(grSunTextPR);
	grSunTextPR = mem_create(width*4, height*4, 1);
    }

    switch (sunCurCharSize)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:	/* a 5x8 font, 1 pixel space between */
	    font = grSunFontSmall;
	    break;
	case GR_TEXT_MEDIUM:	/* an 7x12 font, 1 pixel space between */
	    font = grSunFontMedium;
	    break;
	case GR_TEXT_LARGE:	/* a 10x16 font, 2 pixel space between */
	    font = grSunFontLarge;
	    break;
	case GR_TEXT_XLARGE:	/* a 14x24 font, 2 pixel space between */
	    font = grSunFontXLarge;
	    break;
	default:
	    TxError("Unusual text size (internal error)\n");
	    font = grSunFontSmall;
	    break;
    }

    location.r_xbot = pos->p_x + size.r_xbot;
    location.r_xtop = pos->p_x + size.r_xtop;
    location.r_ybot = pos->p_y + size.r_ybot;
    location.r_ytop = pos->p_y + size.r_ytop;

    /* put the text in a black & white bitmap */
    where.pos.x = size.r_xbot;
    where.pos.y = size.r_ytop;
    where.pr = grSunTextPR;
    pf_text(where, PIX_SRC | PIX_COLOR(1) | PIX_DONTCLIP, font, text);

#ifdef	NOTDEF
    /* Debugging code */
    pr_rop(grSunCpr, 18, 18, width + 4, height + 4, PIX_SRC | PIX_COLOR(1),
	NULL, 0, 0);
    pr_rop(grSunCpr, 20, 20, width, height, PIX_SRC, grSunTextPR, 0, 0);
#endif	NOTDEF

    /* erase parts of the bitmap that are obscured */
    for (ob = obscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH(&ob->r_r, &location))
	{
	    overlap = location;
	    GeoClip(&overlap, &ob->r_r);
	    pr_rop(grSunTextPR, 
		overlap.r_xbot - location.r_xbot,
		overlap.r_ytop - location.r_ytop,
		overlap.r_xtop - overlap.r_xbot + 1,
		overlap.r_ytop - overlap.r_ybot + 1,
		PIX_SRC | PIX_COLOR(0) | PIX_DONTCLIP,
		NULL, 0, 0);
#ifdef	DEBUG
    pr_rop(grSunCpr, 20, 20, width, height, PIX_SRC, grSunTextPR, 0, 0);
#endif	DEBUG
	}
    }

    overlap = location;
    GeoClip(&overlap, clip);

    /* copy the text to the color screen */
    if ((overlap.r_xbot <= overlap.r_xtop) && 
	(overlap.r_ybot <= overlap.r_ytop))
    {
	int ulx, uly;	/* upper left corner of area to be drawn */

	ulx = MAX(location.r_xbot, overlap.r_xbot);
	uly = MIN(location.r_ytop, overlap.r_ytop);

	pr_stencil(grSunCpr, ulx, GrScreenRect.r_ytop - uly,
	    overlap.r_xtop - overlap.r_xbot + 1, 
	    overlap.r_ytop - overlap.r_ybot + 1, 
	    PIX_SRC | PIX_COLOR(sunColor), 
	    grSunTextPR, 
	    ulx - location.r_xbot, location.r_ytop - uly,
	    NULL, 0, 0);
    }
}

#endif 	sun
