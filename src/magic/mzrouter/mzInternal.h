/*
 * mzInternal.h --
 *
 * This file defines data structures, variables, and constants internal to
 * the maze router module.
 *
 *
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
 *
 * rcsid $Header: mzInternal.h,v 6.1 90/08/28 19:35:17 mayo Exp $
 */

/* 
 * Structures etc. that are exported by the mzrouter are defined in
 * mzrouter.h.
 *
 *  Structures (etc.) defined here are shared between files in this module. 
 *
 *  Structures local to a given source 
 *  file are defined at the top of that source file.
 *
 *  Structures specific to a given function are defined at 
 *  the head of that function.
 */

#ifndef _MAGIC
int err0 = Need_to_include_magic_header;
#endif
#ifndef _DATABASE
int err1 = Need_to_include_database_header;
#endif
#ifndef _MZROUTER
int err2 = Need_to_include_mzrouter_header;
#endif
#ifndef _LIST
int err3 = Need_to_include_list_header;
#endif
#ifndef _DOUBLEINT
int err4 = Need_to_include_doubleint_header;
#endif
#ifndef _HEAP
int err5 = Need_to_include_heap_header;
#endif

/* ------------------------ Version String --------------------------- */
#define MZROUTER_VERSION "0.6"
 
/* ------------------------ Debugging flags ----------------------------- */
extern ClientData mzDebugID;
#include "mzDebug.h"

/* ---------- Default Parameter Values ----------------------------------- */

/* Penalty factor applied to cost estimate in excess of max window bound 
 * represented as mantissa / 2**nExponent 
 */
/* (2048/2**1 = 1028) */
#define DEF_PENALTY_MANTISSA 2048
#define DEF_PENALTY_NEXPONENT 1

/* Window rate in cost/bloom */
#define DEF_WINDOW_RATE 500

/* Window width in cost */
#define DEF_WINDOW_WIDTH 10000

/* Maximum cost increase allowed during blooming */
#define DEF_BLOOM_DELTA_COST 1

/* minimum radius of blockage plane info required around point being expanded.
 * value of -1 causes mzrouter to compute its own best guess.
 * (Areas twice this size are gened. whenever the minimum is not met.)
 */
#define DEF_BOUNDS_INCREMENT -1

/* If reset, degenerate estimation plane used (just 4 tiles - one for each 
 * quadrant with respect to destination point).
 */
#define DEF_ESTIMATE 1
/* If set, routes may terminate at any geometry that is electrically connected
 * to specified dest regions.
 */
#define DEF_EXPAND_DESTS 0
/* If set only hints in toplevel cell are recognized - speeds up processing
 * of hint planes when there are lots of expanded subcells.
 */
#define DEF_TOP_HINTS_ONLY 0
/* Maximum distance route can penetrate blocked areas to reach destination.
 * Note only "same node" blockage will be penetrated, i.e. blocks steming from
 * spacing to unrelated nodes are always honored.
 * 
 * A value of -1 causes the max penetration to be recomputed prior to each
 * route based on the design rules for the active routetypes.
 */
#define DEF_MAX_WALK_LENGTH -1
/* controls message printing:  0 = errors and warnings only, 1 = brief, 2 =
 * lots of statistics.
 */
#define DEF_VERBOSITY	1
/* if positive, puts upper limit on number of blooms during search.
 * After limit is reached routing is terminated and best complete route
 * to date is returned.
 */
#define DEF_BLOOM_LIMIT	0

/* This struc used to make list of named parameter sets for maze routing. 
 * Only the MazeParameters themselves are passed to the routines 
 * in this module.
 */
typedef struct mazestyle
{
    char *ms_name;		/* name of style */
    List *ms_spacingL;		/* used to store spacing during tech readin. */
    MazeParameters ms_parms;	/* parameter settings for this style */
    struct mazestyle *ms_next;  
} MazeStyle;


/* ----- TileTypes, Paint Tables, and Paint Planes, and Internal Cells  ---- */
/* RESULT CELL */
extern CellDef *mzResultDef;
extern CellUse *mzResultUse;

