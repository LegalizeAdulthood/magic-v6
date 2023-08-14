/* grX10a2.c -
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 *
 * This file contains additional functions to manipulate an X
 * color display.  Included here are rectangle drawing and color map
 * loading.
 */


/* X10 driver "a" (from Lawrence Livermore National Labs) */


#ifndef lint
static char rcsid[]="$Header: grX10a2.c,v 6.0 90/08/28 18:41:25 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grX10aInt.h"
#include "hash.h"
#include "tile.h"
#include "database.h"
#include "main.h"

/* imports from grX1.c
 */
extern struct xstate grCurrent;
extern int grxBasePixelMask;
extern int grPixels[];
extern int grTiles[];
extern Pattern grStipples[] [8];

extern int grCurrent_stipple;
extern DisplayHeight, DisplayWidth;

static Pixmap grXStippleTiles[GR_NUM_STIPPLES];	/* Tiles with stipple pat. */
static Pixmap grXStippleInvTiles[GR_NUM_STIPPLES];
				/* Tiles with inverted stipple pat. */
static int grXStippleTileValid[GR_NUM_STIPPLES]; /* Starts out all zeros. */
HashTable	grX10WindowTable;

/*---------------------------------------------------------
 * GrXSetCMap:
 *	XSetCMap outputs new values to the X color map.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The values in the color map are set from the array indicated
 *	by pmap.
 *
 * Errors:		None.
 *
 * Note:
 *	In X, we can only allocate 253 color cells.  Since the
 *	Magic colors 128 - 255 are the same, they all share the
 *	same X pixel id and tile id.  Hence, only 129 colors are
 *	allocated.
 *
 * Additional note:
 *	Some X applications already will have allocated colors
 *	starting at color (pixel value) 0.  This is bad, because
 *	it means that the next available color to us will be some
 *	color greater than 0, so the Magic sense of or-ing bits
 *	in colors won't work correctly.  To avoid this problem,
 *	we always allocate the top 128 colors and remap Magic
 *	colors so they always have the high-order bit set.
 *	This restricts us to only 7 bits of color, but makes
 *	Magic coexist nicely with the rest of X.
 *
 *---------------------------------------------------------
 */

