/*
 * gaMaze.c -
 *
 * Code to interface to mzrouter for harder stems.
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
static char rcsid[] = "$Header: gaMaze.c,v 6.0 90/08/28 18:52:22 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "doubleint.h"
#include "heap.h"
#include "tile.h"
#include "database.h"
#include "windows.h"
#include "main.h"
#include "dbwind.h"
#include "signals.h"
#include "netmenu.h"
#include "gcr.h"
#include "router.h"
#include "grouter.h"
#include "garouter.h"
#include "gaInternal.h"
#include "netlist.h"
#include "textio.h"
#include "styles.h"
#include "malloc.h"
#include "drc.h"
#include "debug.h"
#include "list.h"
#include "../mzrouter/mzrouter.h"

/*-------------- Global data only referenced within this file ---------------*/
/* Parameter settings passed to mzrouter (initialized when first needed.) */
MazeParameters *gaMazeParms; 
/* Top Level Cell used by mzrouter during stem generation.
 * Contains a fence, to limit the scope of the search, 
 * and the edit cell as a subcell */
CellUse *gaMazeTopUse;
CellDef *gaMazeTopDef;
CellUse *gaMazeTopSub;


/*
 * ----------------------------------------------------------------------------
 *
 * gaMazeInit --
 *
 * Initialize ga maze routing code.

 * Results:
 *	TRUE if successful, FALSE if unsuccessful.
 *
 * Side effects:
 *	sets up gaMazeTop cell.
 *
 * ----------------------------------------------------------------------------
 */

bool
gaMazeInit(routeUse)
    CellUse *routeUse;		/* Cell routing will be done in - made subcell
				 * of gaMazeTop
				 */ 
{
    UndoDisable();

    /* setup maze parms - if necessary */
    if(!gaMazeParms)
    {
	bool gaMazeInitParms();

	if(gaMazeInitParms()==FALSE)
	{
	    return FALSE;
	}
    }

    /* allocate gaMazeTop - if necessary */
    if(!gaMazeTopUse)
    {
	DBNewYank("__GAMAZETOP", &gaMazeTopUse, &gaMazeTopDef);
    }

    /* unlink old subcell - if any */
    if(gaMazeTopSub)
    {
	DBUnLinkCell(gaMazeTopSub, gaMazeTopDef);
	DBDeleteCell(gaMazeTopSub);
        DBCellDeleteUse(gaMazeTopSub);
    }

    /* make routeUse a subcell of gaMazeTop (using identity transform) */
    {
	gaMazeTopSub = DBCellNewUse(routeUse->cu_def, "__MAZE_TOP_SUB");
	DBPlaceCell(gaMazeTopSub, gaMazeTopDef);
    }
    
    UndoEnable();
    return TRUE;
}        


/*
 * ----------------------------------------------------------------------------
 *
 * gaMazeInitParms -- Initialize ga maze routing parameters.
 *
 * Called by gaMazeInit the first time it is invoked.

 * Results:
 *	TRUE if successful, FALSE if unsuccessful
 *
 * Side effects:
 *	gaMazeParms setup.
 *
 * ----------------------------------------------------------------------------
 */
