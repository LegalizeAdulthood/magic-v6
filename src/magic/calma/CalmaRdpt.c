/*
 * CalmaReadpaint.c --
 *
 * Input of Calma GDS-II stream format.
 * Processing of paint (paths, boxes, and boundaries) and text.
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
static char rcsid[]="$Header: CalmaRdpt.c,v 6.2 90/09/03 15:29:59 stark Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "utils.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "malloc.h"
#include "tech.h"
#include "cif.h"
#include "CIFint.h"
#include "CIFread.h"
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "textio.h"
#include "calmaInt.h"
#include <sys/types.h>

extern int calmaNonManhattan;

/*
 * ----------------------------------------------------------------------------
 *
 * calmaElementBoundary --
 *
 * Read a polygon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints one or more rectangles into one of the CIF planes.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaElementBoundary()
{
    int dt, layer, ciftype;
    CIFPath *pathheadp;
    LinkedRect *rp;
    Plane *plane;

    /* Skip CALMA_ELFLAGS, CALMA_PLEX */
    calmaSkipSet(calmaElementIgnore);

    /* Read layer and data type */
    if (!calmaReadI2Record(CALMA_LAYER, &layer)
	    || !calmaReadI2Record(CALMA_DATATYPE, &dt))
    {
	calmaReadError("Missing layer or datatype in boundary/box.\n");
	return;
    }

    /* Set current plane */
    ciftype = CIFCalmaLayerToCifLayer(layer, dt);
    if (ciftype < 0)
    {
	plane = NULL;
	calmaLayerError("Unknown layer/datatype in boundary", layer, dt);
    }
    else plane = cifCurReadPlanes[ciftype];

    /* Read the path itself, building up a path structure */
    if (!calmaReadPath(&pathheadp)) 
    {
	calmaReadError("Error while reading path for boundary/box; ignored.\n");
	return;
    }

    /* Convert the polygon to rectangles. */
    rp = CIFPolyToRects(pathheadp);
    CIFFreePath(pathheadp);
    if (rp == NULL)
    {
	calmaReadError("Can't convert boundary/box into rects; ignored.\n");
	return;
    }

    /* Paint the rectangles */
    for (; rp != NULL ; rp = rp->r_next)
    {
	if (plane)
	    DBPaintPlane(plane, &rp->r_r, CIFPaintTable, (PaintUndoInfo *)NULL);
	FREE((char *) rp);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaElementBox --
 *
 * Read a box.
 * This is an optimized version of calmaElementBoundary
 * that handles rectangular polygons.  These polygons each
 * have five vertex points, with the first and last point
 * being the same, and all sides parallel to one of the two
 * coordinate axes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints one rectangle into one of the CIF planes.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaElementBox()
{
    int nbytes, rtype, npoints;
    int dt, layer, ciftype;
    Plane *plane;
    Point p;
    Rect r;

    /* Skip CALMA_ELFLAGS, CALMA_PLEX */
    calmaSkipSet(calmaElementIgnore);

    /* Read layer and data type */
    if (!calmaReadI2Record(CALMA_LAYER, &layer)
	    || !calmaReadI2Record(CALMA_BOXTYPE, &dt))
    {
	calmaReadError("Missing layer or datatype in boundary/box.\n");
	return;
    }

    /* Set current plane */
    ciftype = CIFCalmaLayerToCifLayer(layer, dt);
    if (ciftype < 0)
    {
	calmaLayerError("Unknown layer/datatype in box", layer, dt);
	return;
    }
    else plane = cifCurReadPlanes[ciftype];

    /*
     * Read the path itself.
     * Since it is Manhattan, we can build our rectangle directly.
     */
    r.r_xbot = r.r_ybot = INFINITY;
    r.r_xtop = r.r_ytop = MINFINITY;

    /* Read the record header */
    READRH(nbytes, rtype);
    if (nbytes < 0)
    {
	calmaReadError("EOF when reading box.\n");
	return;
    }
    if (rtype != CALMA_XY)
    {
	calmaUnexpected(CALMA_XY, rtype);
	return;
    }

    /* Read this many points (pairs of four-byte integers) */
    npoints = (nbytes - CALMAHEADERLENGTH) / 8;
    if (npoints != 5)
    {
	calmaReadError("Box doesn't have 5 points.\n");
	(void) calmaSkipBytes(nbytes - CALMAHEADERLENGTH);
	return;
    }
    while (npoints-- > 0)
    {
	READPOINT(&p);
	if (p.p_x < r.r_xbot) r.r_xbot = p.p_x;
	if (p.p_y < r.r_ybot) r.r_ybot = p.p_y;
	if (p.p_x > r.r_xtop) r.r_xtop = p.p_x;
	if (p.p_y > r.r_ytop) r.r_ytop = p.p_y;
    }

    /* Paint the rectangle */
    DBPaintPlane(plane, &r, CIFPaintTable, (PaintUndoInfo *)NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaElementPath --
 *
 * Read a centerline wire.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May paint rectangles into CIF planes.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaElementPath()
{
    static int ignore[] = { CALMA_BGNEXTN, CALMA_ENDEXTN, -1 };
    int nbytes, rtype;
    int layer, dt, width, halfwidth, pathtype, ciftype;
    int xmin, ymin, xmax, ymax, temp;
    CIFPath *pathheadp, *pathp, *previousp;
    Rect segment;
    Plane *plane;
    int first,last;

    /* Skip CALMA_ELFLAGS, CALMA_PLEX */
    calmaSkipSet(calmaElementIgnore);

    /* Grab layer and datatype */
    if (!calmaReadI2Record(CALMA_LAYER, &layer)) return;
    if (!calmaReadI2Record(CALMA_DATATYPE, &dt)) return;

    /* Describes the shape of the ends of the path */
    pathtype = CALMAPATH_SQUAREFLUSH;
    PEEKRH(nbytes, rtype);
    if (nbytes > 0 && rtype == CALMA_PATHTYPE)
	if (!calmaReadI2Record(CALMA_PATHTYPE, &pathtype)) return;

    if (pathtype == CALMAPATH_ROUND)
	calmaReadError("Warning: pathtype %d unsupported.\n", pathtype);

    /*
     * Width of this path.
     * Allow zero-width paths; we will ignore them later.
     */
    width = 0;
    PEEKRH(nbytes, rtype) 
    if (nbytes > 0 && rtype == CALMA_WIDTH)
    {
	if (!calmaReadI4Record(CALMA_WIDTH, &width)) 
	{
	    calmaReadError("Error in reading WIDTH in calmaElementPath()\n") ;
	    return;
	}
    }
    halfwidth = (width * calmaReadScale1)/(calmaReadScale2 * 2);

    /* Skip BGNEXTN, ENDEXTN */
    calmaSkipSet(ignore);

    /* Read the points in the path */
    if (!calmaReadPath(&pathheadp)) 
    {
	calmaReadError("Improper path; ignored.\n");
	return;
    }

    /* Don't process zero-width paths any further */
    if (width <= 0)
	goto freeit;

    /* Make sure we know about this type */
    ciftype = CIFCalmaLayerToCifLayer(layer, dt);
    if (ciftype < 0)
    {
	calmaLayerError("Unknown layer/datatype in path", layer, dt);
	goto freeit;
    }

    plane = cifCurReadPlanes[ciftype];
    for (previousp = pathheadp,pathp=pathheadp->cifp_next,first=TRUE,last=FALSE;
	    pathp != NULL; pathp = pathp->cifp_next)
    {
	if (pathp->cifp_next == NULL) last = TRUE;
	xmin = previousp->cifp_x;
	xmax = pathp->cifp_x;
	ymin = previousp->cifp_y;
	ymax = pathp->cifp_y;

	if (xmin != xmax && ymin != ymax)
	{
	     calmaReadError("Non-orthgonal path; ignored.\n");
	     return;
	}
	if (first == FALSE && last == FALSE)
	{
	     /* middle segment; expand the endpoints */
	     if (xmax < xmin) temp = xmin, xmin = xmax, xmax = temp;
	     if (ymax < ymin) temp = ymin, ymin = ymax, ymax = temp;
	     segment.r_xbot = xmin-halfwidth;
	     segment.r_ybot = ymin-halfwidth;
	     segment.r_xtop = xmax+halfwidth;
	     segment.r_ytop = ymax+halfwidth;
	}
	else 
	{
	     if (xmin == xmax) /* vertical wire */
	     {
	          segment.r_xbot = xmin-halfwidth;
	          segment.r_xtop = xmax+halfwidth;
		  if (ymax < ymin)
		  {
		       segment.r_ybot = ymax;
		       segment.r_ytop = ymin;
		       if (first == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_ytop += halfwidth;
		       }
		       if (last == FALSE || pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_ybot -= halfwidth;
		       }
		  }
		  else
		  {
		       segment.r_ytop = ymax;
		       segment.r_ybot = ymin;
		       if (first == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_ybot -= halfwidth;
		       }
		       if (last  == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_ytop += halfwidth;
		       }
		  }
	     }
	     if (ymin == ymax) /* horiz wire */
	     {
	          segment.r_ybot = ymin-halfwidth;
	          segment.r_ytop = ymax+halfwidth;
		  if (xmax < xmin)
		  {
		       segment.r_xbot = xmax;
		       segment.r_xtop = xmin;
		       if (first == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_xtop += halfwidth;
		       }
		       if (last  == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_xbot -= halfwidth;
		       }
		  }
		  else
		  {
		       segment.r_xtop = xmax;
		       segment.r_xbot = xmin;
		       if (first == FALSE|| pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_xbot -= halfwidth;
		       }
		       if (last == FALSE || pathtype != CALMAPATH_SQUAREFLUSH)
		       {
		       	    segment.r_xtop += halfwidth;
		       }
		  }
	     }
	}
	DBPaintPlane(plane, &segment, CIFPaintTable, (PaintUndoInfo *)NULL);
	previousp = pathp;
	first = FALSE;
    }

    /* All done */
freeit:
    CIFFreePath(pathheadp);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaElementText --
 *
 * Read labels.
 * CURRENTLY UNIMPLEMENTED.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Currently, none.
 *	Eventually, may add labels to our label list.
 *
 * ----------------------------------------------------------------------------
 */

bool
calmaElementText()
{
    static int ignore[] = { CALMA_PRESENTATION, CALMA_PATHTYPE, CALMA_WIDTH,
			    CALMA_STRANS, CALMA_MAG, CALMA_ANGLE, -1 };
    char textbody[CALMANAMELENGTH + 2];
    int nbytes, rtype;
    int layer, textt, cifnum;
    TileType type;
    Rect r;

    /* Skip CALMA_ELFLAGS, CALMA_PLEX */
    calmaSkipSet(calmaElementIgnore);

    /* Grab layer and texttype */
    if (!calmaReadI2Record(CALMA_LAYER, &layer)) return;
    if (!calmaReadI2Record(CALMA_TEXTTYPE, &textt)) return;
    cifnum = CIFCalmaLayerToCifLayer(layer, textt);
    if (cifnum < 0)
    {
	calmaLayerError("Label on unknown layer/datatype", layer, textt);
	type = TT_SPACE;
    }
    else type = cifCurReadStyle->crs_labelLayer[cifnum];

    /* Skip presentation, pathtype, width, and transform */
    calmaSkipSet(ignore);

    /* Coordinates of text */
    READRH(nbytes, rtype)
    if (nbytes < 0) return;
    if (rtype != CALMA_XY)
    {
	calmaUnexpected(CALMA_XY, rtype);
	return;
    }
    nbytes -= CALMAHEADERLENGTH;
    if (nbytes < 8)
    {
	calmaReadError("Not enough bytes in point record.\n");
    }
    else
    {
	READPOINT(&r.r_ll);
	nbytes -= 8;
    }
    if (!calmaSkipBytes(nbytes)) return;
    r.r_ll.p_x /= cifCurReadStyle->crs_scaleFactor;
    r.r_ll.p_y /= cifCurReadStyle->crs_scaleFactor;
    r.r_ur = r.r_ll;

    /* String itself */
    if (!calmaReadStringRecord(CALMA_STRING, textbody)) return;

    /* Eliminate strange characters. */
    {
	static bool algmsg = FALSE;
	bool changed = FALSE;
	char *cp;
	for (cp = textbody; *cp; cp++)
	{
	    if (*cp <= ' ' | *cp > '~') 
	    {
		changed = TRUE;
		if (*cp == '\r' && *(cp+1) == '\0')
		    *cp = '\0';
		else if (*cp == '\r') 
		    *cp = '_';
		else if (*cp == ' ')
		    *cp = '_';
		else
		    *cp = '?';
	    }
	}
	if (changed) {
	    TxError("Warning:  weird characters fixed in label '%s'\n", textbody);
	    if (!algmsg) {
		algmsg = TRUE;
		TxError("    (algorithm used:  trailing <CR> dropped, <CR> and ' ' changed to '_', \n    other non-printables changed to '?')\n");
	    }
	}
    }

    /* Place the label */
    if (type >= 0)
    {
    	 (void) DBPutLabel(cifReadCellDef, &r, -1, textbody, type);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaReadPath --
 *
 * This procedure parses a Calma path, which is an XY record
 * containing one or more points.
 *
 * Results:
 *	TRUE is returned if the path was parsed successfully,
 *	FALSE otherwise.
 *
 * Side effects:
 *	Modifies the parameter pathheadpp to point to the path
 *	that is constructed.
 *
 * ----------------------------------------------------------------------------
 */

bool
calmaReadPath(pathheadpp)
    CIFPath **pathheadpp;
{
    CIFPath path, *pathtailp, *newpathp;
    int nbytes, rtype, npoints;
    bool nonManhattan = FALSE;

    *pathheadpp = (CIFPath *) NULL;
    pathtailp = (CIFPath *) NULL;
    path.cifp_next = (CIFPath *) NULL;

    /* Read the record header */
    READRH(nbytes, rtype);
    if (nbytes < 0)
    {
	calmaReadError("EOF when reading path.\n");
	return (FALSE);
    }
    if (rtype != CALMA_XY)
    {
	calmaUnexpected(CALMA_XY, rtype);
	return (FALSE);
    }

    /* Read this many points (pairs of four-byte integers) */
    npoints = (nbytes - CALMAHEADERLENGTH) / 8;
    while (npoints--)
    {
	READPOINT(&path.cifp_point);
	if (feof(calmaInputFile))
	{
	    CIFFreePath(*pathheadpp);
	    return (FALSE);
	}

	MALLOC(CIFPath *, newpathp, sizeof (CIFPath));
	*newpathp = path;
	if (*pathheadpp)
	{
	    /*
	     * Check that this segment is Manhattan.  If not, remember the
	     * fact and later introduce extra stair-steps to make the path
	     * Manhattan.  We don't do the stair-step introduction here for
	     * two reasons: first, the same code is also used by the Calma
	     * module, and second, it is important to know which side of
	     * the polygon is the outside when generating the stair steps.
	     */
	    if (pathtailp->cifp_x != newpathp->cifp_x
		    && pathtailp->cifp_y != (newpathp->cifp_y))
	    {
		if (!nonManhattan)
		    calmaNonManhattan++;
		nonManhattan = TRUE;
	    }
	    pathtailp->cifp_next = newpathp;
	}
	else *pathheadpp = newpathp;
	pathtailp = newpathp;
    }

    if (nonManhattan)
	CIFMakeManhattanPath(*pathheadpp);

    return (*pathheadpp != NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaLayerError --
 *
 * This procedure is called when (layer, dt) doesn't map to a valid
 * Calma layer.  The first time this procedure is called for a given
 * (layer, dt) pair, we print an error message; on subsequent times,
 * no error message is printed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is printed if the first time for this (layer, dt).
 *	Adds an entry to the HashTable calmaLayerHash if one is not
 *	already present for this (layer, dt) pair.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaLayerError(mesg, layer, dt)
    char *mesg;
    int layer;
    int dt;
{
    CalmaLayerType clt;
    HashEntry *he;

    clt.clt_layer = layer;
    clt.clt_type = dt;
    he = HashFind(&calmaLayerHash, (char *) &clt);
    if (HashGetValue(he) == (ClientData) NULL)
    {
	HashSetValue(he, (ClientData) 1);
	calmaReadError("%s, layer=%d type=%d\n", mesg, layer, dt);
    }
}
