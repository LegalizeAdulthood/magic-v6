/*
 * DBtiles.c --
 *
 * Low-level tile primitives for the database.
 * This includes area searching and all other primitives that
 * need to know what lives in a tile body.
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
static char rcsid[] = "$Header: DBtiles.c,v 6.0 90/08/28 18:10:24 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "signals.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "malloc.h"

/* Used by DBCheckMaxHStrips() and DBCheckMaxVStrips() */
struct dbCheck
{
    int		(*dbc_proc)();
    Rect	  dbc_area;
    ClientData    dbc_cdata;
};

int dbCheckMaxHFunc(), dbCheckMaxVFunc();

/*
 * --------------------------------------------------------------------
 *
 * DBSrPaintArea --
 *
 * Find all tiles overlapping a given area whose types are contained
 * in the mask supplied.  Apply the given procedure to each such tile.
 * The procedure should be of the following form:
 *
 *	int
 *	func(tile, cdata)
 *	    Tile *tile;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * Func normally should return 0.  If it returns 1 then the search
 * will be aborted.  WARNING: THE CALLED PROCEDURE MAY NOT MODIFY
 * THE PLANE BEING SEARCHED!!!
 *
 *			NOTE:
 *
 * THIS IS THE PREFERRED WAY TO FIND ALL TILES IN A GIVEN AREA;
 * TiSrArea IS OBSOLETE FOR ALL BUT THE SUBCELL PLANE.
 *
 * Results:
 *	0 is returned if the search completed normally.  1 is returned
 *	if it aborted.
 *
 * Side effects:
 *	Whatever side effects result from application of the
 *	supplied procedure.
 *
 * --------------------------------------------------------------------
 */

int
DBSrPaintArea(hintTile, plane, rect, mask, func, arg)
    Tile *hintTile;		/* Tile at which to begin search, if not NULL.
				 * If this is NULL, use the hint tile supplied
				 * with plane.
				 */
    register Plane *plane;	/* Plane in which tiles lie.  This is used to
				 * provide a hint tile in case hintTile == NULL.
				 * The hint tile in the plane is updated to be
				 * the last tile visited in the area
				 * enumeration.
				 */
    register Rect *rect;	/* Area to search.  This area should not be
				 * degenerate.  Tiles must OVERLAP the area.
				 */
    TileTypeBitMask *mask;	/* Mask of those paint tiles to be passed to
				 * func.
				 */
    int (*func)();		/* Function to apply at each tile */
    ClientData arg;		/* Additional argument to pass to (*func)() */
{
    Point start;
    register Tile *tp, *tpnew;

    start.p_x = rect->r_xbot;
    start.p_y = rect->r_ytop - 1;
    tp = hintTile ? hintTile : plane->pl_hint;
    GOTOPOINT(tp, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tp) > rect->r_ybot)
    {
	/* Each iteration enumerates another tile */
enumerate:
	plane->pl_hint = tp;
	if (SigInterruptPending)
	    return (1);

	if (TTMaskHasType(mask, TiGetType(tp)) && (*func)(tp, arg))
	    return (1);

	tpnew = TR(tp);
	if (LEFT(tpnew) < rect->r_xtop)
	{
	    while (BOTTOM(tpnew) >= rect->r_ytop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tp) > rect->r_xbot)
	{
	    if (BOTTOM(tp) <= rect->r_ybot)
		return (0);
	    tpnew = LB(tp);
	    tp = BL(tp);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* At left edge -- walk down to next tile along the left edge */
	for (tp = LB(tp); RIGHT(tp) <= rect->r_xbot; tp = TR(tp))
	    /* Nothing */;
    }
    return (0);
}

/*
 * --------------------------------------------------------------------
 *
 * DBSrPaintAreaReverse --
 *
 * Identical to DBSrPaintArea, except that tiles are visited
 * from right to left, bottom to top.  Used when plowing either
 * up or to the left.
 *
 * Results:
 *	0 is returned if the search completed normally.  1 is returned
 *	if it aborted.
 *
 * Side effects:
 *	Whatever side effects result from application of the
 *	supplied procedure.
 *
 * --------------------------------------------------------------------
 */

