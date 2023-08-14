/* grSunW5.c -
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
static char rcsid[]="$Header: grSunW5.c,v 6.0 90/08/28 18:41:18 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include <suntool/tool_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 

#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunWInt.h"

/* imports from other graphics files */
extern GrGlyphs *grCursorGlyphs;

/* our cursors */
#define MAX_CURSORS	32
struct cursor grSunWCursors[MAX_CURSORS];
bool grSunWCursorOn = FALSE;
int grSunWCurCursor = -1;

 
 
/*
 * ----------------------------------------------------------------------------
 * sunCacheFree --
 *
 *	Free a glyph cache
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

Void
sunCacheFree(pr)
    struct pixrect *pr;
{
    if (pr != NULL) pr_close(pr);
}

/*
 * ----------------------------------------------------------------------------
 * sunFixGlyphCache --
 *
 *	This device driver caches glyphs in malloc'ed memory pixrects for
 *	efficiency.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in glyph->gr_cache.
 * ----------------------------------------------------------------------------
 */

void
sunFixGlyphCache(gl)
    GrGlyph *gl;
{
    int xsize, ysize;
    int *pixelp, x, y;
    struct pixrect *cache;

    xsize = gl->gr_xsize;
    ysize = gl->gr_ysize;
    pixelp = gl->gr_pixels;
    cache = mem_create(xsize, ysize, grCurSunWData->gr_pw->pw_pixrect->pr_depth);

    for (y = 0; y < ysize; y++)
    {
	for (x = 0; x < xsize; x++)
	{
	    int color;
	    color = grStyleTable[*pixelp].color;
	    pr_put(cache, x, ysize - (y + 1), (color & grBitPlaneMask));
	    pixelp++;
	}
    }

    gl->gr_cache = (ClientData) cache;
    gl->gr_free = sunCacheFree;
}


/*
 * ----------------------------------------------------------------------------
 * SunWDrawGlyph --
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

SunWDrawGlyph(gl, p)
    GrGlyph *gl;		/* A single glyph to draw */
    Point *p;			/* screen pos of lower left corner */
{
    int xsize, ysize;
    int *pixelp, x, y;
    struct pixrect *pr;
    int i, j;

    GR_CHECK_LOCK();
    DIDDLE_LOCK();
    xsize = gl->gr_xsize;
    ysize = gl->gr_ysize;

    if (gl->gr_cache == NULL)
	sunFixGlyphCache(gl);
    
    pr = (struct pixrect *)  gl->gr_cache;
    x = p->p_x;
    y = INVERTWY(p->p_y) - ysize + 1;

    pw_writebackground(grCurSunWData->gr_pw, x, y, xsize, ysize, PIX_SRC);
    /****
    pw_write(grCurSunWData->gr_pw, x, y, xsize, ysize, PIX_SRC, pr, 0, 0);
    ****/
    pw_rop(grCurSunWData->gr_pw, x, y, xsize, ysize, PIX_SRC, pr, 0, 0);
}


/*
 * ----------------------------------------------------------------------------
 * sunWDefineCursor:
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
sunWDefineCursor(glyphs)
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
	    grSunWCursors[i].cur_shape = pr;
	    grSunWCursors[i].cur_function = PIX_SRC | PIX_DST;
	    grSunWCursors[i].cur_xhot = g->gr_origin.p_x;
	    grSunWCursors[i].cur_yhot = 15 - g->gr_origin.p_y;

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
		SunWSetCursor(i);
		getchar();
		printf("\n");
	    }
	    *****/
	}
	else
	{
	    grSunWCursors[i].cur_xhot = grSunWCursors[i].cur_yhot = 0;
	    grSunWCursors[i].cur_shape = NULL;
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 * SunWSetCursor:
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
SunWSetCursor(cursorNum)
int cursorNum;		/* The cursor number as defined in the display
			 * styles file.
			 */
{
    static struct cursor nullCursor;
    static bool haveNull = FALSE;
    extern int sunWCursorFunc();
    struct cursor *cursor;

    if (cursorNum < 0) {
	if (!haveNull) {
	    nullCursor.cur_shape = mem_create(0, 0, 1);
	    nullCursor.cur_xhot = nullCursor.cur_yhot = 0;
	    nullCursor.cur_function = PIX_SRC;
	    haveNull = TRUE;
	}
	cursor = &nullCursor;
    } else {
	cursor = &grSunWCursors[cursorNum];
	grSunWCurCursor = cursorNum;
    }

    if (grSunWCursorOn && cursor != NULL && cursor->cur_shape != NULL)
	WindSearch(NULL, NULL, NULL, sunWCursorFunc, (ClientData) cursor);
}

/* Proc for each window, load the new cursor into each */
int
sunWCursorFunc(w, cdata)
    Window *w;
    ClientData cdata;
{
    grSunWRec *grdata;
    struct cursor *cursor;

    grdata = (grSunWRec *) w->w_grdata;
    ASSERT(grdata != NULL, "SunWSetCursor");
    cursor = (struct cursor *) cdata;
    ASSERT(cursor != NULL, "SunWSetCursor");

    win_setcursor(grdata->gr_fd, cursor);
    pw_putattributes(grdata->gr_pw, &grBitPlaneMask);

    return 0;
}
#endif 	sun