/* Types are offset to coincide with hint types.
 * This facilitates display for debugging.
 */
#define TT_OFF	(TT_MAGNET - 1)

/* BLOCKAGE PLANES - AND CELL FOR DISPLAY */
/* Higher numbered blockage types ALWAYS take priority during painting */
    /* Block due to destnode only */
#define TT_SAMENODE_BLOCK (1 + TT_OFF)  
    /* Can reach dest via contact */
#define TT_CONTACT_WALK	  (2 + TT_OFF)
    /* Approach to dest area */
#define TT_LEFT_WALK	  (3 + TT_OFF)  
#define TT_RIGHT_WALK     (4 + TT_OFF)
#define TT_TOP_WALK       (5 + TT_OFF)
#define TT_BOTTOM_WALK    (6 + TT_OFF)
    /* route destination area */
#define TT_DEST_AREA      (7 + TT_OFF)  
    /* Can't route in this space */
#define	TT_BLOCKED	  (8 + TT_OFF)	
extern PaintResultType mzBlockPaintTbl[TT_MAXTYPES][TT_MAXTYPES];
extern CellDef *mzBlockDef ;

/* BOUNDS PLANE */
    /* Blockage planes generated here */
#define TT_INBOUNDS	(1 + TT_OFF)	
#define TT_GENBLOCK	(2 + TT_OFF)
extern PaintResultType mzBoundsPaintTbl[TT_MAXTYPES][TT_MAXTYPES];
extern Plane *mzHBoundsPlane;
extern Plane *mzVBoundsPlane;

/* ESTIMATION PLANE */
#define TT_EST_SUBCELL 	(1 + TT_OFF)
#define TT_EST_FENCE 	(2 + TT_OFF)
#define TT_EST_DEST	(3 + TT_OFF)
extern PaintResultType mzEstimatePaintTbl[TT_MAXTYPES][TT_MAXTYPES];
extern Plane *mzEstimatePlane;

/* HINT TYPES */
extern TileTypeBitMask mzHintTypesMask;     /* map of hint, fence
						and rotate types */

/* HINT (magnet) PLANES */
extern Plane *mzHHintPlane;
extern Plane *mzVHintPlane;

/* FENCE PLANE */
extern Plane *mzHFencePlane;

/* ROTATE PLANES */
extern Plane *mzHRotatePlane;
extern Plane *mzVRotatePlane;

/* Number Lines - used to mark points aligned with dest tile corners */
typedef struct numberLine
{
    int		nl_sizeAlloced;  /* Number of entries allocated */
    int		nl_sizeUsed;
    int  	*nl_entries;	
} NumberLine;

/*
 * Macros for allocating segments of a RoutePath.
 * During maze-routing, we allocate RoutePaths from our own private area.
 * When the time comes to return a RoutePath, it is copied to permanent
 * storage (i.e., MALLOC/FREE-managed) and the rest of the temporary storage
 * is reclaimed.  This approach avoids the need for additional pointers in
 * each RoutePath that would otherwise be required for storage reclamation.
 */
#define	PATHSPERSEG	200	/* Number of RoutePaths per segment */

typedef struct routePage
{
    struct routePage	*rpp_next;
    int			 rpp_free;
    RoutePath		 rpp_array[PATHSPERSEG];
} RoutePage;

/* First, last, and current RoutePages on list for allocating RoutePaths */
extern RoutePage *mzFirstPage;
extern RoutePage *mzLastPage;
extern RoutePage *mzCurPage;

/* Allocate a new RoutePath */
#define	NEWPATH() \
	((mzCurPage == NULL \
		    || mzCurPage->rpp_free >= PATHSPERSEG) \
	    ? mzAllocRPath() \
	    : (RoutePath *) (&mzCurPage->rpp_array[mzCurPage->rpp_free++]))

/*---------------------- Start Terminal ------------------------------------ */
/* A start terminal is a point and a tiletype to start on.
 * This struc is used to gather list of legal start points.
 */
