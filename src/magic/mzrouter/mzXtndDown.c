/*
 * mzExtendDown.c --
 *
 * Code for finding next interesting point down.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1988, 1990 Michael H. Arnold and the Regents of the *
 *     * University of California.                                         *
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
static char rcsid[] = "$Header: mzXtndDown.c,v 6.1 90/08/28 19:35:49 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "../tiles/tile.h"
#include "doubleint.h"
#include "hash.h"
#include "heap.h"
#include "database.h"
#include "signals.h"
#include "textio.h"
#include "list.h"
#include "debug.h"
#include "mzrouter.h"
#include "mzInternal.h"


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendDown --
 *
 * Find next interesting point down.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to added extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzExtendDown(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pStep;	/* one unit from pOrg in direction of extension */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    RouteLayer *rL;     /* temp variable for routelayers */
    int reasons;	/* Reasons point is interesting.  Used to
			 * avoid extensions in uninteresting directions.
			 */
    int extendCode;	/* Interesting directions to extend in */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("EXTENDING DOWN\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* pStep = point just one unit beyond pOrg */
    pStep = pOrg;
    --(pStep.p_y);

    /* Initial pNew to BOUNDS edge.  Must stop there
     * since blockage planes haven't been generated past there.
     */
    {
	Tile *tp;

	/* get bounds tile under pOrg */
	tp = TiSrPointNoHint(mzVBoundsPlane, &pOrg);
	pNew.p_x = pOrg.p_x;
	pNew.p_y = BOTTOM(tp);
	reasons = RC_BOUNDS;
    }

    /*
     *  Initial pNew to next pt where there is a change in the amount of 
     *  space on the BLOCKAGE plane in the direction perpendicular to the 
     *  extension.  (A special case of this is a actual block
     *  of the extension).  Want to consider jogs at such points.
     *  
     */
    {
	Tile *tpThis, *tpNext;
	bool covered;

        /* get tpThis, extending bounds plane and iterating, until
	 * completely covered by bounds plane.
	 */
	covered = FALSE;
	while(!covered)
	{
	    Tile *tpBounds;

	    /* get blockage plane tile under pOrg */
	    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_hBlock, 
				     &pOrg);

	    /* org point should not be blocked */
	    ASSERT(TiGetType(tpThis)==TT_SPACE,"mzExtendDown");

	    /* check to see if covered, if not extend bounds and start over */
	    covered = TRUE;
	    tpBounds = TiSrPointNoHint(mzHBoundsPlane, &pOrg);
	    while(TOP(tpBounds)>=BOTTOM(tpThis) &&
		  TOP(tpBounds)>=mzRouteUse->cu_def->cd_bbox.r_ybot &&
		  covered)
	    {
		if(TiGetType(tpBounds) == TT_SPACE)
		{
		   /* hit edge of bounds before jog found */
		   goto downEndJog;
		}
		else if(RIGHT(tpBounds)<=RIGHT(tpThis) && 
			RIGHT(tpBounds)<=mzRouteUse->cu_def->cd_bbox.r_xtop)
		{
		    Point p;

		    p.p_x = RIGHT(tpBounds);
		    p.p_y = MIN(TOP(tpBounds), pOrg.p_y);
		
		    mzExtendBlockBounds(&p);
		    if(SigInterruptPending) return;
		
		    covered = FALSE;
		}
		else if(LEFT(tpBounds)>=LEFT(tpThis) &&
			LEFT(tpBounds)>=mzRouteUse->cu_def->cd_bbox.r_xbot)
		{
		    Point p;
		    p.p_x = LEFT(tpBounds);
		    p.p_y = MIN(TOP(tpBounds), pOrg.p_y);
		
		    mzExtendBlockBounds(&p);
		    if(SigInterruptPending) return;
		
		    covered = FALSE;
		}

		/* move to next tile in bounds plane */
		if(covered)
		{
		    NEXT_TILE_DOWN(tpBounds, tpBounds, pOrg.p_x);
		}
	    }
	}

	/* also get next tile over */ 
	NEXT_TILE_DOWN(tpNext, tpThis, pOrg.p_x);

        if(TiGetType(tpNext)!=TT_SPACE)
	{
	    /* path blocked */
	    if(BOTTOM(tpThis)==pOrg.p_y)
	    {
		/* pOrg right up against block */
		if(TiGetType(tpNext)==TT_TOP_WALK)
		{
		    /* Block is walk, enter it */
		    pNew.p_y = pOrg.p_y - 1;
		    reasons = RC_WALK;
		    goto donePruning;
		}
		else if(TiGetType(tpNext)==TT_CONTACT_WALK)
		{
		    /* Block is contact walk, enter it */
		    pNew.p_y = pOrg.p_y - 1;
		    reasons = RC_WALKC;
		    goto donePruning;
		}
		else if(TiGetType(tpNext)==TT_DEST_AREA)
		{
		    /* Block is destination, enter it */
		    pNew.p_y = pOrg.p_y - 1;
		    reasons = RC_DONE;
		    goto donePruning;
		}
		else
		{
		    /* Path blocked from expansion, just return */
		    return;
		}
	    }
	    else
	    {
		/* prune pNew to just this side of block */
		PRUNE_TO_MAX(pNew.p_y, BOTTOM(tpThis), reasons, RC_JOG);
	    }
	}
	else
	{
	    /* path not blocked */
	    if((RIGHT(tpNext)<RIGHT(tpThis) || LEFT(tpNext)>LEFT(tpThis)) &&
		BOTTOM(tpThis)<pOrg.p_y)
	    {
		/* space is constricting, prune pNew to far 
		 * end of this tile 
		 */
		PRUNE_TO_MAX(pNew.p_y, BOTTOM(tpThis), reasons, RC_JOG);
	    }
	    else
	    {
		/* initial pNew to just inside next tile */
		PRUNE_TO_MAX(pNew.p_y, BOTTOM(tpThis)-1, reasons, RC_JOG);
	    }
	}
	downEndJog:;
    }

    /*
     *  Prune pNew to next pt where there is a change in the amount of 
     *  space on other active BLOCKAGE planes in the direction perpendicular 
     *  to the 
     *  extension.  (A special case of this is a actual block
     *  of the extension).  Want to consider jogs at such points.
     *  
     */
    for(rL=mzActiveRLs; rL!=NULL; rL=rL->rl_nextActive)
    {
	Tile *tpThis, *tpNext;
	bool covered;

	/* skip current layer (already handled above) */
	if(rL == path->rp_rLayer) 
	{
	    continue;
	}

	/* get blockage plane tile under pOrg */
	tpThis = TiSrPointNoHint(rL->rl_routeType.rt_hBlock, 
				 &pOrg);

	if(TiGetType(tpThis)!=TT_SPACE)
	{
	    /* ORG POINT BLOCKED */
	    /* this case handled by contact code below, so skip to next layer*/

	    continue;
	}
	else
	{
	    /* ORG POINT NOT BLOCKED, LOOK FOR NARROWING OR GROWING OF SPACE */

	    /* get tpThis, extending bounds plane and iterating, until
	     * completely covered by bounds plane.
	     */
	    covered = FALSE;
	    while(!covered)
	    {
		Tile *tpBounds;

		/* get blockage plane tile under pOrg */
		tpThis = TiSrPointNoHint(rL->rl_routeType.rt_hBlock, &pOrg);

		/* org point should not be blocked */
		ASSERT(TiGetType(tpThis)==TT_SPACE,"mzExtendDown, other");

		/* check to see if covered, 
		   if not extend bounds and start over */
		covered = TRUE;
		tpBounds = TiSrPointNoHint(mzHBoundsPlane, &pOrg);
		while(TOP(tpBounds)>=BOTTOM(tpThis) &&
		      TOP(tpBounds)>=mzRouteUse->cu_def->cd_bbox.r_ybot &&
		      covered)
		{
		    if(TiGetType(tpBounds) == TT_SPACE)
		    {
			/* hit edge of bounds before jog found */
			goto downNextLayer;
		    }
		    else if(RIGHT(tpBounds)<=RIGHT(tpThis) && 
			    RIGHT(tpBounds)<=
			    mzRouteUse->cu_def->cd_bbox.r_xtop)
		    {
			Point p;

			p.p_x = RIGHT(tpBounds);
			p.p_y = MIN(TOP(tpBounds), pOrg.p_y);
		
			mzExtendBlockBounds(&p);
			if(SigInterruptPending) return;
		
			covered = FALSE;
		    }
		    else if(LEFT(tpBounds)>=LEFT(tpThis) &&
			    LEFT(tpBounds)>=mzRouteUse->cu_def->cd_bbox.r_xbot)
		    {
			Point p;
			p.p_x = LEFT(tpBounds);
			p.p_y = MIN(TOP(tpBounds), pOrg.p_y);
		
			mzExtendBlockBounds(&p);
			if(SigInterruptPending) return;
		
			covered = FALSE;
		    }

		    /* move to next tile in bounds plane */
		    if(covered)
		    {
			NEXT_TILE_DOWN(tpBounds, tpBounds, pOrg.p_x);
		    }
		}
	    }

	    /* get next tile over */ 
	    NEXT_TILE_DOWN(tpNext, tpThis, pOrg.p_x);

	    if(TiGetType(tpNext)!=TT_SPACE)
	    {
		/* path blocked */
		if(BOTTOM(tpThis)==pOrg.p_y)
		{
		    /* pOrg right up against obstacle, can't extend so 
		       go to next layer */
		    continue;
		}
		else
		{
		    /* prune pNew to just this side of block */
		    PRUNE_TO_MAX(pNew.p_y, 
				 BOTTOM(tpThis),
				 reasons,
				 RC_ALIGNOTHER);
		}
	    }
	    else
	    {
		/* path not blocked */
		if((RIGHT(tpNext)<RIGHT(tpThis) || LEFT(tpNext)>LEFT(tpThis)) 
		   && BOTTOM(tpThis)<pOrg.p_y)
		{
		    /* space is constricting, initial pNew to far 
		     * end of this tile 
		     */
		    PRUNE_TO_MAX(pNew.p_y, 
				 BOTTOM(tpThis),
				 reasons,
				 RC_ALIGNOTHER);
		}
		else
		{
		    /* initial pNew to just inside next tile */
		    PRUNE_TO_MAX(pNew.p_y, 
				 BOTTOM(tpThis)-1,
				 reasons,
				 RC_ALIGNOTHER);
		}
	    }
	}
	downNextLayer:;
    }

    /* Prune pNew to next alignment with a DESTINATION terminal */
    {
	int *yAlign = &(mzNLGetContainingInterval(&mzYAlignNL,pOrg.p_y)[0]);
	if(*yAlign == pOrg.p_y)
	/* On alignment mark, get previous one */
	{
	    yAlign--;
	}
	PRUNE_TO_MAX(pNew.p_y, *yAlign, reasons, RC_ALIGNGOAL);
    }

    /* Prune pNew to alignment with perpendicular HINT edges */
    {
	Tile *tp;

	tp = TiSrPointNoHint(mzHHintPlane, &pStep);
	PRUNE_TO_MAX(pNew.p_y, BOTTOM(tp), reasons, RC_HINT);
    }

    /* Prune pNew to either side  of tile edges on ROTATE plane (organized
     * into maximum strips in perpendicular direction).  Jogging
     * at such points can effect cost.
     * NOTE:  Also must have intermediate path points at boundaries between 
     * rotate and non-rotate since edge cost different in these regions.
     */
    {
	Tile *tp;

	tp = TiSrPointNoHint(mzHRotatePlane, &pStep);

	if(TOP(tp)-1 < pOrg.p_y)
	    /* prune to beginning of tile */
	    PRUNE_TO_MAX(pNew.p_y, TOP(tp)-1, reasons, RC_ROTBEFORE);
	else
	    /* prune to end of tile */
	    PRUNE_TO_MAX(pNew.p_y, BOTTOM(tp), reasons, RC_ROTINSIDE);
    }

    /* Prune to just before or just after CONTACT BLOCKS.
     * These are last and first opportunites for contacts.
     */
    {
	List *cL;
	Tile *tp;

	/* Loop thru contact types connecting to current route layer */
	for (cL=path->rp_rLayer->rl_contactL; cL!=NULL; cL=LIST_TAIL(cL))
	{
	    /* find tile in contact blockage plane under pOrg */
	    tp = TiSrPointNoHint(
		((RouteContact*) LIST_FIRST(cL))->rc_routeType.rt_vBlock, 
		&pStep);

	    if (TiGetType(tp) == TT_SPACE)
	    {
		/* SPACE TILE */ 
		if(TOP(tp)-1 < pOrg.p_y)
		    /* prune to beginning of tile */
		    PRUNE_TO_MAX(pNew.p_y, TOP(tp)-1, reasons, RC_CONTACT);
		else
		    /* prune to end of tile */
		    PRUNE_TO_MAX(pNew.p_y, BOTTOM(tp), reasons, RC_CONTACT);
	    }
	    else
	    {
		/* BLOCK TILE - so prune to just beyond tile */
		PRUNE_TO_MAX(pNew.p_y, BOTTOM(tp)-1, reasons, RC_CONTACT);
	    }
	}
    }

    donePruning:;
    /* DONE PRUNING pNew */

    /* debug - print point and reasons its interesting. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
    {
	TxPrintf("Done Pruning, new point: (%d, %d) ",
		 pNew.p_x, pNew.p_y);
	TxPrintf("is interesting because:\n  ");
	if(reasons & RC_JOG)
	{
	    TxPrintf("jog ");
	}
	if(reasons & RC_ALIGNOTHER)
	{
	    TxPrintf("alignOther ");
	}
	if(reasons & RC_CONTACT)
	{
	    TxPrintf("contact ");
	}
	if(reasons & RC_ALIGNGOAL)
	{
	    TxPrintf("alignGoal ");
	}
	if(reasons & RC_HINT)
	{
	    TxPrintf("hint ");
	}
	if(reasons & RC_ROTBEFORE)
	{
	    TxPrintf("rotBefore ");
	}
	if(reasons & RC_ROTINSIDE)
	{
	    TxPrintf("rotInside ");
	}
	if(reasons & RC_BOUNDS)
	{
	    TxPrintf("bounds ");
	}
	if(reasons & RC_WALK)
	{
	    TxPrintf("walk ");
	}
	if(reasons & RC_WALKC)
	{
	    TxPrintf("walkc ");
	}
	if(reasons & RC_DONE)
	{
	    TxPrintf("done ");
	}
	TxPrintf("\n");
    }

    /* Compute extend code - i.e. interesting directions to extend from
     * new point */
    if(reasons & (RC_WALK | RC_WALKC | RC_DONE))
    {
	if(reasons & RC_WALK)
	{
	    extendCode = EC_WALKDOWN;
	}
	else if(reasons & RC_WALKC)
	{
	    extendCode = EC_WALKCONTACT;
	}
	else 
	{
	    extendCode = EC_COMPLETE;
	}
    }
    else
    {
	/* initial with just straight ahead */
	extendCode = EC_DOWN;

	if(reasons & (RC_ALIGNOTHER | RC_CONTACT | RC_ALIGNGOAL |
		      RC_HINT | RC_ROTINSIDE))
	{
	    extendCode |= EC_CONTACTS;
	}

	if(reasons & (RC_JOG | RC_ALIGNGOAL | RC_HINT | RC_ROTINSIDE))
	{
	    extendCode |= EC_RIGHT | EC_LEFT;
	}
    }

    /* compute cost of path segment from pOrg to pNew */
    {
	Tile *tp;
        bool rotate;

	tp = TiSrPointNoHint(mzHRotatePlane, &pOrg);
	rotate = (TiGetType(tp) != TT_SPACE);

	if (rotate)
	    segCost = DoubleMultII(pOrg.p_y - pNew.p_y,
		path->rp_rLayer->rl_hCost);
	else
	    segCost = DoubleMultII(pOrg.p_y - pNew.p_y, 
		path->rp_rLayer->rl_vCost);
    }

    /* Compute additional cost for paralleling nearest hint segment */
    /* (Start at low end of segment and move to high end computing hint cost
     *  as we go)
     */
    {
	Tile *tp;
	DoubleInt hintCost;
	int deltaRight, deltaLeft, delta;
	Point lowPt;

	for(lowPt = pNew; lowPt.p_y < pOrg.p_y; lowPt.p_y = TOP(tp))
	{
	    /* find tile in hint plane containing lowPt */
	    tp = TiSrPointNoHint(mzHHintPlane,&lowPt);

	    /* find nearest hint segment and add appropriate cost */
	    if(TiGetType(tp) != TT_MAGNET)
	    {
		deltaRight = (TiGetType(TR(tp)) == TT_MAGNET) ?
		    RIGHT(tp) - lowPt.p_x : -1;
		deltaLeft = (TiGetType(BL(tp)) == TT_MAGNET) ?
		    lowPt.p_x - LEFT(tp) : -1;

		/* delta = distance to nearest hint */
		if (deltaRight < 0) 
		{
		    if (deltaLeft < 0)
			delta = 0;
		    else
			delta = deltaLeft;
		}
		else
		{
		    if (deltaLeft < 0)
			delta = deltaRight;
		    else
			delta = MIN(deltaRight,deltaLeft);
		}

		if(delta>0)
		{
		    hintCost = DoubleMultII( 
			MIN(TOP(tp),pOrg.p_y) - lowPt.p_y,
			path->rp_rLayer->rl_hintCost);
		    hintCost = DoubleMultI(hintCost, delta);
		    DOUBLE_ADD(segCost,segCost,hintCost);
		}
	    }
	}
    }

    /* Process the new point */
    mzAddPoint(path, &pNew, path->rp_rLayer, 'V', extendCode, segCost);

    return;
}

