/* grSun5.c -
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
static char rcsid[]="$Header: grSun5.c,v 6.0 90/08/28 18:41:03 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include <sunwindow/window_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunInt.h"

/* imports from other graphics files */
extern GrGlyphs *grCursorGlyphs;

/* our cursors */
#define MAX_CURSORS	32
struct cursor grSunCursors[MAX_CURSORS];



/*
 * ----------------------------------------------------------------------------
 * SunDrawGlyph --
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

SunDrawGlyph(gl, p)
    GrGlyph *gl;		/* A single glyph to draw */
    Point *p;			/* screen pos of lower left corner */
{
    Rect bbox, sunBbox;
    bool anyObscure;
    LinkedRect *ob;
    int xsize, ysize;

    GR_CHECK_LOCK();
    xsize = gl->gr_xsize;
    ysize = gl->gr_ysize;

    bbox.r_xbot = p->p_x;
    bbox.r_ybot = p->p_y;
    bbox.r_xtop = (p->p_x + xsize - 1);
    bbox.r_ytop = (p->p_y + ysize - 1);

    sunBbox.r_xbot = bbox.r_xbot;
    sunBbox.r_ybot = GrScreenRect.r_ytop - bbox.r_ytop;
    sunBbox.r_xtop = bbox.r_xtop;
    sunBbox.r_ytop = GrScreenRect.r_ytop - bbox.r_ybot;

    anyObscure = FALSE;
    for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH( &(ob->r_r), &bbox)) anyObscure = TRUE;
    }

    if ((!anyObscure) && (GEO_SURROUND(&grCurClip, &bbox)) )
    {
	int *pixelp, x, y;

	/* no clipping, try to go quickly */
	pixelp = gl->gr_pixels;
	for (y = 0; y < ysize; y++)
	{
	    for (x = 0; x < xsize; x++)
	    {
		int color;
		color = grStyleTable[*pixelp].color;
		pr_put(grSunCpr, sunBbox.r_xbot + x, 
		    sunBbox.r_ybot + ysize - (y + 1), color);
		pixelp++;
	    }
	}
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
		    pixelp = &( gl->gr_pixels[y*gl->gr_xsize + 
			    (startx - bbox.r_xbot)]);
		    for ( ; startx <= endx; startx++) {
			int color;
			color = grStyleTable[*pixelp].color;
			pr_put(grSunCpr, startx, 
			    GrScreenRect.r_ytop - yloc, color);
			pixelp++;
		    };

		}
	    }
	    yloc++;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 * sunDefineCursor:
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
sunDefineCursor(glyphs)
    GrGlyphs *glyphs;
{
    int i;

    for (i = 0; i < MAX_CURSORS; i++)
    {
	if (i < glyphs->gr_num)
	{
	    int *p;
	    GrGlyph *g;
	    struct pixrect *pr;
	    int x, y;

	    g = glyphs->gr_glyph[i];

	    if ((g->gr_xsize != 16) || (g->gr_ysize != 16))
	    {
		TxError("Sun cursors must be 16 X 16 pixels.\n");
		return;
	    }

	    pr = mem_create(16, 16, 1);
	    grSunCursors[i].cur_shape = pr;
	    grSunCursors[i].cur_function = PIX_SRC | PIX_DST;
	    grSunCursors[i].cur_xhot = g->gr_origin.p_x;
	    grSunCursors[i].cur_yhot = 15 - g->gr_origin.p_y;

	    p = &(g->gr_pixels[0]);
	    for (y = 0; y < 16; y++)
	    {
		for (x = 0; x < 16; x++)
		{
		    int color;
		    color = (grStyleTable[*p].color != 0);
		    pr_put(pr, x, 15 - y, color);
		    p++;
		}
	    }

	    /***** FOR DEBUGGING
	    {
		SunSetCursor(i);
		getchar();
		printf("\n");
	    }
	    *****/
	}
	else
	{
	    grSunCursors[i].cur_xhot = grSunCursors[i].cur_yhot = 0;
	    grSunCursors[i].cur_shape = NULL;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 * SunSetCursor:
 *
 *	Make the cursor be a new pattern, as defined in the display styles file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the cursor is turned back on it will take on the new pattern.
 * ----------------------------------------------------------------------------
 */

Void
SunSetCursor(cursorNum)
int cursorNum;		/* The cursor number as defined in the display
			 * styles file.
			 */
{
    if (grSunCursors[cursorNum].cur_shape != NULL)
    {
	grSunCurCursor = cursorNum;
	if (grSunCursorOn)
	    win_setcursor(grSunColorWindowFD, &grSunCursors[cursorNum]);
    }
}

#endif 	sun
