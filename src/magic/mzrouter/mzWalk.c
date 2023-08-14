/*
 * mzWalk.c --
 *
 * Code for Completing final legs of route within the blocked areas ajacent to
 * dest areas.
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
static char rcsid[] = "$Header:";
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
 * mzWalkRight --
 *
 * Extend path inside a to-the-right walk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzWalkRight(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("WALKING RIGHT\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_hBlock,&pOrg);

    /* org point should be in walk to left of dest area */
    ASSERT(TiGetType(tpThis)==TT_LEFT_WALK,"mzWalkRight");

    /* traverse to right edge of this walk to get to dest area */
    pNew.p_x = RIGHT(tpThis);
    pNew.p_y = pOrg.p_y;

    /* mark as complete path */
    extendCode = EC_COMPLETE;

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


/*
 * ----------------------------------------------------------------------------
 *
 * mzWalkLeft --
 *
 * Extend path inside a to-the-left walk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzWalkLeft(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("WALKING LEFT\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_hBlock,&pOrg);

    /* org point should be in walk to right of dest area */
    ASSERT(TiGetType(tpThis)==TT_RIGHT_WALK,"mzWalkLeft");

    /* traverse just past left edge of this walk to get to dest area */
    pNew.p_x = LEFT(tpThis) - 1;
    pNew.p_y = pOrg.p_y;

    /* mark as complete path */
    extendCode = EC_COMPLETE;

    /* compute cost of path segment from pOrg to pNew */
    {
	Tile *tp;
        bool rotate;

	tp = TiSrPointNoHint(mzVRotatePlane, &pOrg);
	rotate = (TiGetType(tp) != TT_SPACE);

	if (rotate)
	    segCost = DoubleMultII(pOrg.p_x - pNew.p_x,
		path->rp_rLayer->rl_vCost);
	else
	    segCost = DoubleMultII(pOrg.p_x - pNew.p_x, 
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

	for(lowPt = pNew; lowPt.p_x < pOrg.p_x; lowPt.p_x = RIGHT(tp))
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
			MIN(RIGHT(tp),pOrg.p_x) - lowPt.p_x,
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


/*
 * ----------------------------------------------------------------------------
 *
 * mzWalkUp --
 *
 * Extend path inside a up-walk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzWalkUp(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("WALKING UP\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_vBlock,&pOrg);

    /* org point should be in walk to left of dest area */
    ASSERT(TiGetType(tpThis)==TT_BOTTOM_WALK,"mzWalkUp");

    /* traverse to top edge of this walk to get to dest area */
    pNew.p_x = pOrg.p_x;
    pNew.p_y = TOP(tpThis);

    /* mark as complete path */
    extendCode = EC_COMPLETE;

    /* compute cost of path segment from pOrg to pNew */
    {
	Tile *tp;
        bool rotate;

	tp = TiSrPointNoHint(mzHRotatePlane, &pOrg);
	rotate = (TiGetType(tp) != TT_SPACE);

	if (rotate)
	    segCost = DoubleMultII(pNew.p_y - pOrg.p_y,
		path->rp_rLayer->rl_hCost);
	else
	    segCost = DoubleMultII(pNew.p_y - pOrg.p_y, 
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

	for(lowPt = pOrg; lowPt.p_y < pNew.p_y; lowPt.p_y = TOP(tp))
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
			MIN(TOP(tp),pNew.p_y) - lowPt.p_y,
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


/*
 * ----------------------------------------------------------------------------
 *
 * mzWalkDown --
 *
 * Extend path inside a down-walk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzWalkDown(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("WALKING DOWN\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_vBlock,&pOrg);

    /* org point should be in walk to left of dest area */
    ASSERT(TiGetType(tpThis)==TT_TOP_WALK,"mzWalkDown");

    /* traverse to just past edge of this walk to get to dest area */
    pNew.p_x = pOrg.p_x;
    pNew.p_y = BOTTOM(tpThis) -1;

    /* mark as complete path */
    extendCode = EC_COMPLETE;

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


/*
 * ----------------------------------------------------------------------------
 *
 * mzWalkContact --
 *
 * Extend path to dest area (above or below) via contact.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add extended path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzWalkContact(path)
    register RoutePath *path;
{
    Point pOrg;		/* point to extend from */
    int extendCode;	/* Interesting directions to extend in */
    RouteContact *rC;   /* Route contact to make connection with */
    RouteLayer *newRL;	/* Route layer of dest area */ 
    DoubleInt conCost;	/* Cost of final contact */

    /* DEBUG - trace calls to this routine. */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("WALKING HOME VIA CONTACT\n");

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* find contact type that connects to route layer. */
    for(rC=mzRouteContacts; rC!=NULL; rC=rC->rc_next)
    {
	/* if not active, skip it */
        if(!(rC->rc_routeType.rt_active)) continue;

	/* if it doesn't connect to current layer, skip it */
	if((rC->rc_rLayer1 != path->rp_rLayer) && 
	   (rC->rc_rLayer2 != path->rp_rLayer)) continue;

	/* if contact blocked, skip it */
	{
	    Tile *tp = TiSrPointNoHint(rC->rc_routeType.rt_hBlock,&pOrg);
	    if(TiGetType(tp) == TT_BLOCKED) continue;
	}

	/* if we got this far we found our contact, break out of the loop */
	break;
    }

    /* There should always be an rC that works */
    ASSERT(rC!=NULL,"mzWalkContact");

    /* Compute the new route layer */
    if(rC->rc_rLayer1 != path->rp_rLayer)
    {
	newRL = rC->rc_rLayer1;
    }
    else
    {
	newRL = rC->rc_rLayer2;
    }
	
    /* compute contact cost */
    {
	int i = rC->rc_cost;
	DOUBLE_CREATE(conCost, i);
    }

    /* Add final point */
    mzAddPoint(path,&pOrg,newRL,'O',EC_COMPLETE,conCost);

    return;
}


