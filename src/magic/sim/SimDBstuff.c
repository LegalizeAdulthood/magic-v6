
/* SimDBstuff.c -
 *
 *	This file contains routines that extract electrically connected
 *	regions of a layout for Magic.   This extractor operates 
 *	hierarchically, across cell boundaries (SimTreeCopyConnect), as
 *	well as within a single cell (SimSrConnect).
 *
 *	This also contains routines corresponding to those in the DBWind
 *	module.
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
 * University of California
 */


#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "textio.h"
#include "signals.h"
#include "malloc.h"
#include "extractInt.h"
#include "sim.h"
#include "windows.h"
#include "dbwind.h"
#include "commands.h"
#include "txcommands.h"
#include "styles.h"
#include "graphics.h"

/* The following structure is used to hold several pieces
 * of information that must be passed through multiple
 * levels of search function.
 */
	
struct conSrArg
{
    CellDef *csa_def;			/* Definition being searched. */
    Plane *csa_plane;			/* Current plane being searched. */
    TileTypeBitMask *csa_connect;	/* Table indicating what connects
					 * to what.
					 */
    int (*csa_clientFunc)();		/* Client function to call. */
    ClientData csa_clientData;		/* Argument for clientFunc. */
    bool csa_clear;			/* FALSE means pass 1, TRUE
					 * means pass 2.
					 */
    Rect csa_bounds;			/* Area that limits search. */
};

/* For SimTreeSrConnect, the extraction proceeds in one pass, copying
 * all connected stuff from a hierarchy into a single cell.  A list
 * is kept to record areas that still have to be searched for
 * hierarchical stuff.
 */

struct conSrArg2
{
    Rect csa2_area;			/* Area that we know is connected, but
					 * which may be connected to other
					 * stuff.
					 */
    int csa2_type;			/* Type of material we found. */
    struct conSrArg2 *csa2_next;	/* Pointer to next in list. */
};

/* We need to maintain where in the cell hierarchy we are as we search
 * the tile database when extracting a node name.  To do this, a stack is
 * maintained of the hierarchy of cell definitions (a search context)
 * as the search progresses.  This stack is doubly linked to allow fast
 * pathname construction.
 */

typedef struct scx_stack_elt
{
    SearchContext  se_scx;		/* current cell def we're looking at */
    struct scx_stack_elt *se_next;
    struct scx_stack_elt *se_prev;
} ScxStackElt;

extern char *DBPrintUseId();
extern int  dbcUnconnectFunc();

static ScxStackElt	*stackBase = (ScxStackElt *) NULL;
					/* stack of search contexts */
static char 		bestName[256];


/*
 * ----------------------------------------------------------------------------
 *
 *	SimFirstTile
 *
 *	This procedure is based upon the function dbcConnectFunc in the
 *	database module.
 *
 * 	This procedure is invoked by SimTreeSrTiles from SimTreeCopyConnect,
 *	whenever a tile is found that is connected to the current area
 *	being processed.  If the tile overlaps the search area in a non-
 *	trivial way (i.e. more than a 1x1 square of overlap at a corner)
 *	then the area of the tile is added onto the list of things to check.
 *	The "non-trivial" overlap check is needed to prevent caddy-corner
 *	tiles from being considered as connected.
 *
 * Results:
 *	Always returns 1 when a tile has been found.
 *
 * Side effects:
 *	Adds a new record to the current check list.
 *
 * ----------------------------------------------------------------------------
 */

