/* DBconnect.c -
 *
 *	This file contains routines that extract electrically connected
 *	regions of a layout for Magic.  There are two extractors, one
 *	that operates only within the paint of a single cell (DBSrConnect),
 *	and one	that operates hierarchically, across cell boundaries
 *	(DBTreeCopyConnect).
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
static char rcsid[] = "$Header: DBconnect.c,v 6.1 90/09/03 14:50:06 stark Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "signals.h"
#include "malloc.h"

/* General note for DBSrConnect:
 *
 * The connectivity extractor works in two passes, in order to avoid
 * circularities.  During the first pass, each connected tile gets
 * marked, using the ti_client field.  This marking is needed to
 * avoid infinite searches on circular structures.  The second pass
 * is used to clear the markings again.
 */

/* The following structure is used to hold several pieces
 * of information that must be passed through multiple
 * levels of search function.
 */
	
struct conSrArg
{
    CellDef *csa_def;			/* Definition being searched. */
    Plane *csa_plane;			/* Current plane being searched. */
    TileTypeBitMask *csa_connect;	/* Table indicating what connects
					 * to what.
					 */
    int (*csa_clientFunc)();		/* Client function to call. */
    ClientData csa_clientData;		/* Argument for clientFunc. */
    bool csa_clear;			/* FALSE means pass 1, TRUE
					 * means pass 2.
					 */
    Rect csa_bounds;			/* Area that limits search. */
};

/* For DBTreeSrConnect, the extraction proceeds in one pass, copying
 * all connected stuff from a hierarchy into a single cell.  A list
 * is kept to record areas that still have to be searched for
 * hierarchical stuff.
 */

struct conSrArg2
{
    Rect csa2_area;			/* Area that we know is connected, but
					 * which may be connected to other
					 * stuff.
					 */
    int csa2_type;			/* Type of material we found. */
    struct conSrArg2 *csa2_next;	/* Pointer to next in list. */
};

int dbcReturn;				/* dbcConnectFunc always returns this
					 * value.  It can be set to 1 by a
					 * higher-level procedure to abort
					 * searches after the first value is
					 * found.
					 */


/*
 * ----------------------------------------------------------------------------
 *
 * DBSrConnect --
 *
 * 	Search through a cell to find all paint that is electrically
 *	connected to things in a given starting area.
 *
 * Results:
 *	0 is returned if the search finished normally.  1 is returned
 *	if the search was aborted.
 *
 * Side effects:
 *	The search starts from one (random) non-space tile in "startArea"
 *	that matches the types in the mask parameter.  For every paint
 *	tile that is electrically connected to the initial tile and that
 *	intersects the rectangle "bounds", func is called.  Func should
 *	have the following form:
 *
 *	    int
 *	    func(tile, clientData)
 *		Tile *tile;
 *		ClientData clientData;
 *    	    {
 *	    }
 *
 *	The clientData passed to func is the same one that was passed
 *	to us.  Func returns 0 under normal conditions;  if it returns
 *	1 then the search is aborted.
 *
 *				*** WARNING ***
 *	
 *	Func should not modify any paint during the search, since this
 *	will mess up pointers kept by these procedures and likely cause
 *	a core-dump.
 *
 * ----------------------------------------------------------------------------
 */

int
DBSrConnect(def, startArea, mask, connect, bounds, func, clientData)
    CellDef *def;		/* Cell definition in which to carry out
				 * the connectivity search.  Only paint
				 * in this definition is considered.
				 */
    Rect *startArea;		/* Area to search for an initial tile.  Only
				 * tiles OVERLAPPING the area are considered.
				 * This area should have positive x and y
				 * dimensions.
				 */
    TileTypeBitMask *mask;	/* Only tiles of one of these types are used
				 * as initial tiles.
				 */
    TileTypeBitMask *connect;	/* Pointer to a table indicating what tile
				 * types connect to what other tile types.
				 * Each entry gives a mask of types that
				 * connect to tiles of a given type.
				 */
    Rect *bounds;		/* Area, in coords of scx->scx_use->cu_def,
				 * that limits the search:  only tiles
				 * overalapping this area will be returned.
				 * Use TiPlaneRect to search everywhere.
				 */
    int (*func)();		/* Function to apply at each connected tile. */
    ClientData clientData;	/* Client data for above function. */

