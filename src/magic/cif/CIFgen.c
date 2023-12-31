/* CIFgen.c -
 *
 *	This file provides routines that generate CIF from Magic
 *	tile information, using one of the styles read from the
 *	technology file.
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
static char rcsid[] = "$Header: CIFgen.c,v 6.0 90/08/28 18:04:59 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "CIFint.h"

/* The following global arrays hold pointers to the CIF planes
 * generated by the procedures in this module.  There are two
 * kinds of planes:  real CIF, which will ostensibly be output,
 * and temporary layers used to build up more complex geometric
 * functions.
 */

global Plane *CIFPlanes[MAXCIFLAYERS];

/* The following are paint tables used by the routines that implement
 * the geometric operations on mask layers, such as AND, OR, GROW,
 * etc.  There are really only two tables.  The first will paint
 * CIF_SOLIDTYPE over anything else, and the second will paint
 * space over anything else.  These tables are used on planes with
 * only two tile types, TT_SPACE and CIF_SOLIDTYPE, so they only
 * have two entries each.
 */

PaintResultType CIFPaintTable[] = {CIF_SOLIDTYPE, CIF_SOLIDTYPE};
PaintResultType CIFEraseTable[] = {TT_SPACE, TT_SPACE};

/* The following local variables are used as a convenience to pass
 * information between CIFGen and the various search functions.
 */

static int growDistance;	/* Distance to grow stuff. */
static Plane *cifPlane;		/* Plane acted on by search functions. */
static int cifScale;		/* Scale factor to use on tiles. */


/*
 * ----------------------------------------------------------------------------
 *
 * cifPaintFunc --
 *
 * 	This search function is called during CIF_AND and CIF_OR
 *	and CIF_ANDNOT operations for each relevant tile.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	For the area of the tile, either paints or erases in
 *	cifNewPlane, depending on the paint table passed as parameter.
 *	The tile's area is scaled by cifScale first.
 * ----------------------------------------------------------------------------
 */

