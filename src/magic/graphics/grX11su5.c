/* grX11su5.c -
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
 *	Manipulate the programable cursor on the graphics display.
 *
 */

#include <stdio.h>
#include "magic.h"
#include "hash.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grX11conf.h"
#include <X11/Xlib.h>
#include "grX11Int.h"

extern GrGlyphs *grCursorGlyphs;
extern HashTable	grX11WindowTable;
extern XColor colors[];
/* locals */

Cursor grCursors[MAX_CURSORS];


/*
 * ----------------------------------------------------------------------------
 * GrXDrawGlyph --
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

GrX11DrawGlyph (gl, p)
    GrGlyph *gl;		/* A single glyph */
    Point *p;			/* screen pos of lower left corner */
{
    Rect bBox;
    bool anyObscure;
    LinkedRect *ob;
    
    GR_CHECK_LOCK();
    bBox.r_ll = *p;
    bBox.r_xtop = p->p_x + gl->gr_xsize - 1;
    bBox.r_ytop = p->p_y + gl->gr_ysize - 1;

    anyObscure = false;
    for (ob = grCurObscure; ob != NULL; ob = ob->r_next) {
	if (GEO_TOUCH( &(ob->r_r), &bBox)) {
	    anyObscure = true;
	    break;
	}
    }

    XSetPlaneMask(grXdpy, grGCGlyph, grPlanes[127]);
    if ((!anyObscure) && (GEO_SURROUND(&grCurClip, &bBox)) ) {
	int *pixelp, x, y;

	/* no clipping, try to go quickly */
	pixelp = gl->gr_pixels;
	for (y = 0; y < gl->gr_ysize; y++) {
	    int x1, y1;

	    y1 = grMagicToX(bBox.r_ybot + y);
	    for (x = 0; x < gl->gr_xsize; x++) {
		int color;
	        color = grStyleTable[*pixelp++].color;
		x1 = bBox.r_xbot + x;
		XSetForeground(grXdpy, grGCGlyph, grPixels[color]);
		XDrawPoint(grXdpy, grCurrent.window, grGCGlyph, x1, y1);
	    }
	}
    } else {
	/* do pixel by pixel clipping */
	int y, yloc;

	yloc = bBox.r_ybot;
	for (y = 0; y < gl->gr_ysize; y++) {
	    int startx, endx;
	    if ( (yloc <= grCurClip.r_ytop) && (yloc >= grCurClip.r_ybot) ) {
		int laststartx;
		laststartx = bBox.r_xbot - 1;
		for (startx = bBox.r_xbot; startx <= bBox.r_xtop; 
			startx = endx + 1) {
		    int *pixelp;

		    startx = MAX(startx, grCurClip.r_xbot);
		    endx = MIN(bBox.r_xtop, grCurClip.r_xtop);

		    if (anyObscure) {
			for (ob = grCurObscure; ob != NULL; ob = ob->r_next) {
			    if ( (ob->r_r.r_ybot <= yloc) && 
				 (ob->r_r.r_ytop >= yloc) ) {
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
			    (startx - bBox.r_xbot)]);
		    for ( ; startx <= endx; startx++) {
			int color;
			color = grStyleTable[*pixelp++].color;
			XSetForeground(grXdpy, grGCGlyph, grPixels[color]);
			XDrawPoint(grXdpy, grCurrent.window, grGCGlyph,
				  startx, grMagicToX(yloc));
		    }
		    startx = endx + 1;
		}
	    }
	    yloc++;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 * grxDefineCursor:
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
grx11DefineCursor(glyphs)
    GrGlyphs *glyphs;
{
    int glyphnum;
    Rect oldClip;
    XColor curcolor;
    Pixmap source,mask;

    if (glyphs->gr_num <= 0) return;

    if (glyphs->gr_num > MAX_CURSORS)
    {
	TxError("X only has room for %d cursors\n", MAX_CURSORS);
	return;
    }

    /* expand clipping amount for off-screen access on the X */
    GrLock(GR_LOCK_SCREEN, FALSE);
    oldClip = grCurClip;
    grCurClip = GrScreenRect;
    grCurClip.r_ytop += 16;
    
    
    /* what color should the cursor be?  The following makes it the
       opposite of what the background "mostly" is.
    */
    curcolor.pixel = grPixels[grStyleTable[ 0 ].color];
    XQueryColor(grXdpy,grXcmap,&curcolor); 

    if ((long)curcolor.red+(long)curcolor.green+(long)curcolor.blue > 0x18000)
    {
    	 curcolor.pixel = BlackPixel(grXdpy,grXscrn);
	 curcolor.red = 0;
	 curcolor.green = 0;
	 curcolor.blue = 0;
	 curcolor.flags |= DoRed|DoGreen|DoBlue;
    }
    else
    {
    	 curcolor.pixel = WhitePixel(grXdpy,grXscrn);
	 curcolor.red = 65535;
	 curcolor.green = 65535;
	 curcolor.blue = 65535;
	 curcolor.flags |= DoRed|DoGreen|DoBlue;
    }
    
    /* enter the glyphs */
    for (glyphnum = 0; glyphnum < glyphs->gr_num; glyphnum++) {
	int *p;
	GrGlyph *g;
	int x, y;
	unsigned char curs[32];
	
	g = glyphs->gr_glyph[glyphnum];
	if ((g->gr_xsize != 16) || (g->gr_ysize != 16)) {
	    TxError("Cursors for the X must be 16 X 16 pixels.\n");
	    return;
	}
	
	/* Perform transposition on the glyph matrix since X displays
	 * the least significant bit on the left hand side.
	 */
	p = &(g->gr_pixels[0]);
	for (y = 0; y < 32; y += 2) {
	    int i;

	    curs[i = 31 - y - 1] = 0;
	    for (x = 0; x < 8; x++) 
	    {
		if (grStyleTable[*p].color != 0)
		{
		     curs[i] |= 1 << x; 
		}
		p++;
	    }
	    curs[i += 1] = 0;
	    for (x = 0; x < 8; x++) 
	    {
		if (grStyleTable[*p].color != 0)
		{
		     curs[i] |= 1 << x; 
		}
		p++;
	    }
	}
	source = XCreateBitmapFromData(grXdpy, DefaultRootWindow(grXdpy),
				       curs, 16, 16);
	mask = XCreateBitmapFromData(grXdpy, DefaultRootWindow(grXdpy),
				     curs, 16, 16);
	grCursors[glyphnum] = XCreatePixmapCursor(grXdpy, source, mask,
						  &curcolor, &curcolor,
						  g->gr_origin.p_x,
						  (15 - g->gr_origin.p_y));
    }

    /* Restore clipping */
    grCurClip = oldClip;
    GrUnlock(GR_LOCK_SCREEN);
}


/*
 * ----------------------------------------------------------------------------
 * GrXSetCursor:
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
GrX11SetCursor(cursorNum)
int cursorNum;		/* The cursor number as defined in the display
		         * styles file.
		         */
{
    HashEntry	*entry;
    HashSearch	hs;

    grCurrent.cursor = grCursors[cursorNum];
    
    HashStartSearch(&hs);
    while (entry = HashNext(&grX11WindowTable,&hs))
    {
    	 if (HashGetValue(entry))
         	XDefineCursor(grXdpy,(Window)entry->h_key.h_ptr,grCurrent.cursor);
    }
}