int
DBSrPaintAreaReverse(hintTile, plane, rect, mask, func, arg)
    Tile *hintTile;		/* Tile at which to begin search, if not NULL.
				 * If this is NULL, use the hint tile supplied
				 * with plane.
				 */
    register Plane *plane;	/* Plane in which tiles lie.  This is used to
				 * provide a hint tile in case hintTile == NULL.
				 * The hint tile in the plane is updated to be
				 * the last tile visited in the area
				 * enumeration.
				 */
    register Rect *rect;	/* Area to search */
    TileTypeBitMask *mask;	/* Mask of those paint tiles to be passed to
				 * func.
				 */
    int (*func)();		/* Function to apply at each tile */
    ClientData arg;		/* Additional argument to pass to (*func)() */
{
    Point start;
    register Tile *tp, *tpnew;
    Rect biggerRect;

    /* If the rectangle has a zero x- or y-size, make a copy of it
     * with a size of one.  For finding purposes, a degenerate area
     * is equivalent to one with a dimension of 1, except that this
     * code doesn't work right with a degenerate area if it falls
     * on a tile boundary.
     */
    
    if ((rect->r_xbot == rect->r_xtop) || (rect->r_ybot == rect->r_ytop))
    {
	biggerRect = *rect;
	rect = &biggerRect;
	if (rect->r_xbot == rect->r_xtop) rect->r_xtop++;
	if (rect->r_ybot == rect->r_ytop) rect->r_ytop++;
    }

    start.p_x = rect->r_xtop - 1;
    start.p_y = rect->r_ybot;
    tp = hintTile ? hintTile : plane->pl_hint;
    GOTOPOINT(tp, &start);

    /* Each iteration visits another tile on the RHS of the search area */
    while (BOTTOM(tp) < rect->r_ytop)
    {
	/* Each iteration enumerates another tile */
enumerate:
	plane->pl_hint = tp;
	if (SigInterruptPending)
	    return (1);

	if (TTMaskHasType(mask, TiGetType(tp)) && (*func)(tp, arg))
	    return (1);

	tpnew = BL(tp);
	if (RIGHT(tpnew) > rect->r_xbot)
	{
	    while (TOP(tpnew) <= rect->r_ybot) tpnew = RT(tpnew);
	    if (TOP(tpnew) <= TOP(tp) || TOP(tp) >= rect->r_ytop)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the right */
	while (RIGHT(tp) < rect->r_xtop)
	{
	    if (TOP(tp) >= rect->r_ytop)
		return (0);
	    tpnew = RT(tp);
	    tp = TR(tp);
	    if (TOP(tpnew) <= TOP(tp) || TOP(tp) >= rect->r_ytop)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* At right edge -- walk down to next tile along the right edge */
	for (tp = RT(tp); LEFT(tp) >= rect->r_xtop; tp = BL(tp))
	    /* Nothing */;
    }
    return (0);
}

/*
 * --------------------------------------------------------------------
 *
 * DBSrPaintClient --
 *
 * Find all tiles overlapping a given area whose types are contained
 * in the mask supplied, and whose ti_client field matches 'client'.
 * Apply the given procedure to each such tile.  The procedure should
 * be of the following form:
 *
 *	int
 *	func(tile, cdata)
 *	    Tile *tile;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * Func normally should return 0.  If it returns 1 then the search
 * will be aborted.
 *
 * Results:
 *	0 is returned if the search completed normally.  1 is returned
 *	if it aborted.
 *
 * Side effects:
 *	Whatever side effects result from application of the
 *	supplied procedure.
 *
 * --------------------------------------------------------------------
 */

int
DBSrPaintClient(hintTile, plane, rect, mask, client, func, arg)
    Tile *hintTile;		/* Tile at which to begin search, if not NULL.
				 * If this is NULL, use the hint tile supplied
				 * with plane.
				 */
    register Plane *plane;	/* Plane in which tiles lie.  This is used to
				 * provide a hint tile in case hintTile == NULL.
				 * The hint tile in the plane is updated to be
				 * the last tile visited in the area
				 * enumeration.
				 */
    register Rect *rect;	/* Area to search.  This area should not be
				 * degenerate.  Tiles must OVERLAP the area.
				 */
    TileTypeBitMask *mask;	/* Mask of those paint tiles to be passed to
				 * func.
				 */
    ClientData client;		/* The ti_client field of each tile must
				 * match this.
				 */
    int (*func)();		/* Function to apply at each tile */
    ClientData arg;		/* Additional argument to pass to (*func)() */
{
    Point start;
    register Tile *tp, *tpnew;

    start.p_x = rect->r_xbot;
    start.p_y = rect->r_ytop - 1;
    tp = hintTile ? hintTile : plane->pl_hint;
    GOTOPOINT(tp, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tp) > rect->r_ybot)
    {
	/* Each iteration enumerates another tile */
enumerate:
	plane->pl_hint = tp;
	if (SigInterruptPending)
	    return (1);

	if (TTMaskHasType(mask, TiGetType(tp)) && tp->ti_client == client
		&& (*func)(tp, arg))
	    return (1);

	tpnew = TR(tp);
	if (LEFT(tpnew) < rect->r_xtop)
	{
	    while (BOTTOM(tpnew) >= rect->r_ytop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tp) > rect->r_xbot)
	{
	    if (BOTTOM(tp) <= rect->r_ybot)
		return (0);
	    tpnew = LB(tp);
	    tp = BL(tp);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* At left edge -- walk down to next tile along the left edge */
	for (tp = LB(tp); RIGHT(tp) <= rect->r_xbot; tp = TR(tp))
	    /* Nothing */;
    }
    return (0);
}

/*
 * --------------------------------------------------------------------
 *
 * DBResetTilePlane --
 *
 * Reset the ti_client fields of all tiles in a paint tile plane to
 * the value 'cdata'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the ti_client fields of all tiles.
 *
 * --------------------------------------------------------------------
 */

DBResetTilePlane(plane, cdata)
    Plane *plane;	/* Plane whose tiles are to be reset */
    ClientData cdata;
{
    register Tile *tp, *tpnew;
    register Rect *rect = &TiPlaneRect;

    /* Start with the leftmost non-infinity tile in the plane */
    tp = plane->pl_left->ti_tr;

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tp) > rect->r_ybot)
    {
	/* Each iteration frees another tile */
enumerate:
	tp->ti_client = cdata;

	/* Move along to the next tile */
	tpnew = TR(tp);
	if (LEFT(tpnew) < rect->r_xtop)
	{
	    while (BOTTOM(tpnew) >= rect->r_ytop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tp) > rect->r_xbot)
	{
	    if (BOTTOM(tp) <= rect->r_ybot)
		return;
	    tpnew = LB(tp);
	    tp = BL(tp);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* At left edge -- walk down to next tile along the left edge */
	for (tp = LB(tp); RIGHT(tp) <= rect->r_xbot; tp = TR(tp))
	    /* Nothing */;
    }
}

/*
 * --------------------------------------------------------------------
 *
 * DBFreePaintPlane --
 *
 * Deallocate all tiles in a paint tile plane of a given CellDef.
 * Don't allocate the four boundary tiles, or the plane itself.
 *
 * This is a procedure internal to the database.  The only reason
 * it lives in DBtiles.c rather than DBcellsubr.c is that it requires
 * intimate knowledge of the contents of paint tiles and tile planes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates a lot of memory.  
 *
 *			*** WARNING ***
 *
 * This procedure uses a non-recursive area enumeration algorithm to
 * visit all the tiles in this plane and give them to FREE().  For this
 * to work correctly, we depend on the fact that any storage freed with
 * a call to FREE() can still be used until the next call to MALLOC().
 *
 * --------------------------------------------------------------------
 */

void
DBFreePaintPlane(plane)
    Plane *plane;	/* Plane whose storage is to be freed */
{
    register Tile *tp, *tpnew;
    register Rect *rect = &TiPlaneRect;

    /* Start with the leftmost non-infinity tile in the plane */
    tp = plane->pl_left->ti_tr;

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tp) > rect->r_ybot)
    {
	/* Each iteration frees another tile */
enumerate:
	FREE((char *) tp);

	/* Move along to the next tile */
	tpnew = TR(tp);
	if (LEFT(tpnew) < rect->r_xtop)
	{
	    while (BOTTOM(tpnew) >= rect->r_ytop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tp) > rect->r_xbot)
	{
	    if (BOTTOM(tp) <= rect->r_ybot)
		return;
	    tpnew = LB(tp);
	    tp = BL(tp);
	    if (BOTTOM(tpnew) >= BOTTOM(tp) || BOTTOM(tp) <= rect->r_ybot)
	    {
		tp = tpnew;
		goto enumerate;
	    }
	}

	/* At left edge -- walk down to next tile along the left edge */
	for (tp = LB(tp); RIGHT(tp) <= rect->r_xbot; tp = TR(tp))
	    /* Nothing */;
    }
}

