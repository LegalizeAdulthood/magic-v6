/*
 * mzMain.c --
 *
 * Global Data Definitions and interface procedures for the Maze Router.
 * 
 * OTHER ENTRY POINTS (not in this file):
 *    Technology readin - mzTech.c
 *    Initialization (after tech readin) - mzInit.c  
 *    Test command interface - TestCmd.c
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
static char rcsid[] = "$Header: mzMain.c,v 6.1 90/08/28 19:35:20 mayo Exp $";
#endif  not lint

/*--- includes --- */
#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"
#include "select.h"
#include "signals.h"
#include "textio.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "debug.h"
#include "undo.h"
#include "txcommands.h"
#include "malloc.h"
#include "main.h"
#include "geofast.h"
#include "../utils/list.h"
#include "doubleint.h"
#include "heap.h"
#include "touchingtypes.h"
#include "mzrouter.h"
#include "mzInternal.h"

/*---- Global Data Definitions ----*/
/* (Actual storage for static global structures for maze router defined here)*/

/* Debug flags */
ClientData mzDebugID;
int mzDebMaze;		/* identify flags to debug module */
int mzDebStep;

/* parameter sets - from tech file */
MazeStyle *mzStyles;

/* Toplevel cell visible to the router */
CellUse *mzRouteUse;	

/* Route types */
/* (Specifies what types are permitted during routing, and related design
 * rules.)
 */
RouteType *mzRouteTypes;
RouteType *mzActiveRTs;		/* Only route types that are turned on */
RouteLayer *mzRouteLayers;
RouteLayer *mzActiveRLs;	/* Only route layers that are turned on */
RouteContact *mzRouteContacts;

/* Routing is confined to this rectangle */
Rect mzBoundingRect;

/* Expansion mask - defines which subcells to treat as expanded */
int mzCellExpansionMask;

/* If reset, degenerate estimation plane used (just 4 tiles - one for each 
 * quadrant with respect to destination point).
 */
int mzEstimate;

/* If set dest areas are expanded to include all electrically 
 * connected geometry.
 */
int mzExpandDests;
/* If set only hints in toplevel cell are recognized */
int mzTopHintsOnly;

/* Maximum distance route will extend into blocked area to connect to dest.
 * terminal.  If set to -1, a max value is computed as a function of the
 * design rules for the active route types prior to each route.
 */
int mzMaxWalkLength;

/* if nonnull, limits area of search for performance.  
 * (NOTE: USER MUST LIMIT ROUTE TO THIS AREA WITH FENCES - OTHERWISE
 *        RESULT IS UNPREDICTABLE).
 */
Rect *mzBoundsHint;

/* how many messages to print */
int mzVerbosity;
/* if positive, upper bound on number of blooms */
int mzBloomLimit;

/* maskdata tiles, and unexpanded subcells, marked because they are part of 
 * dest. nodes. */
List *mzMarkedTilesList;
List *mzMarkedCellsList;

/* start terminals */
List *mzStartTerms;

/* internal cell for dest areas */
CellDef *mzDestAreasDef = (CellDef *) NULL;
CellUse *mzDestAreasUse = (CellUse *) NULL;

/* Fence parity */
bool mzInsideFence;

/* largest design rule distance - used during incremental blockage gen. */
int mzContextRadius; 

/* Internal cell for completed route */
CellDef *mzResultDef = (CellDef *) NULL;
CellUse *mzResultUse = (CellUse *) NULL;

/* HINT PLANES */
TileTypeBitMask mzHintTypesMask;
Plane *mzHHintPlane;
Plane *mzVHintPlane;

/* FENCE PLANE */
TileTypeBitMask mzFenceTypesMask;
Plane *mzHFencePlane;

/* ROTATE PLANES */
TileTypeBitMask mzRotateTypesMask;
Plane *mzHRotatePlane;
Plane *mzVRotatePlane;

/* BOUNDS PLANES */
PaintResultType mzBoundsPaintTbl[TT_MAXTYPES][TT_MAXTYPES];
Plane *mzHBoundsPlane;
Plane *mzVBoundsPlane;