bool
gaMazeInitParms()
{

    /* Initialize to copy of default garouter  parameters */
    gaMazeParms = MZCopyParms(MZFindStyle("garouter"));

    if(gaMazeParms == NULL)
    {
	TxError("No garouter style in mzrouter section of technology file.\n");
	return FALSE;
    }

    /* optimize for this application */
    gaMazeParms->mp_expandDests = TRUE;
    gaMazeParms->mp_topHintsOnly = TRUE;
    gaMazeParms->mp_bloomLimit = MAZE_TIMEOUT;

    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * gaMazeRoute --
 *
 * Try to maze route from pinPoint to terminalLoc 
 * ending up on a layer in `pinLayerMask' while constraining
 * all wiring to stay within boundingRect. 
 *
 * Uses the Magic maze router (mzrouter module).
 *
 * Results:
 *	Returns TRUE if a route is possible, FALSE if not.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
gaMazeRoute(routeUse, terminalLoc, pinPoint, pinLayerMask, side, writeFlag)
    CellUse *routeUse;		/* Cell to route in - top level cell visible
				 * to router and also cell route is painted
				 * into.  All subcells are treated as expanded
				 * by the router, i.e. their contents are
				 * examined.  And routing across the tops of
				 * subcells is permitted.
				 */ 
    NLTermLoc *terminalLoc;	/* Terminal to connect to - somewhere in
				 * interior of cell */
    Point *pinPoint;		/* Point to connect from (on edge of cell) */
    TileTypeBitMask pinLayerMask;	/* layer at pin (ONLY ONE BIT SHOULD
					 * BE SET)
					 */
    int side;			/* side of cell destPoint lies on */
    bool writeFlag;	        /* If non-null, paint back result (into
				 * routeUse), otherwise just check if 
				 * route is possible.
				 */
{
    Rect routeBounds;
    bool done = FALSE;

    /* setup bounding box */
    {
	/* compute route bounds */
	gaMazeBounds(terminalLoc, pinPoint, &routeBounds);

	/* Enforce route bounds with fence */
	UndoDisable();
	DBPaint(gaMazeTopDef, &routeBounds, TT_FENCE);
	DBReComputeBbox(gaMazeTopDef);
	UndoEnable();

	/* Set bounds hint to fence - improves mzrouter performance */
	gaMazeParms->mp_boundsHint = &routeBounds;
    }

    /* Initialize Maze Route */
    {
	MZInitRoute(gaMazeParms, gaMazeTopUse, /* all subcells visible */ 0);
    }

    /* set maze router start point to pinPoint */
    {
	TileType type;

	/* determine starting type from mask */
	{
	    RouteLayer *rL;

	    /* find route layer corresponding to mask */
	    for(rL = gaMazeParms->mp_rLayers; rL!=NULL; rL=rL->rl_next)
	    {
		if(TTMaskHasType(&pinLayerMask,rL->rl_routeType.rt_tileType))
		{
		    break;
		}
	    }
	
	    /* make sure we found a routelayer */
	    if (rL == NULL)
	    {
		TxError("gaMaze.c:  no routetypes in destLayerMask\n");
		goto abort;
	    }

	    /* set type to tiletype of routelayer */
	    type = rL->rl_routeType.rt_tileType;
	}

	/* give the mzrouter the  start point and type */
	MZAddStart(pinPoint, type);
    }

    /* set maze router dest area to terminal loc and layer*/
    {
	MZAddDest(&(terminalLoc->nloc_rect),
		  terminalLoc->nloc_label->lab_type);
    }

    /* Do the Route */
    {
	RoutePath *path;

	/* search for path */
	path = MZRoute(); 
	if(SigInterruptPending) goto abort;

	/* If no path found, abort */
	if(path == NULL)
	{
	    goto abort;
	}

	/* if write flag set, paint back route path */
	if (writeFlag) 
	{
	    CellUse *resultUse;

	    /* Have MazeRouter paint the path into a cell*/
	    resultUse = MZPaintPath(path);
	    if(SigInterruptPending) goto abort;

	    /* Copy path to route cell */
	    {
		SearchContext scx;

		scx.scx_use = resultUse;
		scx.scx_area = resultUse->cu_def->cd_bbox;
		scx.scx_trans = GeoIdentityTransform;
		(void) DBCellCopyPaint(&scx, &DBAllButSpaceBits, 0, routeUse);
		DBReComputeBbox(routeUse->cu_def);
	    }

	    /* Notify dbwind module (for redisplay), and DRC module 
	     * of changed area */
	    {
		Rect changedArea;
		changedArea= routeUse->cu_def->cd_bbox;

		DBWAreaChanged(routeUse->cu_def, &changedArea, DBW_ALLWINDOWS,
			       &DBAllButSpaceBits);
		DRCCheckThis(routeUse->cu_def, TT_CHECKPAINT, &changedArea);
	    }
	}
    }

    /* Make sure we got here without interruption */
    if(SigInterruptPending) goto abort;

    /* We are done */
    done = TRUE;

abort:
    /* cleanup and return */
    UndoDisable();
    DBErase(gaMazeTopDef, &routeBounds, TT_FENCE);
    UndoEnable();

    /* cleanup after the mzrouter */
    if(!DebugIsSet(gaDebugID, gaDebNoClean))
    {
	MZClean();
    }

    return done;
}


/*
 * ----------------------------------------------------------------------------
 *
 * gaMazeBounds --
 *
 * Pick a rectangular area for mazerouting containing terminalLoc and 
 * enough extra slop to make sure connection and contacts are possible
 * at the end points.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets *r to the area described above.
 *
 * ----------------------------------------------------------------------------
 */

Void
gaMazeBounds(terminalLoc, pinPoint, r)
    register NLTermLoc *terminalLoc;	/* Terminal */
    register Point *pinPoint;	/* Grid point */
    register Rect *r;		/* Set to bounding area for maze route */
{

    /* set containing rectangle */
    r->r_xbot = MIN(terminalLoc->nloc_rect.r_xbot, pinPoint->p_x);
    r->r_ybot = MIN(terminalLoc->nloc_rect.r_ybot, pinPoint->p_y);
    r->r_xtop = MAX(terminalLoc->nloc_rect.r_xtop, pinPoint->p_x);
    r->r_ytop = MAX(terminalLoc->nloc_rect.r_ytop, pinPoint->p_y);

    /* Adjust for width of routes */
    {
	int width = 0;

	/* compute max active width */
	{
	    RouteType *rT;
	 
	    for(rT=gaMazeParms->mp_rTypes; rT!=NULL; rT=rT->rt_next)
	    {
		if(rT->rt_active)
		{
		    width = MAX(width, rT->rt_width);
		}
	    }
	}

	/* grow to top and right by max active width */
	{
	    r->r_xtop += width;
	    r->r_ytop += width;
	}
    }

    return;
}