/*
 * --------------------------------------------------------------------
 *
 * DBFreeCellPlane --
 *
 * Deallocate all tiles in the cell tile plane of a given CellDef.
 * Also deallocates the lists of CellTileBodies and their associated
 * CellUses, but not their associated CellDefs.
 * Don't free the cell tile plane itself or the four boundary tiles.
 *
 * Since cell tile planes contain less stuff than paint tile planes
 * usually, we don't have to be as performance-conscious here.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates a lot of memory.  
 *
 * --------------------------------------------------------------------
 */

void
DBFreeCellPlane(plane)
    Plane *plane;	/* Plane whose storage is to be freed */
{
    int dbFreeCellFunc();

    /* Don't let this search be interrupted. */

    SigDisableInterrupts();
    (void) TiSrArea((Tile *) NULL, plane, &TiPlaneRect,
	    dbFreeCellFunc, (ClientData) NULL);
    SigEnableInterrupts();
}

/*
 * Filter function called via TiSrArea on behalf of DBFreeCellPlane()
 * above.  Deallocates each tile it is passed.  If the tile has a vanilla
 * body, only the tile is deallocated; otherwise, the tile body and its
 * label list are both deallocated along with the tile itself.
 */

int
dbFreeCellFunc(tile)
    register Tile *tile;
{
    register CellTileBody *body;
    register CellUse *use;
    register Rect *bbox;

    for (body = (CellTileBody *) TiGetBody(tile);
	    body != NULL;
	    body = body->ctb_next)
    {
	use = body->ctb_use;
	ASSERT(use != (CellUse *) NULL, "dbCellSrFunc");

	bbox = &use->cu_bbox;
	if ((BOTTOM(tile) <= bbox->r_ybot) && (RIGHT(tile) >= bbox->r_xtop))
	{
	    /* The parent must be null before DBCellDeleteUse will work */
	    use->cu_parent = (CellDef *) NULL;
	    (void) DBCellDeleteUse(use);
	}
	FREE((char *) body);
    }

    FREE((char *) tile);
    return 0;
}