/* BLOCKAGE PLANES */
PaintResultType mzBlockPaintTbl[TT_MAXTYPES][TT_MAXTYPES];

/* ESTIMATE PLANE */
PaintResultType mzEstimatePaintTbl[TT_MAXTYPES][TT_MAXTYPES];
Plane *mzEstimatePlane;

/* dest terminal alignment coordinates */
NumberLine mzXAlignNL;
NumberLine mzYAlignNL;

/* minimum radius of blockage plane info required around point being expanded.
 * (Areas twice this size are gened. whenever the minimum is not met.)
 */
int mzBoundsIncrement;

/* minimum estimated total cost for initial path */
DoubleInt mzMinInitialCost;

/* where current path came from */
int mzPathSource;

/* Parameters controlling search */
RouteFloat mzPenalty;
DoubleInt mzWRate;
DoubleInt mzBloomDeltaCost;
DoubleInt mzWWidth;

/* Statistics */
DoubleInt mzInitialEstimate;  /* Initial estimated cost of route */
int mzNumBlooms;
int mzNumOutsideBlooms;	/* num blooms from outside window */
int mzNumComplete;	/* number of complete paths so far */
int mzBlockGenCalls;	/* # of calls to blockage gen. code */
double mzBlockGenArea;   /* area over which blockage planes 
			    have been gened. */
int mzNumPathsGened;	/* number of partial paths added to heap */
int mzNumPaths;		/* number of paths processed */
int mzReportInterval;    /* frequency that # of paths etc. 
				 * is reported. */
int mzPathsTilReport;	/* counts down to next path report */

/* Variables controling search */
DoubleInt mzWInitialMinToGo;
DoubleInt mzWInitialMaxToGo;
DoubleInt mzBloomMaxCost;	

/* Search status */
DoubleInt mzWindowMinToGo; /* Window location */
DoubleInt mzWindowMaxToGo;

/* Hash table to avoid repeated expansion from same point during search */
HashTable mzPointHash;

/* Queues for partial paths */
Heap mzMaxToGoHeap;		/* paths nearer destination than WINDOW */
Heap mzMinCostHeap;		/* paths in WINDOW */
Heap mzMinAdjCostHeap;	 	/* paths farther from dest than WINDOW*/
List *mzBloomStack;		/* paths in current local focus */
List *mzStraightStack;		/* focus paths expanded in a straight line */
List *mzDownHillStack;		/* focus paths expanded as long as
				 * estimated total cost doesn't increase.
				 */
List *mzWalkStack;		/* paths in walks, i.e. blocks adjacent
				 * to dest areas.
				 */
Heap mzMinCostCompleteHeap;	/* completed paths */

/*----------- static data - referenced only in this file ------------------- */

/* set when storage that needs to be reclaimed has been allocated by router */
bool mzDirty = FALSE;

/* set when path queues and hast table have been allocated */
bool mzPathsDirty = FALSE;

/* macro for adding address pairs to translation table */
#define AT_EQUIV(a1,a2) \
if(TRUE) \
{ \
   HashSetValue(HashFind(&aT, (ClientData) (a1)), (ClientData) (a2)); \
   HashSetValue(HashFind(&aT, (ClientData) (a2)), (ClientData) (a1)); \
} else

/* macro for translating address to address paired with it in address table */
#define AT(type,a) \
if (TRUE) \
{ \
  HashEntry *he = HashLookOnly(&aT, (ClientData) (a)); \
  if(he) \
  { \
      a = (type) HashGetValue(he); \
  } \
} else


/*
 * ----------------------------------------------------------------------------
 *
 * MZCopyParms --
 *
 * Allocate and setup duplicate maze parameters with same values.
 * (Duplicates substructures as well)
 *
 * Results:
 *	Pointer to new parameters.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */
MazeParameters *
MZCopyParms(oldParms)
    MazeParameters *oldParms;	/* Maze routing parameters */
{    
    MazeParameters *newParms;
    HashTable aT;	/* Address translation hash table */

    /* If passed NULL parms, just return NULL */
    if(oldParms==NULL)
    {
	return NULL;
    }
	
    /* Initialize address translation table */
    HashInit(&aT, 1000, HT_WORDKEYS);

    /* allocate new structure and copy MazeParameters */
    {
	MALLOC(MazeParameters *, newParms, sizeof(MazeParameters));
	*newParms = *oldParms;
    }

    /* Copy RouteLayers (and sub-structures) */
    {
	RouteLayer *rLOld;

	for(rLOld = oldParms->mp_rLayers; 
	    rLOld != NULL; 
	    rLOld = rLOld->rl_next)
	{
	    RouteLayer *rLNew;
	    
	    /* allocate and equivalence new rL and its rT */
	    {
		MALLOC(RouteLayer *, rLNew, sizeof(RouteLayer));
		AT_EQUIV(rLOld, rLNew);
		AT_EQUIV(&(rLOld->rl_routeType),&(rLNew->rl_routeType));
	    }

	    /* copy the rL */
	    *rLNew = *rLOld;

	    /* make a copy of the routeLayer contact list */
	    LIST_COPY(rLOld->rl_contactL, rLNew->rl_contactL);

	    /* allocate new blockage planes */
	    rLNew->rl_routeType.rt_hBlock = DBNewPlane((ClientData) TT_SPACE);
	    rLNew->rl_routeType.rt_vBlock = DBNewPlane((ClientData) TT_SPACE);
	}
    }

    /* Copy RouteContacts (and sub-structures) */
    {
	RouteContact *rCOld;

	for(rCOld = oldParms->mp_rContacts; 
	    rCOld != NULL; 
	    rCOld = rCOld->rc_next)
	{
	    RouteContact *rCNew;
	    
	    /* allocate and equivalence new rC and its rT */
	    {
		MALLOC(RouteContact *, rCNew, sizeof(RouteContact));
		AT_EQUIV(rCOld, rCNew);
		AT_EQUIV(&(rCOld->rc_routeType),&(rCNew->rc_routeType));
	    }

	    /* copy the rC */
	    *rCNew = *rCOld;

	    /* allocate new blockage planes */
	    rCNew->rc_routeType.rt_hBlock = DBNewPlane((ClientData) TT_SPACE);
	    rCNew->rc_routeType.rt_vBlock = DBNewPlane((ClientData) TT_SPACE);
	}
    }

    /* Translate addresses in MazeParameters */
    AT(RouteLayer *, newParms->mp_rLayers);
    AT(RouteContact *, newParms->mp_rContacts);
    AT(RouteType *, newParms->mp_rTypes);

    /* Translate addresses in RouteLayers (and sub-structures) */
    {
	RouteLayer *rLOld;

	for(rLOld = oldParms->mp_rLayers; 
	    rLOld != NULL; 
	    rLOld = rLOld->rl_next)
	{
	    RouteLayer *rLNew = rLOld;
	    AT(RouteLayer *, rLNew);

	    AT(RouteLayer *, rLNew->rl_next);
	    AT(RouteType *, rLNew->rl_routeType.rt_next);
	    
	    /* translate RouteContact addresses in contact list */
	    {
		List *l;
		for(l = rLNew->rl_contactL; l!=NULL; l=LIST_TAIL(l))
		{
		    AT(ClientData, LIST_FIRST(l));
		}
	    }

	}
    }

    /* Translate addresses in RouteContacts (and sub-structures) */
    {
	RouteContact *rCOld;

	for(rCOld = oldParms->mp_rContacts; 
	    rCOld != NULL; 
	    rCOld = rCOld->rc_next)
	{
	    RouteContact *rCNew =rCOld;
	    AT(RouteContact *, rCNew);

	    AT(RouteLayer *, rCNew->rc_rLayer1);
	    AT(RouteLayer *, rCNew->rc_rLayer2);
	    AT(RouteContact *, rCNew->rc_next);
	    AT(RouteType *, rCNew->rc_routeType.rt_next);
	}
    }
	    
    return newParms;
}


