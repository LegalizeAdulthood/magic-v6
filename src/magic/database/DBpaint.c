/*
 * DBpaint.c --
 *
 * Fast paint primitive.
 * This uses a very fast, heavily tuned algorithm for painting.
 * The basic outer loop is a non-recursive area enumeration, and
 * the inner loop attempts to avoid merging as much as possible.
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

/* #define	PAINTDEBUG /* For debugging */

#ifndef lint
static char rcsid[] = "$Header: DBpaint.c,v 6.0 90/08/28 18:10:04 mayo Exp $";
#endif  not lint

#include <sys/types.h>
#include <stdio.h>
#include "magic.h"
#include "malloc.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "graphics.h"
#include "windows.h"
#include "dbwind.h"
#include "signals.h"
#include "textio.h"
#include "undo.h"

/* ---------------------- Imports from DBundo.c ----------------------- */

extern CellDef *dbUndoLastCell;
extern UndoType dbUndoIDPaint;

/* ----------------------- Forward declarations ----------------------- */

Tile *dbPaintMerge();
Tile *dbMergeType();
Tile *dbPaintMergeVert();

#ifdef	PAINTDEBUG
int dbPaintDebug = 0;
#endif	PAINTDEBUG

/* ----------------------- Flags to dbPaintMerge ---------------------- */

#define MRG_TOP		0x01
#define	MRG_LEFT	0x02
#define	MRG_RIGHT	0x04
#define	MRG_BOTTOM	0x08

/* -------------- Macros to see if merging is possible ---------------- */

#define	CANMERGE_Y(t1, t2)	(	LEFT(t1) == LEFT(t2) \
				    &&  TiGetType(t1) == TiGetType(t2) \
				    &&	RIGHT(t1) == RIGHT(t2) )

#define	CANMERGE_X(t1, t2)	(	BOTTOM(t1) == BOTTOM(t2) \
				    &&  TiGetType(t1) == TiGetType(t2) \
				    &&	TOP(t1) == TOP(t2) )

#define	SELMERGE_Y(t1, t2, msk)	(       LEFT(t1) == LEFT(t2) \
				    &&  TiGetType(t1) == TiGetType(t2) \
				    &&	RIGHT(t1) == RIGHT(t2) \
				    &&  ! TTMaskHasType(msk, TiGetType(t1)) )

#define	SELMERGE_X(t1, t2, msk)	(	BOTTOM(t1) == BOTTOM(t2) \
				    &&  TiGetType(t1) == TiGetType(t2) \
				    &&	TOP(t1) == TOP(t2) \
				    &&  ! TTMaskHasType(msk, TiGetType(t1)) )


/* This macro seems to buy us about 15% in speed */
#define	TISPLITX(res, otile, xcoord) \
    { \
	register Tile *xtile = otile, *xxnew, *xp; \
	register int x = xcoord; \
 \
	MALLOC(Tile *, xxnew, sizeof (Tile)); \
	xxnew->ti_client = (ClientData) MINFINITY; \
 \
	LEFT(xxnew) = x, BOTTOM(xxnew) = BOTTOM(xtile); \
	BL(xxnew) = xtile, TR(xxnew) = TR(xtile), RT(xxnew) = RT(xtile); \
 \
	/* Left edge */ \
	for (xp = TR(xtile); BL(xp) == xtile; xp = LB(xp)) BL(xp) = xxnew; \
	TR(xtile) = xxnew; \
 \
	/* Top edge */ \
	for (xp = RT(xtile); LEFT(xp) >= x; xp = BL(xp)) LB(xp) = xxnew; \
	RT(xtile) = xp; \
 \
	/* Bottom edge */ \
	for (xp = LB(xtile); RIGHT(xp) <= x; xp = TR(xp)) /* nothing */; \
	for (LB(xxnew) = xp; RT(xp) == xtile; RT(xp) = xxnew, xp = TR(xp)); \
	res = xxnew; \
    }

/* Record undo information */
#define	DBPAINTUNDO(tile, newType, undo) \
    { \
	register paintUE *xxpup; \
\
	if (undo->pu_def != dbUndoLastCell) dbUndoEdit(undo->pu_def); \
\
	xxpup = (paintUE *) UndoNewEvent(dbUndoIDPaint, sizeof(paintUE)); \
	if (xxpup) \
	{ \
	    xxpup->pue_rect.r_xbot = LEFT(tile); \
	    xxpup->pue_rect.r_xtop = RIGHT(tile); \
	    xxpup->pue_rect.r_ybot = BOTTOM(tile); \
	    xxpup->pue_rect.r_ytop = TOP(tile); \
	    xxpup->pue_oldtype = TiGetType(tile); \
	    xxpup->pue_newtype = newType; \
	    xxpup->pue_plane = undo->pu_pNum; \
	} \
    }