Void
GrXSetCMap (pmap)
    char *pmap;			/* A pointer to 256*3 bytes containing the
				 * new values for the color map.  The first
				 * three values are red, green, and blue
				 * intensities for color 0, and so on.
				 */
{
if (strcmp(grDStyleType ,"bw") == 0)
{
    grPixels[1] = BlackPixel;
    grTiles[1] = XMakeTile(BlackPixel);
    grPixels[0] = WhitePixel;
    grTiles[0] = XMakeTile(WhitePixel);
}
else
{
#define BY_PLANE 1
#define BY_COLOR 2
    int	ncolors=NCOLORS;
    static bool firstCall = true;
    Color colors[NCOLORS];	/* Unique colors used by Magic */
    int planes,planes2;			/* Bit planes we were allocated */
    register char *p;		/* Pointer into the Magic color map. */
    int i, status, lastPixel, pixel;
    short red, green, blue;
    Pixmap lastTile;
    static int pixels[ NCOLORS];/* Pixel id's */
    int alloctype;

    if ( firstCall ) {
        if (strcmp(grCMapType,"6bit") ==0)
        {
	     fprintf(stderr,"Allocating %d planes\n",6);
	     fflush(stderr);
	     status = XGetColorCells(1,1,NPLANES,&planes,&grxBasePixelMask);
	     if (status)
	         status = XGetColorCells( 1, ncolors-(1<<NPLANES), 0, &planes2, 
	    					   &(pixels[(1<<NPLANES)]));
	     alloctype = BY_PLANE;
        }
	else
	{
	     status = XGetColorCells(1,1,7,&planes,&grxBasePixelMask);
	     if (status == NULL)
	     {
	     	  TxError("Couldn't get 7 planes; allocating by color\n");
                  status = XGetColorCells( 1, ncolors, 0, &planes, pixels );
	          grxBasePixelMask = 0;
	          alloctype = BY_COLOR;
	     }
	     else
	     {
	     	  alloctype = BY_PLANE;
	     }
	}
	if( status == NULL ){
	        TxError( "%s\n", "grX2.GrXSetCMap: Failed to get color cells." );
		return false;
	}
    }
    else
    {
         for( i = 0; i < ncolors; i++ ) XFreePixmap( grTiles[ i ] );
    }

    /* Define colors
     */
    p = pmap;
    if (alloctype == BY_PLANE)
    {
         if ((planes & 0x1) ==0)
	 {
	      TxError("Warning: allocated planes not contiguous\n");
	 }
	 for( i = 0; i < ncolors-(1<<NPLANES); i++) 
	 {
	     pixels[i] = i | grxBasePixelMask;
         }
    }
    for( i = 0; i < ncolors; i++) {
	     colors[ i ].pixel = pixels[ i ];
	     colors[ i ].red = *p++ << 8;
	     colors[ i ].green = *p++ << 8;
	     colors[ i ].blue = *p++ << 8;
    }
    XStoreColors( ncolors, colors );

    /* Assign to grPixels[] and grTiles[]
     */
    if (alloctype == BY_PLANE)
    {
        for( i = 0; i < 256; i++ ) {
	    if( i < ncolors ) {
		grPixels[ i ] = pixels[ i ];
	        grTiles[ i ] = XMakeTile( pixels [ i ] );
	        if( grTiles[ i ] == NULL ) {
		    TxError( "%s\n", "grX2.GrXSetCMap: Failed to obtain tiles.");
		    return false;
	        };
	        if( i == ( ncolors  ) ) {
		     lastPixel = grPixels[ i ];
		     lastTile = grTiles[ i ];
		     red = colors[ i ].red >> 8;
		     green = colors[ i ].green >> 8;
		     blue = colors[ i ].blue >> 8;
	        };
	    } else {
	        grPixels[ i ] = lastPixel;
	        grTiles[ i ] = lastTile;
	        if( *p++ != red  || *p++ != blue || *p++ != green ) {
		    TxError( "%s\n", "grX2.GrXSetCMap: color map mixup." );
		    return false;
	        }
	    }
        }
    }
    else
    {
        for( i = 0; i < 256; i++ ) {
	    if( i < ncolors ) {
	        grPixels[ i ] = pixels[ i ];
	        grTiles[ i ] = XMakeTile( pixels [ i ] );
	        if( grTiles[ i ] == NULL ) {
		    TxError( "%s\n", "grX2.GrXSetCMap: Failed to obtain tiles.");
		    return false;
	        };
	        if( i == ( ncolors - 1 ) ) {
		     lastPixel = grPixels[ i ];
		     lastTile = grTiles[ i ];
		     red = colors[ i ].red >> 8;
		     green = colors[ i ].green >> 8;
		     blue = colors[ i ].blue >> 8;
	        };
	    } else {
	        grPixels[ i ] = lastPixel;
	        grTiles[ i ] = lastTile;
	        if( *p++ != red  || *p++ != blue || *p++ != green ) {
		    TxError( "%s\n", "grX2.GrXSetCMap: color map mixup." );
		    return false;
	        }
	    }
        }
    }

    firstCall = false;
}
}


/*---------------------------------------------------------
 * grxDrawLine:
 *	This routine draws a line.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Draw a line for (x1, y1) to (x2, y2) inclusive.
 *	X window coordinates start from the UPPER left corner, 
 *	so the Magic y-coordinates need to be inverted using the
 *	macro grMagicToX().
 *---------------------------------------------------------
 */
