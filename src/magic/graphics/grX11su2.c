/* grX11su2.c -
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
 * This file contains additional functions to manipulate an X
 * color display.  Included here are rectangle drawing and color map
 * loading.
 */

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grX11conf.h"
#include <X11/Xlib.h>
#include "grX11Int.h"

unsigned long grPixels[128];
unsigned long grPlanes[128];
XColor colors[256];	/* Unique colors used by Magic */
unsigned long grCompleteMask;
Colormap grXcmap=NULL;

/* disgusting machine-dependent constants - see below */
#ifdef macII
#define X_COLORMAP_BASE		128
#define X_COLORMAP_RESERVED	4
#else
#define X_COLORMAP_BASE		0
#define X_COLORMAP_RESERVED	2
#endif


/*---------------------------------------------------------
 * GrXSetCMap:  I've seen about 6 versions of this procedure now,
 	all of them flawed in some way or another.  I expect this
	one to be no different...

	Anyway, the idea here is to first try allocating the required
	planes out of the default colormap.  This is the kindest,
	gentlest thing to do because it doesn't cause all the other
	windows to go technicolor when the cursor is in a magic window.
	If this fails, we go ahead and allocate a colormap specifically
	for magic.  The problem now is using this colormap in such
	a way that the other windows' colors get mangled the least.
	Unfortunately, doing this is X-server dependent.  This is where
	the constants above come in.  X_COLORMAP_BASE indicates
	which part of the colormap (assuming the number of planes
	required is less than the number of planes in the display)
	to fill in the colors magic requires.  X_COLORMAP_RESERVED
	tells how many high-end colors the server won't let us touch;
	if we even try to query these colors, we get an X error.
	If, starting at X_COLORMAP_BASE, the number of colors required
	would push us into the top X_COLORMAP_RESERVED colors, then
	we won't be able to set all the colors the user wanted us
	to set.  The top colors will remain identical to those
	in the default colormap.
	
	There are clearly some shortcomings in this approach.  For
	example, a server might reserve colors at the bottom of
	the colormap.  Since I haven't seen such a server yet
	and I'm really sick of X11, this eventuality is not supported.
	
	The best way to use Magic under X is to pick a 	number of 
	planes small enough to fix in the default colormap, and not
	to waste random colors on things like xterms, xclocks and xbiffs.
	Do you really need a yellow and black xterm and a red xclock?
	Think about it.
 *	
 *
 * Results:	None.
 *
 * Side Effects:
 *	The values in the color map are set from the array indicated
 *	by pmap. X color cells are allocated if this display has
 *	more than 1 plane.
 *
 * Errors:		None.
 *
 *---------------------------------------------------------
 */