/*
 * ----------------------------------------------------------------------------
 *
 * DBPaintPlane --
 *
 * Paint a rectangular area ('area') on a single tile plane ('plane').
 *
 * The argument 'resultTbl' is a table, indexed by the type of each tile
 * found while enumerating 'area', that gives the result type for this
 * operation.  The semantics of painting, erasing, and "writing" (storing
 * a new type in the area without regard to the previous contents) are
 * all encapsulated in this table.  (Note:  There is now a procedure called
 * DBPaintPlaneByProc for those cases where a table doesn't suffice.)
 *
 * If undo is desired, 'undo' should point to a PaintUndoInfo struct
 * that contains everything needed to build an undo record.  Otherwise,
 * 'undo' can be NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * REMINDER:
 *	Callers should remember to set the CDMODIFIED and CDGETNEWSTAMP
 *	bits in the cell definition containing the plane being painted.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBPaintPlane(plane, area, resultTbl, undo)
    Plane *plane;		/* Plane whose paint is to be modified */
    register Rect *area;	/* Area to be changed */
    PaintResultType *resultTbl;	/* Table, indexed by the type of tile already
				 * present in the plane, giving the type to
				 * which the existing tile must change as a
				 * result of this paint operation.
				 */
    PaintUndoInfo *undo;	/* Record containing everything needed to
				 * save undo entries for this operation.
				 * If NULL, the undo package is not used.
				 */
{
    Point start;
    int clipTop, mergeFlags;
    TileType oldType, newType;
    register Tile *tile, *tpnew;	/* Used for area search */
    register Tile *newtile, *tp;	/* Used for paint */

    if (area->r_xtop <= area->r_xbot || area->r_ytop <= area->r_ybot)
	return;

    /*
     * The following is a modified version of the area enumeration
     * algorithm.  It expects the in-line paint code below to leave
     * 'tile' pointing to the tile from which we should continue the
     * search.
     */

    start.p_x = area->r_xbot;
    start.p_y = area->r_ytop - 1;
    tile = plane->pl_hint;
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
enumerate:
	if (SigInterruptPending)
	    break;

	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;
	oldType = TiGetType(tile);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE PAINT CODE ----------
	 ***/

	/*
	 * Set up the directions in which we will have to
	 * merge initially.  Clipping can cause some of these
	 * to be turned off.
	 */
	mergeFlags = MRG_TOP | MRG_LEFT;
	if (RIGHT(tile) >= area->r_xtop) mergeFlags |= MRG_RIGHT;
	if (BOTTOM(tile) <= area->r_ybot) mergeFlags |= MRG_BOTTOM;

	/*
	 * The following is a kludge for plowing that should go away
	 * once the plowing code gets stable.  Make sure that the intermediate
	 * coordinate of this tile is reset to "uninitialized".
	 */
	tile->ti_client = (ClientData) MINFINITY;

	/*
	 * Determine new type of this tile.
	 * Change the type if necessary.
	 */
	newType = resultTbl[oldType];
	if (oldType != newType)
	{
	    /*
	     * Clip the tile against the clipping rectangle.
	     * Merging is only necessary if we clip to the left or to
	     * the right, and then only to the top or the bottom.
	     * We do the merge in-line for efficiency.
	     */

	    /* Clip up */
	    if (TOP(tile) > area->r_ytop)
	    {
		newtile = TiSplitY(tile, area->r_ytop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_TOP;
	    }

	    /* Clip down */
	    if (BOTTOM(tile) < area->r_ybot)
	    {
		newtile = tile, tile = TiSplitY(tile, area->r_ybot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_BOTTOM;
	    }

	    /* Clip right */
	    if (RIGHT(tile) > area->r_xtop)
	    {
		TISPLITX(newtile, tile, area->r_xtop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_RIGHT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

	    /* Clip left */
	    if (LEFT(tile) < area->r_xbot)
	    {
		newtile = tile;
		TISPLITX(tile, tile, area->r_xbot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_LEFT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "after clip");
#endif	PAINTDEBUG
	}

	/*
	 * Merge the tile back into the parts of the plane that have
	 * already been visited.  Note that if we clipped in a particular
	 * direction we avoid merging in that direction.
	 *
	 * We avoid calling dbPaintMerge if at all possible.
	 */
	if (mergeFlags & MRG_LEFT)
	{
	    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags, undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_LEFT;
	}
	if (mergeFlags & MRG_RIGHT)
	{
	    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags, undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_RIGHT;
	}

	/*
	 * Cheap and dirty merge -- we don't have to merge to the
	 * left or right, so the top/bottom merge is very fast.
	 *
	 * Now it's safe to change the type of this tile, and
	 * record the event on the undo list.
	 */
	if (undo && oldType != newType && UndoIsEnabled())
	    DBPAINTUNDO(tile, newType, undo);
	TiSetBody(tile, newType);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "changed type");
#endif	PAINTDEBUG

	if (mergeFlags & MRG_TOP)
	{
	    tp = RT(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged up (CHEAP)");
#endif	PAINTDEBUG
	}
	if (mergeFlags & MRG_BOTTOM)
	{
	    tp = LB(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged down (CHEAP)");
#endif	PAINTDEBUG
	}


	/***
	 ***		END OF PAINT CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
paintdone:
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto done;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

done:
    plane->pl_hint = tile;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBPaintPlaneByProc --
 *
 * Just like "DBPaintPlane", but it calls a client-supplied procedure to
 * compute the new tile type instead of looking it up in a table.
 *
 * See "DBPaintPlane" for more details on how this works.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * IMPLEMENTATION NOTE:
 *	This procedure is identical to "DBPaintPlane" except for the arguments
 *	and the line marked "CHANGED".  Be sure to keep the two procedures in 
 *	sync when updating code!
 *
 * REMINDER:
 *	Callers should remember to set the CDMODIFIED and CDGETNEWSTAMP
 *	bits in the cell definition containing the plane being painted.
 *
 * ----------------------------------------------------------------------------
 */
Void
DBPaintPlaneByProc(plane, area, proc, undo)
    Plane *plane;		/* Plane whose paint is to be modified */
    register Rect *area;	/* Area to be changed */
    Void (*proc)();		/* Proc to determine new type of material.
				 * This proc should be of the form:
				 *
				 *    int 
				 *    foo(oldtype)
				 *        int oldtype;
				 *    {
				 *        return int;
				 *    }
				 *
				 * Oldtype is the type of material that
				 * is already there, and the proc should
				 * return the type that is to replace it.
				 */
    PaintUndoInfo *undo;	/* Record containing everything needed to
				 * save undo entries for this operation.
				 * If NULL, the undo package is not used.
				 */

{
    Point start;
    int clipTop, mergeFlags;
    TileType oldType, newType;
    register Tile *tile, *tpnew;	/* Used for area search */
    register Tile *newtile, *tp;	/* Used for paint */

    if (area->r_xtop <= area->r_xbot || area->r_ytop <= area->r_ybot)
	return;

    /*
     * The following is a modified version of the area enumeration
     * algorithm.  It expects the in-line paint code below to leave
     * 'tile' pointing to the tile from which we should continue the
     * search.
     */

    start.p_x = area->r_xbot;
    start.p_y = area->r_ytop - 1;
    tile = plane->pl_hint;
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
enumerate:
	if (SigInterruptPending)
	    break;

	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;
	oldType = TiGetType(tile);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE PAINT CODE ----------
	 ***/

	/*
	 * Set up the directions in which we will have to
	 * merge initially.  Clipping can cause some of these
	 * to be turned off.
	 */
	mergeFlags = MRG_TOP | MRG_LEFT;
	if (RIGHT(tile) >= area->r_xtop) mergeFlags |= MRG_RIGHT;
	if (BOTTOM(tile) <= area->r_ybot) mergeFlags |= MRG_BOTTOM;

	/*
	 * The following is a kludge for plowing that should go away
	 * once the plowing code gets stable.  Make sure that the intermediate
	 * coordinate of this tile is reset to "uninitialized".
	 */
	tile->ti_client = (ClientData) MINFINITY;

	/*
	 * Determine new type of this tile.
	 * Change the type if necessary.
	 */
	newType = (*proc)(oldType);		/* CHANGED */
	if (oldType != newType)
	{
	    /*
	     * Clip the tile against the clipping rectangle.
	     * Merging is only necessary if we clip to the left or to
	     * the right, and then only to the top or the bottom.
	     * We do the merge in-line for efficiency.
	     */

	    /* Clip up */
	    if (TOP(tile) > area->r_ytop)
	    {
		newtile = TiSplitY(tile, area->r_ytop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_TOP;
	    }

	    /* Clip down */
	    if (BOTTOM(tile) < area->r_ybot)
	    {
		newtile = tile, tile = TiSplitY(tile, area->r_ybot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_BOTTOM;
	    }

	    /* Clip right */
	    if (RIGHT(tile) > area->r_xtop)
	    {
		TISPLITX(newtile, tile, area->r_xtop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_RIGHT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

	    /* Clip left */
	    if (LEFT(tile) < area->r_xbot)
	    {
		newtile = tile;
		TISPLITX(tile, tile, area->r_xbot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_LEFT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "after clip");
#endif	PAINTDEBUG
	}

	/*
	 * Merge the tile back into the parts of the plane that have
	 * already been visited.  Note that if we clipped in a particular
	 * direction we avoid merging in that direction.
	 *
	 * We avoid calling dbPaintMerge if at all possible.
	 */
	if (mergeFlags & MRG_LEFT)
	{
	    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags, undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_LEFT;
	}
	if (mergeFlags & MRG_RIGHT)
	{
	    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags, undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_RIGHT;
	}

	/*
	 * Cheap and dirty merge -- we don't have to merge to the
	 * left or right, so the top/bottom merge is very fast.
	 *
	 * Now it's safe to change the type of this tile, and
	 * record the event on the undo list.
	 */
	if (undo && oldType != newType && UndoIsEnabled())
	    DBPAINTUNDO(tile, newType, undo);
	TiSetBody(tile, newType);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "changed type");
#endif	PAINTDEBUG

	if (mergeFlags & MRG_TOP)
	{
	    tp = RT(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged up (CHEAP)");
#endif	PAINTDEBUG
	}
	if (mergeFlags & MRG_BOTTOM)
	{
	    tp = LB(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged down (CHEAP)");
#endif	PAINTDEBUG
	}


	/***
	 ***		END OF PAINT CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
paintdone:
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto done;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

done:
    plane->pl_hint = tile;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBPaintPlaneMergeOnce --
 *
 * Paint a rectangular area ('area') on a single tile plane ('plane').
 * This is identical to DBPaintPlane(), except that we work in two
 * passes:
 *
 *	Pass 1: clip all tiles to lie inside the area to be painted,
 *		merging all outside tiles as required.  Change the
 *		types of each of these internal tiles.
 *
 *	Pass 2:	re split and merge to insure that the database is
 *		once again in maximal horizontal strips.
 *
 * See DBPaintPlane for other comments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBPaintPlaneMergeOnce(plane, area, resultTbl, undo)
    Plane *plane;		/* Plane whose paint is to be modified */
    register Rect *area;	/* Area to be changed */
    PaintResultType *resultTbl;	/* Table, indexed by the type of tile already
				 * present in the plane, giving the type to
				 * which the existing tile must change as a
				 * result of this paint operation.
				 */
    PaintUndoInfo *undo;	/* Record containing everything needed to
				 * save undo entries for this operation.
				 * If NULL, the undo package is not used.
				 */
{
    Point start;
    int clipTop, mergeFlags;
    TileType oldType, newType;
    register Tile *tile, *tpnew;	/* Used for area search */
    register Tile *newtile, *tp;	/* Used for paint */

    if (area->r_xtop <= area->r_xbot || area->r_ytop <= area->r_ybot)
	return;

    /*
     * The following is a modified version of the area enumeration
     * algorithm.  It expects the in-line paint code below to leave
     * 'tile' pointing to the tile from which we should continue the
     * search.
     */

    start.p_x = area->r_xbot;
    start.p_y = area->r_ytop - 1;
    tile = plane->pl_hint;
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
enumerate:
	if (SigInterruptPending)
	    break;

	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;
	oldType = TiGetType(tile);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "first area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE PAINT CODE ----------
	 ***/

	/*
	 * Determine new type of this tile.
	 * Change the type if necessary.
	 */
	newType = resultTbl[oldType];
	if (oldType != newType)
	{
	    /*
	     * Clip the tile against the clipping rectangle.
	     * Merging of the outside tiles is only necessary if we clip
	     * to the left or to the right, and then only to the top or
	     * the bottom.  We do the merge in-line for efficiency.
	     */

	    /* Clip up */
	    if (TOP(tile) > area->r_ytop)
	    {
		newtile = TiSplitY(tile, area->r_ytop);
		TiSetBody(newtile, TiGetBody(tile));
	    }

	    /* Clip down */
	    if (BOTTOM(tile) < area->r_ybot)
	    {
		newtile = tile, tile = TiSplitY(tile, area->r_ybot);
		TiSetBody(tile, TiGetBody(newtile));
	    }

	    /* Clip right */
	    if (RIGHT(tile) > area->r_xtop)
	    {
		TISPLITX(newtile, tile, area->r_xtop);
		TiSetBody(newtile, TiGetBody(tile));

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

	    /* Clip left */
	    if (LEFT(tile) < area->r_xbot)
	    {
		newtile = tile;
		TISPLITX(tile, tile, area->r_xbot);
		TiSetBody(tile, TiGetBody(newtile));

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp)) TiJoinY(newtile, tp, plane);
	    }

#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "after clip");
#endif	PAINTDEBUG

	    /* Record the type of the new tile */
	    if (undo && UndoIsEnabled())
		DBPAINTUNDO(tile, newType, undo);
	    TiSetBody(tile, newType);
	}

	/***
	 ***		END OF PAINT CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto changedone;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

changedone:
    /*
     * Done with the area enumeration to change the types of all tiles
     * in this area.  Now go back and re-merge everything to form
     * maximal horizontal strips.  The following is another in-line
     * version of area enumeration, but is non-interruptible.
     */
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
mergenum:
	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;
	oldType = TiGetType(tile);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "merge area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE MERGE CODE ----------
	 ***/

	/* Set up initial merge directions */
	mergeFlags = MRG_TOP | MRG_LEFT;
	if (RIGHT(tile) >= area->r_xtop) mergeFlags |= MRG_RIGHT;
	if (BOTTOM(tile) <= area->r_ybot) mergeFlags |= MRG_BOTTOM;

	/*
	 * Merge the tile back into the parts of the plane that have
	 * already been visited.  Note that if we clipped in a particular
	 * direction we avoid merging in that direction.
	 * We avoid calling dbPaintMerge if at all possible.
	 */
	newType = TiGetType(tile);
	if (mergeFlags & MRG_LEFT)
	{
	    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags,
			    (PaintUndoInfo *) NULL);
		    goto mergedone;
		}
	    mergeFlags &= ~MRG_LEFT;
	}
	if (mergeFlags & MRG_RIGHT)
	{
	    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMerge(tile, newType, plane, mergeFlags,
			    (PaintUndoInfo *) NULL);
		    goto mergedone;
		}
	    mergeFlags &= ~MRG_RIGHT;
	}

	/*
	 * Cheap and dirty merge -- we don't have to merge to the
	 * left or right, so the top/bottom merge is very fast.
	 */

	if (mergeFlags & MRG_TOP)
	{
	    tp = RT(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged up (CHEAP)");
#endif	PAINTDEBUG
	}
	if (mergeFlags & MRG_BOTTOM)
	{
	    tp = LB(tile);
	    if (CANMERGE_Y(tile, tp)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged down (CHEAP)");
#endif	PAINTDEBUG
	}


	/***
	 ***		END OF MERGE CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
mergedone:
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto mergenum;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto done;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto mergenum;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

done:
    plane->pl_hint = tile;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbPaintMerge -- 
 *
 * The tile 'tp' is to be changed to type 'newtype'.  To maintain
 * maximal horizontal strips, it may be necessary to merge the new
 * 'tp' with its neighbors.
 *
 * This procedure splits off the biggest segment along the top of the
 * tile 'tp' that can be merged with its neighbors to the left and right
 * (depending on which of MRG_LEFT and MRG_RIGHT are set in the merge flags),
 * then changes the type of 'tp' to 'newtype' and merges to the left, right,
 * top, and bottom (in that order).
 *
 * Results:
 *	Returns a pointer to the topmost tile resulting from any splits
 *	and merges of the original tile 'tp'.  By the maximal horizontal
 *	strip property and the fact that the original tile 'tp' gets
 *	painted a single color, we know that this topmost resulting tile
 *	extends across the entire top of the area occupied by 'tp'.
 *
 *	NOTE: the only tile whose type is changed is 'tp'.  Any tiles
 *	resulting from splits below this tile will not have had their
 *	types changed.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * THIS IS SLOW, SO SHOULD BE AVOIDED IF AT ALL POSSIBLE.
 * THE CODE ABOVE GOES TO GREAT LENGTHS TO DO SO.
 *
 * ----------------------------------------------------------------------------
 */

Tile *
dbPaintMerge(tile, newType, plane, mergeFlags, undo)
    register Tile *tile;	/* Tile to be merged with its neighbors */
    register TileType newType;	/* Type to which we will change 'tile' */
    Plane *plane;		/* Plane on which this resides */
    int mergeFlags;		/* Specify which directions to merge */
    PaintUndoInfo *undo;	/* See DBPaintPlane() above */
{
    register Tile *tp, *tpLast;
    register int ysplit;

    ysplit = BOTTOM(tile);
    if (mergeFlags & MRG_LEFT)
    {
	/*
	 * Find the split point along the LHS of tile.
	 * If the topmost tile 'tp' along the LHS is of type 'newType'
	 * the split point will be no lower than the bottom of 'tp'.
	 * If the topmost tile is NOT of type 'newType', then the split
	 * point will be no lower than the top of the first tile along
	 * the LHS that is of type 'newType'.
	 */
	for (tpLast = NULL, tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
	    if (TiGetType(tp) == newType)
		tpLast = tp;

	/* If the topmost LHS tile is not of type 'newType', we don't merge */
	if (tpLast == NULL || TOP(tpLast) < TOP(tile))
	{
	    mergeFlags &= ~MRG_LEFT;
	    if (tpLast && TOP(tpLast) > ysplit) ysplit = TOP(tpLast);
	}
	else if (BOTTOM(tpLast) > ysplit) ysplit = BOTTOM(tpLast);
    }

    if (mergeFlags & MRG_RIGHT)
    {
	/*
	 * Find the split point along the RHS of 'tile'.
	 * If the topmost tile 'tp' along the RHS is of type 'newType'
	 * the split point will be no lower than the bottom of 'tp'.
	 * If the topmost tile is NOT of type 'newType', then the split
	 * point will be no lower than the top of the first tile along
	 * the RHS that is of type 'newType'.
	 */
	tp = TR(tile);
	if (TiGetType(tp) == newType)
	{
	    if (BOTTOM(tp) > ysplit) ysplit = BOTTOM(tp);
	}
	else
	{
	    /* Topmost RHS tile is not of type 'newType', so don't merge */
	    do
		tp = LB(tp);
	    while (TiGetType(tp) != newType && TOP(tp) > ysplit);
	    if (TOP(tp) > ysplit) ysplit = TOP(tp);
	    mergeFlags &= ~MRG_RIGHT;
	}
    }

    /*
     * If 'tile' must be split horizontally, do so.
     * Any merging to the bottom will be delayed until the split-off
     * bottom tile is processed on a subsequent iteration of the area
     * enumeration loop in DBPaintPlane().
     */
    if (ysplit > BOTTOM(tile))
    {
	mergeFlags &= ~MRG_BOTTOM;
	tp = TiSplitY(tile, ysplit);
	TiSetBody(tp, TiGetType(tile));
	tile = tp;
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) after split");
#endif	PAINTDEBUG
    }

    /*
     * Set the type of the new tile.
     * Record any undo information.
     */
    if (undo && TiGetType(tile) != newType && UndoIsEnabled())
	DBPAINTUNDO(tile, newType, undo);
    TiSetBody(tile, newType);
#ifdef	PAINTDEBUG
    if (dbPaintDebug)
	dbPaintShowTile(tile, undo, "(DBMERGE) changed type");
#endif	PAINTDEBUG

    /*
     * Do the merging.
     * We are guaranteed that at most one tile abuts 'tile' on
     * any side that we will merge to, and that this tile is
     * of type 'newType'.
     */
    if (mergeFlags & MRG_LEFT)
    {
	tp = BL(tile);
	if (TOP(tp) > TOP(tile))
	    tpLast = TiSplitY(tp, TOP(tile)), TiSetBody(tpLast, newType);
	if (BOTTOM(tp) < BOTTOM(tile)) tp = TiSplitY(tp, BOTTOM(tile));
	TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged left");
#endif	PAINTDEBUG
    }
    if (mergeFlags & MRG_RIGHT)
    {
	tp = TR(tile);
	if (TOP(tp) > TOP(tile))
	    tpLast = TiSplitY(tp, TOP(tile)), TiSetBody(tpLast, newType);
	if (BOTTOM(tp) < BOTTOM(tile)) tp = TiSplitY(tp, BOTTOM(tile));
	TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged right");
#endif	PAINTDEBUG
    }
    if (mergeFlags&MRG_TOP)
    {
	tp = RT(tile);
	if (CANMERGE_Y(tp, tile)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged up");
#endif	PAINTDEBUG
    }
    if (mergeFlags&MRG_BOTTOM)
    {
	tp = LB(tile);
	if (CANMERGE_Y(tp, tile)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged down");
#endif	PAINTDEBUG
    }

    return (tile);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBPaintType --
 *
 * Paint a rectangular area ('area') of type ('newType') on plane ('plane').
 * Merge only with neighbors of the same type and client data.
 *
 * If undo is desired, 'undo' should point to a PaintUndoInfo struct
 * that contains everything needed to build an undo record.  Otherwise,
 * 'undo' can be NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * REMINDER:
 *	Callers should remember to set the CDMODIFIED and CDGETNEWSTAMP
 *	bits in the cell definition containing the plane being painted.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBPaintType(plane, area, resultTbl, client, undo, tileMask)
    Plane *plane;		/* Plane whose paint is to be modified */
    register Rect *area;	/* Area to be changed */
    PaintResultType *resultTbl;	/* Table, indexed by the type of tile already
				 * present in the plane, giving the type to
				 * which the existing tile must change as a
				 * result of this paint operation.
				 */
    ClientData client;		/* ClientData for tile	*/
    PaintUndoInfo *undo;	/* Record containing everything needed to
				 * save undo entries for this operation.
				 * If NULL, the undo package is not used.
				 */
    TileTypeBitMask *tileMask;	/* Mask of un-mergable tile types */
{
    Point start;
    int clipTop, mergeFlags;
    TileType oldType;
    register Tile *tile, *tpnew;	/* Used for area search */
    register Tile *newtile, *tp;	/* Used for paint */
    TileType newType;			/* Type of new tile to be painted */


    if (area->r_xtop <= area->r_xbot || area->r_ytop <= area->r_ybot)
	return;

    /*
     * The following is a modified version of the area enumeration
     * algorithm.  It expects the in-line paint code below to leave
     * 'tile' pointing to the tile from which we should continue the
     * search.
     */

    start.p_x = area->r_xbot;
    start.p_y = area->r_ytop - 1;
    tile = plane->pl_hint;
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
enumerate:

	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE PAINT CODE ----------
	 ***/

	/*
	 * Set up the directions in which we will have to
	 * merge initially.  Clipping can cause some of these
	 * to be turned off.
	 */
	mergeFlags = MRG_TOP | MRG_LEFT;
	if (RIGHT(tile) >= area->r_xtop) mergeFlags |= MRG_RIGHT;
	if (BOTTOM(tile) <= area->r_ybot) mergeFlags |= MRG_BOTTOM;

	/*
	 * Map tile types using the *resultTbl* table.
	 * If the client field of the existing tile differs
	 * from the given client, ignore the type of the existing
	 * tile and treat as painting over space.
	 */

	oldType = TiGetType(tile);
	if ( TiGetClient(tile) == client )
	    newType = resultTbl[oldType];
	else
	{
	    if ( oldType != TT_SPACE )
		/*DEBUG*/ TxPrintf("Overwrite tile type %d\n",oldType);
	    newType = resultTbl[TT_SPACE];
	}

	if (oldType != newType)
	{
	    /*
	     * Clip the tile against the clipping rectangle.
	     * Merging is only necessary if we clip to the left or to
	     * the right, and then only to the top or the bottom.
	     * We do the merge in-line for efficiency.
	     */

	    /* Clip up */
	    if (TOP(tile) > area->r_ytop)
	    {
		newtile = TiSplitY(tile, area->r_ytop);
		TiSetBody(newtile, TiGetBody(tile));
		TiSetClient(newtile, TiGetClient(tile));
		mergeFlags &= ~MRG_TOP;
	    }

	    /* Clip down */
	    if (BOTTOM(tile) < area->r_ybot)
	    {
		newtile = tile, tile = TiSplitY(tile, area->r_ybot);
		TiSetBody(tile, TiGetBody(newtile));
		TiSetClient(tile, TiGetClient(newtile));
		mergeFlags &= ~MRG_BOTTOM;
	    }

	    /* Clip right */
	    if (RIGHT(tile) > area->r_xtop)
	    {
		TISPLITX(newtile, tile, area->r_xtop);
		TiSetBody(newtile, TiGetBody(tile));
		TiSetClient(newtile, TiGetClient(tile));
		mergeFlags &= ~MRG_RIGHT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp) &&
			( (TiGetClient(tp) == TiGetClient(newtile)) ||
			( ! TTMaskHasType(tileMask, TiGetType(tp)) ) ) )
			    TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp) &&
			( (TiGetClient(tp) == TiGetClient(newtile)) ||
			( ! TTMaskHasType(tileMask, TiGetType(tp)) ) ) )
			    TiJoinY(newtile, tp, plane);
	    }

	    /* Clip left */
	    if (LEFT(tile) < area->r_xbot)
	    {
		newtile = tile;
		TISPLITX(tile, tile, area->r_xbot);
		TiSetBody(tile, TiGetBody(newtile));
		TiSetClient(tile, TiGetClient(newtile));
		mergeFlags &= ~MRG_LEFT;

		/* Merge the outside tile to its top */
		tp = RT(newtile);
		if (CANMERGE_Y(newtile, tp) &&
			( (TiGetClient(tp) == TiGetClient(newtile)) ||
			( ! TTMaskHasType(tileMask, TiGetType(tp)) ) ) )
			TiJoinY(newtile, tp, plane);

		/* Merge the outside tile to its bottom */
		tp = LB(newtile);
		if (CANMERGE_Y(newtile, tp) &&
			( (TiGetClient(tp) == TiGetClient(newtile)) ||
			( ! TTMaskHasType(tileMask, TiGetType(tp)) ) ) )
			TiJoinY(newtile, tp, plane);
	    }

#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "after clip");
#endif	PAINTDEBUG
	}

	/*
	 * Merge the tile back into the parts of the plane that have
	 * already been visited.  Note that if we clipped in a particular
	 * direction we avoid merging in that direction.
	 *
	 * We avoid calling dbPaintMerge if at all possible.
	 */
	if (mergeFlags & MRG_LEFT)
	{
	    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
		if ( (TiGetType(tp) == newType) && (tp->ti_client == client) )
		{
		    tile = dbMergeType(tile, newType, plane, mergeFlags, undo, client);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_LEFT;
	}
	if (mergeFlags & MRG_RIGHT)
	{
	    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
		if ( (TiGetType(tp) == newType) && (tp->ti_client == client) )
		{
		    tile = dbMergeType(tile, newType, plane, mergeFlags, undo, client);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_RIGHT;
	}

	/*
	 * Cheap and dirty merge -- we don't have to merge to the
	 * left or right, so the top/bottom merge is very fast.
	 *
	 * Now it's safe to change the type of this tile, and
	 * record the event on the undo list.
	 */
	if (undo && oldType != newType && UndoIsEnabled())
	    DBPAINTUNDO(tile, newType, undo);

	TiSetBody(tile, newType);
	TiSetClient(tile, client);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "changed type");
#endif	PAINTDEBUG

	if (mergeFlags & MRG_TOP)
	{
	    tp = RT(tile);
	    if (CANMERGE_Y(tile, tp) && (tp->ti_client == client))
		TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged up (CHEAP)");
#endif	PAINTDEBUG
	}
	if (mergeFlags & MRG_BOTTOM)
	{
	    tp = LB(tile);
	    if (CANMERGE_Y(tile, tp) && (tp->ti_client == client))
		TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged down (CHEAP)");
#endif	PAINTDEBUG
	}


	/***
	 ***		END OF PAINT CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
paintdone:
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto done;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

done:
    plane->pl_hint = tile;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbMergeType -- 
 *
 * The tile 'tp' is to be changed to type 'newtype'.  To maintain
 * maximal horizontal strips, it may be necessary to merge the new
 * 'tp' with its neighbors.
 *
 * This procedure splits off the biggest segment along the top of the
 * tile 'tp' that can be merged with its neighbors to the left and right
 * (depending on which of MRG_LEFT and MRG_RIGHT are set in the merge flags),
 * then changes the type of 'tp' to 'newtype' and merges to the left, right,
 * top, and bottom (in that order).
 *
 * Results:
 *	Returns a pointer to the topmost tile resulting from any splits
 *	and merges of the original tile 'tp'.  By the maximal horizontal
 *	strip property and the fact that the original tile 'tp' gets
 *	painted a single color, we know that this topmost resulting tile
 *	extends across the entire top of the area occupied by 'tp'.
 *
 *	NOTE: the only tile whose type is changed is 'tp'.  Any tiles
 *	resulting from splits below this tile will not have had their
 *	types changed.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * THIS IS SLOW, SO SHOULD BE AVOIDED IF AT ALL POSSIBLE.
 * THE CODE ABOVE GOES TO GREAT LENGTHS TO DO SO.
 *
 * ----------------------------------------------------------------------------
 */

Tile *
dbMergeType(tile, newType, plane, mergeFlags, undo, client)
    register Tile *tile;	/* Tile to be merged with its neighbors */
    register TileType newType;	/* Type to which we will change 'tile' */
    Plane *plane;		/* Plane on which this resides */
    int mergeFlags;		/* Specify which directions to merge */
    PaintUndoInfo *undo;	/* See DBPaintPlane() above */
    ClientData client;
{
    register Tile *tp, *tpLast;
    register int ysplit;

    ysplit = BOTTOM(tile);
    if (mergeFlags & MRG_LEFT)
    {
	/*
	 * Find the split point along the LHS of tile.
	 * If the topmost tile 'tp' along the LHS is of type 'newType'
	 * the split point will be no lower than the bottom of 'tp'.
	 * If the topmost tile is NOT of type 'newType', then the split
	 * point will be no lower than the top of the first tile along
	 * the LHS that is of type 'newType'.
	 */
	for (tpLast = NULL, tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
	    if ((TiGetType(tp) == newType) && (tp->ti_client == client) )
		tpLast = tp;

	/* If the topmost LHS tile is not of type 'newType', we don't merge */
	if (tpLast == NULL || TOP(tpLast) < TOP(tile))
	{
	    mergeFlags &= ~MRG_LEFT;
	    if (tpLast && TOP(tpLast) > ysplit) ysplit = TOP(tpLast);
	}
	else if (BOTTOM(tpLast) > ysplit) ysplit = BOTTOM(tpLast);
    }

    if (mergeFlags & MRG_RIGHT)
    {
	/*
	 * Find the split point along the RHS of 'tile'.
	 * If the topmost tile 'tp' along the RHS is of type 'newType'
	 * the split point will be no lower than the bottom of 'tp'.
	 * If the topmost tile is NOT of type 'newType', then the split
	 * point will be no lower than the top of the first tile along
	 * the RHS that is of type 'newType'.
	 */
	tp = TR(tile);
	if ((TiGetType(tp) == newType) && (tp->ti_client == client))
	{
	    if (BOTTOM(tp) > ysplit) ysplit = BOTTOM(tp);
	}
	else
	{
	    /* Topmost RHS tile is not of type 'newType', so don't merge */
	    do
		tp = LB(tp);
	    while (TiGetType(tp) != newType && TOP(tp) > ysplit);
	    if (TOP(tp) > ysplit) ysplit = TOP(tp);
	    mergeFlags &= ~MRG_RIGHT;
	}
    }

    /*
     * If 'tile' must be split horizontally, do so.
     * Any merging to the bottom will be delayed until the split-off
     * bottom tile is processed on a subsequent iteration of the area
     * enumeration loop in DBPaintPlane().
     */
    if (ysplit > BOTTOM(tile))
    {
	mergeFlags &= ~MRG_BOTTOM;
	tp = TiSplitY(tile, ysplit);
	TiSetBody(tp, TiGetType(tile));
	TiSetClient(tp, TiGetClient(tile));
	tile = tp;
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) after split");
#endif	PAINTDEBUG
    }

    /*
     * Set the type of the new tile.
     * Record any undo information.
     */
    if (undo && TiGetType(tile) != newType && UndoIsEnabled())
	DBPAINTUNDO(tile, newType, undo);
    TiSetBody(tile, newType);
    TiSetClient(tile, client);
#ifdef	PAINTDEBUG
    if (dbPaintDebug)
	dbPaintShowTile(tile, undo, "(DBMERGE) changed type");
#endif	PAINTDEBUG

    /*
     * Do the merging.
     * We are guaranteed that at most one tile abuts 'tile' on
     * any side that we will merge to, and that this tile is
     * of type 'newType'.
     */
    if (mergeFlags & MRG_LEFT)
    {
	tp = BL(tile);
	if (TOP(tp) > TOP(tile))
	{
	    tpLast = TiSplitY(tp, TOP(tile));
	    TiSetBody(tpLast, newType);
	    TiSetClient(tpLast, client);
	}
	if (BOTTOM(tp) < BOTTOM(tile)) tp = TiSplitY(tp, BOTTOM(tile));
	TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged left");
#endif	PAINTDEBUG
    }
    if (mergeFlags & MRG_RIGHT)
    {
	tp = TR(tile);
	if (TOP(tp) > TOP(tile))
	{
	    tpLast = TiSplitY(tp, TOP(tile));
	    TiSetBody(tpLast, newType);
	    TiSetClient(tpLast, client);
	}
	if (BOTTOM(tp) < BOTTOM(tile)) tp = TiSplitY(tp, BOTTOM(tile));
	TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged right");
#endif	PAINTDEBUG
    }
    if (mergeFlags&MRG_TOP)
    {
	tp = RT(tile);
	if (CANMERGE_Y(tp, tile) && (tp->ti_client == client)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged up");
#endif	PAINTDEBUG
    }
    if (mergeFlags&MRG_BOTTOM)
    {
	tp = LB(tile);
	if (CANMERGE_Y(tp, tile) && (tp->ti_client == client)) TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged down");
#endif	PAINTDEBUG
    }

    return (tile);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBPaintPlaneVert --
 *
 * Paint a rectangular area ('area') on a single tile plane ('plane').
 *
 * --------------------------------------------------------------------
 * This is identical to DBPaintPlane above, except we merge in maximal
 * VERTICAL strips instead of maximal HORIZONTAL.  See the comments for
 * DBPaintPlane for details.
 * --------------------------------------------------------------------
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * REMINDER:
 *	Callers should remember to set the CDMODIFIED and CDGETNEWSTAMP
 *	bits in the cell definition containing the plane being painted.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBPaintPlaneVert(plane, area, resultTbl, undo)
    Plane *plane;		/* Plane whose paint is to be modified */
    register Rect *area;	/* Area to be changed */
    PaintResultType *resultTbl;	/* Table, indexed by the type of tile already
				 * present in the plane, giving the type to
				 * which the existing tile must change as a
				 * result of this paint operation.
				 */
    PaintUndoInfo *undo;	/* Record containing everything needed to
				 * save undo entries for this operation.
				 * If NULL, the undo package is not used.
				 */
{
    Point start;
    int clipTop, mergeFlags;
    TileType oldType, newType;
    register Tile *tile, *tpnew;	/* Used for area search */
    register Tile *newtile, *tp;	/* Used for paint */

    if (area->r_xtop <= area->r_xbot || area->r_ytop <= area->r_ybot)
	return;

    /*
     * The following is a modified version of the area enumeration
     * algorithm.  It expects the in-line paint code below to leave
     * 'tile' pointing to the tile from which we should continue the
     * search.
     */

    start.p_x = area->r_xbot;
    start.p_y = area->r_ytop - 1;
    tile = plane->pl_hint;
    GOTOPOINT(tile, &start);

    /* Each iteration visits another tile on the LHS of the search area */
    while (TOP(tile) > area->r_ybot)
    {
	/***
	 *** AREA SEARCH.
	 *** Each iteration enumerates another tile.
	 ***/
enumerate:
	if (SigInterruptPending)
	    break;

	clipTop = TOP(tile);
	if (clipTop > area->r_ytop) clipTop = area->r_ytop;
	oldType = TiGetType(tile);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "area enum");
#endif	PAINTDEBUG

	/***
	 *** ---------- THE FOLLOWING IS IN-LINE PAINT CODE ----------
	 ***/

	/*
	 * Set up the directions in which we will have to
	 * merge initially.  Clipping can cause some of these
	 * to be turned off.
	 */
	mergeFlags = MRG_TOP | MRG_LEFT;
	if (RIGHT(tile) >= area->r_xtop) mergeFlags |= MRG_RIGHT;
	if (BOTTOM(tile) <= area->r_ybot) mergeFlags |= MRG_BOTTOM;

	/*
	 * Determine new type of this tile.
	 * Change the type if necessary.
	 */
	newType = resultTbl[oldType];
	if (oldType != newType)
	{
	    /*
	     * Clip the tile against the clipping rectangle.
	     * Merging is only necessary if we clip to the top or to
	     * the bottom, and then only to the left or the right.
	     *
	     * *** REMEMBER, THESE ARE MAXIMAL VERTICAL STRIPS HERE ***
	     *
	     * We do the merge in-line for efficiency.
	     */

	    /* Clip right */
	    if (RIGHT(tile) > area->r_xtop)
	    {
		TISPLITX(newtile, tile, area->r_xtop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_RIGHT;
	    }

	    /* Clip left */
	    if (LEFT(tile) < area->r_xbot)
	    {
		newtile = tile;
		TISPLITX(tile, tile, area->r_xbot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_LEFT;
	    }

	    /* Clip up */
	    if (TOP(tile) > area->r_ytop)
	    {
		newtile = TiSplitY(tile, area->r_ytop);
		TiSetBody(newtile, TiGetBody(tile));
		mergeFlags &= ~MRG_TOP;

		/* Merge the outside tile to its left */
		tp = BL(newtile);
		if (CANMERGE_X(newtile, tp)) TiJoinX(newtile, tp, plane);

		/* Merge the outside tile to its right */
		tp = TR(newtile);
		if (CANMERGE_X(newtile, tp)) TiJoinX(newtile, tp, plane);
	    }

	    /* Clip down */
	    if (BOTTOM(tile) < area->r_ybot)
	    {
		newtile = tile, tile = TiSplitY(tile, area->r_ybot);
		TiSetBody(tile, TiGetBody(newtile));
		mergeFlags &= ~MRG_BOTTOM;

		/* Merge the outside tile to its left */
		tp = BL(newtile);
		if (CANMERGE_X(newtile, tp)) TiJoinX(newtile, tp, plane);

		/* Merge the outside tile to its right */
		tp = TR(newtile);
		if (CANMERGE_X(newtile, tp)) TiJoinX(newtile, tp, plane);
	    }

#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "after clip");
#endif	PAINTDEBUG
	}

	/*
	 * Merge the tile back into the parts of the plane that have
	 * already been visited.  Note that if we clipped in a particular
	 * direction we avoid merging in that direction.
	 *
	 * We avoid calling dbPaintMerge if at all possible.
	 */
	if (mergeFlags & MRG_BOTTOM)
	{
	    for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp = TR(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMergeVert(tile, newType, plane, mergeFlags,
						undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_BOTTOM;
	}
	if (mergeFlags & MRG_TOP)
	{
	    for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp))
		if (TiGetType(tp) == newType)
		{
		    tile = dbPaintMergeVert(tile, newType, plane, mergeFlags,
						undo);
		    goto paintdone;
		}
	    mergeFlags &= ~MRG_TOP;
	}

	/*
	 * Cheap and dirty merge -- we don't have to merge to the
	 * top or bottom, so the left/right merge is very fast.
	 *
	 * Now it's safe to change the type of this tile, and
	 * record the event on the undo list.
	 */
	if (undo && oldType != newType && UndoIsEnabled())
	    DBPAINTUNDO(tile, newType, undo);
	TiSetBody(tile, newType);

#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "changed type");
#endif	PAINTDEBUG

	if (mergeFlags & MRG_LEFT)
	{
	    tp = BL(tile);
	    if (CANMERGE_X(tile, tp)) TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged left (CHEAP)");
#endif	PAINTDEBUG
	}
	if (mergeFlags & MRG_RIGHT)
	{
	    tp = TR(tile);
	    if (CANMERGE_X(tile, tp)) TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	    if (dbPaintDebug)
		dbPaintShowTile(tile, undo, "merged right (CHEAP)");
#endif	PAINTDEBUG
	}


	/***
	 ***		END OF PAINT CODE
	 *** ---------- BACK TO AREA SEARCH ----------
	 ***/
paintdone:
	/* Move right if possible */
	tpnew = TR(tile);
	if (LEFT(tpnew) < area->r_xtop)
	{
	    /* Move back down into clipping area if necessary */
	    while (BOTTOM(tpnew) >= clipTop) tpnew = LB(tpnew);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}

	/* Each iteration returns one tile further to the left */
	while (LEFT(tile) > area->r_xbot)
	{
	    /* Move left if necessary */
	    if (BOTTOM(tile) <= area->r_ybot)
		goto done;

	    /* Move down if possible; left otherwise */
	    tpnew = LB(tile); tile = BL(tile);
	    if (BOTTOM(tpnew) >= BOTTOM(tile) || BOTTOM(tile) <= area->r_ybot)
	    {
		tile = tpnew;
		goto enumerate;
	    }
	}
	/* At left edge -- walk down to next tile along the left edge */
	for (tile = LB(tile); RIGHT(tile) <= area->r_xbot; tile = TR(tile))
	    /* Nothing */;
    }

done:
    plane->pl_hint = tile;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbPaintMergeVert -- 
 *
 * The tile 'tp' is to be changed to type 'newtype'.  To maintain
 * maximal vertical strips, it may be necessary to merge the new
 * 'tp' with its neighbors.
 *
 * --------------------------------------------------------------------
 * This is identical to dbPaintMerge above, except we merge in maximal
 * VERTICAL strips instead of maximal HORIZONTAL.  See the comments for
 * dbPaintMerge for details.
 * --------------------------------------------------------------------
 *
 * This procedure splits off the biggest segment along the left of the
 * tile 'tp' that can be merged with its neighbors to the top and bottom
 * (depending on which of MRG_TOP and MRG_BOTTOM are set in the merge flags),
 * then changes the type of 'tp' to 'newtype' and merges to the top, bottom,
 * left, and right (in that order).
 *
 * Results:
 *	Returns a pointer to the leftmost tile resulting from any splits
 *	and merges of the original tile 'tp'.  By the maximal vertical
 *	strip property and the fact that the original tile 'tp' gets
 *	painted a single color, we know that this leftmost resulting tile
 *	extends across the entire LHS of the area occupied by 'tp'.
 *
 *	NOTE: the only tile whose type is changed is 'tp'.  Any tiles
 *	resulting from splits to the right of this tile will not have
 *	had their types changed.
 *
 * Side effects:
 *	Modifies the database plane that contains the given tile.
 *
 * THIS IS SLOW, SO SHOULD BE AVOIDED IF AT ALL POSSIBLE.
 * THE CODE ABOVE GOES TO GREAT LENGTHS TO DO SO.
 *
 * ----------------------------------------------------------------------------
 */

Tile *
dbPaintMergeVert(tile, newType, plane, mergeFlags, undo)
    register Tile *tile;	/* Tile to be merged with its neighbors */
    register TileType newType;	/* Type to which we will change 'tile' */
    Plane *plane;		/* Plane on which this resides */
    int mergeFlags;		/* Specify which directions to merge */
    PaintUndoInfo *undo;	/* See DBPaintPlane() above */
{
    register Tile *tp, *tpLast;
    register int xsplit;

    xsplit = RIGHT(tile);
    if (mergeFlags & MRG_TOP)
    {
	/*
	 * Find the split point along the top of tile.
	 * If the leftmost tile 'tp' along the top is of type 'newType'
	 * the split point will be no further right than the RHS of 'tp'.
	 * If the leftmost tile is NOT of type 'newType', then the split
	 * point will be no further right than the LHS of the first tile
	 * along the top that is of type 'newType'.
	 */
	for (tpLast = NULL, tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp))
	    if (TiGetType(tp) == newType)
		tpLast = tp;

	/* If the leftmost top tile is not of type 'newType', don't merge */
	if (tpLast == NULL || LEFT(tpLast) > LEFT(tile))
	{
	    mergeFlags &= ~MRG_TOP;
	    if (tpLast && LEFT(tpLast) < xsplit) xsplit = LEFT(tpLast);
	}
	else if (RIGHT(tpLast) < xsplit) xsplit = RIGHT(tpLast);
    }

    if (mergeFlags & MRG_BOTTOM)
    {
	/*
	 * Find the split point along the bottom of 'tile'.
	 * If the leftmost tile 'tp' along the bottom is of type 'newType'
	 * the split point will be no further right than the LHS of 'tp'.
	 * If the leftmost tile is NOT of type 'newType', then the split
	 * point will be no further right than the LHS of the first tile
	 * along the bottom that is of type 'newType'.
	 */
	tp = LB(tile);
	if (TiGetType(tp) == newType)
	{
	    if (RIGHT(tp) < xsplit) xsplit = RIGHT(tp);
	}
	else
	{
	    /* Leftmost bottom tile is not of type 'newType', so don't merge */
	    do
		tp = TR(tp);
	    while (TiGetType(tp) != newType && LEFT(tp) < xsplit);
	    if (LEFT(tp) < xsplit) xsplit = LEFT(tp);
	    mergeFlags &= ~MRG_BOTTOM;
	}
    }

    /*
     * If 'tile' must be split vertically, do so.
     * Any merging to the right will be delayed until the split-off
     * right tile is processed on a subsequent iteration of the area
     * enumeration loop in DBPaintPlaneVert().
     */
    if (xsplit < RIGHT(tile))
    {
	mergeFlags &= ~MRG_RIGHT;
	tp = TiSplitX(tile, xsplit);
	TiSetBody(tp, TiGetType(tile));
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) after split");
#endif	PAINTDEBUG
    }

    /*
     * Set the type of the new tile.
     * Record any undo information.
     */
    if (undo && TiGetType(tile) != newType && UndoIsEnabled())
	DBPAINTUNDO(tile, newType, undo);
    TiSetBody(tile, newType);
#ifdef	PAINTDEBUG
    if (dbPaintDebug)
	dbPaintShowTile(tile, undo, "(DBMERGE) changed type");
#endif	PAINTDEBUG

    /*
     * Do the merging.
     * We are guaranteed that at most one tile abuts 'tile' on
     * any side that we will merge to, and that this tile is
     * of type 'newType'.
     */
    if (mergeFlags & MRG_TOP)
    {
	tp = RT(tile);
	if (LEFT(tp) < LEFT(tile)) tp = TiSplitX(tp, LEFT(tile));
	if (RIGHT(tp) > RIGHT(tile))
	    tpLast = TiSplitX(tp, RIGHT(tile)), TiSetBody(tpLast, newType);
	TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged up");
#endif	PAINTDEBUG
    }

    if (mergeFlags & MRG_BOTTOM)
    {
	tp = LB(tile);
	if (LEFT(tp) < LEFT(tile)) tp = TiSplitX(tp, LEFT(tile));
	if (RIGHT(tp) > RIGHT(tile))
	    tpLast = TiSplitX(tp, RIGHT(tile)), TiSetBody(tpLast, newType);
	TiJoinY(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged down");
#endif	PAINTDEBUG
    }

    if (mergeFlags&MRG_LEFT)
    {
	tp = BL(tile);
	if (CANMERGE_X(tp, tile)) TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged left");
#endif	PAINTDEBUG
    }
    if (mergeFlags&MRG_RIGHT)
    {
	tp = TR(tile);
	if (CANMERGE_X(tp, tile)) TiJoinX(tile, tp, plane);
#ifdef	PAINTDEBUG
	if (dbPaintDebug)
	    dbPaintShowTile(tile, undo, "(DBMERGE) merged right");
#endif	PAINTDEBUG
    }

    return (tile);
}

#ifdef	PAINTDEBUG
/*
 * ----------------------------------------------------------------------------
 *
 * dbPaintShowTile -- 
 *
 * Show the tile 'tp' in the cell undo->pu_def in a highlighted style,
 * then print a message, wait for more, and erase the highlights.
 * This procedure is for debugging the new paint code only.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Redisplays.
 *
 * ----------------------------------------------------------------------------
 */

#include "styles.h"

dbPaintShowTile(tile, undo, str)
    Tile *tile;			/* Tile to be highlighted */
    PaintUndoInfo *undo;	/* Cell to which tile belongs is undo->pu_def */
    char *str;			/* Message to be displayed */
{
    char answer[100];
    Rect r;

    if (undo == NULL)
	return;

    TiToRect(tile, &r);
    DBWAreaChanged(undo->pu_def, &r, DBW_ALLWINDOWS, &DBAllButSpaceBits);
    DBWFeedbackAdd(&r, str, undo->pu_def, 1, STYLE_MEDIUMHIGHLIGHTS);
    DBWFeedbackShow();
    WindUpdate();

    TxPrintf("%s --more--", str); fflush(stdout);
    (void) TxGetLine(answer, sizeof answer);
    DBWFeedbackClear();
}
#endif	PAINTDEBUG