/*
 * --------------------------------------------------------------------
 *
 * DBCheckMaxHStrips --
 *
 * Check the maximal horizontal strip property for the
 * tile plane 'plane' over the area 'area'.
 *
 * Results:
 *	Normally returns 0; returns 1 if the procedure
 *	(*proc)() returned 1 or if the search were
 *	aborted with an interrupt.
 *
 * Side effects:
 *	Calls the procedure (*proc)() for each offending tile.
 *	This procedure should have the following form:
 *
 *	int
 *	proc(tile, side, cdata)
 *	    Tile *tile;
 *	    int side;
 *	    ClientData cdata;
 *	{
 *	}
 *
 *	The client data is the argument 'cdata' passed to us.
 *	The argument 'side' is one of GEO_NORTH, GEO_SOUTH,
 *	GEO_EAST, or GEO_WEST, and indicates which side of
 *	the tile the strip property was violated on.
 *	If (*proc)() returns 1, we abort and return 1
 *	to our caller.
 *
 * --------------------------------------------------------------------
 */

int
DBCheckMaxHStrips(plane, area, proc, cdata)
    Plane *plane;	/* Search this plane */
    Rect *area;		/* Process all tiles in this area */
    int (*proc)();	/* Filter procedure: see above */
    ClientData cdata;	/* Passed to (*proc)() */
{
    struct dbCheck dbc;

    dbc.dbc_proc = proc;
    dbc.dbc_area = *area;
    dbc.dbc_cdata = cdata;
    return (DBSrPaintArea((Tile *) NULL, plane, area,
		&DBAllTypeBits, dbCheckMaxHFunc, (ClientData) &dbc));
}

/*
 * dbCheckMaxHFunc --
 *
 * Filter function for above.
 * See the description at the top.
 */