int
SimFirstTile(tile, cx)
    Tile *tile;			/* Tile found. */
    TreeContext *cx;		/* Describes context of search.  The client
				 * data is a pointer to the list head of
				 * the conSrArg2's describing the areas
				 * left to check.
				 */
{
    struct conSrArg2 **pHead = (struct conSrArg2 **) cx->tc_filter->tf_arg;
    struct conSrArg2 *newCsa2;
    Rect tileArea, *srArea;
    static char currentName[256];
    static char buff[256];
    int i;

    TiToRect(tile, &tileArea);
    srArea = &cx->tc_scx->scx_area;
    if (((tileArea.r_xbot >= srArea->r_xtop-1) ||
        (tileArea.r_xtop <= srArea->r_xbot+1)) &&
	((tileArea.r_ybot >= srArea->r_ytop-1) ||
	(tileArea.r_ytop <= srArea->r_ybot+1)))
    {
	/* If the search area is only one unit wide or tall, then it's
	 * OK to have only a small overlap.  This happens only when
	 * looking for an initial search tile.
	 */

	 if (((srArea->r_xtop-1) != srArea->r_xbot)
	    && ((srArea->r_ytop-1) != srArea->r_ybot)) return 0;
    }
    MALLOC(struct conSrArg2 *, newCsa2, sizeof (struct conSrArg2));
    GeoTransRect(&cx->tc_scx->scx_trans, &tileArea, &newCsa2->csa2_area);
    newCsa2->csa2_type = TiGetType(tile);
    newCsa2->csa2_next = *pHead;
    *pHead = newCsa2;

    return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 *	SimConnectFunc
 *
 *	This procedure is based upon the function dbcConnectFunc in the
 *	database module.
 *
 * 	This procedure is invoked by SimTreeSrTiles from SimTreeCopyConnect,
 *	whenever a tile is found that is connected to the current area
 *	being processed.  If the tile overlaps the search area in a non-
 *	trivial way (i.e. more than a 1x1 square of overlap at a corner)
 *	then the area of the tile is added onto the list of things to check.
 *	The "non-trivial" overlap check is needed to prevent caddy-corner
 *	tiles from being considered as connected.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Adds a new record to the current check list.
 *
 * ----------------------------------------------------------------------------
 */

int
SimConnectFunc(tile, cx)
    Tile *tile;			/* Tile found. */
    TreeContext *cx;		/* Describes context of search.  The client
				 * data is a pointer to the list head of
				 * the conSrArg2's describing the areas
				 * left to check.
				 */
{
    struct conSrArg2	**pHead = (struct conSrArg2 **) cx->tc_filter->tf_arg;
    struct conSrArg2	*newCsa2;
    Rect 		tileArea, *srArea;
    char		pathName[256];
    static char		nodeName[256];
    int i;

    TiToRect(tile, &tileArea);
    srArea = &cx->tc_scx->scx_area;
    if (((tileArea.r_xbot >= srArea->r_xtop-1) ||
        (tileArea.r_xtop <= srArea->r_xbot+1)) &&
	((tileArea.r_ybot >= srArea->r_ytop-1) ||
	(tileArea.r_ytop <= srArea->r_ybot+1)))
    {
	return 0;
    }
    MALLOC(struct conSrArg2 *, newCsa2, sizeof (struct conSrArg2));
    GeoTransRect(&cx->tc_scx->scx_trans, &tileArea, &newCsa2->csa2_area);
    newCsa2->csa2_type = TiGetType(tile);
    newCsa2->csa2_next = *pHead;
    *pHead = newCsa2;

    SimMakePathname( pathName );

    /* extract the node name */

    SigDisableInterrupts();
    strcpy( nodeName, SimGetNodeName( cx->tc_scx, tile, pathName ) ); 
    SigEnableInterrupts();

    /* save the "best" name for this node */

    if( bestName[0] == '\0' || efPreferredName( nodeName, bestName ) )
	strcpy( bestName, nodeName );

    /* abort the name search if the name is global or name is in the
     * abort name search table 
     */

    i = strlen( nodeName );
    if( SimSawAbortString  || SigInterruptPending || (nodeName[i - 1] == '!') )
	return 1;

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 *	SimTreeCopyConnect
 *
 *	This procedure is very similar to DBTreeCopyConnect.
 *
 * 	This procedure copies connected information from a given cell
 *	hierarchy to a given (flat) cell.  Starting from the tile underneath
 *	the given area, this procedure finds all paint in all cells
 *	that is connected to that information.  All such paint is
 *	copied into the result cell.  If there are several electrically
 *	distinct nets underneath the given area, one of them is picked
 *	at more-or-less random.
 *
 *	Modified so the result cell is NOT first cleared of all paint.  This
 *	allows multiple calls, to highlight incomplete routing nets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the result cell are modified.
 *
 * ----------------------------------------------------------------------------
 */

void
SimTreeCopyConnect(scx, mask, xMask, connect, area, destUse, Node_Name)
    SearchContext *scx;			/* Describes starting area.  The
					 * scx_use field gives the root of
					 * the hierarchy to search, and the
					 * scx_area field gives the starting
					 * area.  An initial tile must overlap
					 * this area.  The transform is from
					 * coords of scx_use to destUse.
					 */
    TileTypeBitMask *mask;		/* Tile types to start from in area. */
    int xMask;				/* Information must be expanded in all
					 * of the windows indicated by this
					 * mask.  Use 0 to consider all info
					 * regardless of expansion.
					 */
    TileTypeBitMask *connect;		/* Points to table that defines what
					 * each tile type is considered to
					 * connect to.  Use DBConnectTbl as
					 * a default.
					 */
    Rect *area;				/* The resulting information is
					 * clipped to this area.  Pass
					 * TiPlaneRect to get everything.
					 */
    CellUse *destUse;			/* Result use in which to place
					 * anything connected to material of
					 * type mask in area of rootUse.
					 */
    char *Node_Name;			/* Name of node returned.
					 * NOTE:  Don't call this "NodeName",
					 * because that conflicts with reserved
					 * words in some compilers.
					 */
{
    SearchContext scx2;
    TileTypeBitMask notConnectMask, *connectMask;
    struct conSrArg2 *list, *current;
    CellDef *def = destUse->cu_def;
    int pNum;

    /* Get one initial tile and put it our list. */

    list = NULL;
    (void) DBTreeSrTiles(scx, mask, xMask, SimFirstTile, (ClientData) &list );

    SimInitScxStk();
    bestName[0] = '\0';

    /* Enter the main processing loop, pulling areas from the list
     * and processing them one at a time until the list is empty.
     */
    
    scx2 = *scx;
    while (list != NULL)
    {
	current = list;
	list = current->csa2_next;

	/* Clip the current area down to something that overlaps the
	 * area of interest.
	 */
	
	GeoClip(&current->csa2_area, area);
	if (GEO_RECTNULL(&current->csa2_area)) goto endOfLoop;

	/* See if the destination cell contains stuff over the whole
	 * current area (on its home plane) that is connected to it.
	 * If so, then there's no need to process the current area,
	 * since any processing that is needed was already done before.
	 */
	
	connectMask = &connect[current->csa2_type];
        if (TTMaskHasType(&DBContactBits,current->csa2_type))
        {
             TTMaskSetOnlyType(&notConnectMask, current->csa2_type);
             TTMaskCom(&notConnectMask);
        }
        else
        {
             TTMaskCom2(&notConnectMask, connectMask);
        }
	pNum = DBPlane(current->csa2_type);
	if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
	    &current->csa2_area, &notConnectMask,
	    dbcUnconnectFunc, (ClientData) &current->csa2_area) != 0)
	{
	    /* Since the whole area of this tile hasn't been recorded,
	     * we must process its area to find any other tiles that
	     * connect to it.  Add each of them to the list of things
	     * to process.  We have to expand the search area by 1 unit
	     * on all sides because SimTreeSrTiles only returns things
	     * that overlap the search area, and we want things that
	     * even just touch.
	     */

	    scx2.scx_area = current->csa2_area;
	    scx2.scx_area.r_xbot -= 1;
	    scx2.scx_area.r_ybot -= 1;
	    scx2.scx_area.r_xtop += 1;
	    scx2.scx_area.r_ytop += 1;
	    if( SimTreeSrTiles(&scx2, connectMask, xMask, SimConnectFunc,
	      (ClientData) &list) )
	    {
		FREE( (char *) current );
		break;
	    }
	}

	/* Lastly, paint this tile into the destination cell.  This
	 * marks its area has having been processed.  Then recycle
	 * the storage for the current list element.
	 */
	
	DBPaintPlane(def->cd_planes[pNum], &current->csa2_area,
	    DBStdPaintTbl(current->csa2_type, pNum), (PaintUndoInfo *) NULL);
	endOfLoop: FREE((char *) current);
    }

	/* Just in case we got aborted, or an abort string was seen */
    while (list != NULL )
    {
	current = list;
	list = current->csa2_next;
	FREE( (char *) current );
    }

    /* Finally, when all done, recompute the bounding box of the
     * destination and record its area for redisplay.
     */
    
    strcpy( Node_Name, bestName );
    DBReComputeBbox(def);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efPreferredName --
 *
 * This is the same function used in the ext2sim module.  We need this
 * function for the rsim interface to Magic.
 *
 * Determine which of two names is more preferred.  The most preferred
 * name is a global name.  Given two non-global names, the one with the
 * fewest pathname components is the most preferred.  If the two names
 * have equally many pathname components, we choose the shortest.
 *
 * Results:
 *	TRUE if 'name1' is preferable to 'name2', FALSE if not.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
efPreferredName(name1, name2)
    char *name1, *name2;
{
    register int nslashes1, nslashes2;
    register char *np1, *np2;

    if( name1[0] == '@' && name1[1] == '=' )
	return( TRUE );
    else if( name2[0] == '@' && name2[1] == '=' )
	return( FALSE );

    for (nslashes1 = 0, np1 = name1; *np1; ) {
	if (*np1++ == '/')
	    nslashes1++;
    }

    for (nslashes2 = 0, np2 = name2; *np2; ) {
	if (*np2++ == '/')
	    nslashes2++;
    }

    --np1;
    --np2;

    /* both are global names */
    if ((*np1 == '!') && (*np2 == '!')) {
	/* check # of pathname components */
	if (nslashes1 < nslashes2) return (TRUE);
	if (nslashes1 > nslashes2) return (FALSE);

	/* same # of pathname components; check length */
        if (np1 - name1 < np2 - name2) return (TRUE);
    	if (np1 - name1 > np2 - name2) return (FALSE);

	/* same # of pathname components; same length; use lex order */
	if (strcmp(name1, name2) > 0) 
	    return(TRUE);
	else
	    return(FALSE);
    }
    if (*np1 == '!') return(TRUE);
    if (*np2 == '!') return(FALSE);

    /* neither name is global */
    /* chose label over generated name */
    if (*np1 != '#' && *np2 == '#') return (TRUE);
    if (*np1 == '#' && *np2 != '#') return (FALSE);

    /* either both are labels or generated names */
    /* check pathname components */
    if (nslashes1 < nslashes2) return (TRUE);
    if (nslashes1 > nslashes2) return (FALSE);

    /* same # of pathname components; check length */
    if (np1 - name1 < np2 - name2) return (TRUE);
    if (np1 - name1 > np2 - name2) return (FALSE);

    /* same # of pathname components; same length; use lex ordering */
    if (strcmp(name1, name2) > 0) 
	return(TRUE);
    else
	return(FALSE);
}



/*
 * ----------------------------------------------------------------------------
 *
 *	SimSrConnect
 *
 *	This is similar to the procedure DBSrConnect, except that the
 *	marks on each tile in the cell are not erased.
 *
 * 	Search through a cell to find all paint that is electrically
 *	connected to things in a given starting area.
 *
 * Results:
 *	0 is returned if the search finished normally.  1 is returned
 *	if the search was aborted.
 *
 * Side effects:
 *	The search starts from one (random) non-space tile in "startArea"
 *	that matches the types in the mask parameter.  For every paint
 *	tile that is electrically connected to the initial tile and that
 *	intersects the rectangle "bounds", func is called.  Func should
 *	have the following form:
 *
 *	    int
 *	    func(tile, clientData)
 *		Tile *tile;
 *		ClientData clientData;
 *    	    {
 *	    }
 *
 *	The clientData passed to func is the same one that was passed
 *	to us.  Func returns 0 under normal conditions;  if it returns
 *	1 then the search is aborted.
 *
 *				*** WARNING ***
 *	
 *	Func should not modify any paint during the search, since this
 *	will mess up pointers kept by these procedures and likely cause
 *	a core-dump.  
 *
 * ----------------------------------------------------------------------------
 */

int
SimSrConnect(def, startArea, mask, connect, bounds, func, clientData)
    CellDef *def;		/* Cell definition in which to carry out
				 * the connectivity search.  Only paint
				 * in this definition is considered.
				 */
    Rect *startArea;		/* Area to search for an initial tile.  Only
				 * tiles OVERLAPPING the area are considered.
				 * This area should have positive x and y
				 * dimensions.
				 */
    TileTypeBitMask *mask;	/* Only tiles of one of these types are used
				 * as initial tiles.
				 */
    TileTypeBitMask *connect;	/* Pointer to a table indicating what tile
				 * types connect to what other tile types.
				 * Each entry gives a mask of types that
				 * connect to tiles of a given type.
				 */
    Rect *bounds;		/* Area, in coords of scx->scx_use->cu_def,
				 * that limits the search:  only tiles
				 * overalapping this area will be returned.
				 * Use TiPlaneRect to search everywhere.
				 */
    int (*func)();		/* Function to apply at each connected tile. */
    ClientData clientData;	/* Client data for above function. */

{
    struct conSrArg csa;
    int startPlane, result;
    Tile *startTile;			/* Starting tile for search. */
    extern int dbSrConnectFunc();	/* Forward declaration. */
    extern int dbSrConnectStartFunc();

    result = 0;
    csa.csa_def = def;
    csa.csa_bounds = *bounds;

    /* Find a starting tile (if there are many tiles underneath the
     * starting area, pick any one).  The search function just saves
     * the tile address and returns.
     */

    startTile = NULL;
    for (startPlane = PL_TECHDEPBASE; startPlane < DBNumPlanes; startPlane++)
    {
	if (DBSrPaintArea((Tile *) NULL,
	    def->cd_planes[startPlane], startArea, mask,
	    dbSrConnectStartFunc, (ClientData) &startTile) != 0) break;
    }
    if (startTile == NULL) return 0;

    /* Pass 1.  During this pass the client function gets called. */

    csa.csa_clientFunc = func;
    csa.csa_clientData = clientData;
    csa.csa_clear = FALSE;
    csa.csa_connect = connect;
    csa.csa_plane = def->cd_planes[startPlane];
    if (dbSrConnectFunc(startTile, &csa) != 0) result = 1;

    return result;
}


/*
 * SimInitScxStk
 *
 *	This procedure initializes the doubly-linked stack of search
 *	contexts which describes the current hierarchy of the search.
 *	
 *
 * Results:
 *	The stack is set to NULL.
 *
 * Side effects:
 *	Space in the stack is freed if the stack is not empty.
 *	
 */

SimInitScxStk()
{
    if (stackBase == (ScxStackElt *) NULL) {
	MALLOC(ScxStackElt *, stackBase, sizeof(ScxStackElt));
	stackBase->se_scx.scx_use = (CellUse *) NULL;
	stackBase->se_next = stackBase->se_prev = stackBase;
    }
    else {
	while (stackBase != stackBase->se_next) {
	    SimPopScx();
	}
    }
}

/*
 * SimPushScx
 *
 *	This procedure pushes a new search context onto the search
 *	context stack.  A search context is pushed only if it is
 *	different from the one on top of the stack.
 *
 * Results:
 *	Search context is pushed.
 *
 * Side effects:
 *	None.
 *
 */

SimPushScx(newscx) 

SearchContext *newscx;
{

    ScxStackElt *dummy;

    /* check to see if the search context is different from the
     * one on top of the stack
     */

    if (stackBase->se_prev->se_scx.scx_use != newscx->scx_use) {
	MALLOC(ScxStackElt *, dummy, sizeof(ScxStackElt));
	dummy->se_scx = *newscx;
	dummy->se_next = stackBase->se_next;
	stackBase->se_next = dummy;
	dummy->se_prev = stackBase;
	dummy->se_next->se_prev = dummy;
    }
}

/*
 * SimPopScx
 *
 *	This procedure pops the search context on top of the stack.
 *
 * Results:
 *	The stack is popped.
 *
 * Side effects:
 *	None.
 *
 */

SimPopScx()
{
    ScxStackElt *dummy;
    if (stackBase == stackBase->se_next) {
	TxPrintf("SimPopScx -- stack empty\n");
	return;
    }
    else {
	dummy = stackBase->se_next;
	stackBase->se_next = dummy->se_next;
	stackBase->se_next->se_prev = stackBase;
	FREE(dummy);
    }
}

/*
 * SimMakePathname
 *
 * 	This procedure creates a pathname for the current hierarchy
 *	by walking the search context stack in reverse order.
 *
 * Results:
 *	The pathname is returned in the parameter pathname.
 *
 * Side effects:
 *	None.
 *
 */

SimMakePathname(pathname)

char *pathname;

{
    ScxStackElt *dummy;
    char buff[100];

    *pathname = '\0';
    if (stackBase == stackBase->se_next) {
        return;
    } 
    else {
	for (dummy = stackBase->se_prev; dummy != stackBase; 
	     dummy = dummy->se_prev) {
	    DBPrintUseId(&(dummy->se_scx), buff, 100);
	    strcat(pathname, buff);
	    strcat(pathname, "/");
	}
    }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SimTreeSrTiles
 *
 * Similar to the procedure DBTreeSrTiles.
 *
 * Recursively search downward from the supplied CellUse for
 * all visible paint tiles matching the supplied type mask.
 *
 * The procedure should be of the following form:
 *	int
 *	func(tile, cxp)
 *	    Tile *tile;
 *	    TreeContext *cxp;
 *	{
 *	}
 *
 * The SearchContext is stored in cxp->tc_scx, and the user's arg is stored
 * in cxp->tc_filter->tf_arg.
 *
 * In the above, the scx transform is the net transform from the coordinates
 * of tile to "world" coordinates (or whatever coordinates the initial
 * transform supplied to SimTreeSrTiles was a transform to).  Func returns
 * 0 under normal conditions.  If 1 is returned, it is a request to
 * abort the search.
 *
 *			*** WARNING ***
 *
 * The client procedure should not modify any of the paint planes in
 * the cells visited by SimTreeSrTiles, because we use DBSrPaintArea
 * instead of TiSrArea as our paint-tile enumeration function.
 *
 * Results:
 *	0 is returned if the search finished normally.  1 is returned
 *	if the search was aborted.
 *
 * Side effects:
 *	Whatever side effects are brought about by applying the
 *	procedure supplied.
 *
 *-----------------------------------------------------------------------------
 */

int
SimTreeSrTiles(scx, mask, xMask, func, cdarg)
    SearchContext *scx;		/* Pointer to search context specifying
				 * a cell use to search, an area in the
				 * coordinates of the cell's def, and a
				 * transform back to "root" coordinates.
				 */
    TileTypeBitMask *mask;	/* Only tiles with a type for which
				 * a bit in this mask is on are processed.
				 */
    int xMask;			/* All subcells are visited recursively
				 * until we encounter uses whose flags,
				 * when anded with xMask, are not
				 * equal to xMask.
				 */
    int (*func)();		/* Function to apply at each qualifying tile */
    ClientData cdarg;		/* Client data for above function */
{
    int SimCellTileSrFunc();
    TreeContext context;
    TreeFilter filter;
    CellUse *cellUse = scx->scx_use;
    CellDef *def = cellUse->cu_def;
    int pNum;

    ASSERT(def != (CellDef *) NULL, "SimTreeSrTiles");
    if (!DBIsExpand(cellUse, xMask))
	return 0;

    if ((def->cd_flags & CDAVAILABLE) == 0)
	if (!DBCellRead(def, (char *) NULL, TRUE)) return 0;

    filter.tf_func = func;
    filter.tf_arg = cdarg;
    filter.tf_mask = mask;
    filter.tf_xmask = xMask;
    filter.tf_planes = DBTechTypesToPlanes(mask);

    context.tc_scx = scx;
    context.tc_filter = &filter;

    /*
     * Apply the function first to any of the tiles in the planes
     * for this CellUse's CellDef that match the mask.
     */

    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (PlaneMaskHasPlane(filter.tf_planes, pNum))
	{
	    if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
		    &scx->scx_area, mask, func, (ClientData) &context))
		return 1;
	}

    /*
     * Now apply ourselves recursively to each of the CellUses
     * in our tile plane.
     */

    if (DBCellSrArea(scx, SimCellTileSrFunc, (ClientData) &filter)) {
	return 1;
    }
    else { 
	return 0;
    }
}