typedef struct
{
    Point 	cp_point;	/* (x, y) */
    TileType    cp_type;	/* layer to start on */
} ColoredPoint;


/*---------------------- Dest Terminal ------------------------------------ */
/* A destination terminal is a rectangle and a type to connect on. 
 * This struc is used to create lists of legal destination areas.
 * (This structure also used in other situations requiring rects with
 *  an associated tiletype.)
 */
typedef struct
{
    Rect 	cr_rect;
    TileType	cr_type;
} ColoredRect;

/*----------------------Path Hash Table and Queues ----------------------- */

/*
 * The following hash table is used to index points
 * so that redundant extension from points reached more than once 
 * is avoided.
 * It is indexed by (x,y,routeLayer, orientation).  Orientation is either
 * Horizontal, vertical, or original (point on layer).  Horizontal and
 * vertical entries to the point are both extended from since they
 * may vary in number of jogs and hence cost.
 */
typedef struct
{
    Point	 pk_point;	/* (x, y) */
    RouteLayer  *pk_rLayer;	/* "z" - Layer we are on */
    int 	 pk_orient;	/* Because of jogCost, we must distinguish
				 * between endpts of horizontal and vertical
				 * segments.
				 */
} PointKey;
extern HashTable mzPointHash;
#define INITHASHSIZE	64

/* Queues for partial paths */
extern Heap mzMaxToGoHeap;	/* paths nearer destination than WINDOW */
extern Heap mzMinCostHeap;	/* paths in WINDOW */
extern Heap mzMinAdjCostHeap;   /* paths farther from dest than WINDOW*/
extern List *mzBloomStack;	/* paths in current local focus */
extern List *mzStraightStack;	/* focus paths being extended in straightline*/
extern List *mzDownHillStack;	/* focus paths followed only until cost
				   increases */
extern List *mzWalkStack;	/* paths inside walk (i.e. inside blocked
				 * area adjacent to destination.
				 */
extern Heap mzMinCostCompleteHeap;  /* completed paths */
#define	INITHEAPSIZE	64

/* ------------------------ Routines Global to MazeRouter --------------- */
extern RoutePath *mzAllocRPath();
extern char *mzCost2String();
extern char *mzDICost2String();
extern DoubleInt mzEstimatedCost();
extern RouteType *mzFindRouteType();
extern Void mzNLClear();
extern int *mzNLGetContainingInterval();
extern Void mzNLInit();
extern Void mzNLInsert();
extern RoutePath *mzSearch();
extern Void mzTechFinal();
extern TileTypeBitMask mzTouchingTypes();

/* --------------------- Variables Global to MazeRouter ----------------- */

/* Parameter sets for mzrouting - from tech file. */
extern MazeStyle *mzStyles;

/* Source of path currently being extended */
extern int mzPathSource;
#define SOURCE_INIT 0
#define SOURCE_BLOOM 1
#define SOURCE_STRAIGHT 2
#define SOURCE_DOWNHILL 3
#define SOURCE_WALK 4

/* Paint generated by router */
extern CellDef *mzResultDef;
extern CellUse *mzResultUse;

/* Cell to do routing in */
extern CellUse *mzRouteUse;

/* Mask giving expanded subcells */
extern int mzCellExpansionMask;

/* Don't route outside of this area */
extern Rect mzBoundingRect;

/* List of Route starting terminals */
extern List *mzStartTerms;

/* Cell containing dest areas */
extern CellDef *mzDestAreasDef;
extern CellUse *mzDestAreasUse;

/* dest terminal alignment coordinates */
extern NumberLine mzXAlignNL;
extern NumberLine mzYAlignNL;
/* initial size of above number lines */
#define INITIAL_ALIGN_SIZE 100
 
/* Fence parity */
extern bool mzInsideFence;

/* largest design rule distance - used during incremental blockage gen. */
extern int mzContextRadius; 

