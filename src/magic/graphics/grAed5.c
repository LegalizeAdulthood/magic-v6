/* grAed5.c -
 *
 *	Manipulate the programable cursor on the graphics display.
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
static char rcsid[]="$Header: grAed5.c,v 6.0 90/08/28 18:40:34 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grAedInt.h"

extern GrGlyphs *grCursorGlyphs;	/* our programmable cursors */
int aedCursOriginX, aedCursOriginY;

#define MAX_CURSORS	31



/*
 * ----------------------------------------------------------------------------
 * AedDrawGlyph --
 *
 *	Draw one glyph on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws pixels.
 * ----------------------------------------------------------------------------
 */

AedDrawGlyph(gl, p)
    GrGlyph *gl;		/* A single glyph */
    Point *p;			/* screen pos of lower left corner */
{
    Rect bbox;
    bool anyObscure;
    LinkedRect *ob;
    extern int aedColor;

    GR_CHECK_LOCK();

    bbox.r_ll = *p;
    bbox.r_xtop = p->p_x + gl->gr_xsize - 1;
    bbox.r_ytop = p->p_y + gl->gr_ysize - 1;

    (void) aedSetWMandC(0377, aedColor);

    anyObscure = FALSE;
    for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH( &(ob->r_r), &bbox)) anyObscure = TRUE;
    }

    if ((!anyObscure) && (GEO_SURROUND(&grCurClip, &bbox)) )
    {
	/* no clipping -- do a run-length encoding */
	int i, pixnum, lastpixel, *pixelp;

	aedSetPos(bbox.r_xbot, bbox.r_ybot);
	putc('r', grAedOutput);
	aedOutxy20(bbox.r_xtop, bbox.r_ytop);

	putc('\\', grAedOutput);
	pixelp = gl->gr_pixels;
	lastpixel = *pixelp;
	pixnum = 0;
	for(i = 0; i < (gl->gr_xsize * gl->gr_ysize); i++)
	{
	    if (lastpixel != *pixelp)
	    {
		putc(pixnum, grAedOutput);
		putc(grStyleTable[lastpixel].color, grAedOutput);
		lastpixel = *pixelp;
		pixnum = 1;
	    }
	    else
		pixnum++;
	    pixelp++;
	}
	putc(pixnum, grAedOutput);
	putc(grStyleTable[lastpixel].color, grAedOutput);
	putc(0, grAedOutput);
    }
    else
    {
	/* do pixel by pixel clipping */
	int y, yloc;

	yloc = bbox.r_ybot;
	for (y = 0; y < gl->gr_ysize; y++)
	{
	    int startx, endx;
	    if ( (yloc <= grCurClip.r_ytop) && (yloc >= grCurClip.r_ybot) )
	    {
		int laststartx;
		laststartx = bbox.r_xbot - 1;
		for (startx = bbox.r_xbot; startx <= bbox.r_xtop; 
			startx = endx + 1)
		{
		    int *pixelp;

		    startx = MAX(startx, grCurClip.r_xbot);
		    endx = MIN(bbox.r_xtop, grCurClip.r_xtop);

		    if (anyObscure)
		    {
			for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
			{
			    if ( (ob->r_r.r_ybot <= yloc) && 
				 (ob->r_r.r_ytop >= yloc) )
			    {
				if (ob->r_r.r_xbot <= startx)
				    startx = MAX(startx, ob->r_r.r_xtop + 1);
				else if (ob->r_r.r_xbot <= endx)
				    endx = MIN(endx, ob->r_r.r_xbot - 1);
			    }
			}
		    }

		    /* stop if we aren't advancing */
		    if (startx == laststartx) break;  
		    laststartx = startx;
		    if (startx > endx) continue;

		    /* draw a section of this scan line */
		    aedSetPos(startx, yloc);
		    putc('r', grAedOutput);
		    aedOutxy20(endx, yloc);
		    putc('X', grAedOutput);
		    pixelp = &( gl->gr_pixels[y*gl->gr_xsize + 
			    (startx - bbox.r_xbot)]);
		    for ( ; startx <= endx; startx++)
		    {
			putc(grStyleTable[*pixelp++].color, grAedOutput);
		    }
		}
	    }
	    yloc++;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 * aedDefineCursor:
 *
 *	Define a new set of cursors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given matrix is stored in the graphics display, and it can be
 *	used as the cursor by calling GrSetCursor.
 * ----------------------------------------------------------------------------
 */

Void
aedDefineCursor(glyphs)
    GrGlyphs *glyphs;
{
    int glyphnum;
    Point p;
    Rect oldClip;

    if (glyphs->gr_num <= 0) return;

    if (glyphs->gr_num > MAX_CURSORS)
    {
	TxError("The AED only has room for %d cursors\n", MAX_CURSORS);
	return;
    }

    /* expand clipping amount for off-screen access on the Aed */
    GrLock(GR_LOCK_SCREEN, FALSE);
    oldClip = grCurClip;
    grCurClip = GrScreenRect;
    grCurClip.r_ytop += 16;

    /* draw the glyphs */
    p.p_x = 0;
    p.p_y = aedCursorRow;
    for (glyphnum = 0; glyphnum < glyphs->gr_num; glyphnum++)
    {
	GrGlyph *gl;
	gl = glyphs->gr_glyph[glyphnum];
	if ((gl->gr_xsize != 16) || (gl->gr_ysize != 16))
	{
	    TxError("Cursors for the AED must be 16 X 16 pixels.\n");
	    return;
	}
	GrDrawGlyph(gl, &p);
	p.p_x += 16;
    }

    /* Restore clipping */
    grCurClip = oldClip;
    GrUnlock(GR_LOCK_SCREEN);
}


/*
 * ----------------------------------------------------------------------------
 * AedSetCursor:
 *
 *	Make the cursor be a new pattern, as defined in the display styles file.
 *
 *	Note: This routine has a special hack in it for AED displays: cursor
 *	pattern 0 is ignored and instead turned into the blinking cross that the
 *	AED supports normally.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the cursor is turned back on it will take on the new pattern.
 * ----------------------------------------------------------------------------
 */

Void
AedSetCursor(cursorNum)
int cursorNum;		/* The cursor number as defined in the display
		         * styles file.
		         */
{
    int xpos;

    ASSERT(cursorNum >= 0, "AedSetCursor");
    if (!aedHasProgCursor || (cursorNum > MAX_CURSORS) || 
	(cursorNum == 0 && BLINKING))
    {
	/* Use built-in cursor */
	aedCursOriginX = 8;
	aedCursOriginY = 8;
	fputs("]+", grAedOutput);
	putc(0, grAedOutput);
	putc(0, grAedOutput);
    }
    else
    {
	xpos = cursorNum * 16;  

	putc(26, grAedOutput);
	aedOutxy20(xpos, aedCursorRow);

	putc(30, grAedOutput);
	aedCursOriginX = grCursorGlyphs->gr_glyph[cursorNum]->gr_origin.p_x;
	aedCursOriginY = grCursorGlyphs->gr_glyph[cursorNum]->gr_origin.p_y;
	putc(aedCursOriginX, grAedOutput);
	putc(aedCursOriginY, grAedOutput);
	
	fputs("]P", grAedOutput);
	putc(0, grAedOutput);
	putc(0, grAedOutput);

    }
}