/*
 * Filter procedure applied to subcells by SimTreeSrTiles().
 */

int
SimCellTileSrFunc(scx, fp)
    register SearchContext *scx;
    register TreeFilter *fp;
{
    TreeContext context;
    CellDef *def = scx->scx_use->cu_def;
    int pNum;

    ASSERT(def != (CellDef *) NULL, "SimCellTileSrFunc");
    if (!DBIsExpand(scx->scx_use, fp->tf_xmask))
	return 0;
    if ((def->cd_flags & CDAVAILABLE) == 0)
	if (!DBCellRead(def, (char *) NULL, TRUE)) return 0;

    context.tc_scx = scx;
    context.tc_filter = fp;
    SimPushScx(scx);

    /*
     * Apply the function first to any of the tiles in the planes
     * for this CellUse's CellDef that match the mask.
     */

    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (PlaneMaskHasPlane(fp->tf_planes, pNum))
	{
	    if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
		    &scx->scx_area, fp->tf_mask,
		    fp->tf_func, (ClientData) &context)) {
		SimPopScx();
		return 1;
	    }
	}

    /*
     * Now apply ourselves recursively to each of the CellUses
     * in our tile plane.
     */

    if (DBCellSrArea(scx, SimCellTileSrFunc, (ClientData) fp)) {
	SimPopScx();
	return 1;
    }
    else {
	SimPopScx();
	return 0;
    }
}





