/*
 * mzExtendRight.c --
 *
 * Code for finding next interesting point to the right.
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
static char rcsid[] = "$Header: mzXtndRght.c,v 6.1 90/08/28 19:35:57 mayo Exp $";
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
 * mzExtendRight --
 *
 * Find next interesting point to the right.
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
mzExtendRight(path)
register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pStep;	/* one unit from pOrg in direction of extension */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost;	/* cost of segment between pOrg and pNew */
    RouteLayer *rL;	/* temp variable for routelayers */
    int reasons;	/* Reasons point is interesting.  Used to
			 * avoid extensions in uninteresting directions.
			 */
    int extendCode;	/* Interesting directions to extend in */

    /* debug - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
    {
	TxPrintf("EXTENDING RIGHT\n");
    }

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* pStep = point just one unit beyond pOrg */
    pStep = pOrg;
    ++(pStep.p_x);

    /* Initial pNew to BOUNDS edge.  Must stop there
     * since blockage planes haven't been generated past there.
     */
    {
	Tile *tp;

	/* get bounds tile under pOrg */
	tp = TiSrPointNoHint(mzHBoundsPlane, &pOrg);
	pNew.p_x = RIGHT(tp);
	pNew.p_y = pOrg.p_y;
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
	    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_vBlock, 
				     &pOrg);

	    /* org point should not be blocked */
	    ASSERT(TiGetType(tpThis)==TT_SPACE,"mzExtendRight");

	    /* check to see if covered, if not extend bounds and start over */
	    covered = TRUE;
	    tpBounds = TiSrPointNoHint(mzVBoundsPlane, &pOrg);
	    while(LEFT(tpBounds)<=RIGHT(tpThis) &&
		  LEFT(tpBounds)<=mzRouteUse->cu_def->cd_bbox.r_xtop &&
		  covered)
	    {
		if(TiGetType(tpBounds) == TT_SPACE)
		{
		   /* hit edge of bounds before jog found */
		   goto rightEndJog;
		}
		else if(TOP(tpBounds)<=TOP(tpThis) && 
			TOP(tpBounds)<=mzRouteUse->cu_def->cd_bbox.r_ytop)
		{
		    Point p;

		    p.p_x = MAX(LEFT(tpBounds), pOrg.p_x);
		    p.p_y = TOP(tpBounds);
		
		    mzExtendBlockBounds(&p);
		    if(SigInterruptPending) return;
		
		    covered = FALSE;
		}
		else if(BOTTOM(tpBounds)>=BOTTOM(tpThis) &&
			BOTTOM(tpBounds)>=mzRouteUse->cu_def->cd_bbox.r_ybot)
		{
		    Point p;
		    p.p_x = MAX(LEFT(tpBounds), pOrg.p_x);
		    p.p_y = BOTTOM(tpBounds);
		
		    mzExtendBlockBounds(&p);
		    if(SigInterruptPending) return;
		
		    covered = FALSE;
		}

		/* move to next tile in bounds plane */
		if(covered)
		{
		    NEXT_TILE_RIGHT(tpBounds, tpBounds, pOrg.p_y);
		}
	    }
	}

	/* also get next tile over */ 
	NEXT_TILE_RIGHT(tpNext, tpThis, pOrg.p_y);

	if(TiGetType(tpNext)!=TT_SPACE)
	{
	    /* path blocked */
	    if(LEFT(tpNext)==pStep.p_x)
	    {
		/* pOrg right up against block */
		if(TiGetType(tpNext)==TT_LEFT_WALK)
		{
		    /* Block is walk, enter it */
		    pNew.p_x = pOrg.p_x + 1;
		    reasons = RC_WALK;
		    goto donePruning;
		}
		else if(TiGetType(tpNext)==TT_CONTACT_WALK)
		{
		    /* Block is contact walk, enter it */
		    pNew.p_x = pOrg.p_x + 1;
		    reasons = RC_WALKC;
		    goto donePruning;
		}
		else if(TiGetType(tpNext)==TT_DEST_AREA)
		{
		    /* Block is detination, enter it */
		    pNew.p_x = pOrg.p_x + 1;
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
		PRUNE_TO_MIN(pNew.p_x, LEFT(tpNext)-1, reasons, RC_JOG);
	    }
	}
	else
	{
	    /* path not blocked */
	    if((TOP(tpNext)<TOP(tpThis) || BOTTOM(tpNext)>BOTTOM(tpThis)) &&
	       RIGHT(tpThis)-1>pOrg.p_x)
	    {
		/* space is constricting, prune pNew to far 
		 * end of this tile 
		 */
		PRUNE_TO_MIN(pNew.p_x, LEFT(tpNext)-1, reasons, RC_JOG);
	    }
	    else
	    {
		/* prune to just inside next tile */
		PRUNE_TO_MIN(pNew.p_x, LEFT(tpNext), reasons, RC_JOG);
	    }
	}
	rightEndJog:;
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
	tpThis = TiSrPointNoHint(rL->rl_routeType.rt_vBlock, 
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
		tpThis = TiSrPointNoHint(rL->rl_routeType.rt_vBlock, 
					 &pOrg);

		/* org point should not be blocked */
		ASSERT(TiGetType(tpThis)==TT_SPACE,
		       "mzExtendRight, others");

		/* check to see if covered, 
		   if not extend bounds and start over */
		covered = TRUE;
		tpBounds = TiSrPointNoHint(mzVBoundsPlane, &pOrg);
		while(LEFT(tpBounds)<=RIGHT(tpThis) &&
		      LEFT(tpBounds)<=mzRouteUse->cu_def->cd_bbox.r_xtop &&
		      covered)
		{
		    if(TiGetType(tpBounds) == TT_SPACE)
		    {
			/* hit edge of bounds before jog found */
			goto rightNextLayer;
		    }
		    else if(TOP(tpBounds)<=TOP(tpThis) && 
			    TOP(tpBounds)<=mzRouteUse->cu_def->cd_bbox.r_ytop)
		    {
			Point p;

			p.p_x = MAX(LEFT(tpBounds), pOrg.p_x);
			p.p_y = TOP(tpBounds);
		
			mzExtendBlockBounds(&p);
			if(SigInterruptPending) return;
		
			covered = FALSE;
		    }
		    else if(BOTTOM(tpBounds)>=BOTTOM(tpThis) &&
			    BOTTOM(tpBounds)>=
			    mzRouteUse->cu_def->cd_bbox.r_ybot)
		    {
			Point p;
			p.p_x = MAX(LEFT(tpBounds), pOrg.p_x);
			p.p_y = BOTTOM(tpBounds);
		
			mzExtendBlockBounds(&p);
			if(SigInterruptPending) return;
		
			covered = FALSE;
		    }

		    /* move to next tile in bounds plane */
		    if(covered)
		    {
			NEXT_TILE_RIGHT(tpBounds, tpBounds, pOrg.p_y);
		    }
		}
	    }

	    /* get next tile over */ 
	    NEXT_TILE_RIGHT(tpNext, tpThis, pOrg.p_y);

	    if(TiGetType(tpNext)!=TT_SPACE)
	    {
		/* path blocked */
		if(LEFT(tpNext)==pStep.p_x)
		{
		    /* pOrg right up against obstacle, can't extend so 
		       go to next layer */
		    continue;
		}
		else
		{
		    /* prune pNew to just this side of block */
		    PRUNE_TO_MIN(pNew.p_x, 
				 LEFT(tpNext)-1, 
				 reasons, 
				 RC_ALIGNOTHER);
		}
	    }
	    else
	    {
		/* path not blocked */
		if((TOP(tpNext)<TOP(tpThis) || BOTTOM(tpNext)>BOTTOM(tpThis)) 
		   && RIGHT(tpThis)-1>pOrg.p_x)
		{
		    /* space is constricting, prune pNew to far 
		     * end of this tile 
		     */
		    PRUNE_TO_MIN(pNew.p_x, 
				 LEFT(tpNext)-1, 
				 reasons, 
				 RC_ALIGNOTHER);
		}
		else
		{
		    /* prune pNew to just inside next tile */
		    PRUNE_TO_MIN(pNew.p_x, 
				 LEFT(tpNext), 
				 reasons, 
				 RC_ALIGNOTHER);
		}
	    }
	}
	rightNextLayer:;
    }

    /* Prune pNew to next alignment with a DESTINATION terminal */
    {
	int *xAlign = &(mzNLGetContainingInterval(&mzXAlignNL,pOrg.p_x)[1]);
	PRUNE_TO_MIN(pNew.p_x, *xAlign, reasons, RC_ALIGNGOAL);
    }

    /* Prune pNew to alignment with perpendicular HINT edges */
    {
	Tile *tp;

	tp = TiSrPointNoHint(mzVHintPlane, &pOrg);
	PRUNE_TO_MIN(pNew.p_x, RIGHT(tp), reasons, RC_HINT);
    }

    /* Prune pNew to either side  of tile edges on ROTATE plane (organized
     * into maximum strips in perpendicular direction).  Jogging
     * at such points can effect cost.
     * NOTE:  Also must have intermediate path points at boundaries between 
     * rotate and non-rotate since edge cost different in these regions.
     */
    {
	Tile *tp;

	tp = TiSrPointNoHint(mzVRotatePlane, &pStep);

	if(LEFT(tp) > pOrg.p_x)
	/* prune to beginning of tile */
	PRUNE_TO_MIN(pNew.p_x, LEFT(tp), reasons, RC_ROTBEFORE);
	else
	/* prune to end of tile */
	PRUNE_TO_MIN(pNew.p_x, RIGHT(tp)-1, reasons, RC_ROTINSIDE);
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
				 ((RouteContact*) LIST_FIRST(cL))->rc_routeType.rt_hBlock, 
				 &pStep);

	    if (TiGetType(tp) == TT_SPACE)
	    {
		/* SPACE TILE */ 
		if(LEFT(tp) > pOrg.p_x)
		/* prune to beginning of tile */
		PRUNE_TO_MIN(pNew.p_x, LEFT(tp), reasons, RC_CONTACT);
		else
		/* prune to end of tile */
		PRUNE_TO_MIN(pNew.p_x, RIGHT(tp)-1, reasons, RC_CONTACT);
	    }
	    else
	    {
		/* BLOCK TILE - so prune to just beyond tile */
		PRUNE_TO_MIN(pNew.p_x, RIGHT(tp), reasons, RC_CONTACT);
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
	    extendCode = EC_WALKRIGHT;
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
	extendCode = EC_RIGHT;

	if(reasons & (RC_ALIGNOTHER | RC_CONTACT | RC_ALIGNGOAL |
		      RC_HINT | RC_ROTINSIDE))
	{
	    extendCode |= EC_CONTACTS;
	}

	if(reasons & (RC_JOG | RC_ALIGNGOAL | RC_HINT | RC_ROTINSIDE))
	{
	    extendCode |= EC_UP | EC_DOWN;
	}
    }

    /* compute cost of path segment from pOrg to pNew */
    {
	Tile *tp;
	bool rotate;

	tp = TiSrPointNoHint(mzVRotatePlane, &pOrg);
	rotate = (TiGetType(tp) != TT_SPACE);

	if (rotate)
	segCost = DoubleMultII(pNew.p_x - pOrg.p_x,
			       path->rp_rLayer->rl_vCost);
	else
	segCost = DoubleMultII(pNew.p_x - pOrg.p_x, 
			       path->rp_rLayer->rl_hCost);
    }

    /* Compute additional cost for paralleling nearest hint segment */
    /* (Start at low end of segment and move to high end computing hint cost
     *  as we go)
     */
    {
	Tile *tp;
	DoubleInt hintCost;
	int deltaUp, deltaDown, delta;
	Point lowPt;

	for(lowPt = pOrg; lowPt.p_x < pNew.p_x; lowPt.p_x = RIGHT(tp))
	{
	    /* find tile in hint plane containing lowPt */
	    tp = TiSrPointNoHint(mzVHintPlane,&lowPt);

	    /* find nearest hint segment and add appropriate cost */
	    if(TiGetType(tp) != TT_MAGNET)
	    {
		deltaUp = (TiGetType(RT(tp)) == TT_MAGNET) ?
		TOP(tp) - lowPt.p_y : -1;
		deltaDown = (TiGetType(LB(tp)) == TT_MAGNET) ?
		lowPt.p_y - BOTTOM(tp) : -1;

		/* delta = distance to nearest hint */
		if (deltaUp < 0) 
		{
		    if (deltaDown < 0)
		    delta = 0;
		    else
		    delta = deltaDown;
		}
		else
		{
		    if (deltaDown < 0)
		    delta = deltaUp;
		    else
		    delta = MIN(deltaUp,deltaDown);
		}

		if(delta>0)
		{
		    hintCost = DoubleMultII( 
					    MIN(RIGHT(tp),pNew.p_x)
					            - lowPt.p_x,
					    path->rp_rLayer->rl_hintCost);
		    hintCost = DoubleMultI(hintCost, delta);
		    DOUBLE_ADD(segCost,segCost,hintCost);
		}
	    }
	}
    }

    /* Process the new point */
    mzAddPoint(path, &pNew, path->rp_rLayer, 'H', extendCode, segCost);

    return;
}
