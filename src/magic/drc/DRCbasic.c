/*
 * DRCbasic.c --
 *
 * This file provides routines that make perform basic design-rule
 * checking:  given an area of a cell definition, this file will
 * find all of the rule violations and call a client procedure for
 * each one.
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

#ifndef	lint
static char rcsid[] = "$Header: DRCbasic.c,v 6.0 90/08/28 18:12:26 mayo Exp $";
#endif	not lint

#include <sys/types.h>
#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"
#include "signals.h"

int dbDRCDebug = 0;

/* The following DRC cookie is used when there are tiles of type
 * TT_ERROR_S found during the basic DRC.  These arise during
 * hierarchical checking when there are illegal overlaps.
 */

static DRCCookie drcOverlapCookie = {
    0, { 0 }, { 0 }, (DRCCookie *) NULL,
    "Can't overlap those layers",
    0, 0, 0};

/* Forward references: */

extern int areaCheck();
extern int drcTile();

/*
 * ----------------------------------------------------------------------------
 *
 * areaCheck -- 
 *
 * Call the function passed down from DRCBasicCheck() if the current tile
 * violates the rule in the given DRCCookie.  If the rule's connectivity
 * flag is set, then make sure the violating material isn't connected
 * to what's on the initial side of the edge before calling the client
 * error function.
 *
 * This function is called from DBSrPaintArea().
 *
 * Results:
 *	Zero (so that the search will continue).
 *
 * Side effects:
 *      Applies the function passed as an argument.
 *
 * ----------------------------------------------------------------------------
 */

