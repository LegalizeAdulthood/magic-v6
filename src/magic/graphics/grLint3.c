/* grAed3.c -
 *
 * This file contains additional functions to manipulate an Aed512
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
static char rcsid[]="$Header: grAed3.c,v 6.0 90/08/28 18:40:29 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grAedInt.h"
#include "textio.h"
#include "signals.h"
#include "malloc.h"

/* Imports from other AED modules */
extern int aedCurx, aedCury;
extern Void aedSetWMandC(), aedSetLineStyle();
extern Point aedOffPoint;


int aedCurCharSize = -1;		/* Current character size */


/*---------------------------------------------------------
 * aedDrawGrid:
 *	aedDrawGrid adds a grid to the grid layer, using the current
 *	write mask and color.
 *
 * Results:
 *	TRUE is returned normally.  However, if the grid gets too small
 *	to be useful, then nothing is drawn and FALSE is returned.
 *
 * Side Effects:
 *	Vectors are added to the grid layer, to draw a grid with horizontal
 *	and vertical lines running across the screen at intervals given by
 *	the dimensions of prect such that a line runs through each side of
 *	the given rectangle.  The grid is drawn with a dotted line.  All
 *	previous information in the grid layer is erased.  
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

bool
aedDrawGrid(prect, outline, clip)
    Rect *prect;		/* A rectangle that forms the template
			         * for the grid.  Note:  in order to maintain
			         * precision for the grid, the rectangle
			         * coordinates are specified in units of
			         * screen coordinates multiplied by 4096.
			         */
    int outline;		/* the outline style */
    Rect *clip;			/* a clipping rectangle */
{
    int i, xsize, ysize, tmp, xtop, ytop, lineStyle;
    xsize = prect->r_xtop - prect->r_xbot;
    ysize = prect->r_ytop - prect->r_ybot;

    /* Use the standard macro to determine if the grid is too small.
     * However, because we use dotted lines, we can't draw as small
     * as a driver that uses dots or solid lines.  So, we set the
     * minimum size to twice that of other drivers.
     */
    if (GRID_TOO_SMALL(xsize, ysize, GR_NUM_GRIDS/2)) return FALSE;

    lineStyle = outline & 0377;
    lineStyle += lineStyle << 8;

    /* Note: it takes two additional hacks to make the grid come out
     * right if a small piece of it is redrawn.  First, we have to
     * rotate the line style so that various pieces of the grid will
     * line up.  Second, the AED seems always to draw the last pixel
     * in a line, regardless of the line style.  So, if the last pixel
     * is supposed to be off, there's a special hack below to stop
     * the grid line one pixel earlier than normal.
     */

    xtop = clip->r_xtop;
    i = 0200 >> (xtop & 07);
    if (!(lineStyle & i)) xtop -= 1;
    ytop = clip->r_ytop;
    i = 0200 >> (ytop & 07);
    if (!(lineStyle & i)) ytop -= 1;

    (void) aedSetLineStyle((lineStyle << (clip->r_ybot & 07)) >> 8);
    i = prect->r_xbot % xsize;
    while (i < clip->r_xbot<<12) i+=xsize;
    for ( ; i < (clip->r_xtop+1)<<12; i+=xsize)
    {
	if (SigInterruptPending) return TRUE;
	tmp = i >> 12;
	putc('Q', grAedOutput);
	aedOutxy20(tmp, clip->r_ybot);
	putc('A', grAedOutput);
	aedOutxy20(tmp, ytop);
    }
    (void) aedSetLineStyle((lineStyle << (clip->r_xbot & 07)) >> 8);
    i = prect->r_ybot % ysize;
    while (i < clip->r_ybot<<12) i+=ysize;
    for ( ; i < (clip->r_ytop+1)<<12; i+=ysize)
    {
	if (SigInterruptPending) return TRUE;
	tmp = i >> 12;
	putc('Q', grAedOutput);
	aedOutxy20(clip->r_xbot, tmp);
	putc('A', grAedOutput);
	aedOutxy20(xtop, tmp);
    }
    putc('Q', grAedOutput);		/* Restore current access position */
    aedOutxy20(aedCurx, aedCury);
    (void) aedSetLineStyle(0377);
    return TRUE;
}



