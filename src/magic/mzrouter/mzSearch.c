/*
 * mzSearch.c --
 *
 * Search strategy for maze router.
 * Low level of Maze router (finding next interesting point) is done in 
 * mzSearchRight.c etc. 
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
 *
 * SEARCH STRATEGY:
 *
 * Partial paths are expanded one interesting point at a time - that is a
 * path is expanded to the next interesting point left, right, up and down
 * and vias to neighboring layers are considered, then a new (though possibly
 * one of the extensions just created) path is selected for expansion.  
 *
 * The "search strategy" employed determines which partial-path is
 * chosen for exansion at each stage.
 *
 * A windowed search strategy is used.  At any given time the window
 * bounds define a minimum and maximum distance to the goal.  Partial paths
 * are divided into three groups: those farther from the goal than the
 * window, those within the window, and those nearer to the goal than the
 * window.  The window begins at the start point and is slowly shifted
 * toward the goal over time. Normally the partial-path of least 
 * estimated-total-cost WITHIN
 * the window is chosen for expansion, although a further path (i.e. one
 * beyond the window) that has exceptially low estimated-total-cost is
 * sometimes chosen instead.  When the trailing edge of the window reaches
 * the goal, the search is deemed complete and the lowest cost complete
 * path found is returned.  However if no complete-path has been found
 * searching will continue until the first complete path is found.
 * The idea behind the windowed search is to
 * expand the paths that promise to give the best (lowest cost) solution, but
 * to slowly shift attention toward the goal to avoid the VERY long time
 * it would take to examine all possible paths.
 *
 * In the case of many partial paths within the window, the above search may
 * be too scattered; "blooming"  is used to give a local-focus to the search
 * by examining a number of expansions
 * from a given partial path before going on to consider other paths.  Blooming
 * is equivalent to searching the move tree in chess before applyng static
 * evaluations of "final" positions.  In practice it makes the router behave
 * much better.
 *
 * The search strategy is implemented with three heaps and four stacks:
 * 
 * mzMaxToGoHeap - keeps partial paths NEARER the goal than the window.
 *                 these paths are not yet eligible for expansion.
 *                 Whenever the window is moved, the top elements of
 *                 this heap (i.e. those furthest from the goal) are
 *                 moved onto the mzMinCostHeap (i.e. into the window).
 *
 * mzMinCostHeap - keeps partial paths inside the window.  the top of this 
 *                 heap, i.e. the least cost path inside
 *                 the window, is compared with the least (penalized) cost of
 *                 the paths beyond the window, and the lesser of these
 *	           two is chosen for expansion.
 *
 * mzMinAdjCostHeap - keeps partial paths that are further from the goal 
 *                    than the 
 *                    window.  These paths are sorted by adjusted cost.
 *                    Adjusted cost is estimated-total-cost plus a penalty
 *                    proportial to the distance of the path from the window.
 *                    The ordering of the paths is independent of the current
 *                    window position - so a "reference position" is used to
 *                    avoid recomputing adjusted costs for these paths 
 *                    everytime the window moves.  The adjusted cost of the
 *                    top (least cost) element is normalized to the current
 *                    window position before comparison with the least cost
 *                    path on the mzMinCostHeap.  The idea behind the
 *                    mzMinAdjCostHeap is
 *                    to discourage searching of paths beyond the window, but
 *                    to do so in a gentle and flexible way, so that 
 *                    behaviour will degrade gracefully under unusual
 *                    circumstances rather than fail catastrophically.
 *                    For example, if the window moves too fast and all the
 *                    reasonable paths are left behind, the search will
 *                    degenerate to a simple cost-based search biased towards 
 *                    paths nearer to completion - a startegy that is
 *                    adequate in many situations.
 *
 * mzBloomStack - paths in current local foucs = all expansions of a given
 *                partial-path that do not exceed a given estimated-cost
 *                (also see stacks below)
 *
 * mzStraightStack - focus paths being extended in straight line.
 *
 * mzDownHillStack - focus paths followed only unitl cost increases.
 *
 * mzWalkStack - paths insise walk (i.e. inside blocked area adjacent to
 *               destination.
 *
 * If the walk stack is not empty the next path is taken from there (thus
 * paths that have reached the threshold of a dest area are given priority.)
 *
 * Next priority is given to the down hill stack, thus a focus path is always
 * expanded until the estimated cost increases.
 *
 * Next the straight stack is processed.  The purpose of this stack is to
 * consider straight extensions in all directions to some given minimum 
 * distance.
 *
 * Next the bloom stack itself is processed.  All extensions derived from
 * the bloom stack are placed on one of the stacks until a fixed cost increment
 * is exceeded (in practice a small increment is used, thus the local focus
 * is only extended in straight lines, and then downhill).
 *
 * If all the stacks are empty, a new focus is pickd from the heaps and 
 * the bloom stack is initialized with it.
 *
 */