{
    struct conSrArg csa;
    int startPlane, result;
    Tile *startTile;			/* Starting tile for search. */
    extern int dbSrConnectFunc();	/* Forward declaration. */
    extern int dbSrConnectStartFunc();

    result = 0;
    csa.csa_def = def;
    csa.csa_bounds = *bounds;

    /* Find a starting tile (if there are many tiles underneath the
     * starting area, pick any one).  The search function just saves
     * the tile address and returns.
     */

    startTile = NULL;
    for (startPlane = PL_TECHDEPBASE; startPlane < DBNumPlanes; startPlane++)
    {
	if (DBSrPaintArea((Tile *) NULL,
	    def->cd_planes[startPlane], startArea, mask,
	    dbSrConnectStartFunc, (ClientData) &startTile) != 0) break;
    }
    if (startTile == NULL) return 0;

    /* Pass 1.  During this pass the client function gets called. */

    csa.csa_clientFunc = func;
    csa.csa_clientData = clientData;
    csa.csa_clear = FALSE;
    csa.csa_connect = connect;
    csa.csa_plane = def->cd_planes[startPlane];
    if (dbSrConnectFunc(startTile, &csa) != 0) result = 1;

    /* Pass 2.  Don't call any client function, just clear the marks.
     * Don't allow any interruptions.
     */

    SigDisableInterrupts();
    csa.csa_clientFunc = NULL;
    csa.csa_clear = TRUE;
    csa.csa_plane = def->cd_planes[startPlane];
    (void) dbSrConnectFunc(startTile, &csa);
    SigEnableInterrupts();

    return result;
}