/* Route types */
extern RouteLayer *mzRouteLayers;
extern RouteLayer *mzActiveRLs;
extern RouteContact *mzRouteContacts;
extern RouteType *mzRouteTypes;
extern RouteType *mzActiveRTs;

/* minimum radius of blockage plane info required around point being expanded.
 * (Areas twice this size are gened. whenever the minimum is not met.)
 */
extern int mzBoundsIncrement;

/* If reset, degenerate estimation plane used (just 4 tiles - one for each 
 * quadrant with respect to destination point).
 */
extern int mzEstimate;
/* If set, routes may terminate at any geometry that is 
 * electrically connected to specified dest areas.
 */
extern int mzExpandDests;
/* If set, only hints in toplevel cell are recognized */
extern int mzTopHintsOnly;
/* Maximum distance route will extend into blocked area to connect to dest. */
extern int mzMaxWalkLength;
/* limits area of route for performance,
 * NOTE: IF NONNULL, USER MUST MAKE SURE ROUTE IS ACTUALLY LIMITED TO THIS 
 * AREA WITH FENCES, OTHERWISE THE RESULT IS UNPREDICTABLE.
 */
extern Rect *mzBoundsHint;

/* how generous to be with messages */
extern int mzVerbosity;
/* if positive, limit on number of blooms */
extern int mzBloomLimit;

/* minimum estimated total cost for initial paths */
extern DoubleInt mzMinInitialCost;

/* Parameters controlling search */
extern RouteFloat mzPenalty;
extern DoubleInt mzWRate;
extern DoubleInt mzBloomDeltaCost;
extern DoubleInt mzWWidth;

/* Statistics */
extern DoubleInt mzInitialEstimate;  /* Initial estimated cost of route */
extern int mzNumBlooms;
extern int mzNumOutsideBlooms;	/* num blooms from outside window */
extern int mzNumComplete;	/* number of complete paths so far */
extern int mzBlockGenCalls;	/* # of calls to blockage gen. code */
extern double mzBlockGenArea;   /* area over which blockage planes 
				 * have been gened. */
extern int mzNumPathsGened;	/* number of partial paths added to heap */
extern int mzNumPaths;		/* number of paths processed */
extern int mzReportInterval;    /* frequency that # of paths etc. 
				 * is reported. */
extern int mzPathsTilReport;	/* counts down to next path report */

/* Variables controling search */
extern DoubleInt mzWInitialMinToGo;
extern DoubleInt mzWInitialMaxToGo;
extern DoubleInt mzBloomMaxCost;	

/* Search status */
extern DoubleInt mzWindowMinToGo; /* Window location */
extern DoubleInt mzWindowMaxToGo;

/* Marked tile and cell lists */
extern List *mzMarkedTilesList;
extern List *mzMarkedCellsList;

/* ------------ Interesting Point Macros -------------------------------- */
/* The following macros are used in the low-level routines for finding
 * the NEXT interesting point to the Right, Left, Up and Down.  
 *
 * The macros are used to choose the first among 2 interesting points.
 */
#define PRUNE_TO_MIN(a,new,reasonSet,reason) \
    if (1) \
    { \
	if((new)<(a)) \
	{ \
            (a) = (new); \
            (reasonSet) = (reason); \
        } \
	else if ((new)==(a)) \
	{ \
	    (reasonSet) |= (reason); \
	} \
    } else

#define PRUNE_TO_MAX(a,new,reasonSet,reason) \
    if (1) \
    { \
	if((new)>(a)) \
	{ \
            (a) = (new); \
            (reasonSet) = (reason); \
        } \
	else if ((new)==(a)) \
	{ \
	    (reasonSet) |= (reason); \
	} \
    } else

#define RC_JOG		1
#define RC_ALIGNOTHER   2
#define RC_CONTACT	4
#define RC_ALIGNGOAL	8
#define RC_HINT		16
#define RC_ROTBEFORE	32
#define RC_ROTINSIDE	64
#define RC_BOUNDS	128
#define RC_WALK		256
#define RC_WALKC	512
#define RC_DONE		1024
