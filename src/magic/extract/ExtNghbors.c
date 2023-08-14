/*
 * ExtNeighbors.c --
 *
 * Circuit extraction.
 * This file contains the primitive function ExtFindNeighbors()
 * for visiting all neighbors of a tile that connect to it, and
 * applying a filter function at each tile.
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
static char rcsid[] = "$Header: ExtNghbors.c,v 6.0 90/08/28 18:15:21 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "debug.h"
#include "extract.h"
#include "extractInt.h"
#include "signals.h"
#include "stack.h"

/*
 * The algorithm used by ExtFindNeighbors is non-recursive.
 * It uses a stack (extNodeStack) to hold a list of tiles yet to
 * be processed.  To mark a tile as being on the stack, we store
 * the value VISITPENDING in its ti_client field.
 */

/* Used for communicating with extNbrPushFunc */
ClientData extNbrUn;

/*
 * ----------------------------------------------------------------------------
 *
 * ExtFindNeighbors --
 *
 * For each tile adjacent to 'tile' that connects to it (according to
 * arg->fra_connectsTo), and (if it is a contact) for tiles on other
 * planes that connect to it, we recursively visit the tile, call the
 * client's filter procedure (*arg->fra_each)(), if it is non-NULL.
 * The tile is marked as being visited by setting it's ti_client field
 * to arg->fra_region.
 *
 * Results:
 *	Returns 0 normally, or 1 if a client decided to abort the
 *	search, or if an interrupt was seen.
 *
 * Side effects:
 *	See comments above.
 *
 * ----------------------------------------------------------------------------
 */

ExtFindNeighbors(tile, tilePlaneNum, arg)
    register Tile *tile;
    int tilePlaneNum;
    FindRegion *arg;
{
    TileTypeBitMask *connTo = arg->fra_connectsTo;
    register Tile *tp;
    register TileType type;
    register TileTypeBitMask *mask;
    Rect tileArea, biggerArea;
    int pNum, pMask;

    extNbrUn = arg->fra_uninit;
    if (extNodeStack == (Stack *) NULL)
	extNodeStack = StackNew(64);

    /* Mark this tile as pending and push it */
    PUSHTILE(tile);

    while (!StackEmpty(extNodeStack))
    {
	tile = (Tile *) STACKPOP(extNodeStack);
	type = TiGetType(tile);
	mask = &connTo[type];
	tilePlaneNum = DBPlane(TiGetType(tile));

	/*
	 * Since tile was pushed on the stack, we know that it
	 * belongs to this region.  Check to see that it hasn't
	 * been visited in the meantime.  If it's still unvisited,
	 * visit it and process its neighbors.
	 */
	if (tile->ti_client == (ClientData) arg->fra_region)
	    continue;
	tile->ti_client = (ClientData) arg->fra_region;
	if (DebugIsSet(extDebugID, extDebNeighbor))
	    extShowTile(tile, "neighbor", 1);

	/* Top */
	for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp))
	    if (tp->ti_client == extNbrUn && TTMaskHasType(mask, TiGetType(tp)))
		PUSHTILE(tp);

	/* Left */
	for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
	    if (tp->ti_client == extNbrUn && TTMaskHasType(mask, TiGetType(tp)))
		PUSHTILE(tp);

	/* Bottom */
	for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp = TR(tp))
	    if (tp->ti_client == extNbrUn && TTMaskHasType(mask, TiGetType(tp)))
		PUSHTILE(tp);

	/* Right */
	for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
	    if (tp->ti_client == extNbrUn && TTMaskHasType(mask, TiGetType(tp)))
		PUSHTILE(tp);

	/* Apply the client's filter procedure if one exists */
	if (arg->fra_each)
	    if ((*arg->fra_each)(tile, tilePlaneNum, arg))
		goto fail;

	/* If this is a contact, visit all the other planes */
	if (TTMaskHasType(&DBImageBits, type))
	{
	    pMask = DBConnPlanes[type];
	    for (pNum = tilePlaneNum - 1; pNum <= tilePlaneNum + 1; pNum += 2)
		if (PlaneMaskHasPlane(pMask, pNum))
		{
		    Plane *plane = arg->fra_def->cd_planes[pNum];

		    tp = plane->pl_hint;
		    GOTOPOINT(tp, &tile->ti_ll);
		    plane->pl_hint = tp;
		    if (tp->ti_client == extNbrUn
			    && TTMaskHasType(mask, TiGetType(tp)))
			PUSHTILE(tp);
		}
	}

	/*
	 * The hairiest case is when this type connects to stuff on
	 * other planes, but isn't itself connected as a contact.
	 * For example, a CMOS pwell connects to diffusion of the
	 * same doping (p substrate diff).  In a case like this,
	 * we need to search the entire AREA of the tile plus a
	 * 1-lambda halo to find everything it overlaps or touches
	 * on the other plane.
	 */
	if (pMask = DBAllConnPlanes[type])
	{
	    TITORECT(tile, &tileArea);
	    GEO_EXPAND(&tileArea, 1, &biggerArea);
	    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
		if (pNum != tilePlaneNum && PlaneMaskHasPlane(pMask, pNum))
		    (void) DBSrPaintArea((Tile *) NULL,
			    arg->fra_def->cd_planes[pNum], &biggerArea,
			    mask, extNbrPushFunc, (ClientData) &tileArea);
	}
    }

    return (0);

fail:
    /* Flush the stack */
    while (!StackEmpty(extNodeStack))
    {
	tile = (Tile *) STACKPOP(extNodeStack);
	tile->ti_client = (ClientData) arg->fra_region;
    }
    return (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extNbrPushFunc --
 *
 * Called for each tile overlapped by a 1-unit wide halo around the area
 * tileArea.  If the tile overlaps or shares a non-null segment of border
 * with tileArea, and it hasn't already been visited, push it on the stack
 * extNodeStack.
 *
 * Uses the global parameter extNbrUn to determine whether or not a tile
 * has been visited; if the tile's client field is equal to extNbrUn, then
 * this is the first time the tile has been seen.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
extNbrPushFunc(tile, tileArea)
    register Tile *tile;
    register Rect *tileArea;
{
    Rect r;

    /* Ignore tile if it's already been visited */
    if (tile->ti_client != extNbrUn)
	return 0;

    /* Only consider tile if it overlaps tileArea or shares part of a side */
    TITORECT(tile, &r);
    if (!GEO_OVERLAP(&r, tileArea))
    {
	GEOCLIP(&r, tileArea);
	if (r.r_xbot >= r.r_xtop && r.r_ybot >= r.r_ytop)
	    return 0;
    }

    /* Push tile on the stack and mark as being visited */
    PUSHTILE(tile);

    return 0;
}