/*---------------------------------------------------------
 * aedSetCharSize:
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
aedSetCharSize(size)
    int size;		/* Width of characters, in pixels (6 or 8). */
{
    if (size == aedCurCharSize) return;
    aedCurCharSize = size;

    switch (size)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:
	    fputs("^15\6\1L", grAedOutput);
	    break;
	case GR_TEXT_MEDIUM:
	    fputs("^17\10\1L", grAedOutput);
	    break;
	case GR_TEXT_LARGE:
	    fputs("^25\14\1L", grAedOutput);
	    break;
	case GR_TEXT_XLARGE:
	    fputs("^27\20\1L", grAedOutput);
	    break;
	default:
	    TxError("Unknown size for text (internal error)\n");
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 * aedGetCharSize:
 *	Compute the size of a character and related parameters.
 *	This is only used by the routines aedPutText and AedTextSize.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All parameters are modified, except the first.
 * ----------------------------------------------------------------------------
 */

Void
aedGetCharSize(charSize, width, above, below, space)
    int charSize;	/* the character size (small, medium, etc.) */
    int *width;		/* Filled in with the width of a character, in pixels */
    int *above;		/* Filled in with the number of pixels above the
			 * baseline of a character.
			 */
    int *below;		/* Filled in with the number of pixels below the
			 * baseline (decenders).
			 */
    int *space;		/* Filled in with the spacing between characters */
{
    switch (charSize)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:	/* a 5x8 font, 1 pixel space between */
	    *width = 5;
	    *space = 1;
	    *above = 6;
	    *below = 2;
	    break;
	case GR_TEXT_MEDIUM:	/* an 7x12 font, 1 pixel space between */
	    *width = 7;
	    *space = 1;
	    *above = 9;
	    *below = 3;
	    break;
	case GR_TEXT_LARGE:	/* a 10x16 font, 2 pixel space between */
	    *width = 10;
	    *space = 2;
	    *above = 12;
	    *below = 4;
	    break;
	case GR_TEXT_XLARGE:	/* a 14x24 font, 2 pixel space between */
	    *width = 14;
	    *space = 2;
	    *above = 18;
	    *below = 6;
	    break;
	default:
	    *width = 5;
	    *space = 1;
	    *above = 6;
	    *below = 2;
	    TxError("Unusual text size (internal error)\n");
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 * AedTextSize --
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
GrTextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    int numChars;
    int Cabove, Cbelow, Cwidth, Cspace;

    ASSERT(r != NULL, "aedTextSize");
    numChars = strlen(text);
    /* compute size with origin at leftmost baseline */
    aedGetCharSize(size, &Cwidth, &Cabove, &Cbelow, &Cspace);
    r->r_xbot = 0;
    r->r_xtop = (numChars * Cwidth) + ((numChars - 1) * Cspace) - 1;
    r->r_ybot = -Cbelow;
    r->r_ytop = Cabove - 1;
}


/*
 * ----------------------------------------------------------------------------
 * AedReadPixel --
 *
 *	Read one pixel from the screen.
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
GrReadPixel(w, x, y)
    Window *w;
    int x,y;		/* the location of a pixel in screen coords */
{
    extern int aedWMask;
    int pix;

    /* turn on all bits in write mask */
    putc('L', grAedOutput);
    putc(0377, grAedOutput);

    /* do the read */
    pix = -1;
    if (!grAedMouseIsDisabled)
    {
	char str[20];

	aedSetPos(x, y);
	putc(89, grAedOutput);
	(void) fflush(grAedOutput);
	if ((grFgets(str, 20, grAedInput, "graphics display") == NULL) ||
	    (sscanf(str, "%d", &pix) != 1) )
	    TxError("Could not read pixel from the graphics display.\n");
    }

    /* restore write mask */
    putc('L', grAedOutput);
    putc(aedWMask&0377, grAedOutput);

    return pix;
}


/*
 * ----------------------------------------------------------------------------
 * AedBitBlt --
 *
 *	Copy information from one part of the screen to the other.
 *	Do not call 'AedBitBlt' directly, call 'GrBitBlt' instead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	changes the screen.
 * ----------------------------------------------------------------------------
 */

Void
GrBitBlt(r, p)
    Rect *r;
    Point *p;
{
    extern int aedWMask;

    /* turn on all bits in write mask */
    putc('L', grAedOutput);
    putc(0377, grAedOutput);

    /* do the BitBlt */
    putc(24, grAedOutput);
    aedOutxy20(r->r_xbot, r->r_ybot);
    aedOutxy20(r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1);
    aedOutxy20(p->p_x, p->p_y);

    /* restore write mask */
    putc('L', grAedOutput);
    putc(aedWMask&0377, grAedOutput);
}


/*
 * ----------------------------------------------------------------------------
 * aed1024BitBlt --
 *
 *	Copy information from one part of the screen to the other.
 *	Special function for AED1024s, UCB512s use the routine above.
 *	This routine is accessed by indirecting through GrBitBltPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	changes the screen.
 * ----------------------------------------------------------------------------
 */

Void
aed1024BitBlt(r, p)
    Rect *r;
    Point *p;
{
    extern int aedWMask;

    /* turn on all bits in write mask */
    putc('L', grAedOutput);
    putc(0377, grAedOutput);

    /* do the BitBlt */

    /* Change CAP to (x_bot,y_bot) */
    putc('Q', grAedOutput);				/* MOV */
    aedOutxy20(r->r_xbot, r->r_ybot);

    /* Define Area of Interest */
    putc('r', grAedOutput);				/* DAI */
    aedOutxy20(r->r_xtop, r->r_ytop);

    /* Copy Area of Interest */
    fputs("++", grAedOutput);			/* CAI+ */
    aedOutxy20(p->p_x, p->p_y);	

    /* CAP is now unknown:  make sure we don't re-use the old one. */
    aedCurx = aedCury = -1;

    /* restore write mask */
    putc('L', grAedOutput);
    putc(aedWMask&0377, grAedOutput);
}


/*
 * ----------------------------------------------------------------------------
 * aedDrawPartial --
 *
 *	Draw a partial character.  This is difficult on the AED displays
 *	since it can only draw whole characters!  We will have to draw
 *	off-screen and BitBlt parts of it in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes part of a character and modifies part of off-screen memory.
 * ----------------------------------------------------------------------------
 */

aedDrawPartial(c, drawx, drawy, r, clip, obscure)
    char c;
    int drawx, drawy;	/* location to draw character */
    Rect *r;		/* a rectangle that encloses the whole character */
    Rect *clip;
    LinkedRect *obscure;
{
    LinkedRect *ob;
    Rect clipr;
    int xoffset, yoffset; /* Offsets to add to screen coords to get to
			   * off-screen coords.
			   */

    /* clip and see if anything is left */
    clipr = *r;
    GeoClip(&clipr, clip);
    if ((clipr.r_xbot > clipr.r_xtop) || (clipr.r_ybot > clipr.r_ytop))
	return;

    /* first, copy the area to off-screen memory */
    GrBitBlt(r, &aedOffPoint);
    xoffset = aedOffPoint.p_x - r->r_xbot;
    yoffset = aedOffPoint.p_y - r->r_ybot;

    /* now, draw the character in off-screen memory */
    aedSetPos(drawx + xoffset, drawy + yoffset);
    putc('\6', grAedOutput);
    putc(c, grAedOutput);
    fputs("\33Q", grAedOutput);
    aedOutxy20(aedCurx, aedCury);

    /* Go through the list of obscuring regions and BitBlt from the
     * screen onto the off-screen character in order to obscure part of
     * it.
     */
    for (ob = obscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH(&(ob->r_r), &clipr))
	{
	    Point topoint;
	    Rect area;
	    area = ob->r_r;
	    GeoClip(&area, &clipr);
	    /* Area now contains the portion of the rectangle that
	     * obscures the character.
	     */
	    topoint.p_x = aedOffPoint.p_x + (area.r_xbot - r->r_xbot);
	    topoint.p_y = aedOffPoint.p_y + (area.r_ybot - r->r_ybot);
	    GrBitBlt(&area, &topoint);
	}
    }

    /* Now, BitBlt the area that intersects the clip box back onto the
     * screen.
     */
    {
	Point topoint;

	topoint = clipr.r_ll;
	/* slide the clip are to off-screen memory */
	clipr.r_xbot += xoffset;
	clipr.r_xtop += xoffset;
	clipr.r_ybot += yoffset;
	clipr.r_ytop += yoffset;
	/* we have some area to transfer */
	GrBitBlt(&clipr, &topoint); 
    }

}