/*
 * ----------------------------------------------------------------------------
 *
 * SimPutLabel --
 *
 * Same as DBPutLabel, except this does not set the cell modified flag.
 *
 * Place a rectangular label in the database, in a particular cell.
 *
 * It is the responsibility of higher-level routines to insure that
 * the material to which the label is being attached really exists at
 * this point in the cell, and that TT_SPACE is used if there is
 * no single material covering the label's entire area.  The routine
 * DBAdjustLabels is useful for this.
 *
 * Results:
 *	The return value is the actual alignment position used for
 *	the label.  This may be different from align, if align is
 *	defaulted.
 *
 * Side effects:
 *	Updates the label list in the CellDef to contain the label.
 *
 * ----------------------------------------------------------------------------
 */

int
SimPutLabel(cellDef, rect, align, text, type)
    CellDef *cellDef;	/* Cell in which label is placed */
    Rect *rect;		/* Location of label; see above for description */
    int align;		/* Orientation/alignment of text.  If this is < 0,
			 * an orientation will be picked to keep the text
			 * inside the cell boundary.
			 */
    char *text;		/* Pointer to actual text of label */
    TileType type;	/* Type of tile to be labelled */
{
    register Label *lab;
    int len, x1, x2, y1, y2, tmp, labx, laby;

    len = strlen(text) + sizeof (Label) - sizeof lab->lab_text + 1;
    MALLOC(Label *, lab, (unsigned) len);
    strcpy(lab->lab_text, text);

    /* Pick a nice alignment if the caller didn't give one.  If the
     * label is more than BORDER units from an edge of the cell,
     * use GEO_NORTH.  Otherwise, put the label on the opposite side
     * from the boundary, so it won't stick out past the edge of
     * the cell boundary.
     */
    
#define BORDER 5
    if (align < 0)
    {
	tmp = (cellDef->cd_bbox.r_xtop - cellDef->cd_bbox.r_xbot)/3;
	if (tmp > BORDER) tmp = BORDER;
	x1 = cellDef->cd_bbox.r_xbot + tmp;
	x2 = cellDef->cd_bbox.r_xtop - tmp;
	tmp = (cellDef->cd_bbox.r_ytop - cellDef->cd_bbox.r_ybot)/3;
	if (tmp > BORDER) tmp = BORDER;
	y1 = cellDef->cd_bbox.r_ybot + tmp;
	y2 = cellDef->cd_bbox.r_ytop - tmp;
	labx = (rect->r_xtop + rect->r_xbot)/2;
	laby = (rect->r_ytop + rect->r_ybot)/2;

	if (labx <= x1)
	{
	    if (laby <= y1) align = GEO_NORTHEAST;
	    else if (laby >= y2) align = GEO_SOUTHEAST;
	    else align = GEO_EAST;
	}
	else if (labx >= x2)
	{
	    if (laby <= y1) align = GEO_NORTHWEST;
	    else if (laby >= y2) align = GEO_SOUTHWEST;
	    else align = GEO_WEST;
	}
	else
	{
	    if (laby <= y1) align = GEO_NORTH;
	    else if (laby >= y2) align = GEO_SOUTH;
	    else align = GEO_NORTH;
	}
    }

    lab->lab_pos = align;
    lab->lab_type = type;
    lab->lab_rect = *rect;
    lab->lab_next = NULL;
    if (cellDef->cd_labels == NULL)
	cellDef->cd_labels = lab;
    else
    {
	ASSERT(cellDef->cd_lastLabel->lab_next == NULL, "SimPutLabel");
	cellDef->cd_lastLabel->lab_next = lab;
    }
    cellDef->cd_lastLabel = lab;

    DBUndoPutLabel(cellDef, rect, align, text, type);
    return align;
}



