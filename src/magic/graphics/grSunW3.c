/* grSunW3.c -
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
static char rcsid[]="$Header: grSunW3.c,v 6.0 90/08/28 18:41:15 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include <suntool/tool_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 

#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "grSunWInt.h"
#include "signals.h"

/* our fonts */
struct pixfont *grSunWFontSmall, *grSunWFontMedium;
struct pixfont *grSunWFontLarge, *grSunWFontXLarge, *grSunWFontSys;
	


int sunWCurCharSize = -1;                /* Current character size */
 


struct pixfont *
grSunWFont(fontname)
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
 * grSunWTextInit --
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
grSunWTextInit()
{
    grSunWFontSmall = grSunWFont("screen.r.7");
    grSunWFontMedium = grSunWFont("screen.r.11");
    grSunWFontLarge = grSunWFont("screen.b.14");
    grSunWFontXLarge = grSunWFont("gallant.r.19");
    grSunWFontSys = pw_pfsysopen();
}


/*---------------------------------------------------------
 * sunWDrawGrid --
 *	sunWDrawGrid adds a grid to the grid layer, using the current
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
sunWDrawGrid(prect, outline, clip)
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

    ASSERT(grCurSunWData != NULL, "sunWDrawGrid");

    xsize = prect->r_xtop - prect->r_xbot;
    ysize = prect->r_ytop - prect->r_ybot;
    if (GRID_TOO_SMALL(xsize, ysize, GR_NUM_GRIDS)) return FALSE;
    useLines = TRUE;	/* Lines are MUCH faster. */

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
	DIDDLE_LOCK();
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
	    DIDDLE_LOCK();
	    for (y = ystart; y < (clip->r_ytop+1)<<12; y += ysize)
	    {
		pw_put(grCurSunWData->gr_pw, x >> 12, 
		    INVERTWY(y >> 12), sunWColor);
	    }
	}
    }

    return TRUE;
}


/*---------------------------------------------------------
 * sunwSetCharSize:
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
sunWSetCharSize(size)
    int size;		/* Width of characters, in pixels (6 or 8). */
{
    sunWCurCharSize = size;
}



/*
 * ----------------------------------------------------------------------------
 * SunWReadPixel --
 *
 *	Read one pixel.
 *
 * Results:
 *	An integer containing the pixel, or -1 if an error ocurred.
 *
 * Side effects:
 *	none.
 * ----------------------------------------------------------------------------
 */

int
SunWReadPixel(w, x, y)
    Window *w;
    int x, y;		/* a pixel on the screen */
{
    int val;

    if (w == (Window *) NULL) 
    {
	TxError("You must be pointing to a window.\n");
	return -1;
    }

    GrLock(w, FALSE);
    GR_CHECK_LOCK();
    val = pw_get(grCurSunWData->gr_pw, x, INVERTWY(y));
    if (!rl_empty(&grCurSunWData->gr_pw->pw_fixup)) {
	TxError("Could not read pixel value.\n");
	val = -1;
    } else {
	val = val & grBitPlaneMask;
    }
    GrUnlock(w);

    return val;
}


/*
 * ----------------------------------------------------------------------------
 * SunWBitBlt --
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
SunWBitBlt(r, p)
    Rect *r;		/* the source of pixels */
    Point *p;		/* the destination */
{
    ASSERT(grCurSunWData != NULL, "SunWBitBlt");
    /* NOT DEBUGGED */
    pw_rop(grCurSunWData->gr_pw, r->r_xbot, INVERTWY(r->r_ybot),
	r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
	PIX_SRC,
	grCurSunWData->gr_pw, p->p_x, INVERTWY(p->p_y));
}



/*
 * ----------------------------------------------------------------------------
 * SunWTextSize --
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
SunWTextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    struct pixfont *font;
    struct pr_size prsize;

    switch (size)
    {
	case GR_TEXT_SMALL:
	    font = grSunWFontSmall;
	    break;
	case GR_TEXT_MEDIUM:	
	    font = grSunWFontMedium;
	    break;
	case GR_TEXT_LARGE:
	    font = grSunWFontLarge;
	    break;
	case GR_TEXT_XLARGE:	
	    font = grSunWFontXLarge;
	    break;
	case GR_TEXT_DEFAULT:
	    font = grSunWFontSys;
	    break;
	default:
	    TxError("Unusual text size (internal error)\n");
	    font = grSunWFontSys;
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


/*---------------------------------------------------------
 *
 * sunWPutText --
 *
 * This routine puts a chunk of text on the screen.
 * It is tuned for speed.
 *
 * General strategy:
 *
 *	Figure out how big the clipped text will have to be, and
 *	ensure we have a big enough memory pixrect to hold it.
 *
 *	Write the text into this region using pf_text (NOTE: this
 *	is NOT transparent text writing).  Clipping is enabled.
 *
 *	Use pw_rop to copy that region to the screen (for B&W
 *	displays) or pw_stencil (for color displays)
 *
 * Results:	
 *	none.
 *
 * Side Effects:
 *	The text is drawn on the screen.  
 *
 *---------------------------------------------------------
 */

    /*ARGSUSED*/