int
cifPaintFunc(tile, table)
    Tile *tile;
    PaintResultType *table;		/* Used for painting. */
{
    Rect area;

    TiToRect(tile, &area);
    area.r_xbot *= cifScale;
    area.r_xtop *= cifScale;
    area.r_ybot *= cifScale;
    area.r_ytop *= cifScale;
    DBPaintPlane(cifPlane, &area, table, (PaintUndoInfo *) NULL);
    CIFTileOps += 1;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifGrowFunc --
 *
 * 	Called for each relevant tile during grow or shrink operations.
 *	For growing, these are solid tiles.  For shrinking, these are
 *	space tiles.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	Scales the tile by cifScale, then expands its area by the
 *	distance in the current CIFOp, then paints this area into
 *	cifNewPlane using the table passed as parameter.
 * ----------------------------------------------------------------------------
 */

int
cifGrowFunc(tile, table)
    Tile *tile;
    PaintResultType *table;		/* Table to be used for painting. */
{
    Rect area;
    TiToRect(tile, &area);

    /* In scaling the tile, watch out for infinities!!  If something
     * is already infinity, don't change it. */

    if (area.r_xbot > TiPlaneRect.r_xbot)
        area.r_xbot = area.r_xbot*cifScale - growDistance;
    if (area.r_ybot > TiPlaneRect.r_ybot)
        area.r_ybot = area.r_ybot*cifScale - growDistance;
    if (area.r_xtop < TiPlaneRect.r_xtop)
        area.r_xtop = area.r_xtop*cifScale + growDistance;
    if (area.r_ytop < TiPlaneRect.r_ytop)
        area.r_ytop = area.r_ytop*cifScale + growDistance;
    DBPaintPlane(cifPlane, &area, table, (PaintUndoInfo *) NULL);
    CIFTileOps += 1;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifBloatFunc --
 *
 * 	Called once for each tile to be selectively bloated.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	Uses the bloat table in the current CIFOp to expand the
 *	tile depending on which tiles it abuts, then paints the
 *	expanded area into the new CIF plane.  The bloat table
 *	contains one entry for each tile type.  When that tile
 *	type is seen next to the current tile, the area of the current
 *	tile is bloated by the table value in that location.  The
 *	only exception to this rule is that internal edges (between
 *	two tiles of the same type) cause no bloating.
 *	Note:  the original tile is scaled into CIF coordinates.
 * ----------------------------------------------------------------------------
 */

int
cifBloatFunc(tile, bloatTable)
    Tile *tile;
    int *bloatTable;		/* Table of bloat values, indexed by
				 * tile type.
				 */
{
    Rect tileArea, cifArea, bloat;
    TileType type, topLeftType, bottomRightType;
    Tile *t;

    type = TiGetType(tile);
    TiToRect(tile, &tileArea);

    /* Output the original area of the tile. */

    cifArea = tileArea;
    cifArea.r_xbot *= cifScale;
    cifArea.r_xtop *= cifScale;
    cifArea.r_ybot *= cifScale;
    cifArea.r_ytop *= cifScale;
    DBPaintPlane(cifPlane, &cifArea, CIFPaintTable, (PaintUndoInfo *) NULL);

    /* Go around the tile, scanning the neighbors along each side.
     * Start with the left side, and output the bloats along that
     * side, if any.
     */
    
    bloat.r_ybot = cifArea.r_ybot - bloatTable[TiGetType(LB(tile))];
    bloat.r_xtop = cifArea.r_xbot;
    for (t = BL(tile); BOTTOM(t) < TOP(tile); t = RT(t))
    {
	topLeftType = TiGetType(t);
	bloat.r_xbot = bloat.r_xtop - bloatTable[topLeftType];
	if (TOP(t) > tileArea.r_ytop)
	    bloat.r_ytop = cifArea.r_ytop;
	else bloat.r_ytop = cifScale * TOP(t);
	if ((bloatTable[topLeftType] != 0) && (topLeftType != type))
	    DBPaintPlane(cifPlane, &bloat, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	bloat.r_ybot = bloat.r_ytop;
    }

    /* Now do the top side.  Use the type of the top-left tile to
     * side-extend the left end of the top bloat in order to match
     * things up at the corner.
     */

    bloat.r_ybot = cifArea.r_ytop;
    bloat.r_xtop = cifArea.r_xtop;
    for (t = RT(tile); RIGHT(t) > LEFT(tile); t = BL(t))
    {
	TileType otherType = TiGetType(t);
	bloat.r_ytop = bloat.r_ybot + bloatTable[TiGetType(t)];
	if (LEFT(t) <= tileArea.r_xbot)
	    bloat.r_xbot = cifArea.r_xbot - bloatTable[topLeftType];
	else bloat.r_xbot = cifScale * LEFT(t);
	if ((bloatTable[otherType] != 0) && (otherType != type))
	    DBPaintPlane(cifPlane, &bloat, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	bloat.r_xtop = bloat.r_xbot;
    }

    /* Now do the right side. */

    bloat.r_ytop = cifArea.r_ytop + bloatTable[TiGetType(RT(tile))];
    bloat.r_xbot = cifArea.r_xtop;
    for (t = TR(tile); TOP(t) > BOTTOM(tile); t = LB(t))
    {
	bottomRightType = TiGetType(t);
	bloat.r_xtop = bloat.r_xbot + bloatTable[bottomRightType];
	if (BOTTOM(t) < tileArea.r_ybot)
	    bloat.r_ybot = cifArea.r_ybot;
	else bloat.r_ybot = cifScale * BOTTOM(t);
	if ((bloatTable[bottomRightType] != 0) && (bottomRightType != type))
	    DBPaintPlane(cifPlane, &bloat, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	bloat.r_ytop = bloat.r_ybot;
    }

    /* Now do the bottom side.  Use the type of the bottom-right tile
     * to side-extend the right end of the bottom bloat in order to match
     * things up at the corner.
     */

    bloat.r_ytop = cifArea.r_ybot;
    bloat.r_xbot = cifArea.r_xbot;
    for (t = LB(tile); LEFT(t) < RIGHT(tile); t = TR(t))
    {
	TileType otherType = TiGetType(t);
	bloat.r_ybot = bloat.r_ytop - bloatTable[otherType];
	if (RIGHT(t) >= tileArea.r_xtop)
	    bloat.r_xtop = cifArea.r_xtop + bloatTable[bottomRightType];
	else bloat.r_xtop = cifScale * RIGHT(t);
	if ((bloatTable[otherType] != 0) && (otherType != type))
	    DBPaintPlane(cifPlane, &bloat, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	bloat.r_xbot = bloat.r_xtop;
    }

    CIFTileOps += 1;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifBloatMaxFunc --
 *
 * 	Called once for each tile to be selectively bloated or
 *	shrunk using the CIFOP_BLOATMAX or CIFOP_BLOATMIN operation.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	Uses the bloat table in the current CIFOp to expand or shrink
 *	the tile, depending on which tiles it abuts.  The rectangular
 *	area of the tile is modified by moving each side in or out
 *	by the maximum bloat distance for any of its neighbors on
 *	that side.  The result is a new rectangle, which is OR'ed
 *	into the result plane.  Note:  any edge between two tiles of
 *	same type is given a zero bloat factor.
 *
 * ----------------------------------------------------------------------------
 */

int
cifBloatMaxFunc(tile, op)
    Tile *tile;			/* The tile to be processed. */
    CIFOp *op;			/* Describes the operation to be performed
				 * (all we care about is the opcode and
				 * bloat table).
				 */
{
    Rect area;
    int bloat, tmp;
    Tile *t;
    TileType type, otherType;

    /* Get the tile into CIF coordinates. */

    type = TiGetType(tile);
    TiToRect(tile, &area);
    area.r_xbot *= cifScale;
    area.r_ybot *= cifScale;
    area.r_xtop *= cifScale;
    area.r_ytop *= cifScale;

    /* See how much to adjust the left side of the tile. */

    if (op->co_opcode == CIFOP_BLOATMAX) bloat = -10000000;
    else bloat = 10000000;
    for (t = BL(tile); BOTTOM(t) < TOP(tile); t = RT(t))
    {    
	otherType = TiGetType(t);
	if (otherType == type) continue;
	tmp = op->co_bloats[otherType];
	if (op->co_opcode == CIFOP_BLOATMAX)
	{
	    if (tmp > bloat) bloat = tmp;
	}
	else if (tmp < bloat) bloat = tmp;
    }
    if ((bloat < 10000000) && (bloat > -10000000))
	area.r_xbot -= bloat;

    /* Now figure out how much to adjust the top side of the tile. */

    if (op->co_opcode == CIFOP_BLOATMAX) bloat = -10000000;
    else bloat = 10000000;
    for (t = RT(tile); RIGHT(t) > LEFT(tile); t = BL(t))
    {    
	otherType = TiGetType(t);
	if (otherType == type) continue;
	tmp = op->co_bloats[otherType];
	if (op->co_opcode == CIFOP_BLOATMAX)
	{
	    if (tmp > bloat) bloat = tmp;
	}
	else if (tmp < bloat) bloat = tmp;
    }
    if ((bloat < 10000000) && (bloat > -10000000))
	area.r_ytop += bloat;

    /* Now the right side. */
 
    if (op->co_opcode == CIFOP_BLOATMAX) bloat = -10000000;
    else bloat = 10000000;
    for (t = TR(tile); TOP(t) > BOTTOM(tile); t = LB(t))
    {    
	otherType = TiGetType(t);
	if (otherType == type) continue;
	tmp = op->co_bloats[otherType];
	if (op->co_opcode == CIFOP_BLOATMAX)
	{
	    if (tmp > bloat) bloat = tmp;
	}
	else if (tmp < bloat) bloat = tmp;
    }
    if ((bloat < 10000000) && (bloat > -10000000))
	area.r_xtop += bloat;

    /* Lastly, do the bottom side. */

    if (op->co_opcode == CIFOP_BLOATMAX) bloat = -10000000;
    else bloat = 10000000;
    for (t = LB(tile); LEFT(t) < RIGHT(tile); t = TR(t))
    {    
	otherType = TiGetType(t);
	if (otherType == type) continue;
	tmp = op->co_bloats[otherType];
	if (op->co_opcode == CIFOP_BLOATMAX)
	{
	    if (tmp > bloat) bloat = tmp;
	}
	else if (tmp < bloat) bloat = tmp;
    }
    if ((bloat < 10000000) && (bloat > -10000000))
	area.r_ybot -= bloat;

    /* Make sure the tile didn't shrink into negativity.  If it's
     * ok, paint it into the new plane being built.
     */

    if ((area.r_xbot > area.r_xtop) || (area.r_ybot > area.r_ytop))
    {
	TiToRect(tile, &area);
	area.r_xbot *= cifScale;
	area.r_xtop *= cifScale;
	area.r_ybot *= cifScale;
	area.r_ytop *= cifScale;
	CIFError(&area, "tile inverted by shrink");
    }
    else DBPaintPlane(cifPlane, &area, CIFPaintTable, (PaintUndoInfo *) NULL);

    CIFTileOps += 1;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifSquareFunc --
 *
 * 	Called for each relevant tile when chopping areas up into squares.
 *	Chops each tile up into squares and paints into cifPlane.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is painted into cifPlane.
 *
 * ----------------------------------------------------------------------------
 */

int
cifSquareFunc(tile, op)
    Tile *tile;			/* Tile to be diced up. */
    CIFOp *op;			/* Describes how to generate squares. */
{
    Rect area, square;
    int i, nAcross, j, nUp, left;

    TiToRect(tile, &area);

    /* Compute the real border to leave around the sides.  If only
     * one square will fit in a particular direction, center it
     * regardless of the requested border size.  If more than one
     * square will fit, then fit it in extras only if at least the
     * requested border size can be left.  Also center things in the
     * rectangle, so that the box's exact size doesn't matter. This
     * trickiness is done so that (a) coincident contacts from overlapping
     * cells always have their squares line up, regardless of the
     * orientation of the cells, and (b) we can generate contact vias
     * for non-square contact areas, for example poly-metal contacts
     * in the MOSIS CMOS process.
     */

    nAcross = (area.r_xtop - area.r_xbot + op->co_bloats[2]
	    - (2*op->co_bloats[0])) / (op->co_bloats[1] + op->co_bloats[2]);
    if (nAcross == 0)
    {
	left = (area.r_xbot + area.r_xtop - op->co_bloats[1])/2;
	if (left >= area.r_xbot) nAcross = 1;
    }
    else
    {
	left = (area.r_xbot + area.r_xtop + op->co_bloats[2]
		- (nAcross * (op->co_bloats[1] + op->co_bloats[2])))/2;
    }

    nUp= (area.r_ytop - area.r_ybot + op->co_bloats[2]
	    - (2*op->co_bloats[0])) / (op->co_bloats[1] + op->co_bloats[2]);
    if (nUp == 0)
    {
	square.r_ybot = (area.r_ybot + area.r_ytop - op->co_bloats[1])/2;
	if (square.r_ybot >= area.r_ybot) nUp = 1;
    }
    else
    {
	square.r_ybot = (area.r_ybot + area.r_ytop + op->co_bloats[2]
		- (nUp * (op->co_bloats[1] + op->co_bloats[2])))/2;
    }

    for (i = 0; i < nUp; i += 1)
    {
	square.r_ytop = square.r_ybot + op->co_bloats[1];
	square.r_xbot = left;
	for (j = 0; j < nAcross; j += 1)
	{
	    square.r_xtop = square.r_xbot + op->co_bloats[1];
	    DBPaintPlane(cifPlane, &square, CIFPaintTable,
		(PaintUndoInfo *) NULL);
	    CIFTileOps += 1;
	    square.r_xbot += op->co_bloats[2] + op->co_bloats[1];
	}
	square.r_ybot += op->co_bloats[2] + op->co_bloats[1];
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifSrTiles --
 *
 * 	This is a utility procedure that just calls DBSrPaintArea
 *	one or more times for the planes being used in processing
 *	one CIFOp.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure itself has no side effects.  For each of the
 *	paint or temporary planes indicated in cifOp, we call
 *	DBSrPaintArea to find the desired tiles in the desired
 *	area for the operation.  DBSrPaintArea is given func as a
 *	search function, and cdArg as ClientData.
 *
 * ----------------------------------------------------------------------------
 */

void
cifSrTiles(cifOp, area, cellDef, temps, func, cdArg)
    CIFOp *cifOp;		/* Geometric operation being processed. */
    Rect *area;			/* Area of Magic paint to consider. */
    CellDef *cellDef;		/* CellDef to search for paint. */
    Plane *temps[];		/* Planes to use for temporaries. */
    int (*func)();		/* Search function to pass to DBSrPaintArea. */
    ClientData cdArg;		/* Client data for func. */
{
    TileTypeBitMask maskBits;
    TileType t;
    int i;

    /* When reading data from a cell, it must first be scaled to
     * CIF units.
     */

    cifScale = CIFCurStyle->cs_scaleFactor;

    for (i = PL_DRC_CHECK; i < DBNumPlanes; i++)
    {
	maskBits = DBPlaneTypes[i];
	TTMaskAndMask(&maskBits, &cifOp->co_paintMask);
	if (!TTMaskEqual(&maskBits, &DBZeroTypeBits))
	    (void) DBSrPaintArea((Tile *) NULL, cellDef->cd_planes[i],
		area, &cifOp->co_paintMask, func, cdArg);
    }

    /* When processing CIF data, use everything in the plane. */

    cifScale = 1;
    for (t = 0; t < TT_MAXTYPES; t++, temps++)
	if (TTMaskHasType(&cifOp->co_cifMask, t))
	    (void) DBSrPaintArea((Tile *) NULL, *temps, &TiPlaneRect,
		&CIFSolidBits, func, (ClientData) cdArg);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFGenLayer --
 *
 *	This routine will generate one CIF layer.
 *	It provides the core of the CIF generator.
 *
 * Results:
 *	Returns a malloc'ed plane with tiles of type CIF_SOLIDTYPE
 *	marking the area of this CIF layer as built up by op.
 *
 * Side effects:
 *	None, except to create a new plane holding the CIF for the layer.
 *	The CIF that's generated may fall outside of area... it's what
 *	results from considering everything in area.  In most cases the
 *	caller will clip the results down to the desired area.
 *
 * ----------------------------------------------------------------------------
 */

Plane *
CIFGenLayer(op, area, cellDef, temps)
    CIFOp *op;			/* List of CIFOps telling how to make layer. */
    Rect *area;			/* Area to consider when generating CIF.  Only
				 * material in this area will be considered, so
				 * the caller should usually expand his desired
				 * area by one CIF radius.
				 */
    CellDef *cellDef;		/* CellDef to search when paint layers are
				 * needed for operation.
				 */
    Plane *temps[];		/* Temporary layers to be used when needed
				 * for operation.
				 */
{
    Plane *temp;
    static Plane *nextPlane, *curPlane;

    /* Set up temporary planes used during computation.  One of these
     * will be returned as the result (whichever is curPlane at the
     * end of the computation).  The other is saved for later use.
     */

    if (nextPlane == NULL)
	nextPlane = DBNewPlane((ClientData) TT_SPACE);
    curPlane = DBNewPlane((ClientData) TT_SPACE);

    /* Go through the geometric operations and process them one
     * at a time.
     */

    for ( ; op != NULL; op = op->co_next)
    {
	switch (op->co_opcode)
	{
	    /* For AND, first collect all the stuff to be anded with
	     * plane in a temporary plane.  Then find all the places
	     * where there isn't any stuff, and erase from the
	     * current plane.
	     */

	    case CIFOP_AND:
		DBClearPaintPlane(nextPlane);
		cifPlane = nextPlane;
		cifSrTiles(op, area, cellDef, temps, cifPaintFunc,
		    (ClientData) CIFPaintTable);
		cifPlane = curPlane;
		cifScale = 1;
		(void) DBSrPaintArea((Tile *) NULL, nextPlane, &TiPlaneRect,
		    &DBSpaceBits, cifPaintFunc,
		    (ClientData) CIFEraseTable);
		break;
	    
	    /* For OR, just use cifPaintFunc to OR the areas of all
	     * relevant tiles into plane.
	     */

	    case CIFOP_OR:
		cifPlane = curPlane;
		cifSrTiles(op, area, cellDef, temps, cifPaintFunc,
		    (ClientData) CIFPaintTable);
		break;
	    
	    /* For ANDNOT, do exactly the same thing as OR, except erase
	     * instead of paint.
	     */

	    case CIFOP_ANDNOT:
		cifPlane = curPlane;
		cifSrTiles(op, area, cellDef, temps, cifPaintFunc,
		    (ClientData) CIFEraseTable);
		break;
	    
	    /* For GROW, just find all solid tiles in the current plane,
	     * and paint a larger version into a new plane.  The switch
	     * the current and new planes.
	     */
	
	    case CIFOP_GROW:
		growDistance = op->co_distance;
		DBClearPaintPlane(nextPlane);
		cifPlane = nextPlane;
		cifScale = 1;
		(void) DBSrPaintArea((Tile *) NULL, curPlane, &TiPlaneRect,
		    &CIFSolidBits, cifGrowFunc, (ClientData) CIFPaintTable);
		temp = curPlane;
		curPlane = nextPlane;
		nextPlane = temp;
		break;
	    
	    /* SHRINK is just like grow except work from the space tiles. */

	    case CIFOP_SHRINK:
		growDistance = op->co_distance;
		DBClearPaintPlane(nextPlane);
		DBPaintPlane(nextPlane, &TiPlaneRect, CIFPaintTable,
		    (PaintUndoInfo *) NULL);
		cifPlane = nextPlane;
		cifScale = 1;
		(void) DBSrPaintArea((Tile *) NULL, curPlane, &TiPlaneRect,
		    &DBSpaceBits, cifGrowFunc, (ClientData) CIFEraseTable);
		temp = curPlane;
		curPlane = nextPlane;
		nextPlane = temp;
		break;
	    
	    case CIFOP_BLOAT:
		cifPlane = curPlane;
		cifSrTiles(op, area, cellDef, temps,
		    cifBloatFunc, (ClientData) op->co_bloats);
		break;
	    
	    case CIFOP_BLOATMAX:
	    case CIFOP_BLOATMIN:
		cifPlane = curPlane;
		cifSrTiles(op, area, cellDef, temps,
		    cifBloatMaxFunc, (ClientData) op);
		break;
	    
	    case CIFOP_SQUARES:
		DBClearPaintPlane(nextPlane);
		cifPlane = nextPlane;
		(void) DBSrPaintArea((Tile *) NULL, curPlane, &TiPlaneRect,
		    &CIFSolidBits, cifSquareFunc, (ClientData) op);
		temp = curPlane;
		curPlane = nextPlane;
		nextPlane = temp;
		break;

	    default:  continue;
	}
    }

    return curPlane;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFGen --
 *
 * 	This procedure generates a complete set of CIF layers for
 *	a particular area of a particular cell.  NOTE: if the argument
 *	genAllPlanes is FALSE, only planes for those layers having
 *	a bit set in 'layers' are generated; the others are set
 *	to NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The parameters realPlanes and tempPlanes are modified
 *	to hold the CIF and temporary layers for area of cellDef,
 *	as determined by the current CIF generation rules.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFGen(cellDef, area, planes, layers, replace, genAllPlanes)
    CellDef *cellDef;		/* Cell for which CIF is to be generated. */
    Rect *area;			/* Any CIF overlapping this area (in coords
				 * of cellDef) will be generated.  The CIF
				 * will be clipped to this area.
				 */
    Plane **planes;		/* Pointer to array of pointers to planes
				 * to hold "real" CIF layers that are
				 * generated.  Pointers may initially be
				 * NULL.
				 */
    TileTypeBitMask *layers;	/* CIF layers to generate. */
    bool replace;		/* TRUE means that the new CIF is to replace
				 * anything that was previously in planes.
				 * FALSE means that the new CIF is to be
				 * OR'ed in with the current contents of
				 * planes.
				 */
    bool genAllPlanes;		/* If TRUE, generate a tile plane even for
				 * those layers not specified as being
				 * generated in the 'layers' mask above.
				 */
{
    int i;
    Plane *new[MAXCIFLAYERS];
    Rect expanded, clip;

    /*
     * Generate the area in magic coordinates to search, and the area in
     * cif coordinates to use in clipping the results of CIFGenLayer().
     */
    cifGenClip(area, &expanded, &clip);

    /*
     * Generate all of the new layers in a temporary place.
     * If a layer isn't being generated, leave new[i] set to
     * NULL to indicate this fact.
     */
    for (i = 0; i < CIFCurStyle->cs_nLayers; i++)
    {
	if (TTMaskHasType(layers,i))
	{
	    CIFErrorLayer = i;
	    new[i] = CIFGenLayer(CIFCurStyle->cs_layers[i]->cl_ops,
		    &expanded, cellDef, new);
	}
	else if (genAllPlanes) new[i] = DBNewPlane((ClientData) TT_SPACE);
	else new[i] = (Plane *) NULL;
    }

    /*
     * Now mask off all the unwanted material in the new layers, and
     * either OR them into the existing layers or replace the existing
     * material with them.
     */
    for (i = 0; i < CIFCurStyle->cs_nLayers; i += 1)
    {
	if (new[i])
	    cifClipPlane(new[i], &clip);

	if (replace)
	{
	    if (planes[i])
	    {
		DBFreePaintPlane(planes[i]);
		TiFreePlane(planes[i]);
	    }
	    planes[i] = new[i];
	    continue;
	}

	if (planes[i])
	{
	    if (new[i])
	    {
		cifPlane = planes[i];
		cifScale = 1;
		(void) DBSrPaintArea((Tile *) NULL, new[i], &TiPlaneRect,
			&CIFSolidBits, cifPaintFunc,
			(ClientData) CIFPaintTable);
		DBFreePaintPlane(new[i]);
		TiFreePlane(new[i]);
	    }
	}
	else planes[i] = new[i];
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifClipPlane --
 *
 * Erase the portions of the plane 'plane' that lie outside of 'clip'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

Void
cifClipPlane(plane, clip)
    Plane *plane;
    Rect *clip;
{
    Rect r;

    if (clip->r_xtop < TiPlaneRect.r_xtop)
    {
	r = TiPlaneRect;
	r.r_xbot = clip->r_xtop;
	DBPaintPlane(plane, &r, CIFEraseTable, (PaintUndoInfo *) NULL);
    }
    if (clip->r_ytop < TiPlaneRect.r_ytop)
    {
	r = TiPlaneRect;
	r.r_ybot = clip->r_ytop;
	DBPaintPlane(plane, &r, CIFEraseTable, (PaintUndoInfo *) NULL);
    }
    if (clip->r_xbot > TiPlaneRect.r_xbot)
    {
	r = TiPlaneRect;
	r.r_xtop = clip->r_xbot;
	DBPaintPlane(plane, &r, CIFEraseTable, (PaintUndoInfo *) NULL);
    }
    if (clip->r_ybot > TiPlaneRect.r_ybot)
    {
	r = TiPlaneRect;
	r.r_ytop = clip->r_ybot;
	DBPaintPlane(plane, &r, CIFEraseTable, (PaintUndoInfo *) NULL);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifGenClip --
 *
 * Compute two new areas from the original area:  one ('expanded')
 * is expanded by a CIF halo and is used to determine how much of
 * the database to search to find what's relevant for CIF generation;
 * the other ('clip') is the CIF equivalent of area and is used to
 * clip the resulting CIF.  This code is tricky because area may run
 * off to infinity, and we have to be careful not to expand past infinity.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets *expanded and *clip.
 *
 * ----------------------------------------------------------------------------
 */

Void
cifGenClip(area, expanded, clip)
    Rect *area;			/* Any CIF overlapping this area (in coords
				 * of cellDef) will be generated.  The CIF
				 * will be clipped to this area.
				 */
    Rect *expanded, *clip;
{
    if (area->r_xbot > TiPlaneRect.r_xbot)
    {
	clip->r_xbot = area->r_xbot * CIFCurStyle->cs_scaleFactor;
	expanded->r_xbot = area->r_xbot - CIFCurStyle->cs_radius;
    }
    else clip->r_xbot = expanded->r_xbot = area->r_xbot;
    if (area->r_ybot > TiPlaneRect.r_ybot)
    {
	clip->r_ybot = area->r_ybot * CIFCurStyle->cs_scaleFactor;
	expanded->r_ybot = area->r_ybot - CIFCurStyle->cs_radius;
    }
    else clip->r_ybot = expanded->r_ybot = area->r_ybot;
    if (area->r_xtop < TiPlaneRect.r_xtop)
    {
	clip->r_xtop = area->r_xtop * CIFCurStyle->cs_scaleFactor;
	expanded->r_xtop = area->r_xtop + CIFCurStyle->cs_radius;
    }
    else clip->r_xtop = expanded->r_xtop = area->r_xtop;
    if (area->r_ytop < TiPlaneRect.r_ytop)
    {
	clip->r_ytop = area->r_ytop * CIFCurStyle->cs_scaleFactor;
	expanded->r_ytop = area->r_ytop + CIFCurStyle->cs_radius;
    }
    else clip->r_ytop = expanded->r_ytop = area->r_ytop;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFClearPlanes --
 *
 * 	This procedure clears out a collection of CIF planes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each of the planes in "planes" is re-initialized to point to
 *	an empty paint plane.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFClearPlanes(planes)
    Plane **planes;		/* Pointer to an array of MAXCIFLAYERS
				 * planes.
				 */
{
    int i;

    for (i = 0; i < MAXCIFLAYERS; i++)
    {
	if (planes[i] == NULL)
	{
	    planes[i] = DBNewPlane((ClientData) TT_SPACE);
	}
	else
	{
	    DBClearPaintPlane(planes[i]);
	}
    }
}