#define GR_DISP_NONE	0
#define	GR_DISP_ALL	1
#define GR_DISP_PARTIAL	2

static int *grCharDisplay = NULL;
static int grMaxNumChars = -1;

/*---------------------------------------------------------
 * aedPutText:
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
 *	tricky because the AED can only draw whole characters.  The
 *	routine aedDrawPartial does the pixel clipping, this routine
 *	does the overall string processing.  The array grCharDisplay
 *	is used to classify each character in the array according to
 *	whether or not it is completely visible, completely invisible,
 *	or partially visible.  Then this routine draws all the completely
 *	visible characters and calls aedDrawPartial to draw the partially
 *	visible ones.
 *---------------------------------------------------------
 */

Void
aedPutText(text, pos, clip, obscure)
    char *text;			/* The text to be drawn. */
    Point *pos;			/* A point located at the leftmost point of
				 * the baseline for this string.
				 */
    Rect *clip;			/* A rectangle to clip against */
    LinkedRect *obscure;	/* A list of obscuring rectangles */
{
    LinkedRect *ob;
    int numChars;
    int Cabove, Cbelow, Cwidth, Cspace;
    int i;
    register int *charDisplay;
    Rect tr;
    bool anyPartial = FALSE;

    /* Make sure that our grCharDisplay array is big enough and initialized */

    numChars = strlen(text);
    if (numChars > grMaxNumChars)
    {
	if (grCharDisplay != NULL) freeMagic( (char *) grCharDisplay);
	grCharDisplay = (int *) mallocMagic( (unsigned) sizeof(int) * 
	    (numChars + 20));
	grMaxNumChars = numChars + 20;
    }
    charDisplay = grCharDisplay;
    for (i = 0; i < numChars; i++) *charDisplay++ = GR_DISP_ALL;

    /* Compute overall bounding box for text, assuming the whole thing
     * is displayed.
     */

    aedGetCharSize(aedCurCharSize, &Cwidth, &Cabove, &Cbelow, &Cspace);
    tr.r_xbot = pos->p_x;
    tr.r_xtop = pos->p_x + (numChars * Cwidth) + ((numChars - 1) * Cspace) - 1;
    tr.r_ybot = pos->p_y - Cbelow;
    tr.r_ytop = pos->p_y + Cabove - 1;

    /* Clip the text against the clipping area (take quick action if
     * the text is completely invisible or completely visible: defaults
     * are set up to display everything).
     */

    if (!GEO_TOUCH(&tr, clip)) return;
    if (!GEO_SURROUND(clip, &tr))
    {
	char *cp;
	Rect cr;

	cr.r_xbot = tr.r_xbot;
	cr.r_ybot = tr.r_ybot;
	cr.r_ytop = tr.r_ytop;
	cr.r_xtop = cr.r_xbot + Cwidth;
	charDisplay = grCharDisplay;
	for (cp = text; *cp != '\0'; cp++)
	{
	    if (!GEO_TOUCH(&cr, clip))
		*charDisplay = GR_DISP_NONE;
	    else if (!GEO_SURROUND(clip, &cr))
	    {
		*charDisplay = GR_DISP_PARTIAL;
		anyPartial = TRUE;
	    }
	    charDisplay++;
	    cr.r_xbot += Cspace + Cwidth;
	    cr.r_xtop += Cspace + Cwidth;
	}
    }

    /* Now process the obscuring areas.  This is almost the same,
     * except that the sense is inverted:  the obscuring areas
     * describe areas that are NOT to be displayed.
     */

    for (ob = obscure; ob != NULL; ob = ob->r_next)
    {
	char *cp;
	Rect cr;

	/* continue if this doesn't obscure the text at all */
	if (!GEO_TOUCH(&tr, &(ob->r_r))) continue;

	/* return if the text is completely obscured */
	if (GEO_SURROUND(&(ob->r_r), &tr)) return;

	/* now check on a character by character basis */
	cr.r_xbot = tr.r_xbot;
	cr.r_ybot = tr.r_ybot;
	cr.r_ytop = tr.r_ytop;
	cr.r_xtop = cr.r_xbot + Cwidth;
	charDisplay = grCharDisplay;
	for (cp = text; *cp != '\0'; cp++)
	{
	    if (GEO_SURROUND(&(ob->r_r), &cr))
		*charDisplay = GR_DISP_NONE;
	    else if (GEO_TOUCH(&(ob->r_r), &cr))
	    {
		*charDisplay = GR_DISP_PARTIAL;
		anyPartial = TRUE;
	    }
	    charDisplay++;
	    cr.r_xbot += Cspace + Cwidth;
	    cr.r_xtop += Cspace + Cwidth;
	}
    }

    {
	int *disp, xstart, anyChars;
	char *tp;

	/* Put up all the characters that are completely visible,
	 * and space over all those that are completely invisible.
	 * Be careful not to ever output anything partly off-screen,
	 * even a blank.
	 */

	disp = grCharDisplay;
	tp = text;
	xstart = pos->p_x;
	anyChars = FALSE;
	while (*tp != '\0')
	{
	    if (*disp == GR_DISP_ALL)
	    {
		if (!anyChars)
		{
		    aedSetPos(xstart, pos->p_y);
		    putc('\6', grAedOutput);
		    anyChars = TRUE;
		}
		putc(*tp, grAedOutput);
	    }
	    else if (anyChars) 
	    {
		putc(' ', grAedOutput);
	    }
	    else xstart += Cwidth + Cspace;
	    disp++;
	    tp++;
	}
	if (anyChars)
	{
	    fputs("\33Q", grAedOutput);
	    aedOutxy20(aedCurx, aedCury);
	}

	/* Now go back and get all of the partial characters. */

	if (anyPartial)
	{
	    Rect cr;

	    cr.r_xbot = tr.r_xbot;
	    cr.r_ybot = tr.r_ybot;
	    cr.r_ytop = tr.r_ytop;
	    cr.r_xtop = cr.r_xbot + Cwidth;
	    disp = grCharDisplay;
	    tp = text;
	    while (*tp != '\0')
	    {
		if (*disp == GR_DISP_PARTIAL)
		{
		    aedDrawPartial(*tp, cr.r_xbot, cr.r_ybot + Cbelow,
			&cr, clip, obscure);
		}
		cr.r_xbot += Cspace + Cwidth;
		cr.r_xtop += Cspace + Cwidth;
		disp++;
		tp++;
	    }
	}
    }
}