Void
sunWPutText(text, pos, clip, obscure)
    char *text;			/* The text to be drawn. */
    Point *pos;			/* A point located at the leftmost point of
				 * the baseline for this string.
				 */
    Rect *clip;			/* Clip to this area */
    LinkedRect *obscure;	/* List of obscuring windows (ignored) */
{
    static struct pixrect *memPixRect = (struct pixrect *) NULL;
    int offsetx, offsety, clipwidth, clipheight;
    struct pr_subregion bound;
    struct pixfont *font;
    struct pr_prpos where;
    Rect realClip;
    int op;

    /* Which font? */
    switch (sunWCurCharSize)
    {
	case GR_TEXT_SMALL:	
	    font = grSunWFontSmall;
	    break;
	case GR_TEXT_MEDIUM:
	    font = grSunWFontMedium;
	    break;
	case GR_TEXT_LARGE:
	    font = grSunWFontLarge;
	    break;
	case GR_TEXT_XLARGE:
	    font = grSunWFontXLarge;
	    break;
	case GR_TEXT_DEFAULT:
	    font = grSunWFontSys;
	    break;
	default:
	    TxError("Unusual text size (internal error)\n");
	    font = grSunWFontSys;
	    break;
    }

    /* Obtain a bounding box for the text in its own coordinate system */
    pf_textbound(&bound, strlen(text), font, text);

    /*
     * Relocate the resulting area to the place we want to draw the text.
     * Use the intersection of the text area and the clip area as
     * the actual clipping area.
     */
    realClip.r_xbot = pos->p_x + bound.pos.x;
    realClip.r_xtop = realClip.r_xbot + bound.size.x - 1;
    realClip.r_ytop = pos->p_y - bound.pos.y;
    realClip.r_ybot = realClip.r_ytop - bound.size.y + 1;

    GEOCLIP(&realClip, clip);
    clip = &realClip;

    /* Origin of new clip area in pixrect coordinates */
    offsetx = clip->r_xbot;
    offsety = INVERTWY(clip->r_ytop);

    /* Where text goes relative to new clip area's origin */
    where.pos.x = pos->p_x - offsetx;
    where.pos.y = INVERTWY(pos->p_y) - offsety;

    /* Size of new clip area */
    clipwidth = clip->r_xtop - clip->r_xbot + 1;
    clipheight = clip->r_ytop - clip->r_ybot + 1;

    /* Done if no work to do */
    if (clipwidth <= 0 || clipheight <= 0)
	return;

    /* Ensure we have a place big enough to hold the clipped text */
    if (memPixRect == NULL
	    || memPixRect->pr_size.x < clipwidth
	    || memPixRect->pr_size.y < clipheight)
    {
	if (memPixRect) pr_destroy(memPixRect);
	memPixRect = mem_create(clipwidth, clipheight, 1);
    }

    /*
     * Write the text into this region using pf_text.
     * This clips correctly on the bottom and left, but not necessarily
     * on the top and right since memPixRect may be bigger than needed.
     */
    where.pr = memPixRect;
    pf_text(where, PIX_SRC, font, text);

    /*
     * Use pw_rop to copy only the portion of this region that
     * will actually appear in the destination into the destination.
     */
    DIDDLE_LOCK();
    switch (sunWIntType)
    {
	case SUNTYPE_BW:
	    op = sunWColor ? (PIX_SRC|PIX_DST) : (PIX_NOT(PIX_SRC) & PIX_DST);
	    pw_rop(grCurSunWData->gr_pw, offsetx, offsety, clipwidth, clipheight,
		    op, memPixRect, 0, 0);
	    break;
	case SUNTYPE_110:
	case SUNTYPE_160:
	    pw_stencil(grCurSunWData->gr_pw, offsetx, offsety,
		clipwidth, clipheight,
		PIX_SRC | PIX_COLOR(sunWColor),
		memPixRect, 0, 0, 
		NULL, 0, 0);
	    break;
    }
}

#endif 	sun
