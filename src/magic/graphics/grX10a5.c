/* grXa5.c -
 *
 *	Manipulate the programable cursor on the graphics display.
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 */

/* X10 driver "a" (from Lawrence Livermore National Labs) */


#ifndef lint
static char rcsid[]="$Header: grX10a5.c,v 6.0 90/08/28 18:41:30 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "hash.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"	    
#include "graphicsInt.h"
#include "grX10aInt.h"

/* imports from grX1.c
 */
extern struct xstate grCurrent;
extern int grPixels[];
extern int grTiles[];
extern DisplayHeight, DisplayWidth;

extern GrGlyphs *grCursorGlyphs;	/* our programmable cursors */
extern HashTable	grX10WindowTable;

/* locals */

Cursor grCursors[ MAX_CURSORS ];


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

GrXDrawGlyph (gl, p)
    GrGlyph *gl;		/* A single glyph */
    Point *p;			/* screen pos of lower left corner */
{
    Rect bBox;
    bool anyObscure;
    LinkedRect *ob;
    int pixelcolor[1];
    
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

    if ((!anyObscure) && (GEO_SURROUND(&grCurClip, &bBox)) ) {
	int *pixelp, x, y;

	/* no clipping, try to go quickly */
	pixelp = gl->gr_pixels;
	for (y = 0; y < gl->gr_ysize; y++) {
	    int x1, y1;

	    y1 = grMagicToX( bBox.r_ybot + y );
	    for (x = 0; x < gl->gr_xsize; x++) {
		x1 = bBox.r_xbot + x;
		XLine( grCurrent.window,
		       x1, y1, x1, y1, 1, 1,
		       grPixels[ grStyleTable[*pixelp++].color ], 
		       GXcopy, grCurrent.planes );
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
			XLine( grCurrent.window,
			    startx, grMagicToX( yloc + y ),
			    startx, grMagicToX( yloc + y ), 1, 1,
			    grPixels[ grStyleTable[*pixelp++].color ], 
			    GXcopy, grCurrent.planes );
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
grxDefineCursor(glyphs)
    GrGlyphs *glyphs;
{
    int glyphnum;
    Rect oldClip;

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

    /* enter the glyphs */
    for (glyphnum = 0; glyphnum < glyphs->gr_num; glyphnum++) {
	int *p;
	GrGlyph *g;
	int x, y;
	short colored;
	unsigned short curs[16];
	
	g = glyphs->gr_glyph[glyphnum];
	if ((g->gr_xsize != 16) || (g->gr_ysize != 16)) {
	    TxError("Cursors for the X must be 16 X 16 pixels.\n");
	    return;
	}
	
	/* Perform transposition on the glyph matrix since X displays
	 * the least significant bit on the left hand side.
	 */
	p = &(g->gr_pixels[0]);
	for (y = 0; y < 16; y++) {
	    curs[ 15 - y] = 0;
	    for (x = 0; x < 16; x++) {
		colored = (grStyleTable[*p].color != 0);
		curs[ 15 - y ] = curs[ 15 - y ] | ( colored & 001 ) << x;
		p++;
	    }
	}
	if (strcmp(grDStyleType,"bw") ==0)
	{
	grCursors[ glyphnum ] = XCreateCursor( 16, 16, curs, curs,
	    g->gr_origin.p_x, ( 15 - g->gr_origin.p_y ),
	    BlackPixel,BlackPixel,GXcopy );
	}
	else
	{
	grCursors[ glyphnum ] = XCreateCursor( 16, 16, curs, curs,
	    g->gr_origin.p_x, ( 15 - g->gr_origin.p_y ),
	    grCurrent.pixel, grCurrent.pixel, GXcopy );
	}
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
GrXSetCursor(cursorNum)
int cursorNum;		/* The cursor number as defined in the display
		         * styles file.
		         */
{
    HashEntry	*entry;
    HashSearch	hs;

    grCurrent.cursor = grCursors[cursorNum];
    
    HashStartSearch(&hs);
    while (entry = HashNext(&grX10WindowTable,&hs))
    {
    	 if (HashGetValue(entry))
         	XDefineCursor((Window)entry->h_key.h_ptr,grCurrent.cursor);
    }
}
