/* grX10a3.c -
 *
 * This file contains additional functions to manipulate an X window system
 * color display.  Included here are device-dependent routines to draw and 
 * erase text and draw a grid.
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 */

/* X10 driver "a" (from Lawrence Livermore National Labs) */


#ifndef lint
static char rcsid[]="$Header: grX10a3.c,v 6.0 90/08/28 18:41:27 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "signals.h"
#include "grX10aInt.h"

/* imports from grX1.c
 */
extern struct xstate grCurrent;
extern int grPixels[];
extern int grTiles[];
extern DisplayHeight, DisplayWidth;

/* locals */

static FontInfo *grSmallFont, *grMediumFont, *grLargeFont, *grXLargeFont;



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
grxDrawGrid (r, outline, clip)
    Rect *r;			/* A rectangle that forms the template
			         * for the grid.  Note:  in order to maintain
			         * precision for the grid, the rectangle
			         * coordinates are specified in units of
			         * screen coordinates multiplied by 4096.
			         */
    int outline;		/* the outline style */
    Rect *clip;			/* a clipping rectangle */
{
    int xsize, ysize;
    int x, y, x1, y1;
    int xstart, ystart;

    xsize = r->r_xtop - r->r_xbot;
    ysize = r->r_ytop - r->r_ybot;
    if ((xsize < 3<<12) || (ysize < 3<<12)) {
	return false;
    }
    xstart = r->r_xbot % xsize;
    while (xstart < clip->r_xbot<<12) {
	xstart += xsize;
    }
    ystart = r->r_ybot % ysize;
    while (ystart < clip->r_ybot<<12) {
	ystart += ysize;
    }

    /* Draw a grid using dots at the grid points */
    for (x = xstart; x < (clip->r_xtop+1)<<12; x += xsize) {
	if (SigInterruptPending) {
	    return;
	}
	for (y = ystart; y < (clip->r_ytop+1)<<12; y += ysize) {
	    int xcoord, ycoord;

	    x1 = x >> 12;
	    y1 = grMagicToX( y >> 12 );
	    XLine( grCurrent.window, 
		   x1, y1, x1, y1, 1, 1,
		   grCurrent.pixel, GXcopy,
		   grCurrent.planes );
	}
    }
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
grxLoadFont()
{
    if( ( grSmallFont = XOpenFont( "6x10" ) ) == NULL ) {
	TxError( "%s\n", "grxLoadFont: Unable to load 6x10" );
    };
    if( ( grMediumFont = XOpenFont( "6x13" ) ) == NULL ) {
	TxError( "%s\n", "grxLoadFont: Unable to load 6x13" );
    };
    if( ( grLargeFont = XOpenFont( "8x13" ) ) == NULL ) {
	TxError( "%s\n", "grxLoadFont: Unable to load 8x13" );
    };
    if( ( grXLargeFont = XOpenFont( "9x15" ) ) == NULL ) {
	TxError( "%s\n", "grxLoadFont: Unable to load 9x15" );
    };
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
grxSetCharSize (size)
    int size;		/* Width of characters, in pixels (6 or 8). */
{
    grCurrent.fontSize = size;
    switch (size)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:
	    grCurrent.font = grSmallFont->id;
	    break;
	case GR_TEXT_MEDIUM:
	    grCurrent.font = grMediumFont->id;
	    break;
	case GR_TEXT_LARGE:
	    grCurrent.font = grLargeFont->id;
	    break;
	case GR_TEXT_XLARGE:
	    grCurrent.font = grXLargeFont->id;
	    break;
	default:
	    TxError("%s%d\n", "grxSetCharSize: Unknown character size ",
		size );
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 * grxGetCharSize:
 *	Compute the size of a character and related parameters.
 *	This is only used by the routines xPutText and XTextSize.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All parameters are modified, except the first.
 * ----------------------------------------------------------------------------
 */

Void
grxGetCharSize (charSize, width, above, below, space)
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
	case GR_TEXT_SMALL:
	    *width = grSmallFont->width;
	    *above = grSmallFont->baseline;
	    *below = grSmallFont->height - grSmallFont->baseline;
	    *space = CHARPAD;
	    break;
	case GR_TEXT_MEDIUM:
	    *width = grMediumFont->width;
	    *above = grMediumFont->baseline;
	    *below = grMediumFont->height - grMediumFont->baseline;
	    *space = CHARPAD;
	    break;
	case GR_TEXT_LARGE:
	    *width = grLargeFont->width;
	    *above = grLargeFont->baseline;
	    *below = grLargeFont->height - grLargeFont->baseline;
	    *space = CHARPAD;
	    break;
	case GR_TEXT_XLARGE:
	    *width = grXLargeFont->width;
	    *above = grXLargeFont->baseline;
	    *below = grXLargeFont->height - grXLargeFont->baseline;
	    *space = CHARPAD;
	    break;
	default:
	    TxError("%s%d\n", "grxGetCharSize: Unknown character size ",
		charSize );
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
GrXTextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    r->r_xbot = 0;
    r->r_ybot = 0;

    switch (size)
    {
	case GR_TEXT_DEFAULT:
	case GR_TEXT_SMALL:
	    r->r_xtop = strlen( text ) * (grSmallFont->width + CHARPAD);
	    r->r_ytop = grSmallFont->height;
	    break;
	case GR_TEXT_MEDIUM:
	    r->r_xtop = strlen( text ) * (grMediumFont->width + CHARPAD);
	    r->r_ytop = grMediumFont->height;
	    break;
	case GR_TEXT_LARGE:
	    r->r_xtop = strlen( text ) * (grLargeFont->width + CHARPAD);
	    r->r_ytop = grLargeFont->height;
	    break;
	case GR_TEXT_XLARGE:
	    r->r_xtop = strlen( text ) * (grXLargeFont->width + CHARPAD);
	    r->r_ytop = grXLargeFont->height;
	    break;
	default:
	    TxError("%s%d\n", "GrXTextSize: Unknown character size ",
		size );
	    break;
    }
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
 * Note: Can't read a pixel number from the screen in X.  Return the
 *	color 0, the background color.
 *
 * ----------------------------------------------------------------------------
 */

int
GrXReadPixel (w, x, y)
    MagicWindow *w;
    int x,y;		/* the location of a pixel in screen coords */
{
    TxError( "%s\n%s\n",
	"To select a color to edit, point to the Color window and type",
	":color nn, where nn is the color number in octal." );
    return( 0 );
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
GrXBitBlt(r, p)
    Rect *r;
    Point *p;
{
    XCopyArea( grCurrent.window,
	       r->r_xbot, r->r_ybot, p->p_x, p->p_y, 
	       r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1,
	       GXcopy, grCurrent.planes );
}


#define GR_DISP_NONE	0
#define	GR_DISP_ALL	1
#define GR_DISP_PARTIAL	2

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
grxPutText (text, pos, clip, obscure)
    char *text;			/* The text to be drawn. */
    Point *pos;			/* A point located at the leftmost point of
				 * the baseline for this string.
				 */
    Rect *clip;			/* A rectangle to clip against */
    LinkedRect *obscure;	/* A list of obscuring rectangles */

{
    Rect size, location;
    Rect overlap;
    Rect widthHeight;
    LinkedRect *ob;
    int width, height;
    void grGeoSub();

    GrXTextSize( text, grCurrent.fontSize, &widthHeight );
    width = widthHeight.r_xtop;
    height = widthHeight.r_ytop;

    location.r_xbot = pos->p_x + 0;
    location.r_xtop = pos->p_x + width;
    location.r_ybot = pos->p_y;
    location.r_ytop = pos->p_y + height;

    /* erase parts of the bitmap that are obscured */
    for (ob = obscure; ob != NULL; ob = ob->r_next)
    {
	if (GEO_TOUCH(&ob->r_r, &location))
	{
	    overlap = location;
	    GeoClip(&overlap, &ob->r_r);
	    grGeoSub(&location, &overlap);
	}
    }
 
    overlap = location;
    GeoClip(&overlap, clip);

    /* copy the text to the color screen */
    if ((overlap.r_xbot < overlap.r_xtop)&&(overlap.r_ybot <= overlap.r_ytop))
    {
	int numberOfCharsShowing;
	int width, above, below, space;
	int startx, endx, paddedWidth, halfPaddedWidth;

	grxGetCharSize (grCurrent.fontSize, &width, &above, &below, &space);
	paddedWidth = width + space;
	halfPaddedWidth = paddedWidth / 2;
	for(startx = pos->p_x;
		(startx + halfPaddedWidth) < overlap.r_xbot;
		    startx = startx + paddedWidth, text++);
	numberOfCharsShowing = 0;
	for(endx = startx + halfPaddedWidth;
		endx < overlap.r_xtop;
		    endx = endx + paddedWidth, numberOfCharsShowing += 1);
	XTextMaskPad( grCurrent.window,
		startx, grMagicToX( pos->p_y + height ),
		text, numberOfCharsShowing,
		grCurrent.font, CHARPAD, 0,
		grCurrent.pixel,
		GXcopy,
		grCurrent.planes );
    }
}


/* grGeoSub:
 *	return the tallest sub-rectangle of r not obscured by area
 *	area must be within r.
 */

void
grGeoSub(r, area)
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