int
dbSrConnectStartFunc(tile, pTile)
    Tile *tile;			/* This will be the starting tile. */
    Tile **pTile;		/* We store tile's address here. */
{
    *pTile = tile;
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbSrConnectFunc --
 *
 * 	This search function gets called by DBSrPaintArea as part
 *	of DBSrConnect, and also recursively by itself.  Each invocation
 *	is made to process a single tile that is of interest.
 *
 * Results:
 *	0 is returned unless the client function returns a non-zero
 *	value, in which case 1 is returned.
 *
 * Side effects:
 *	If this tile has been seen before, then just return
 *	immediately. If this tile hasn't been seen before, it is
 *	marked and the client procedure is called.  A NULL client
 *	procedure is not called, of course.  In addition, we scan
 *	the tiles perimeter for any connected tiles, and call
 *	ourselves recursively on them.
 *
 * Design note:
 *	This one procedure is used during both the marking and clearing
 *	passes, so "seen before" is a function both of the ti_client
 *	field in the tile and the csa_clear value.
 *
 * ----------------------------------------------------------------------------
 */

int
dbSrConnectFunc(tile, csa)
    Tile *tile;			/* Tile that is connected. */
    struct conSrArg *csa;	/* Contains information about the search. */
{
    register Tile *t2;
    Rect tileArea;
    int i;
    TileTypeBitMask *connectMask;
    unsigned int planes;

    TiToRect(tile, &tileArea);

    /* Make sure this tile overlaps the area we're interested in. */

    if (!GEO_OVERLAP(&tileArea, &csa->csa_bounds)) return 0;

    /* See if we've already been here before, and mark the tile as already
     * visited.
     */

    if (csa->csa_clear)
    {
	if (tile->ti_client == (ClientData) MINFINITY) return 0;
	tile->ti_client = (ClientData) MINFINITY;
    }
    else
    {
	if (tile->ti_client != (ClientData) MINFINITY) return 0;
	tile->ti_client = (ClientData) 1;
    }

    /* Call the client function, if there is one. */

    if (csa->csa_clientFunc != NULL)
    {
	if ((*csa->csa_clientFunc)(tile, csa->csa_clientData) != 0)
	    return 1;
    }

    /* Now search around each of the four sides of this tile for
     * connected tiles.  For each one found, call ourselves
     * recursively.
     */
    
    connectMask = &csa->csa_connect[TiGetType(tile)];

    /* Left side: */

    for (t2 = tile->ti_bl; BOTTOM(t2) < tileArea.r_ytop; t2 = t2->ti_rt)
    {
	if (TTMaskHasType(connectMask, TiGetType(t2)))
	{
	    if (csa->csa_clear)
	    {
		if (t2->ti_client == (ClientData) MINFINITY) continue;
	    }
	    else if (t2->ti_client != (ClientData) MINFINITY) continue;
	    if (dbSrConnectFunc(t2, csa) != 0) return 1;
	}
    }

    /* Bottom side: */

    for (t2 = tile->ti_lb; LEFT(t2) < tileArea.r_xtop; t2 = t2->ti_tr)
    {
	if (TTMaskHasType(connectMask, TiGetType(t2)))
	{
	    if (csa->csa_clear)
	    {
		if (t2->ti_client == (ClientData) MINFINITY) continue;
	    }
	    else if (t2->ti_client != (ClientData) MINFINITY) continue;
	    if (dbSrConnectFunc(t2, csa) != 0) return 1;
	}
    }

    /* Right side: */

    for (t2 = tile->ti_tr; ; t2 = t2->ti_lb)
    {
	if (TTMaskHasType(connectMask, TiGetType(t2)))
	{
	    if (csa->csa_clear)
	    {
		if (t2->ti_client == (ClientData) MINFINITY) goto nextRight;
	    }
	    else if (t2->ti_client != (ClientData) MINFINITY) goto nextRight;
	    if (dbSrConnectFunc(t2, csa) != 0) return 1;
	}
	nextRight: if (BOTTOM(t2) <= tileArea.r_ybot) break;
    }

    /* Top side: */

    for (t2 = tile->ti_rt; ; t2 = t2->ti_bl)
    {
	if (TTMaskHasType(connectMask, TiGetType(t2)))
	{
	    if (csa->csa_clear)
	    {
		if (t2->ti_client == (ClientData) MINFINITY) goto nextTop;
	    }
	    else if (t2->ti_client != (ClientData) MINFINITY) goto nextTop;
	    if (dbSrConnectFunc(t2, csa) != 0) return 1;
	}
	nextTop: if (LEFT(t2) <= tileArea.r_xbot) break;
    }

    /* Lastly, check to see if this tile connects to anything on
     * other planes.  If so, search those planes.
     */
    
    planes = DBConnPlanes[TiGetType(tile)];
    if (planes != 0)
    {
        struct conSrArg newcsa;
	Rect newArea;

	newcsa = *csa;
	TiToRect(tile, &newArea);
	GEO_EXPAND(&newArea, 1, &newArea);
	for (i = PL_TECHDEPBASE; i < DBNumPlanes; i++)
	{
	    newcsa.csa_plane = newcsa.csa_def->cd_planes[i];
	    if (DBSrPaintArea((Tile *) NULL, newcsa.csa_plane,
		&newArea, connectMask, dbSrConnectFunc,
		(ClientData) &newcsa) != 0) return 1;
	}
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbcUnconnectFunc --
 *
 * 	This search function is invoked by DBSrPaintArea from
 *	DBTreeCopyConnect, whenever a tile is found in the result
 *	plane that is NOT connected to the current area.  It
 *	returns 1 so that DBTreeCopyConnect will know it has
 *	to do a hierarchical check for the current area.
 *
 * Results:
 *	If the current tile OVERLAPS the search area, 1 is
 *	returned.  Otherwise 0 is returned.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
dbcUnconnectFunc(tile, area)
    Tile *tile;			/* Current tile. */
    Rect *area;			/* Area of overlap that we're interested in
				 * (passed in as ClientData).
				 */
{
    Rect tileArea;

    TiToRect(tile, &tileArea);
    if (GEO_OVERLAP(&tileArea, area)) return 1;
    else return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbcConnectFunc --
 *
 * 	This procedure is invoked by DBTreeSrTiles from DBTreeCopyConnect,
 *	whenever a tile is found that is connected to the current area
 *	being processed.  If the tile overlaps the search area in a non-
 *	trivial way (i.e. more than a 1x1 square of overlap at a corner)
 *	then the area of the tile is added onto the list of things to check.
 *	The "non-trivial" overlap check is needed to prevent caddy-corner
 *	tiles from being considered as connected.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Adds a new record to the current check list.
 *
 * ----------------------------------------------------------------------------
 */

int
dbcConnectFunc(tile, cx)
    Tile *tile;			/* Tile found. */
    TreeContext *cx;		/* Describes context of search.  The client
				 * data is a pointer to the list head of
				 * the conSrArg2's describing the areas
				 * left to check.
				 */
{
    struct conSrArg2 **pHead = (struct conSrArg2 **) cx->tc_filter->tf_arg;
    struct conSrArg2 *newCsa2;
    Rect tileArea;
    register Rect *srArea;

    TiToRect(tile, &tileArea);
    srArea = &cx->tc_scx->scx_area;
    if (((tileArea.r_xbot >= srArea->r_xtop-1) ||
	(tileArea.r_xtop <= srArea->r_xbot+1)) &&
	((tileArea.r_ybot >= srArea->r_ytop-1) ||
	(tileArea.r_ytop <= srArea->r_ybot+1)))
    {
	/* If the search area is only one unit wide or tall, then it's
	 * OK to have only a small overlap.  This happens only when
	 * looking for an initial search tile.
	 */

	if (((srArea->r_xtop-1) != srArea->r_xbot)
	    && ((srArea->r_ytop-1) != srArea->r_ybot)) return 0;
    }
    MALLOC(struct conSrArg2 *, newCsa2, sizeof (struct conSrArg2));
    GeoTransRect(&cx->tc_scx->scx_trans, &tileArea, &newCsa2->csa2_area);
    newCsa2->csa2_type = TiGetType(tile);
    newCsa2->csa2_next = *pHead;
    *pHead = newCsa2;
    return dbcReturn;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTreeCopyConnect --
 *
 * 	This procedure copies connected information from a given cell
 *	hierarchy to a given (flat) cell.  Starting from the tile underneath
 *	the given area, this procedure finds all paint in all cells
 *	that is connected to that information.  All such paint is
 *	copied into the result cell.  If there are several electrically
 *	distinct nets underneath the given area, one of them is picked
 *	at more-or-less random.
 *
 *	Modified so the result cell is NOT first cleared of all paint.  This
 *	allows multiple calls, to highlight incomplete routing nets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the result cell are modified.
 *
 * ----------------------------------------------------------------------------
 */

void
DBTreeCopyConnect(scx, mask, xMask, connect, area, destUse)
    SearchContext *scx;			/* Describes starting area.  The
					 * scx_use field gives the root of
					 * the hierarchy to search, and the
					 * scx_area field gives the starting
					 * area.  An initial tile must overlap
					 * this area.  The transform is from
					 * coords of scx_use to destUse.
					 */
    TileTypeBitMask *mask;		/* Tile types to start from in area. */
    int xMask;				/* Information must be expanded in all
					 * of the windows indicated by this
					 * mask.  Use 0 to consider all info
					 * regardless of expansion.
					 */
    TileTypeBitMask *connect;		/* Points to table that defines what
					 * each tile type is considered to
					 * connect to.  Use DBConnectTbl as
					 * a default.
					 */
    Rect *area;				/* The resulting information is
					 * clipped to this area.  Pass
					 * TiPlaneRect to get everything.
					 */
    CellUse *destUse;			/* Result use in which to place
					 * anything connected to material of
					 * type mask in area of rootUse.
					 */
{
    SearchContext scx2;
    TileTypeBitMask notConnectMask, *connectMask;
    struct conSrArg2 *list, *current;
    CellDef *def = destUse->cu_def;
    int pNum;

    /* Get one initial tile and put it our list. */

    list = NULL;
    dbcReturn = 1;
    (void) DBTreeSrTiles(scx, mask, xMask, dbcConnectFunc, (ClientData) &list);
    dbcReturn = 0;

    /* Enter the main processing loop, pulling areas from the list
     * and processing them one at a time until the list is empty.
     */
    
    scx2 = *scx;
    while (list != NULL)
    {
	current = list;
	list = current->csa2_next;

	/* Clip the current area down to something that overlaps the
	 * area of interest.
	 */
	
	GeoClip(&current->csa2_area, area);
	if (GEO_RECTNULL(&current->csa2_area)) goto endOfLoop;

	/* See if the destination cell contains stuff over the whole
	 * current area (on its home plane) that is connected to it.
	 * If so, then there's no need to process the current area,
	 * since any processing that is needed was already done before.
	 */
	
	connectMask = &connect[current->csa2_type];
	/* in the case of contact bits, the types underneath
	   must be constituents of the contact before we  punt */
	if (TTMaskHasType(&DBContactBits,current->csa2_type))
	{
	     TTMaskSetOnlyType(&notConnectMask,current->csa2_type);
	     TTMaskCom(&notConnectMask);
	}
	else
	{
	     TTMaskCom2(&notConnectMask, connectMask);
	}
	pNum = DBPlane(current->csa2_type);
	if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
	    &current->csa2_area, &notConnectMask,
	    dbcUnconnectFunc, (ClientData) &current->csa2_area) != 0)
	{
	    /* Since the whole area of this tile hasn't been recorded,
	     * we must process its area to find any other tiles that
	     * connect to it.  Add each of them to the list of things
	     * to process.  We have to expand the search area by 1 unit
	     * on all sides because DBTreeSrTiles only returns things
	     * that overlap the search area, and we want things that
	     * even just touch.
	     */

	    scx2.scx_area = current->csa2_area;
	    scx2.scx_area.r_xbot -= 1;
	    scx2.scx_area.r_ybot -= 1;
	    scx2.scx_area.r_xtop += 1;
	    scx2.scx_area.r_ytop += 1;
	    (void) DBTreeSrTiles(&scx2, connectMask,
		xMask, dbcConnectFunc, (ClientData) &list);
	}

	/* Lastly, paint this tile into the destination cell.  This
	 * marks its area has having been processed.  Then recycle
	 * the storage for the current list element.
	 */
	DBPaintPlane(def->cd_planes[pNum], &current->csa2_area,
	    DBStdPaintTbl(current->csa2_type, pNum), (PaintUndoInfo *) NULL);
	endOfLoop: FREE((char *) current);
    }

    /* Finally, when all done, recompute the bounding box of the
     * destination and record its area for redisplay.
     */
    
    DBReComputeBbox(def);
}