Void
GrX11SetCMap (pmap)
    char *pmap;			/* A pointer to 256*3 bytes containing the
				 * new values for the color map.  The first
				 * three values are red, green, and blue
				 * intensities for color 0, and so on.
				 */
{
    char *p;	
    int i,j;
    static unsigned long planes[8];
    int status;
    int	basepixel;
    static int firstCall = 1;
    static int planeCount;	/* how many planes we want to allocate */
    static int colorCount;	/* how many colors we want to allocate */
    static int realColors;	/* how many colors we actually do allocate */

    if (firstCall)
    {
         firstCall = 0;
	 planeCount = grCurrent.depth;
	 colorCount = 1 << planeCount;
	 realColors = colorCount;
	 grXcmap = XDefaultColormap(grXdpy,grXscrn);

         if(grCurrent.depth) 
	 {
	     status= XAllocColorCells(grXdpy,grXcmap,True,planes,planeCount,
	     							&basepixel,1); 
	     if (status == 0) 
	     /* ok, we tried to be nice; now lets whack the default colormap
	        and put in one of our own.
	     */
	     {
		  int actualColors = 1 << DefaultDepth(grXdpy,grXscrn);
		  int usableColors = actualColors - X_COLORMAP_RESERVED;

	          TxPrintf("Unable to allocate %d planes in default colormap; making a new one.\n",planeCount);
		  grXcmap = XCreateColormap(grXdpy,grCurrent.window,
		  				DefaultVisual(grXdpy,
							grXscrn),
							AllocAll);

        	  for (j=0; j < planeCount; j++) planes[j] = 1<<j;
		  status = 1;
        	  basepixel  = X_COLORMAP_BASE;
 		  for (i=0; i < usableColors; i++) colors[i].pixel = i;
	    	  XQueryColors(grXdpy, DefaultColormap(grXdpy,
			grXscrn), colors, usableColors);
		  XStoreColors(grXdpy, grXcmap, colors, usableColors);
		  realColors = (basepixel+colorCount > usableColors)?
		  				usableColors-basepixel:
						colorCount;
		  if (realColors != colorCount)
		  {
	               TxPrintf("Only %d contigous colors were available.\n",realColors);
		  }
	     }
	     			
	     if (grXcmap == 0 || status ==0) 
	     {
	         TxError( "X11 setup: Unable to allocate %d planes\n",planeCount);
	         MainExit(1);
	     }

	     /* grCompleteMask is a mask of all the planes not used by this 
	        technology.  It is OR'd in with the mask that magic supplies
	        to ensure that unused bits of the pixel are cleared.
	   
	        grNumBitPlanes is set when the dstyles file is read in; for
	        the following to work, the dstyles file must be read before
	        the colormap.  I changed main.c so that this was the case.
	        If you aren't getting mutually exclusive colors, this may
	        be why.
	   					-dcs
	     */
	     grCompleteMask = 0;
	     if (grNumBitPlanes > 7 || grNumBitPlanes <1)
	     {
	          TxError("display_styles must be 0 < p < 8 under X11");
	          GrX11Close();
	          MainExit(1);
	     }
	     if (grNumBitPlanes > planeCount)
	     {
	          TxError("Your display_styles uses %d planes but you only have %d\n",grNumBitPlanes,planeCount);
	          GrX11Close();
	          MainExit(1);
	     }
	     for (i=0;i != grNumBitPlanes;i++)
	     {
	          grCompleteMask |= planes[i];
	     }
	     grCompleteMask = AllPlanes & ~grCompleteMask;

	     for (i=0;i != colorCount;i++)
	     {
	          grPixels[i] = basepixel;
	          grPlanes[i] = grCompleteMask;
	          for (j=0;j != planeCount;j++)  if ( i & (1 <<j))
	          {
	     	       grPixels[i] |= planes[j];
		       grPlanes[i] |= planes[j];
	          } 
	     }
        }
	else
	{
	      grPixels[0] = WhitePixel(grXdpy,grXscrn);
	      grPixels[1] = BlackPixel(grXdpy,grXscrn);
	      grPlanes[0] = 0;
	      grPlanes[1] = AllPlanes;
	}
    }
    if (grCurrent.depth)
    {
	p = pmap;
        for( i = 0; i < realColors; i++) 
	{
             colors[ i ].pixel = grPixels[ i ];
             colors[ i ].red = *p++ << 8;
             colors[ i ].green = *p++ << 8;
             colors[ i ].blue = *p++ << 8;
	     colors[ i ].flags = DoRed|DoGreen|DoBlue;
        }
	XStoreColors(grXdpy, grXcmap, colors, realColors); 
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
 *---------------------------------------------------------
 */
Void
grx11DrawLine (x1, y1, x2, y2)
    int x1, y1;			/* Screen coordinates of first point. */
    int x2, y2;			/* Screen coordinates of second point. */
{
    XDrawLine(grXdpy, grCurrent.window, grGCDraw,
	      x1, grMagicToX(y1), x2, grMagicToX(y2));
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
grx11FillRect (r)
    register Rect *r;	/* Address of a rectangle in screen
			 * coordinates.
			 */
{
    XFillRectangle(grXdpy, grCurrent.window, grGCFill,
		   r->r_xbot, grMagicToX(r->r_ytop),
		   r->r_xtop - r->r_xbot + 1, r->r_ytop - r->r_ybot + 1);
}