Void
grxDrawLine (x1, y1, x2, y2)
    int x1, y1;			/* Screen coordinates of first point. */
    int x2, y2;			/* Screen coordinates of second point. */
{
    Vertex vlist[2];

    vlist[0].x = x1;
    vlist[0].y = grMagicToX( y1 );
    vlist[1].x = x2;
    vlist[1].y = grMagicToX( y2 );
    vlist[0].flags = vlist[1].flags = 0;
    XDrawDashed( grCurrent.window,
		    vlist,
		    2,
		    BRUSHWIDTH,
		    BRUSHHEIGHT,
		    grCurrent.pixel,
		    grCurrent.pattern,
		    GXcopy,
		    grCurrent.planes );
}


/*---------------------------------------------------------
 * grXMakeStippleTile:
 *	Turn Magic's stipple pattern into an X tile.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Fills in an entry in grXStippleTileValid[] and
 *	in grXStippleTiles.
 *---------------------------------------------------------
 */

void
grXMakeStippleTile(stipnum)
    int stipnum;		/* Stipple number. */
{
    Bitmap stipbits;
    Pixmap stipmap;
    short data[16];
    int x, y;
    int row;

    for (y = 0; y < 8; y++)
    {
	row = grStippleTable[stipnum][y];
	data[y] = (row & 0xFF) | ((row & 0xFF) << 8);
    }
    for (y = 8; y < 16; y++)
    {
	data[y] = data[y - 8];
    }

    stipbits = XStoreBitmap(16, 16, data);
    if (strcmp(grDStyleType,"bw") == 0)
    {
    	 grXStippleTiles[stipnum] =
              XMakePixmap(stipbits, WhitePixel, BlackPixel);
         grXStippleInvTiles[stipnum] =
              XMakePixmap(stipbits, BlackPixel, WhitePixel);
    }
    else
    {
    	 grXStippleTiles[stipnum] = 
		XMakePixmap(stipbits, 127, 0);
    	 grXStippleInvTiles[stipnum] = 
	  	XMakePixmap(stipbits, 0, 127);
    }
    grXStippleTileValid[stipnum] = 1;
    XFreeBitmap(stipbits);
}



/*---------------------------------------------------------
 * grxFillRect:
 *	This routine draws a solid rectangle.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A solid rectangle is drawn in the current color and
 *	using the current write mask.  This is an internal
 *	routine used by grFastBox.
 *---------------------------------------------------------
 */

Void
grxFillRect (r)
    register Rect *r;	/* Address of a rectangle in screen
			 * coordinates.
			 */
{
    if( grCurrent_stipple == 0 ) {
	/* Just draw a filled rectangle */
	XTileFill( grCurrent.window,
	    r->r_xbot, grMagicToX( r->r_ytop ),
	    r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
	    grCurrent.tile,
	    0, /* grCurrent.clipmask,*/
	    GXcopy,
	    grCurrent.planes );
    } else {
	int setones, setzeros;

	/* Create our stipple tile if we don't have one already. */
	if (grXStippleTileValid[grCurrent_stipple] == 0) 
	    grXMakeStippleTile(grCurrent_stipple);
	setones = (grCurrent.planes & grCurrent.pixel);	    /* Bits to set. */
	setzeros = (grCurrent.planes & (~grCurrent.pixel)); /* Bits to clr. */
	if((setones & setzeros) != 0)
	    TxError("grxFillRect botched: setones & setzeros != 0\n");

	/* First, fill in the 1's */
	if (setones != 0)
	{
	    XTileFill( grCurrent.window,
		r->r_xbot, grMagicToX( r->r_ytop ),
		r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
		grXStippleTiles[grCurrent_stipple],
		0, /* grCurrent.clipmask,*/
		GXor,		/* Set bits where stipple has a one. */
		setones);
	}

	/* Now, fill in the 0's */
	if (setzeros != 0)
	{
	    XTileFill( grCurrent.window,
		r->r_xbot, grMagicToX( r->r_ytop ),
		r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
		grXStippleInvTiles[grCurrent_stipple],
		0, /* grCurrent.clipmask,*/
		GXand, /* Clear bits where stipple has a one. */
		setzeros);
	}
    }
}

