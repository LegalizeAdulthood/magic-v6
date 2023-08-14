/*
 * mzStart.c --
 *
 * Code for making initial legs of route within the blocked areas ajacent to
 * start areas.
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
 * mzStart --
 *
 * Establish initial path segments from start term, considering inital 
 * contacts and leading out of any SAMENODE blocks
 * present at start point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add paths to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzStart(term)
    ColoredPoint *term;
{
    RouteLayer *rL;
    Tile *tp;

    /* Find routelayer corresponding to type */
    for(rL=mzActiveRLs; rL!=NULL; rL=rL->rl_nextActive)
    {
	if(rL->rl_routeType.rt_tileType == term->cp_type) break;
    }
    
    /* If no corresponding route layer, give warning and return*/
    if(rL==NULL)
    {
	TxError("Start term type does not correspond ");
	TxError("route layer: %s\n", 
		DBTypeLongNameTbl[term->cp_type]);
	return;
    }

    /* call mzExtendInitPath to (recursively) do the real work */
    {
	int directions = EC_RIGHT | EC_LEFT | EC_UP | EC_DOWN;

	mzExtendInitPath(NULL, 			/* path so far */
			 rL, 			/* layer of new point */
			 term->cp_point, 	/* new point */
			 DIZero, 		/* cost of new segment */
			 0, 			/* length of path so far */
			 directions		/* how to extend init path */
			 );
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendInitPath --
 *
 * Central routine for recursively building up initial path inside 
 * SAMENODE_BLOCK.  Adds specified point to an initial path, if resulting
 * path end is outside block, mzAddPoint() is called to add init path
 * to appropriate queue.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Initial paths built up.
 *	mzAddPoint() eventually called to add init paths to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */
Void
mzExtendInitPath(path, rL, point, cost, length, directions)
    RoutePath *path;	/* Initial Path, being extended */
    RouteLayer *rL;     /* routelayer of new point */
    Point point;	/* new point for  initPath */
    DoubleInt cost;     /* cost of new segment */
    int length;		/* length of path (excluding new segment) */
    int directions;	/* directions to extend init path in */
{
    Tile *tp;

    /* Get tile in rL blockage plane under new point */
    tp = TiSrPointNoHint(rL->rl_routeType.rt_hBlock,&point);

    /* If new point blocked by a different node, just return */
    if(TiGetType(tp)==TT_BLOCKED)
    {
	return;
    }

    /* Consider initial contacts */
    if(path==NULL)
    {
	mzAddInitialContacts(rL, point);
    }

    /* If no SAMENODE block, call mzAddPoint() to and initial path to 
     * appropriate queue.
     */
    {
	int extendCode = 0;

	switch (TiGetType(tp))
	{
	    case TT_SPACE:
	    {
		extendCode = EC_RIGHT | EC_LEFT | EC_UP | EC_DOWN 
		| EC_CONTACTS;
	    }
	    break;

	    case TT_LEFT_WALK:
	    {
		extendCode = EC_WALKRIGHT;
	    }
	    break;
			
	    case TT_RIGHT_WALK:
	    {
		extendCode = EC_WALKLEFT;
	    }
	    break;
			
	    case TT_TOP_WALK:
	    {
		extendCode = EC_WALKDOWN;
	    }
	    break;
			
	    case TT_BOTTOM_WALK:
	    {
		extendCode = EC_WALKUP;
	    }
	    break;

	    case TT_CONTACT_WALK:
	    {
		extendCode = EC_WALKCONTACT;
	    }
	    break;
			
	    case TT_DEST_AREA:
	    {
		TxError("Zero length route!\n");
		extendCode = EC_COMPLETE;
	    }
	    break;
	}

	if(extendCode != 0)
	/* An above case fit, add appropriate point and return */
	{
	    int orient;

	    /* determine orientation of new segment */
	    if(path==NULL || path->rp_rLayer!=rL)
	    {
		orient = 'O';
	    }
	    else if(path->rp_entry.p_x==point.p_x)
	    {
		orient = 'V';
	    }
	    else 
	    {
		ASSERT(path->rp_entry.p_y==point.p_y,"mzExtendInitPath");
		orient = 'H';
	    }

	    /* Add initial path to appropriate queue */
	    mzAddPoint(path, &point, rL, orient, extendCode, cost);

	    return;
	}
    }

    /* If we get here path is blocked by SAMENODE_BLOCK, manually add
     * new point to path and then extend to edge of block.
     */
    {
	RoutePath *newPath;

	/* check that SAMENODE_BLOCK is in fact present at start point */
	ASSERT(TiGetType(tp)==TT_SAMENODE_BLOCK,"mzStart");

	/* add new point to path consisting of start point */
	{
	    newPath = NEWPATH();
	    newPath->rp_rLayer = rL;
	    newPath->rp_entry = point;
	    newPath->rp_orient = 'O'; /* doesn't have to be accurate */
	    newPath->rp_cost = cost;
	    newPath->rp_back = path;   /* first element in path */
	}

	/* extend to edge of block */
	{
	    if (directions & EC_RIGHT)
	    {
		mzStartRight(newPath, length);
	    }

	    if (directions & EC_LEFT)
	    {
		mzStartLeft(newPath, length);
	    }

	    if (directions & EC_UP)
	    {
		mzStartUp(newPath, length);
	    }

	    if (directions & EC_DOWN)
	    {
		mzStartDown(newPath, length);
	    }
	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzAddInitialContacts --
 *
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Calls mzExtendInitPath to add contact to initial path.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzAddInitialContacts(rL, point)
    RouteLayer *rL;     /* routelayer of initial point */
    Point point;	/* initial point */
{
    register List *cL;

    /* Loop through contacts that connect to current rLayer */
    for (cL=rL->rl_contactL; cL!=NULL; cL=LIST_TAIL(cL))
    {
	RouteContact *rC = (RouteContact *) LIST_FIRST(cL);
	RouteLayer *newRLayer;
	DoubleInt conCost;
	RoutePath *initPath;

	/* Don't use inactive contacts */
	if (!(rC->rc_routeType.rt_active))
	continue;

	/* Get "other" route Layer contact connects to */
	if (rC->rc_rLayer1 == rL)
	{
	    newRLayer = rC->rc_rLayer2;
	}
	else
	{
	    ASSERT(rC->rc_rLayer2 == rL,"mzStart");
	    newRLayer = rC->rc_rLayer1;
	}

	/* Don't spread to inactive layers */
	if (!(newRLayer->rl_routeType.rt_active))
	{
	    continue;
	}

	/* Don't place contact if blocked */
	{
	    Tile *tp = TiSrPointNoHint(rC->rc_routeType.rt_hBlock,&point);
	    
	    if (TiGetType(tp)==TT_BLOCKED)
	    {
		continue;
	    }
	}
	
	/* compute cost of contact */
	{
	    
	    int i = rC->rc_cost;
	    DOUBLE_CREATE(conCost, i);
	}

	/* build path consisting of initial point */
	{
	    initPath = NEWPATH();
	    initPath->rp_rLayer = rL;
	    initPath->rp_entry = point;
	    initPath->rp_orient = 'O'; 
	    initPath->rp_cost = DIZero;
	    initPath->rp_back = NULL;   
	}

	/* Extend thru new point */
	mzExtendInitPath(initPath,
			 newRLayer,
			 point,
			 conCost,
			 0,
			 (EC_LEFT | EC_RIGHT | EC_UP | EC_DOWN)
			 );
    }

    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * mzStartRight --
 *
 * Extend path toward right 
 * to just outside SAMENODE_BLOCK.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzExtendInitPath() called to augment init path.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzStartRight(path, length)
    register RoutePath *path;
    int length;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */
    Tile *tpNew;	/* Tile containing new point */

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_hBlock,&pOrg);

    /* org point should be in SAMENODE_BLOCK */
    ASSERT(TiGetType(tpThis)==TT_SAMENODE_BLOCK,"mzStartRight");

    /* traverse to right edge of block */
    pNew.p_x = RIGHT(tpThis);
    pNew.p_y = pOrg.p_y;

    /* add new segment to path length */
    length += pNew.p_x - pOrg.p_x;

    /* if length exceeds max, just return */
    if(length>mzMaxWalkLength)
    {
	return;
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
		    {
			delta = 0;
		    }
		    else
		    {
			delta = deltaDown;
		    }
		}
		else
		{
		    if (deltaDown < 0)
		    {
			delta = deltaUp;
		    }
		    else
		    {
			delta = MIN(deltaUp,deltaDown);
		    }
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

    /* Extend thru new point */
    mzExtendInitPath(path, path->rp_rLayer, pNew, segCost, length, EC_RIGHT);

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzStartLeft --
 *
 * Extend path toward left 
 * to just outside SAMENODE_BLOCK
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzStartLeft(path, length)
    register RoutePath *path;
    int length;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */
    Tile *tpNew;	/* Tile containing new point */

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_hBlock,&pOrg);

    /* org point should be in SAMENODE_BLOCK */
    ASSERT(TiGetType(tpThis)==TT_SAMENODE_BLOCK,"mzStartLeft");

    /* traverse just past left edge of block */
    pNew.p_x = LEFT(tpThis) - 1;
    pNew.p_y = pOrg.p_y;

    /* add new segment to path length */
    length += pOrg.p_x - pNew.p_x;

    /* if length exceeds max, just return */
    if(length>mzMaxWalkLength)
    {
	return;
    }

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

    /* Extend thru new point */
    mzExtendInitPath(path, path->rp_rLayer, pNew, segCost, length, EC_LEFT);


    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzStartUp --
 *
 * Extend path inside upward
 * to just outside SMAENODE_BLOCK.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	mzAddPoint() called to add path to appropriate queue.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzStartUp(path, length)
    register RoutePath *path;
    int length;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */
    Tile *tpNew;	/* Tile containing new point */

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_vBlock,&pOrg);

    /* org point should be in walk to left of dest area */
    ASSERT(TiGetType(tpThis)==TT_SAMENODE_BLOCK,"mzStartUp");

    /* traverse to top edge of block */
    pNew.p_x = pOrg.p_x;
    pNew.p_y = TOP(tpThis);

    /* add new segment to path length */
    length += pNew.p_y-pOrg.p_y;

    /* if length exceeds max, just return */
    if(length>mzMaxWalkLength)
    {
	return;
    }

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

    /* Extend thru new point */
    mzExtendInitPath(path, path->rp_rLayer, pNew, segCost, length, EC_UP);

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzStartDown --
 *
 * Extend path downward
 * to just outside SAMENODE_BLOCK
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
mzStartDown(path, length)
    register RoutePath *path;
    int length;
{
    Point pOrg;		/* point to extend from */
    Point pNew;		/* next interesting point in direction of extension */
    DoubleInt segCost; 	/* cost of segment between pOrg and pNew */
    int extendCode;	/* Interesting directions to extend in */
    Tile *tpThis;	/* Tile containing org point */
    Tile *tpNew;	/* Tile containing new point */

    /* pOrg = current end of path */
    pOrg = path->rp_entry;

    /* get blockage plane tile under pOrg */
    tpThis = TiSrPointNoHint(path->rp_rLayer->rl_routeType.rt_vBlock,&pOrg);

    /* org point should be in SAMENODE_BLOCK */
    ASSERT(TiGetType(tpThis)==TT_SAMENODE_BLOCK,"mzStartDown");

    /* traverse to just past edge of this walk to get to dest area */
    pNew.p_x = pOrg.p_x;
    pNew.p_y = BOTTOM(tpThis) -1;

    /* add new segment to path length */
    length += pOrg.p_y - pNew.p_y;

    /* if length inside block exceeds max walk length, just return */
    if(length>mzMaxWalkLength)
    {
	return;
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

    /* Extend thru new point */
    mzExtendInitPath(path, path->rp_rLayer, pNew, segCost, length, EC_DOWN);

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzStartContact --
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
mzStartContact(path)
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