/*
 * ----------------------------------------------------------------------------
 *
 * SimRsimHandler
 *
 * 	This procedure is the button handler for the rsim tool.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Left button:  used to move the whole box by the lower-left corner.
 *	Right button: used to re-size the box by its upper-right corner.
 *		If one of the left or right buttons is pushed, then the
 *		other is pushed, the corner is switched to the nearest
 *		one to the cursor.  This corner is remembered for use
 *		in box positioning/sizing when both buttons have gone up.
 *	Middle button: used to display the rsim node values of whatever
 *		paint is selected.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
void
SimRsimHandler(w, cmd)
    Window *w;			/* Window containing cursor. */
    TxCommand *cmd;		/* Describes what happened. */
{

    static int buttonCorner = TOOL_ILG;
    int button = cmd->tx_button;

    if (button == TX_MIDDLE_BUTTON)
    {
	if (cmd->tx_buttonAction == TX_BUTTON_DOWN)
	    SimRsimMouse(w);
	return;
    }

    if (cmd->tx_buttonAction == TX_BUTTON_DOWN)
    {
	if ((WindNewButtons & (TX_LEFT_BUTTON|TX_RIGHT_BUTTON))
		== (TX_LEFT_BUTTON|TX_RIGHT_BUTTON))
	{
	    /* Both buttons are now down.  In this case, the FIRST
	     * button pressed determines whether we move or size,
	     * and the second button is just used as a signal to pick
	     * the closest corner.
	     */

	    buttonCorner = ToolGetCorner(&cmd->tx_p);
	    if (button == TX_LEFT_BUTTON) button = TX_RIGHT_BUTTON;
	    else button = TX_LEFT_BUTTON;
	}
	else if (button == TX_LEFT_BUTTON) buttonCorner = TOOL_BL;
	else buttonCorner = TOOL_TR;
	dbwButtonSetCursor(button, buttonCorner);
    }
    else
    {
	/* A button has just come up.  If both buttons are down and one
	 * is released, we just change the cursor to reflect the current
	 * corner and the remaining button (i.e. move or size box).
	 */

	if (WindNewButtons != 0)
	{
	    if (button == TX_LEFT_BUTTON)
		dbwButtonSetCursor(TX_RIGHT_BUTTON, buttonCorner);
	    else dbwButtonSetCursor(TX_LEFT_BUTTON, buttonCorner);
	    return;
	}

	/* The last button has been released.  Reset the cursor to normal
	 * form and then move or size the box.
	 */

	GrSetCursor(STYLE_CURS_RSIM);
	switch (button)
	{
	    case TX_LEFT_BUTTON:
		ToolMoveBox(buttonCorner, &cmd->tx_p, TRUE, (CellDef *) NULL);
		break;
	    case TX_RIGHT_BUTTON:
		ToolMoveCorner(buttonCorner, &cmd->tx_p, TRUE,
			(CellDef *) NULL);
	}
    }
}
