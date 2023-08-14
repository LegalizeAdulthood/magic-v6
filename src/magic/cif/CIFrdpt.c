/* CIFreadpaint.c -
 *
 *	This file contains more routines to parse CIF files.  In
 *	particular, it contains the routines to handle paint,
 *	including rectangles, wires, flashes, and polygons.
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
static char rcsid[] = "$Header: CIFrdpt.c,v 6.0 90/08/28 18:05:14 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "main.h"
#include "CIFint.h"
#include "CIFread.h"


/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseBox --
 *
 * 	This procedure parses a CIF box command.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	A box is added to the CIF information for this cell.  The
 *	box better not have corners that fall on half-unit boundaries.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseBox()
{
    int		length;
    int		width;
    Point	center;
    Point	direction;
    int		halflength;
    int		halfwidth;
    Rect	rectangle, r2;
    
    /*	Take the 'B'. */

    TAKE();
    if (cifReadPlane == NULL)
    {
	CIFSkipToSemi();
	return FALSE;
    }
    if (!CIFParseInteger(&length))
    {
	CIFReadError("box, but no length; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    halflength = (length * cifReadScale1)/(2*cifReadScale2);
    if (!CIFParseInteger(&width))
    {
	CIFReadError("box, but no width; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    halfwidth = (width * cifReadScale1)/(2*cifReadScale2);
    if (!CIFParsePoint(&center))
    {
	CIFReadError("box, but no center; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    rectangle.r_xbot = - (halflength);
    rectangle.r_ybot = - (halfwidth);
    rectangle.r_xtop = (halflength);
    rectangle.r_ytop = (halfwidth);

   
    /*	Optional direction vector:  have to build transform to do rotate. */

    if (CIFParseSInteger(&direction.p_x))
    {
	if (!CIFParseSInteger(&direction.p_y))
	{
	    CIFReadError("box, direction botched; box ignored.\n");
	    CIFSkipToSemi();
	    return FALSE;
	}
	GeoTransRect(CIFDirectionToTrans(&direction), &rectangle , &r2);
    }
    else r2 = rectangle;

    /* Offset by center only now that rotation is complete. */

    r2.r_xbot += center.p_x;
    r2.r_ybot += center.p_y;
    r2.r_xtop += center.p_x;
    r2.r_ytop += center.p_y;

    DBPaintPlane(cifReadPlane, &r2, CIFPaintTable, (PaintUndoInfo *) NULL);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseFlash --
 *
 * 	This routine parses and processes a roundflash command.  The syntax is:
 *	roundflash ::= R diameter center
 *
 *	We approximate a roundflash by a box.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Paint is added to the current CIF plane.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseFlash()
{
    int		diameter;
    Point	center;
    int		radius;
    Rect	rectangle;
    
    /* Take the 'R'. */

    TAKE();
    if (cifReadPlane == NULL)
    {
	CIFSkipToSemi();
	return FALSE;
    }
    if (!CIFParseInteger(&diameter))
    {
	CIFReadError("roundflash, but no diameter; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    radius = (diameter * cifReadScale1)/cifReadScale2;
    if (!CIFParsePoint(&center))
    {
	CIFReadError("roundflash, but no center; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    rectangle.r_xbot = (center.p_x - radius)/2;
    rectangle.r_ybot = (center.p_y - radius)/2;
    rectangle.r_xtop = (center.p_x + radius)/2;
    rectangle.r_ytop = (center.p_y + radius)/2;
    DBPaintPlane(cifReadPlane, &rectangle, CIFPaintTable,
	    (PaintUndoInfo *) NULL);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseWire --
 *
 * 	This procedure parses CIF wire commands, and adds paint
 *	to the current CIF cell.  A wire command consists of
 *	an integer width, then a path.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	The current CIF planes are modified.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseWire()
{
    int		width;
    CIFPath	*pathheadp;
    CIFPath	*pathp;
    CIFPath	*previousp;
    int		xmin;
    int		ymin;
    int		xmax;
    int		ymax;
    int		temp;
    Rect	segment;

    /* Take the 'W'. */

    TAKE();
    if (cifReadPlane == NULL)
    {
	CIFSkipToSemi();
	return FALSE;
    }
    if (!CIFParseInteger(&width))
    {
	CIFReadError("wire, but no width; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    width = (width * cifReadScale1)/cifReadScale2;
    if (!CIFParsePath(&pathheadp))
    {
	CIFReadError("wire, but improper path; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    previousp = pathp = pathheadp;
    while (pathp != NULL) {
	xmin = previousp->cifp_x;
	xmax = pathp->cifp_x;
	ymin = previousp->cifp_y;
	ymax = pathp->cifp_y;

	/* Sort the points. */

	if (xmax < xmin)
	{
	    temp = xmin;
	    xmin = xmax; 
	    xmax = temp;
	}
	if (ymax < ymin)
	{
	    temp = ymin;
	    ymin = ymax; 
	    ymax = temp;
	}

	/* Build the wire segment. */

	segment.r_xbot = xmin - width/2;
	segment.r_ybot = ymin - width/2;
	segment.r_xtop = xmax + width/2;
	segment.r_ytop = ymax + width/2;
	DBPaintPlane(cifReadPlane, &segment, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	previousp = pathp;
	pathp = pathp->cifp_next;
    }
    CIFFreePath(pathheadp);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseLayer --
 *
 * 	This procedure parses layer changes.  The syntax is:
 *	layer ::= L { blank } processchar layerchars
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Switches the CIF plane where paint is being saved.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseLayer()
{
#define MAXCHARS 4
    char	name[MAXCHARS+1];
    char	c;
    int		i;
    TileType	type;

    /* Take the 'L'. */

    TAKE();
    CIFSkipBlanks();

    /* Get the layer name. */

    for (i=0; i<=MAXCHARS; i++)
    {
	c = PEEK();
	if (isdigit(c) || isupper(c))
	    name[i] = TAKE();
	else break;
    }
    name[i] = '\0';

    /* Set current plane for use by the routines that parse geometric
     * elements.
     */
    
    type = CIFReadNameToType(name, FALSE);
    if (type < 0)
    {
	cifReadPlane = NULL;
	cifCurLabelType = TT_SPACE;
	CIFReadError("layer %s isn't known in the current style.\n",
		name);
    } else {
	cifCurLabelType = cifCurReadStyle->crs_labelLayer[type];
	cifReadPlane = cifCurReadPlanes[type];
    }

    CIFSkipToSemi();
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParsePoly --
 *
 * 	This procedure reads and processes a polygon command.  The syntax is:
 *	polygon ::= path
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Paint is added to the current CIF plane.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParsePoly()
{
    CIFPath	*pathheadp;
    LinkedRect	*rectp;

    /* Take the 'P'. */

    TAKE();
    if (cifReadPlane == NULL)
    {
	CIFSkipToSemi();
	return FALSE;
    }
    if (!CIFParsePath(&pathheadp)) 
    {
	CIFReadError("polygon, but improper path; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }

    /* Convert the polygon to rectangles. */

    rectp = CIFPolyToRects(pathheadp);
    CIFFreePath(pathheadp);
    if (rectp == NULL)
    {
	CIFReadError("bad polygon; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    for (; rectp != NULL ; rectp = rectp->r_next)
    {
	DBPaintPlane(cifReadPlane, &rectp->r_r, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	freeMagic((char *) rectp);
    }
    return TRUE;
}