/*
 * ----------------------------------------------------------------------------
 *
 * MZFindStyle --
 *
 * Find style of given name.
 *
 * Results:
 *	Pointer to maze parameters for given style, or NULL if not found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
MazeParameters *
MZFindStyle(name)
char *name;	/* name of style we are looking for */
{    
    MazeStyle *style = mzStyles;
    
    while(style!=NULL && strcmp(name,style->ms_name)!=0)
    {
	style = style->ms_next;
    }

    if(style==NULL)
    {
	return NULL;
    }
    else
    {
	return &(style->ms_parms);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MZInitRoute --
 *
 *
 * Set up datastructures for maze route, and initialize per/route statistics
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * NOTE: RouteUse supplied as parm to MZInitRoute is toplevel cell visible
 * to router.  However resulting route is painted to current edit cell. 
 *
 * ----------------------------------------------------------------------------
 */
Void
MZInitRoute(parms, routeUse, expansionMask)
    MazeParameters *parms;	/* Maze routing parameters */
    CellUse *routeUse;		/* toplevel cell router sees */
    int expansionMask;		/* which subcells are expanded - NOTE: the
				 * maze router interpets a 0 mask to mean
				 * all cells are expanded
				 */
{    
    /* Disable undo to avoid overhead on paint operations to internal planes */
    UndoDisable();

    /* Clean up after last route - if necessary */
    if(mzDirty)
    {
	MZClean();
    }

    /* Set dirty flag - since we are about to alloc storage for this route */
    mzDirty = TRUE;

    /* initial source of paths is initialization routine */
    mzPathSource = SOURCE_INIT;

    /* initial estimated cost is infinity */
    mzMinInitialCost = DIMaxInt;

    /* initialize per-route statistics */
    mzBlockGenCalls = 0;
    mzBlockGenArea = 0.0;
    mzNumComplete = 0;
    mzNumPathsGened = 0;
    mzNumPaths = 0;
    mzNumBlooms = 0;
    mzNumOutsideBlooms = 0;
    mzPathsTilReport = mzReportInterval;

    /* Make supplied parms current */
    {
	mzRouteLayers = parms->mp_rLayers;
	mzRouteContacts = parms->mp_rContacts;
	mzRouteTypes = parms->mp_rTypes;

	mzPenalty = parms->mp_penalty;
	mzWWidth = parms->mp_wWidth;
	mzWRate = parms->mp_wRate;
	mzBloomDeltaCost = parms->mp_bloomDeltaCost;

	mzBoundsIncrement = parms->mp_boundsIncrement;
	mzEstimate = parms->mp_estimate;
	mzExpandDests = parms->mp_expandDests;
	mzTopHintsOnly = parms->mp_topHintsOnly;

	mzMaxWalkLength = parms->mp_maxWalkLength;
	mzBoundsHint = parms->mp_boundsHint;
	mzVerbosity = parms->mp_verbosity;
	mzBloomLimit = parms->mp_bloomLimit;
    }

    /* Some parms are computed from the supplied ones */
    mzComputeDerivedParms();

    /* set route cell (toplevel cell visible during routing */
    mzRouteUse = routeUse;

    /* set expansion mask */
    mzCellExpansionMask = expansionMask; 

    /* Build hint fence and rotate planes */
    mzBuildHFR(mzRouteUse, &mzBoundingRect);

    /* Initialize Blockage Planes */
    {
	RouteType *rT;

	/* Clear bounds planes = regions for which blockage 
           has been generated */
	DBClearPaintPlane(mzHBoundsPlane);
	DBClearPaintPlane(mzVBoundsPlane);

	/* Clear blockage planes */
	for (rT=mzRouteTypes; rT!=NULL; rT=rT->rt_next)
	{
	    DBClearPaintPlane(rT->rt_hBlock);
	    DBClearPaintPlane(rT->rt_vBlock);
	}
    }

    /* Initialize Dest Area Cell */
    DBCellClearDef(mzDestAreasUse->cu_def);
    /* take our hold off undo */
    UndoEnable();

    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * MZAddStart --
 *
 * Add a starting terminal for the maze router.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Builds mzStartTerms list.
 *
 * ----------------------------------------------------------------------------
 */

Void
MZAddStart(point, type)
    Point *point;
    TileType type;
{
    /* Disable undo to avoid overhead on paint operations to internal planes */
    UndoDisable();

    /* check fence parity */
    if(mzStartTerms == NULL)
    {
	/* This is first start terminal, set fence parity by it, i.e.
	 * whether route is inside or outside of fence
	 */
	Tile *tFencePlane = TiSrPointNoHint(mzHFencePlane, point);
	mzInsideFence = (TiGetType(tFencePlane) != TT_SPACE);

	/* If inside fence, clip mzBounds to fence bounding box
	 * to save processing.
	 */
	if(mzInsideFence)
	{
	    Rect r;

	    DBBoundPlane(mzHFencePlane, &r);
	    r.r_xbot -= 2*mzContextRadius;
	    r.r_ybot -= 2*mzContextRadius;
	    r.r_xtop += 2*mzContextRadius;
	    r.r_ytop += 2*mzContextRadius;
	    GEOCLIP(&mzBoundingRect, &r);
	}
    }
    else
    {
	/* not first start terminal, check for consistency with respect
	 * to fence parity.
	 */
	Tile *tFencePlane = TiSrPointNoHint(mzHFencePlane, point);
	int newInside = (TiGetType(tFencePlane) != TT_SPACE);

	if(newInside != mzInsideFence)
	{
	    TxPrintf("Start points on both sides of fence.  ");
	    TxPrintf("Arbitrarily choosing those %s fence.\n",
		     (mzInsideFence ? "inside" : "outside"));

	    return;
	}
    }

    /* mark tiles connected to start point */
    {
	Rect rect;

	/* build degenerate rect containing point to initiate the
	 * marking process;
	 */
	rect.r_ll = *point;
	rect.r_ur = *point;

	mzMarkConnectedTiles(&rect, 
			     type,
			     FALSE /* Don't make connected tiles dest areas! */
			     );
    }

    /* add point to start terminal list */
    {
	ColoredPoint *newTerm;

	MALLOC(ColoredPoint *, newTerm, sizeof(ColoredPoint));
	newTerm->cp_point = *point;
	newTerm->cp_type = type;
	LIST_ADD(newTerm, mzStartTerms);
    }

    /* Take our hold of undo */
    UndoEnable();

    /* and return */
    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * MZAddDest --
 *
 * Add a destination terminal.  
 * 
 * Results:
 *	none.
 *
 * Side effects:
 *      Paints dest area into mzDestAreasDef.
 *
 *	Marks mask data tiles connected to supplied dest area (rect and type
 *      passed to this func), also keeps list of marked tiles for cleanup. 
 *
 *      Tiles are marked with TRUE on the clientdata field.  The default 
 *      clientdata value of MINFINITY should be restored by the router 
 *      before it returns.
 *
 * ----------------------------------------------------------------------------
 */

Void
MZAddDest(rect, type)
    Rect *rect;
    TileType type;
{
    ColoredRect *dTerm;

    UndoDisable();

    /* Paint dest area into mzDestAreas cell */
    {
	DBPaint(mzDestAreasUse->cu_def, rect, type);
    }

    /* Mark all tiles connected to dest terminal. */ 
    mzMarkConnectedTiles(rect, 
			 type,
			 mzExpandDests /* if set, make connected tiles dest
					* areas. */
			 );

    UndoEnable();
    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * MZRoute --
 *
 * Do the route.
 * 
 * Results:
 *	Zero-width route path.  NOTE: route path is allocated from temporary
 *      storage that will be reused for next route.
 *
 * Side effects:
 *	
 *
 * ----------------------------------------------------------------------------
 */
RoutePath *
MZRoute()
{
    RoutePath *path;	/* handle for result of search */

    /* Disable undo to avoid overhead on paint operations to internal planes */
    UndoDisable();

    /* Clear result cell */
    DBCellClearDef(mzResultDef);

    /* Generate dest areas and walks in blockage planes.
     * (also adds alignment coords for dest areas to alignment structs.)
     */
    mzBuildDestAreaBlocks();

    /* Check that there is an unblocked destination */
    if(mzXAlignNL.nl_sizeUsed==2)
    /* No alignment marks, so no destination areas */
    {
	TxPrintf("No reachable destination area!\n");
	goto abort;
    }

    /* Build Estimate Plane. 
     * (allowing for end points in unexpanded subcells)
     */
    mzBuildEstimate(mzStartTerms);
    if (SigInterruptPending) goto abort;

    /* allocating queues and hashtable so set dirty flag */
    mzPathsDirty = TRUE;

    /*
     * Initialize queues (actually heaps and lists) for partial paths
     * Double Precision Integer keys used in cost keyed heaps
     * to avoid overflow.
     */
    HeapInitType(&mzMaxToGoHeap, INITHEAPSIZE, TRUE, FALSE, HE_DINT);
    HeapInitType(&mzMinCostHeap, INITHEAPSIZE, FALSE, FALSE, HE_DINT);
    HeapInitType(&mzMinAdjCostHeap, INITHEAPSIZE, FALSE, FALSE, HE_DINT);
    HeapInitType(&mzMinCostCompleteHeap, INITHEAPSIZE, FALSE, FALSE, HE_DINT);
    mzBloomStack = NULL;
    mzStraightStack = NULL;
    mzDownHillStack = NULL;
    mzWalkStack = NULL;

    /*
     * A hash table is used to hold all points reached,
     * so we can avoid redundant expansion.
     */
    HashInit(&mzPointHash, INITHASHSIZE, HashSize(sizeof (PointKey)));

    /* Build blockage planes at start points and create initial
     * partial paths 
     */
    {
	List *terms;
	
	/* set bloom threshold to zero, so that initial points are placed
	 * on Max ToGo heap.
	 */
	mzBloomMaxCost = DIZero;

	for(terms=mzStartTerms; 
	    terms!=NULL; 
	    terms=LIST_TAIL(terms))
	{
	    ColoredPoint *term = (ColoredPoint *) LIST_FIRST(terms);

	    /* Build blockage planes around start point */
	    mzExtendBlockBounds(&(term->cp_point));

	    /* Generate initial paths for start point */ 
	    mzStart(term);
	}
    }

    /* initialize search window */
    {
	List *terms;

	/* estimated total cost is min estimated cost for initial paths */
        mzInitialEstimate = mzMinInitialCost;

	mzWInitialMinToGo = mzInitialEstimate;
	DOUBLE_ADD(mzWInitialMaxToGo, mzWInitialMinToGo, mzWWidth);
    }

    /* Make sure we got here without interruption */
    if (SigInterruptPending) goto abort;

    /* Do the route */
    path = mzSearch();	/* On interruption mzSearch returns best complete path
			 * found prior to interruption 
			 */

    UndoEnable();
    return path;

    abort:
    UndoEnable();
    return NULL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * MZPaintPath --
 *
 * Given a RoutePath constructed by mzRoute, convert it to paint.
 * The input RoutePath specifies a sequence of points completely, so
 * each leg can be painted as we go.
 *
 * Results:
 *	Pointer to result cell containing painted path.
 *
 * Side effects:
 *	Paints into result cell.
 *
 * ----------------------------------------------------------------------------
 */
CellUse *
MZPaintPath(pathList)
    RoutePath *pathList;
{
    register RoutePath *path, *prev;

    /*
     * Each segment of the path contains no bends, so is
     * either horizontal, vertical, or a contact.
     */
    for (path = pathList; 
	 (prev = path->rp_back)!= NULL && !SigInterruptPending; 
	 path = prev)
    {
	RouteLayer *rL;
	Rect r;
	int t;

	/*
	 * Special handling for a contact if different planes.
	 * In this case, no x- or y- motion is allowed.
	 */
	if (path->rp_rLayer != prev->rp_rLayer)
	{
	    ASSERT(path->rp_entry.p_x == prev->rp_entry.p_x, "MZPaintPath");
	    ASSERT(path->rp_entry.p_y == prev->rp_entry.p_y, "MZPaintPath");
	    mzPaintContact(path, prev);
	    continue;
	}

	/*
	 * Leg on the same plane.
	 * Generate a box between the start and end points
	 * with the width specified for this layer.
	 * Flip the rectangle as necessary to ensure that
	 * LL <= UR.
	 */
	r.r_ll = path->rp_entry;
	r.r_ur = prev->rp_entry;
	if (r.r_xbot > r.r_xtop)
	    t = r.r_xbot, r.r_xbot = r.r_xtop, r.r_xtop = t;
	if (r.r_ybot > r.r_ytop)
	    t = r.r_ybot, r.r_ybot = r.r_ytop, r.r_ytop = t;
	r.r_xtop += path->rp_rLayer->rl_routeType.rt_width;
	r.r_ytop += path->rp_rLayer->rl_routeType.rt_width;

	rL = path->rp_rLayer;
	DBPaintPlane(mzResultDef->cd_planes[rL->rl_planeNum], 
	    &r,
	    DBStdPaintTbl(rL->rl_routeType.rt_tileType,rL->rl_planeNum),
	    (PaintUndoInfo *) NULL);
    }

    /* Update bounding box of result cell */
    DBReComputeBbox(mzResultDef);

    /* return pointer to result cell use */
    return mzResultUse;

}


/*
 * ----------------------------------------------------------------------------
 *
 * MZClean --
 *
 * Reclaim storage space gobbled up during route, and reset tile client
 * fields.  After a MZInitRoute() has been issued, MZClean() should always
 * be called prior to returning from Magic command.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */
Void
MZClean()
{
    if(mzDirty)
    {
	/* clear estimate plane */
	mzCleanEstimate();

	/* Reclaim storage and reset mzStartList */
	{
	    ListDeallocC(mzStartTerms);
	    mzStartTerms = NULL;
	}

	/* Reset dest alignment structures */
	mzNLClear(&mzXAlignNL);
	mzNLClear(&mzYAlignNL);

	/* Unmark marked tiles, and cells and dealloc marked lists */
	{
	    List *l;

	    /* Reset Marked tiles client fields to MINFINITY */
	    for(l=mzMarkedTilesList; l!=NULL; l=LIST_TAIL(l))
	    {
		Tile *tile = (Tile *) LIST_FIRST(l);
		
		/* Restore tile client field to its "unmarked" value */
		tile->ti_client = (ClientData) MINFINITY;
	    }

	    /* Dealloc list of marked tiles */
	    ListDealloc(mzMarkedTilesList);
	    mzMarkedTilesList = NULL;

	    /* Reset Marked subcell client fields to MINFINITY */
	    for(l=mzMarkedCellsList; l!=NULL; l=LIST_TAIL(l))
	    {
		CellUse *cu = (CellUse *) LIST_FIRST(l);
		
		/* Restore celluse client field to its "unmarked" value */
		cu->cu_client = (ClientData) MINFINITY;
	    }

	    /* Dealloc list of marked cells */
	    ListDealloc(mzMarkedCellsList);
	    mzMarkedCellsList = NULL;
	}

	/* Free up route-path queues */
	if(mzPathsDirty)
	{
	    HeapKill(&mzMaxToGoHeap, (void (*)()) NULL);
	    HeapKill(&mzMinCostHeap, (void (*)()) NULL);
	    HeapKill(&mzMinAdjCostHeap, (void (*)()) NULL);
	    HeapKill(&mzMinCostCompleteHeap, (void (*)()) NULL);
	    ListDealloc(mzBloomStack);
	    ListDealloc(mzStraightStack);
	    ListDealloc(mzDownHillStack);
	    ListDealloc(mzWalkStack);

	    /* Free up hash table */
	    HashKill(&mzPointHash);

	    /* Reclaims route path entries */
	    mzFreeAllRPaths();
	    
	    /* reset flag */
	    mzPathsDirty = FALSE;
	}

	/* reset flag */
	mzDirty = FALSE;
    }

    return;
}