int
dbCheckMaxHFunc(tile, dbc)
    register Tile *tile;
    register struct dbCheck *dbc;
{
    register Tile *tp;

    /*
     * Property 1:
     * No tile to the left or to the right should have the same
     * type as 'tile'.
     */
    if (RIGHT(tile) < dbc->dbc_area.r_xtop)
	for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
	    if (TiGetType(tp) == TiGetType(tile))
		if ((*dbc->dbc_proc)(tile, GEO_EAST, dbc->dbc_cdata))
		    return (1);
    if (LEFT(tile) > dbc->dbc_area.r_xbot)
	for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
	    if (TiGetType(tp) == TiGetType(tile))
		if ((*dbc->dbc_proc)(tile, GEO_WEST, dbc->dbc_cdata))
		    return (1);

    /*
     * Property 2:
     * No tile to the top or bottom should be of the same type and
     * have the same width.
     */
    if (TOP(tile) < dbc->dbc_area.r_ytop)
    {
	tp = RT(tile);
	if (TiGetType(tp) == TiGetType(tile)
		&& LEFT(tp) == LEFT(tile)
		&& RIGHT(tp) == RIGHT(tile))
	    if ((*dbc->dbc_proc)(tile, GEO_NORTH, dbc->dbc_cdata))
		return (1);
    }
    if (BOTTOM(tile) > dbc->dbc_area.r_ybot)
    {
	tp = LB(tile);
	if (TiGetType(tp) == TiGetType(tile)
		&& LEFT(tp) == LEFT(tile)
		&& RIGHT(tp) == RIGHT(tile))
	    if ((*dbc->dbc_proc)(tile, GEO_SOUTH, dbc->dbc_cdata))
		return (1);
    }

    return (0);
}

/*
 * --------------------------------------------------------------------
 *
 * DBCheckMaxVStrips --
 *
 * Check the maximal vertical strip property for the
 * tile plane 'plane' over the area 'area'.
 *
 * Results:
 *	Normally returns 0; returns 1 if the procedure
 *	(*proc)() returned 1 or if the search were
 *	aborted with an interrupt.
 *
 * Side effects:
 *	See DBCheckMaxHStrips() above.
 *
 * --------------------------------------------------------------------
 */

int
DBCheckMaxVStrips(plane, area, proc, cdata)
    Plane *plane;	/* Search this plane */
    Rect *area;		/* Process all tiles in this area */
    int (*proc)();	/* Filter procedure: see above */
    ClientData cdata;	/* Passed to (*proc)() */
{
    struct dbCheck dbc;

    dbc.dbc_proc = proc;
    dbc.dbc_area = *area;
    dbc.dbc_cdata = cdata;
    return (DBSrPaintArea((Tile *) NULL, plane, area,
		&DBAllTypeBits, dbCheckMaxVFunc, (ClientData) &dbc));
}

/*
 * dbCheckMaxVFunc --
 *
 * Filter function for above.
 * See the description at the top.
 */

int
dbCheckMaxVFunc(tile, dbc)
    register Tile *tile;
    register struct dbCheck *dbc;
{
    register Tile *tp;

    /*
     * Property 1:
     * No tile to the top or to the bottom should have the same
     * type as 'tile'.
     */
    if (TOP(tile) < dbc->dbc_area.r_ytop)
	for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp))
	    if (TiGetType(tp) == TiGetType(tile))
		if ((*dbc->dbc_proc)(tile, GEO_NORTH, dbc->dbc_cdata))
		    return (1);
    if (BOTTOM(tile) > dbc->dbc_area.r_ybot)
	for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp = TR(tp))
	    if (TiGetType(tp) == TiGetType(tile))
		if ((*dbc->dbc_proc)(tile, GEO_SOUTH, dbc->dbc_cdata))
		    return (1);

    /*
     * Property 2:
     * No tile to the left or right should be of the same type and
     * have the same height.
     */
    if (RIGHT(tile) < dbc->dbc_area.r_xtop)
    {
	tp = TR(tile);
	if (TiGetType(tp) == TiGetType(tile)
		&& BOTTOM(tp) == BOTTOM(tile)
		&& TOP(tp) == TOP(tile))
	    if ((*dbc->dbc_proc)(tile, GEO_EAST, dbc->dbc_cdata))
		return (1);
    }
    if (LEFT(tile) > dbc->dbc_area.r_xbot)
    {
	tp = BL(tile);
	if (TiGetType(tp) == TiGetType(tile)
		&& BOTTOM(tp) == BOTTOM(tile)
		&& TOP(tp) == TOP(tile))
	    if ((*dbc->dbc_proc)(tile, GEO_WEST, dbc->dbc_cdata))
		return (1);
    }

    return (0);
}