int
areaCheck(tile, arg) 
    register Tile *tile;
    register struct drcClientData *arg;
{
    Rect rect;		/* Area where error is to be recorded. */

    /* If the tile has a legal type, then return. */

    if (TTMaskHasType(&arg->dCD_cptr->drcc_mask, TiGetType(tile))) return 0;

    /* Only consider the portion of the suspicious tile that overlaps
     * the clip area for errors.
     */

    TiToRect(tile, &rect);
    GeoClip(&rect, arg->dCD_clip);
    GeoClip(&rect, arg->dCD_constraint);
    if ((rect.r_xbot >= rect.r_xtop) || (rect.r_ybot >= rect.r_ytop))
	return 0;

    (*(arg->dCD_function)) (arg->dCD_celldef, &rect,
	arg->dCD_cptr, arg->dCD_clientData);
    (*(arg->dCD_errors))++;
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCBasicCheck --
 *
 * This is the top-level routine for basic design-rule checking.
 *
 * Results:
 *	Number of errors found.
 *
 * Side effects:
 *	Calls function for each design-rule violation in celldef
 *	that is triggered by an edge in rect and whose violation
 *	area falls withing clipRect.  This routine makes a flat check:
 *	it considers only information in the paint planes of celldef,
 *	and does not expand children.  Function should have the form:
 *	void
 *	function(def, area, rule, cdarg)
 *	    CellDef *def;
 *	    Rect *area;
 *	    DRCCookie *rule;
 *	    ClientData cdarg;
 *	{
 *	}
 *
 *	In the call to function, def is the definition containing the
 *	basic area being checked, area is the actual area where a
 *	rule is violated, rule is the rule being violated, and cdarg
 *	is the client data passed through all of our routines.
 *
 * Note:
 *	If an interrupt occurs (SigInterruptPending gets set), then
 *	the basic will be aborted immediately.  This means the check
 *	may be incomplete.
 *
 * ----------------------------------------------------------------------------
 */

int
DRCBasicCheck (celldef, checkRect, clipRect, function, cdata)
    CellDef *celldef;	/* CellDef being checked */
    Rect *checkRect;	/* Check rules in this area -- usually two Haloes
			 * larger than the area where changes were made.
			 */
    Rect *clipRect;	/* Clip error tiles against this area. */
    void (*function)();	/* Function to apply for each error found. */
    ClientData cdata;	/* Passed to function as argument. */
{
    struct drcClientData arg;
    int	errors;
    int planeNum;

		/*  Insist on top quality rectangles. */

    if ((checkRect->r_xbot >= checkRect->r_xtop)
	    || (checkRect->r_ybot >= checkRect->r_ytop))
	 return (0);

    errors = 0;

    arg.dCD_celldef = celldef;
    arg.dCD_rect = checkRect;
    arg.dCD_errors = &errors;
    arg.dCD_function = function;
    arg.dCD_clip = clipRect;
    arg.dCD_clientData = cdata;

    for (planeNum = PL_TECHDEPBASE; planeNum < DBNumPlanes; planeNum++)
    {
        arg.dCD_plane = celldef->cd_planes[planeNum];
        (void) DBSrPaintArea ((Tile *) NULL, arg.dCD_plane,
		checkRect, &DBAllTypeBits, drcTile, (ClientData) &arg);
    }
    return (errors);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcTile --
 *
 * This is a search function invoked once for each tile in
 * the area to be checked.  It checks design rules along the left
 * and bottom of the given tile.  If the tile extends beyond the
 * clipping rectangle in any direction, then the boundary on that
 * side of the tile will be skipped.
 *
 * Results:
 *	Zero (so that the search will continue), unless an interrupt
 *	occurs, in which case 1 is returned to stop the check.
 *
 * Side effects:
 *	Calls the client's error function if errors are found.
 *
 * ----------------------------------------------------------------------------
 */

int
drcTile (tile, arg)
    register Tile *tile;	/* Tile being examined */
    struct drcClientData *arg;
{
    register DRCCookie *cptr;	/* Current design rule on list */
    register Tile *tp;		/* Used for corner checks */
    Rect *rect = arg->dCD_rect;	/* Area being checked */
    Rect errRect;		/* Area checked for an individual rule */
    TileTypeBitMask tmpMask;

    arg->dCD_constraint = &errRect;

    /*
     * If we were interrupted, we want to
     * abort the check as quickly as possible.
     */
    if (SigInterruptPending) return 1;
    DRCstatTiles++;

    /* If this tile is an error tile, it arose because of an illegal
     * overlap between things in adjacent cells.  This means that
     * there's an automatic violation over the area of the tile.
     */
    
    if (TiGetType(tile) == TT_ERROR_S)
    {
	TiToRect(tile, &errRect);
	GeoClip(&errRect, rect);
        (*(arg->dCD_function)) (arg->dCD_celldef, &errRect,
	    &drcOverlapCookie, arg->dCD_clientData);
        (*(arg->dCD_errors))++;
    }

    /*
     * Check design rules along a vertical boundary between two tiles.
     *
     *			      1 | 4
     *				T
     *				|
     *			tpleft	|  tile
     *				|
     *				B
     *			      2 | 3
     *
     * The labels "T" and "B" indicate pointT and pointB respectively.
     *
     * If a rule's direction is FORWARD, then check from left to right.
     *
     *	    * Check the top right corner if the 1x1 lambda square
     *	      on the top left corner (1) of pointT matches the design
     *	      rule's "corner" mask.
     *
     *	    * Check the bottom right corner if the rule says check
     *	      BOTHCORNERS and the 1x1 lambda square on the bottom left
     *	      corner (2) of pointB matches the design rule's "corner" mask.
     *
     * If a rule's direction is REVERSE, then check from right to left.
     *
     *	    * Check the bottom left corner if the 1x1 lambda square
     *	      on the bottom right corner (3) of pointB matches the design
     *	      rule's "corner" mask.
     *
     *	    * Check the top left corner if the rule says check BOTHCORNERS
     *	      and the 1x1 lambda square on the top right corner (4) of
     *	      pointT matches the design rule's "corner" mask.
     */

    if (LEFT(tile) >= rect->r_xbot)		/* check tile against rect */
    {
	register Tile *tpleft;
	int edgeTop, edgeBot;
        int top = MIN(TOP(tile), rect->r_ytop);
        int bottom = MAX(BOTTOM(tile), rect->r_ybot);
	int edgeX = LEFT(tile);

        for (tpleft = BL(tile); BOTTOM(tpleft) < top; tpleft = RT(tpleft))
        {
	    /* Don't check synthetic edges, i.e. edges with same type on
             * both sides.  Such "edges" have no physical significance, and
	     * depend on internal-details of how paint is spit into tiles.
	     * Thus checking them just leads to confusion.  (When edge rules
	     * involving such edges are encountered during technology readin
	     * the user is warned that such edges are not checked).
	     */
	    if(TiGetType(tpleft) == TiGetType(tile))
	        continue;

	    /*
	     * Go through list of design rules triggered by the
	     * left-to-right edge.
	     */
	    edgeTop = MIN(TOP (tpleft), top);
	    edgeBot = MAX(BOTTOM(tpleft), bottom);
	    if (edgeTop <= edgeBot)
		continue;

	    for (cptr = DRCRulesTbl[TiGetType(tpleft)][TiGetType(tile)];
		    cptr != (DRCCookie *) NULL;
		    cptr = cptr->drcc_next)
	    {
		DRCstatRules++;
		errRect.r_ytop = edgeTop;
		errRect.r_ybot = edgeBot;

		if (cptr->drcc_flags & DRC_REVERSE)
		{
		    /*
		     * Determine corner extensions.
		     * Find the point (3) to the bottom right of pointB
		     */
		    for (tp = tile; BOTTOM(tp) >= errRect.r_ybot; tp = LB(tp))
			/* Nothing */;
		    if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			errRect.r_ybot -= cptr->drcc_cdist;

		    if (cptr->drcc_flags & DRC_BOTHCORNERS)
		    {
			/*
			 * Check the other corner by finding the
			 * point (4) to the top right of pointT.
			 */
			if (TOP(tp = tile) <= errRect.r_ytop)
			    for (tp = RT(tp); LEFT(tp) > edgeX; tp = BL(tp))
				/* Nothing */;
			if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			    errRect.r_ytop += cptr->drcc_cdist;
		    }

		    /*
		     * Just for grins, see if we could avoid a messy search
		     * by looking only at tpleft.
		     */
		    errRect.r_xbot = edgeX - cptr->drcc_dist;
		    if (LEFT(tpleft) <= errRect.r_xbot
			&& BOTTOM(tpleft) <= errRect.r_ybot
			&& TOP(tpleft) >= errRect.r_ytop
			&& !(cptr->drcc_flags & DRC_XPLANE)
			&& TTMaskHasType(&cptr->drcc_mask, TiGetType(tpleft)))
			    continue;
		    errRect.r_xtop = edgeX;
		    arg->dCD_initial = tile;
		}
		else
		{
		    /*
		     * Determine corner extensions.
		     * Find the point (1) to the top left of pointT
		     */
		    for (tp = tpleft; TOP(tp) <= errRect.r_ytop; tp = RT(tp))
			/* Nothing */;
		    if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			errRect.r_ytop += cptr->drcc_cdist;

		    if (cptr->drcc_flags & DRC_BOTHCORNERS)
		    {
			/*
			 * Check the other corner by finding the
			 * point (2) to the bottom left of pointB.
			 */
			if (BOTTOM(tp = tpleft) >= errRect.r_ybot)
			    for (tp = LB(tp); RIGHT(tp) < edgeX; tp = TR(tp))
				/* Nothing */;
			if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			    errRect.r_ybot -= cptr->drcc_cdist;
		    }

		    /*
		     * Just for grins, see if we could avoid a messy search
		     * by looking only at tile.
		     */
		    errRect.r_xtop = edgeX + cptr->drcc_dist;
		    if (RIGHT(tile) >= errRect.r_xtop
			&& BOTTOM(tile) <= errRect.r_ybot
			&& TOP(tile) >= errRect.r_ytop
			&& !(cptr->drcc_flags & DRC_XPLANE)
			&& TTMaskHasType(&cptr->drcc_mask, TiGetType(tile)))
			    continue;
		    errRect.r_xbot = edgeX;
		    arg->dCD_initial= tpleft;
		}

		DRCstatSlow++;
		arg->dCD_cptr = cptr;
		TTMaskCom2(&tmpMask, &cptr->drcc_mask);
		(void) DBSrPaintArea((Tile *) NULL,
		    arg->dCD_celldef->cd_planes[cptr->drcc_plane],
		    &errRect, &tmpMask, areaCheck, (ClientData) arg);
	    }

	    DRCstatEdges++;
        }
    }


    /*
     * Check design rules along a horizontal boundary between two tiles.
     *
     *			 4	tile	    3
     *			--L----------------R--
     *			 1	tpbot	    2
     *
     * The labels "L" and "R" indicate pointL and pointR respectively.
     * If a rule's direction is FORWARD, then check from bottom to top.
     *
     *      * Check the top left corner if the 1x1 lambda square on the bottom
     *        left corner (1) of pointL matches the design rule's "corner" mask.
     *
     *      * Check the top right corner if the rule says check BOTHCORNERS and
     *        the 1x1 lambda square on the bottom right (2) corner of pointR
     *	      matches the design rule's "corner" mask.
     *
     * If a rule's direction is REVERSE, then check from top to bottom.
     *
     *	    * Check the bottom right corner if the 1x1 lambda square on the top
     *	      right corner (3) of pointR matches the design rule's "corner"
     *	      mask.
     *
     *	    * Check the bottom left corner if the rule says check BOTHCORNERS
     *	      and the 1x1 lambda square on the top left corner (4) of pointL
     *	      matches the design rule's "corner" mask.
     */

    if (BOTTOM(tile) >= rect->r_ybot)
    {
	register Tile *tpbot;
	int edgeLeft, edgeRight;
        int left = MAX(LEFT(tile), rect->r_xbot);
        int right = MIN(RIGHT(tile), rect->r_xtop);
	int edgeY = BOTTOM(tile);

	/* Go right across bottom of tile */
        for (tpbot = LB(tile); LEFT(tpbot) < right; tpbot = TR(tpbot))
        {

	    /* Don't check synthetic edges, i.e. edges with same type on
             * both sides.  Such "edges" have no physical significance, and
	     * depend on internal-details of how paint is spit into tiles.
	     * Thus checking them just leads to confusion.  (When edge rules
	     * involving such edges are encountered during technology readin
	     * the user is warned that such edges are not checked).
	     */
	    if(TiGetType(tpbot) == TiGetType(tile))
	        continue;

	    /*
	     * Check to insure that we are inside the clip area.
	     * Go through list of design rules triggered by the
	     * bottom-to-top edge.
	     */
	    edgeLeft = MAX(LEFT(tpbot), left);
	    edgeRight = MIN(RIGHT(tpbot), right);
	    if (edgeLeft >= edgeRight)
		continue;

	    for (cptr = DRCRulesTbl[TiGetType(tpbot)][TiGetType(tile)];
		    cptr != (DRCCookie *) NULL;
		    cptr = cptr->drcc_next)
	    {
		DRCstatRules++;
		errRect.r_xbot = edgeLeft;
		errRect.r_xtop = edgeRight;

		/* top to bottom */
		if (cptr->drcc_flags & DRC_REVERSE)
		{
		    /*
		     * Determine corner extensions.
		     * Find the point (3) to the top right of pointR
		     */
		    if (RIGHT(tp = tile) <= errRect.r_xtop)
			for (tp = TR(tp); BOTTOM(tp) > edgeY; tp = LB(tp))
			    /* Nothing */;
		    if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			errRect.r_xtop += cptr->drcc_cdist; 	

		    if (cptr->drcc_flags & DRC_BOTHCORNERS)
		    {
			/*
			 * Check the other corner by finding the
			 * point (4) to the top left of pointL.
			 */
			for (tp = tile; LEFT(tp) >= errRect.r_xbot; tp = BL(tp))
			    /* Nothing */;
			if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			    errRect.r_xbot -= cptr->drcc_cdist; 	
		    }

		    /*
		     * Just for grins, see if we could avoid
		     * a messy search by looking only at tpbot.
		     */
		    errRect.r_ybot = edgeY - cptr->drcc_dist;
		    if (BOTTOM(tpbot) <= errRect.r_ybot
			&& LEFT(tpbot) <= errRect.r_xbot
			&& RIGHT(tpbot) >= errRect.r_xtop
			&& !(cptr->drcc_flags & DRC_XPLANE)
			&& TTMaskHasType(&cptr->drcc_mask, TiGetType(tpbot)))
			    continue;
		    errRect.r_ytop = edgeY;
		    arg->dCD_initial = tile;
		}
		else
		{
		    /*
		     * Determine corner extensions.
		     * Find the point (1) to the bottom left of pointL
		     */
		    if (LEFT(tp = tpbot) >= errRect.r_xbot)
			for (tp = BL(tp); TOP(tp) < edgeY; tp = RT(tp))
			    /* Nothing */;

		    if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			errRect.r_xbot -= cptr->drcc_cdist;

		    if (cptr->drcc_flags & DRC_BOTHCORNERS)
		    {
			/*
			 * Check the other corner by finding the
			 * point (2) to the bottom right of pointR.
			 */
			for (tp=tpbot; RIGHT(tp) <= errRect.r_xtop; tp=TR(tp))
			    /* Nothing */;
			if (TTMaskHasType(&cptr->drcc_corner, TiGetType(tp)))
			    errRect.r_xtop += cptr->drcc_cdist;
		    }

		    /*
		     * Just for grins, see if we could avoid
		     * a messy search by looking only at tile.
		     */
		    errRect.r_ytop = edgeY + cptr->drcc_dist;
		    if (TOP(tile) >= errRect.r_ytop
			&& LEFT(tile) <= errRect.r_xbot
			&& RIGHT(tile) >= errRect.r_xtop
			&& !(cptr->drcc_flags & DRC_XPLANE)
			&& TTMaskHasType(&cptr->drcc_mask, TiGetType(tile)))
			    continue;
		    errRect.r_ybot = edgeY;
		    arg->dCD_initial = tpbot;
		}

		DRCstatSlow++;
		arg->dCD_cptr = cptr;
		TTMaskCom2(&tmpMask, &cptr->drcc_mask);
		(void) DBSrPaintArea((Tile *) NULL,
		    arg->dCD_celldef->cd_planes[cptr->drcc_plane],
		    &errRect, &tmpMask, areaCheck, (ClientData) arg);
	    }
	    DRCstatEdges++;
        }
    }
    return (0);
}