#ifndef lint
static char rcsid[] = "$Header: mzSearch.c,v 6.1 90/08/28 19:35:28 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "doubleint.h"
#include "hash.h"
#include "heap.h"
#include "../tiles/tile.h"
#include "database.h"
#include "signals.h"
#include "textio.h"
#include "malloc.h"
#include "list.h"
#include "debug.h"
#include "styles.h"
#include "windows.h"
#include "dbwind.h"
#include "mzrouter.h"
#include "mzInternal.h"


/*
 * ----------------------------------------------------------------------------
 *
 * mzSearch --
 *
 * Find a path between one of the starting terminals and the
 * destination.  The path must lie within the
 * area mzBoundingRect.
 *
 * Results:
 *	Returns a pointer to best complete RoutePath found (NULL
 *	if none found).  This RoutePath may be passed to
 *	MZPaintPath() to be converted into paint.
 *
 * Side effects:
 *	Allocates RoutePaths from our own private storage area.
 *	The caller must copy these to permanent storage with
 *	mzCopyPath() prior to calling MZClean; otherwise,
 *	the returned RoutePath will be lost.
 *
 * ----------------------------------------------------------------------------
 */

RoutePath *
mzSearch()
{
    RoutePath *path;		/* solution */
    bool morePartialPaths = TRUE;
    bool bloomLimitHit = FALSE;
    bool windowSweepDoneAndCompletePathFound = FALSE;

    /* Report intial cost estimate */
    if(mzVerbosity>=VERB_STATS)
    {
	TxPrintf("Initial Cost Estimate:   %.0f\n",
		 DoubleToDFloat(mzInitialEstimate));
    }

    if (DebugIsSet(mzDebugID, mzDebMaze))
    {
	TxPrintf("\nBEGINNING SEARCH.\n");
	TxPrintf("\tmzWRate = %.0f\n", 
		 DoubleToDFloat(mzWRate));
	TxPrintf("\tmzWInitialMinToGo = %.0f\n", 
		 DoubleToDFloat(mzWInitialMinToGo));
	TxPrintf("\tmzWInitialMaxToGo = %.0f\n", 
		 DoubleToDFloat(mzWInitialMaxToGo));
	TxPrintf("\tmzBloomDeltaCost = %.0f\n",
		 DoubleToDFloat(mzBloomDeltaCost));
    }

    while (morePartialPaths && 
	   !windowSweepDoneAndCompletePathFound &&
	   !bloomLimitHit &&
	   !SigInterruptPending)
    /* Find next path to extend from */
    {
	if (DebugIsSet(mzDebugID, mzDebMaze))
	{
	    TxPrintf("\nABOUT TO SELECT NEXT PATH TO EXTEND.\n");
	    TxMore("");
	}

	/* pop a stack */
	path = NULL;
	if(mzWalkStack != NULL)
	{
	    mzPathSource = SOURCE_WALK;
	    path = (RoutePath *) ListPop(&mzWalkStack);
	 	
	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("POPPING TOP OF WALK STACK for extension.\n");
		mzPrintPathHead(path);
	    }
	}   
	else if(mzDownHillStack != NULL)
	{
	    mzPathSource = SOURCE_DOWNHILL;
	    path = (RoutePath *) ListPop(&mzDownHillStack);
	
	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("POPPING TOP OF DOWNHILL STACK for extension.\n");
		mzPrintPathHead(path);
	    }
	}
	else if(mzStraightStack != NULL)
	{
	    mzPathSource = SOURCE_STRAIGHT;
	    path = (RoutePath *) ListPop(&mzStraightStack);
	
	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("POPPING TOP OF STRAIGHT STACK for extension.\n");
		mzPrintPathHead(path);
	    }
	}
	else if(mzBloomStack != NULL)
	{
	    mzPathSource = SOURCE_BLOOM;
	    path = (RoutePath *) ListPop(&mzBloomStack);

	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("POPPING TOP OF BLOOM STACK for extension.\n");
		mzPrintPathHead(path);
	    }
	}

	/* Stacks contained a path, go about the buisness of extending it */
	if(path)
	{
	    /* Check hashtable to see if path already obsolete,
	     * (i.e. cheaper path to its enpt found.) 
	     */
	    {
		HashEntry *he;
		PointKey pk;
		RoutePath *hashedPath;

		pk.pk_point = path->rp_entry;
		pk.pk_rLayer = path->rp_rLayer;
		pk.pk_orient = path->rp_orient;

		he = HashFind(&mzPointHash, (ClientData) &pk);
		hashedPath = (RoutePath *) HashGetValue(he);
		ASSERT(hashedPath!=NULL,"mzFindPath");

		if (path!=hashedPath)
		{
		    /* DEBUG - report path rejected due to hash value. */
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("HASH LOOKUP reveals better path, REJECT path.\n");
		    }
		    
		    /* better path to this pt already exists, 
		     * skip to next path
		     */
		    continue;
		}
	    }

	    /* Make sure blockage planes have been generated around the path
	     * end.
	     */
	    {
		Point *point = &(path->rp_entry);
		Tile *tp = TiSrPointNoHint(mzHBoundsPlane, point);

		if (TiGetType(tp)==TT_SPACE ||
		    point->p_x == LEFT(tp) || point->p_x == RIGHT(tp))
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("Path ends on vertical boundary of blockage");
			TxPrintf(" planes, BLOCKAGE PLANES BEING EXTENDED.\n");
		    }

		    mzExtendBlockBounds(point);
		    if(SigInterruptPending) continue;
		}

		else
		{

		    tp = TiSrPointNoHint(mzVBoundsPlane, point);
		    if (point->p_y == BOTTOM(tp) || point->p_y == TOP(tp))
		    {
			if (DebugIsSet(mzDebugID, mzDebMaze))
			{
			    TxPrintf("Path ends on horizontal boundary");
			    TxPrintf("of blockage planes,  BLOCKAGE PLANES");
			    TxPrintf("BEING EXTENDED.\n");
			}

			mzExtendBlockBounds(point);
			if(SigInterruptPending) continue;
		    }
		}
	    }
	
	    /* DEBUG - if single-stepping, print data, show path end visually,
	     *	and pause.
	     */
	    if (DebugIsSet(mzDebugID, mzDebStep))
	    {
		Rect box;
		CellDef *boxDef;

		/* print stats on this path */
		{
		    int i;
		    RoutePath *p;

		    /* path # */
		    TxPrintf("READY TO EXTEND PATH ");
		    TxPrintf("(blooms: %d, points-processed: %d):\n", 
			     mzNumBlooms,
			     mzNumPaths);
		    mzPrintPathHead(path);

		    /* # of segments in path segments */
		    i=0;
		    for(p=path; p->rp_back!=0; p=p->rp_back)
		    i++;
		    TxPrintf("  (%d segments in path)\n", i);
		}
	    
		/* move box to path end-point */
		if(ToolGetBox(&boxDef,&box))
		{
		    int deltaX = box.r_xtop - box.r_xbot;
		    int deltaY = box.r_ytop - box.r_ybot;
		    box.r_ll = path->rp_entry;
		    box.r_ur.p_x = path->rp_entry.p_x + deltaX;
		    box.r_ur.p_y = path->rp_entry.p_y + deltaY;
		    DBWSetBox(mzRouteUse->cu_def, &box);
		    WindUpdate();
		}

		/* wait for user to continue */
		TxMore("");
	    }

	    /* Extend Path */
	    mzExtendPath(path);

	    /* update statistics */
	    mzNumPaths++;	/* increment number of paths processed */
	    if(--mzPathsTilReport == 0)
	    {
		mzPathsTilReport = mzReportInterval;
		mzMakeStatReport();
	    }

	}
	else
	/* stacks are empty, choose path from heaps to initial them with */
	{
	    HeapEntry maxToGoTopBuf, minCostTopBuf, buf;
	    HeapEntry *maxToGoTop, *minCostTop, *minAdjCostTop;
	    
	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("BLOOM STACK EMPTY.  ");
                TxPrintf("Choosing path from heaps to initial it with.\n");
	    }

	    /* If the bloom limit has been exceeded, stop searching */
	    if(mzBloomLimit >0 && 
	       mzNumBlooms > mzBloomLimit)
	    {
		if(mzVerbosity>=VERB_BRIEF)
		{
		    TxPrintf("Bloom limit (%d) hit.\n", mzBloomLimit);
		}

		bloomLimitHit = TRUE;
		continue;
	    }

	    /* set window thresholds */
	    {
		DoubleInt offset;

		offset = DoubleMultI(mzWRate, mzNumBlooms);

		if(DOUBLE_LE(offset, mzWInitialMinToGo))
		{
		    DOUBLE_SUB(mzWindowMinToGo, mzWInitialMinToGo, offset);
		}
		else
		{
		    mzWindowMinToGo = DIZero;
		}

		if(DOUBLE_LE(offset, mzWInitialMaxToGo))
		{
		    DOUBLE_SUB(mzWindowMaxToGo, mzWInitialMaxToGo, offset);
		}
		else
		{
		    mzWindowMaxToGo = DIZero;
		}

	        if (DebugIsSet(mzDebugID, mzDebMaze))
	        {
		    TxPrintf("New window thresholds:  ");
                    TxPrintf("windowMinToGo = %.0f, ",
			     DoubleToDFloat(mzWindowMinToGo));
		    TxPrintf("windowMaxToGo = %.0f\n ",
			     DoubleToDFloat(mzWindowMaxToGo));
  	        }
	    }

	    /* If window sweep is complete, and a complete path
	     * has been found, stop searching.
	     */
	    if(DOUBLE_EQUAL(mzWindowMaxToGo, DIZero) && 
	       HeapLookAtTop(&mzMinCostCompleteHeap))
	    {
	        if (DebugIsSet(mzDebugID, mzDebMaze))
	        {
		    TxPrintf("WINDOW SWEEP DONE AND COMPLETE PATH EXISTS.");
		    TxPrintf("  Stop searching.\n");
  	        }

		windowSweepDoneAndCompletePathFound = TRUE;
		continue;
	    }

	    /* Move points that meet the minTogo threshold to window 
	    * (Points are moved from the MaxToGo heap to the minCost heap)
	    */
	    {
		if (DebugIsSet(mzDebugID, mzDebMaze))
		{
		    TxPrintf("Moving paths into window "); 
		    TxPrintf("(maxTogoHeap -> minCostHeap):  \n");
		}

		while((maxToGoTop=HeapRemoveTop(&mzMaxToGoHeap,
						  &maxToGoTopBuf)) != NULL
		      && 
		      DOUBLE_GE(maxToGoTop->he_union.hu_dint, 
				mzWindowMinToGo))
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			mzPrintPathHead((RoutePath*)(maxToGoTop->he_id));
		    }

		    HeapAdd(&mzMinCostHeap, 
			      &(((RoutePath *)(maxToGoTop->he_id))->rp_cost), 
			      (char *) (maxToGoTop->he_id));
		}
		if(maxToGoTop!=NULL)
		{
		    HeapAdd(&mzMaxToGoHeap,
			      &(maxToGoTop->he_union.hu_dint), 
			      (char *) (maxToGoTop->he_id));
		}
	    }

	    /* Clear top of minCostHeap of points that no longer meet the
	     * mzWindowMaxToGo threshold.
	     * (minCostHeap -> minAdjCostHeap)
	     */
	    {
		if (DebugIsSet(mzDebugID, mzDebMaze))
		{
		    TxPrintf("Moving paths out of window ");
		    TxPrintf("(minCostHeap -> minAdjCostHeap):  \n");
		}

		while((minCostTop=HeapRemoveTop(&mzMinCostHeap,
						  &minCostTopBuf))!=NULL 
		      &&
		      DOUBLE_GREATER(((RoutePath *)(minCostTop->he_id))
				     ->rp_togo,mzWindowMaxToGo))
		{
		    DoubleInt adjCost;

		    /* compute adjusted cost */
		    adjCost = ((RoutePath *)(minCostTop->he_id))->rp_togo;
		    adjCost = DoubleMultI(adjCost, mzPenalty.rf_mantissa);
		    DOUBLE_SHIFTRIGHT(adjCost, adjCost, 
				      mzPenalty.rf_nExponent);
		    DOUBLE_ADD(adjCost, adjCost, minCostTop->he_union.hu_dint);

		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			mzPrintPathHead((RoutePath*)(minCostTop->he_id));
			TxPrintf("  Heap-key adjCost = %.0f\n",
				 DoubleToDFloat(adjCost));
		    }

		    /* add to adjusted cost heap */
		    HeapAdd(&mzMinAdjCostHeap, 
			      &adjCost,
			      (char *) (minCostTop->he_id));
		}

		if(minCostTop!=NULL)
		{
		    HeapAdd(&mzMinCostHeap,
			      &(minCostTop->he_union.hu_dint), 
			      (char *) (minCostTop->he_id));
		}
	    }

	    /* Peek at tops of heaps 
	     * (equal cost elements might have got shuffled above when
	     * we placed the last poped element back on the heap.)
	     */
	    minAdjCostTop = HeapLookAtTop(&mzMinAdjCostHeap);
	    maxToGoTop = HeapLookAtTop(&mzMaxToGoHeap);
	    minCostTop = HeapLookAtTop(&mzMinCostHeap);
	   
	    /* Print tops of maxToGo, minCost and minAdjCost heaps */
	    if (DebugIsSet(mzDebugID, mzDebMaze))
	    {
		TxPrintf("Max togo top:\n");
		if(maxToGoTop)
		{
		    mzPrintPathHead((RoutePath*)(maxToGoTop->he_id));
		}
		else
		{
		    TxPrintf("  nil\n");
		}
		TxPrintf("Min cost top:\n");
		if(minCostTop)
		{
		    mzPrintPathHead((RoutePath*)(minCostTop->he_id));
		}
		else
		{
		    TxPrintf("  nil\n");
		}
		TxPrintf("Min adjcost top:\n");
		if(minAdjCostTop)
		{
		    TxPrintf("  Heap-key adjCost:  %.0f\n", 
			     DoubleToDFloat(minAdjCostTop->he_union.hu_dint));
		}
		else
		{
		    TxPrintf("  nil\n");
		}
	    }

	    if(minCostTop && minAdjCostTop)
	    /* need to compare minCostTop and minAdjCostTop */
	    {
		/* compute adjusted cost corresponding to current window
		 * position (we only penalize for amount toGo exceeding
		 * mzWindowMaxToGo)
		 */
		DoubleInt cost, adjCost;

		cost = ((RoutePath *)(minAdjCostTop->he_id))->rp_cost;
		adjCost = ((RoutePath *)(minAdjCostTop->he_id))->rp_togo;
		ASSERT(DOUBLE_GE(adjCost, mzWindowMaxToGo),"mzSearch");
		DOUBLE_SUB(adjCost, adjCost, mzWindowMaxToGo);
		adjCost = DoubleMultI(adjCost, mzPenalty.rf_mantissa);
		DOUBLE_SHIFTRIGHT(adjCost, adjCost, mzPenalty.rf_nExponent);
		DOUBLE_ADD(adjCost, adjCost, cost);
		if (DebugIsSet(mzDebugID, mzDebMaze))
		{		
		    TxPrintf("WINDOW-CORRECTED ADJCOST:  %.0f\n",
			     DoubleToDFloat(adjCost));
		}
		if(DOUBLE_LE(minCostTop->he_union.hu_dint, adjCost))
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("INITIALIZING BLOOM STACK ");
			TxPrintf("WITH TOP OF MIN COST HEAP.\n");
		    }
		    minCostTop = HeapRemoveTop(&mzMinCostHeap, &buf);
		    mzBloomInit((RoutePath *) (minCostTop->he_id));

		}
		else
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("INITIALIZING BLOOM STACK ");
			TxPrintf("WITH TOP OF MIN ADJCOST HEAP.\n");
		    }
		    minAdjCostTop = HeapRemoveTop(&mzMinAdjCostHeap, &buf);
		    mzBloomInit((RoutePath *) (minAdjCostTop->he_id));
		    mzNumOutsideBlooms++;

		}
	    }
	    else if(minCostTop)
	    /* minAdjCostHeap empty, so bloom from minCostTop */
	    {
		if (DebugIsSet(mzDebugID, mzDebMaze))
		{
		    TxPrintf("INITIALIZING BLOOM STACK ");
		    TxPrintf("WITH TOP OF MIN COST HEAP.\n");
		}
		minCostTop = HeapRemoveTop(&mzMinCostHeap, &buf);
		mzBloomInit((RoutePath *) (minCostTop->he_id));
   	    }
	    else if(minAdjCostTop)
	    /* minCost Heap empty, so bloom from minAdjCostTop */
	    {
		if (DebugIsSet(mzDebugID, mzDebMaze))
		{
		    TxPrintf("INITIALIZING BLOOM STACK ");
		    TxPrintf("WITH TOP OF MIN ADJCOST HEAP.\n");
		}
		minAdjCostTop = HeapRemoveTop(&mzMinAdjCostHeap, &buf);
		mzBloomInit((RoutePath *) (minAdjCostTop->he_id));
		mzNumOutsideBlooms++;
	    }
	    else
	    /* minCost and minAdjCost heaps empty, 
	     * bloom from top of TOGO heap */
	    {
		if(maxToGoTop)
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("INITIALIZING BLOOM STACK ");
			TxPrintf("WITH TOP OF MAX TOGO HEAP.\n");
		    }
		    maxToGoTop = HeapRemoveTop(&mzMaxToGoHeap, &buf);
		    mzBloomInit((RoutePath *) (maxToGoTop->he_id));
		    mzNumOutsideBlooms++;
		}
		else
		/* No paths left to extend from */
		{
		    if (DebugIsSet(mzDebugID, mzDebMaze))
		    {
			TxPrintf("NO PATHS LEFT TO EXTEND FROM.\n");
		    }
		    morePartialPaths = FALSE;
		}
	    }
	}
    }

    /* Give final stat report. */
    mzMakeStatReport();

    /* Return best complete path */
    {
	HeapEntry heEntry;

	if(HeapRemoveTop(&mzMinCostCompleteHeap,&heEntry))
	{
	    return (RoutePath *)(heEntry.he_id);
	}
	else
	{
	    return NULL;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendPath --
 *
 * Spread out to next interesting point in four directions, and to adjacent
 * layers through contacts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds new routepaths to the queues (ie. heaps and bloomstack).
 *
 * ----------------------------------------------------------------------------
 */

Void
mzExtendPath(path)
    register RoutePath *path;
{
    register int extendCode = path->rp_extendCode;

    if (extendCode & EC_RIGHT)
    {
	mzExtendRight(path);
    }

    if (extendCode & EC_LEFT)
    {
	mzExtendLeft(path);
    }

    if (extendCode & EC_UP)
    {
	mzExtendUp(path);
    }

    if (extendCode & EC_DOWN)
    {
	mzExtendDown(path);
    }

    if (extendCode & EC_CONTACTS)
    {
	mzExtendViaContacts(path);
    }

    if (extendCode >= EC_WALKRIGHT)
    {
	if (extendCode & EC_WALKRIGHT)
	{
	    mzWalkRight(path);
	}
	else if (extendCode & EC_WALKLEFT)
	{
	    mzWalkLeft(path);
	}

	else if (extendCode & EC_WALKUP)
	{
	    mzWalkUp(path);
	}

	else if (extendCode & EC_WALKDOWN)
	{
	    mzWalkDown(path);
	}
	else if (extendCode & EC_WALKCONTACT)
	{
	    mzWalkContact(path);
	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendViaContacts --
 *
 * Spread from a point to other planes reachable from it,
 * by using contacts.  Stacked contacts are not allowed.
 *
 * Results	None.
 *
 * Side effects:
 *	Adds RoutePaths to the heap (mzReservePathCostHeap).
 *
 * ----------------------------------------------------------------------------
 */
Void
mzExtendViaContacts(path)
    RoutePath *path;
{
    register Point *p = &path->rp_entry;
    register RouteLayer *rLayer = path->rp_rLayer; 
    register List *cL;
    RouteLayer *newRLayer;

    /* DEBUG - trace calls to this routine */
    if (DebugIsSet(mzDebugID, mzDebMaze))
	TxPrintf("EXTENDING WITH CONTACTS\n");

    /* Loop through contacts that connect to current rLayer */
    for (cL=rLayer->rl_contactL; cL!=NULL; cL=LIST_TAIL(cL))
    {
	RouteContact *rC = (RouteContact *) LIST_FIRST(cL);

	/* Don't use inactive contacts */
	if (!(rC->rc_routeType.rt_active))
	    continue;

	/* Get "other" route Layer contact connects to */
	if (rC->rc_rLayer1 == rLayer)
	{
	    newRLayer = rC->rc_rLayer2;
	}
	else
	{
	    ASSERT(rC->rc_rLayer2 == rLayer,
	    "mzExtendViaContacts");
	    newRLayer = rC->rc_rLayer1;
	}

	/* Don't spread to inactive layers */
	if (!(newRLayer->rl_routeType.rt_active))
	    continue;

	/* Find tile on contact plane that contains point. */
	{
	    Tile *tp = TiSrPointNoHint(rC->rc_routeType.rt_hBlock,p);

	    /* if blocked, don't place a contact */
	    if (TiGetType(tp) != TT_SPACE)
	    {
		continue;
	    }
	}

	/* OK to drop a contact here */
	{
	    DoubleInt conCost;
	    int extendCode;

	    /* set contact cost */
	    {
		int i = rC->rc_cost;
		DOUBLE_CREATE(conCost, i);
	    }

	    /* determine appropriate extendcode */
	    {
		Tile *tp = TiSrPointNoHint(newRLayer->rl_routeType.rt_hBlock,
					   p);
		TileType type = TiGetType(tp);

		switch (type)
		{
		    case TT_SPACE:
		    {
			extendCode = EC_RIGHT | EC_LEFT | EC_UP | EC_DOWN;
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
			/* skip this contact after all, to avoid stacked
			 * contacts.
			 */
			continue;
		    }
		    break;

		    case TT_DEST_AREA:
		    {
			extendCode = EC_COMPLETE;
		    }
		    break;

		    default:
		    {
			/* this shouldn't happen */
			ASSERT(FALSE,"mzExtendViaContacts");
		    }
		}
	    }

	    /* Finally add point after contact */
	    mzAddPoint(path, 
		       p, 
		       newRLayer, 
		       'O', 
		       extendCode,
		       conCost);
	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzAddPoint --
 *
 * Process interesting point.  If point within bounds and not redundant, 
 * link to previous path, update cost, and add to heap.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds a RoutePath to the heap.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzAddPoint(path, p, rLayer, orient, extendCode, cost)
    RoutePath *path;	/* path that new point extends */
    Point *p;		/* new point */
    RouteLayer *rLayer;	/* Route Layer of new point */
    int orient;	/* 'H' = endpt of hor seg, 'V' = endpt of vert seg, 
			 * 'O' = contact, 'B' = first point in path and blocked
			 */
    int extendCode;	/* interesting directions to extend in */
    DoubleInt cost;	/* Incremental cost of new path segment */

{
    register RoutePath *newPath;
    RoutePath *hashedPath;
    register HashEntry *he;
    PointKey pk;
    DoubleInt togo;

    /* DEBUG - print candidate frontier point */
    if (DebugIsSet(mzDebugID, mzDebMaze))
    {
	TxPrintf(
	    "mzAddPoint called:  point=(%d,%d), layer=%s, orient='%c'\n",
	    p->p_x,
	    p->p_y,
	    DBTypeLongNameTbl[rLayer->rl_routeType.rt_tileType],
	    orient);
    }

    ASSERT(DOUBLE_GE(cost,DIZero),"mzAddPoint");

    /* Ignore if outside of bounding box */
    if (!GEO_ENCLOSE(p, &mzBoundingRect))
	return;

    /* get estimated distance togo */
    if(extendCode == EC_COMPLETE)
    {
	togo = DIZero;
    }
    else
    {
	togo = mzEstimatedCost(p);
    }

    /* compute (total) cost for new path */
    {
	/* initially cost = cost of new leg in path */

	/* Add jogcost if appropriate */
	if(path!=NULL && 
	   path->rp_rLayer == rLayer &&
	   path->rp_orient != 'O' &&
	   path->rp_orient != orient)
	{
	    DOUBLE_ADDI(cost, cost, rLayer->rl_jogCost);
	}

	/* Add estimated total cost prior to new point, 
	 * (or cost so far in the case of initial paths).
	 */
	if(path!=NULL)
	{
	    DOUBLE_ADD(cost, cost, path->rp_cost);
	}
	/* If not initial path, subtract out old estimated cost togo */
	if(mzPathSource!=SOURCE_INIT)
	{
	    DOUBLE_SUB(cost, cost, path->rp_togo);
	}

	/* Add new estimated cost to completion */
	DOUBLE_ADD(cost, cost, togo);
    }

    /* Check hash table to see if cheaper path to this point already
     * found - if so don't add this point.
     */
    pk.pk_point = *p;
    pk.pk_rLayer = rLayer;
    pk.pk_orient = orient;
    he = HashFind(&mzPointHash, (ClientData) &pk);
    hashedPath = (RoutePath *) HashGetValue(he);

    if (hashedPath != NULL
	&& DOUBLE_LE(hashedPath->rp_cost, cost))
    {
	if (DebugIsSet(mzDebugID, mzDebMaze))
	{
	    TxPrintf("new point NOT added, at least as good path to pt already exists:  ");
	    TxPrintf("new cost = %.0f, ",
		     DoubleToDFloat(cost));
	    TxPrintf("cheapest cost = %.0f\n",
		     DoubleToDFloat(hashedPath->rp_cost));
	}
	return;
    }

    /* If initial path, update min initial cost */
    if(mzPathSource==SOURCE_INIT)
    {
	if(DOUBLE_LESS(cost,mzMinInitialCost))
	{
	    mzMinInitialCost = cost;
	}
    }

    /* Create new path element */
    newPath = NEWPATH();
    newPath->rp_rLayer = rLayer;
    newPath->rp_entry = *p;
    newPath->rp_orient = orient;
    newPath->rp_cost = cost;
    newPath->rp_extendCode = extendCode;
    newPath->rp_togo = togo;
    newPath->rp_back = path;

    /* keep statistics */
    mzNumPathsGened++;

    /* Enter in hash table */
    HashSetValue(he, (ClientData) newPath);

    /* Add to appropriate queue or stack */
    if(extendCode == EC_COMPLETE)
    {
	if (DebugIsSet(mzDebugID, mzDebMaze))
	{
	    TxPrintf("PATH COMPLETE (WALKED IN).  Add to complete heap.\n");
	}

	HeapAdd(&mzMinCostCompleteHeap, 
		  &(newPath->rp_cost), 
		  (char *) newPath);

	/* compute stats and make completed path report */
	{		    
	    mzNumComplete++;
	    
	    if(mzVerbosity>=VERB_STATS)
	    {
		DoubleInt cost, excessCost;
		double excessPercent;

		mzMakeStatReport();

		TxPrintf("PATH #%d  ", mzNumComplete);
		

		cost = newPath->rp_cost;
		TxPrintf("cst:%.0f, ",
			 DoubleToDFloat(newPath->rp_cost));
		if(DOUBLE_LESS(cost, mzInitialEstimate))
		{
		    TxPrintf("(<est)");
		}
		else
		{
		    DOUBLE_SUB(excessCost, cost, mzInitialEstimate);
		    excessPercent = 100.0 *
		    DoubleToDFloat(excessCost)/
		    DoubleToDFloat(mzInitialEstimate);
		    
		    TxPrintf("overrun: %.0f%%",
			     excessPercent);
		}

		TxPrintf("\n");
	    }
	}
    }
    else if(extendCode >= EC_WALKRIGHT)
    {
	LIST_ADD(newPath, mzWalkStack);
    }
    else
    {
	switch(mzPathSource)
	{
	    case SOURCE_BLOOM:
	    if(orient=='O')
	    {
		/* just changing layers, add back to bloom */
		LIST_ADD(newPath, mzBloomStack);
	    }
	    else if((orient=='H' && rLayer->rl_hCost<=rLayer->rl_vCost) ||
		    (orient=='V' && rLayer->rl_vCost<=rLayer->rl_hCost))
	    {
		/* going in preferred direction */
		LIST_ADD(newPath, mzStraightStack);
	    }
	    else
	    {
		/* non preferred, add to heaps */
		HeapAdd(&mzMaxToGoHeap, &togo, (char *) newPath);
	    }
	    break;

	    case SOURCE_STRAIGHT:
	    if(path->rp_orient==orient && DOUBLE_LE(cost,mzBloomMaxCost))
	    {
		/* straight and within bounds, keep going*/
		LIST_ADD(newPath, mzStraightStack);
	    }
	    else
	    {
		/* from here on, follow downhill only */
		LIST_ADD(newPath, mzDownHillStack);
	    }
	    break;

	    case SOURCE_DOWNHILL:
	    {
		DoubleInt oldCostPlusOne;

		DOUBLE_ADDI(oldCostPlusOne, path->rp_cost, 1);
		if(DOUBLE_LESS(cost, oldCostPlusOne))
		{
		    /* cost within a unit, keep following down hill */
		    LIST_ADD(newPath, mzDownHillStack);
		}
		else
		{
		    /* increased cost, add to heaps */
		    HeapAdd(&mzMaxToGoHeap, &togo, (char *) newPath);
		}
	    }
	    break;

	    case SOURCE_INIT:
	    /* Initial points go on heap */
	    HeapAdd(&mzMaxToGoHeap, &togo, (char *) newPath);
	    break;
	}
    }
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzBloomInit --
 *
 * Initialize bloom stack with given path.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds a routepath to bloom stack, and sets mzBloomMaxCost.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzBloomInit(path)
    RoutePath *path;	/* path that new point extends */
{
    ASSERT(mzBloomStack==NULL,"mzBloomInit");

    LIST_ADD(path, mzBloomStack);
    DOUBLE_ADD(mzBloomMaxCost, path->rp_cost, mzBloomDeltaCost);

    mzNumBlooms++;

    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * mzMakeStatReport --
 *
 * Print out route statistics
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output via TxPrintf()
 *
 * ----------------------------------------------------------------------------
 */

Void
mzMakeStatReport()
{
    
    /* if we aren't being verbose, skip this */
    if(mzVerbosity<VERB_STATS) 	return;

    TxPrintf("  Blms:%d(%d)",
	     mzNumBlooms - mzNumOutsideBlooms,
	     mzNumBlooms);

    TxPrintf("  Wndw:%.0f(%.0f%%)",
	     DoubleToDFloat(mzWindowMaxToGo),
	     (100.0*
	      (1- DoubleToDFloat(mzWindowMaxToGo)/
	       (DoubleToDFloat(mzInitialEstimate) +
		DoubleToDFloat(mzWWidth)))));

    TxPrintf("  Pts:%d(%d)",
	     mzNumPaths,
	     mzNumPathsGened);

    TxPrintf("  Blkgen: %dx%.0f",
	     mzBlockGenCalls,
	     mzBlockGenArea/mzBlockGenCalls);

    TxPrintf("(%.0f/icst)",
	     mzBlockGenArea/DoubleToDFloat(mzInitialEstimate));
    
    TxPrintf("\n");
    return;
}

