/*
 * mzBlock.c --
 *
 * Construction of blockage planes.
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
 * A blockage plane is used to determine the legal areas for routing.
 * Each point on the interior of a space tile in a blockage plane is
 * a legal position to place the lower-left corner of a piece of wiring.
 *
 * To build a blockage plane, each solid mask tile in the 
 * layout is bloated in
 * all four directions and painted into the blockage plane.  To the
 * top and right, it is only bloated by the minimum separation from
 * that tile to routing on that plane (s).  To the bottom and left,
 * though, it is bloated by this distance PLUS one less than the
 * minimum width of routing on that layer (w-1):
 *
 *
 *
 *		  s+w-1			 s
 *		<------->	        <-->
 *		+---------------------------+	^
 *		|			    |	| s
 *		|	+---------------+   |	v
 *		|	|		|   |
 *		|	|     tile	|   |
 *		|	|		|   |
 *		|	+---------------+   |	^
 *		|			    |	|
 *		|			    |	| s+w-1
 *		|			    |	|
 *		+---------------------------+	v
 *
 * Blockage planes are kept for each routeLayer and routeContact.  They
 * are part of ther RouteType datastructure.
 *
 * There are two blockage planes for each routeType.  One is merged into
 * maximal horizontal strips, and the other
 * into maximal vertical strips.
 */

#ifndef lint
static char rcsid[] = "$Header: mzBlock.c,v 6.1 90/08/28 19:34:51 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "signals.h"
#include "textio.h"
#include "windows.h"
#include "dbwind.h"
#include "malloc.h"
#include "list.h"
#include "debug.h"
#include "textio.h"
#include "doubleint.h"
#include "heap.h"
#include "mzrouter.h"
#include "mzInternal.h"


/*
 * ----------------------------------------------------------------------------
 *
 * mzBuildMaskDataBlocks --
 *
 * Build blockage info from paint in buildArea.
 *
 * The design rules are used 
 * to build a map of blocked areas in blockage planes of RouteTypes.
 * This map will consist of TT_SPACE tiles, where a zero-width
 * path will yield a 
 * legal route when flushed out to wire width paint, and various
 * types of block tiles.   Multiple block types are needed to handle route 
 * termination at the destination node properly.
 *
 * TT_BLOCKED and TT_SAMENODE_BLOCK are generated from the mask data
 * (SAMENODE_BLOCK areas are only blocked by the destination node itself).
 *
 * In addition to blockage planes for route layers, there are
 * blockage planes for contacts, assumed to connect two planes.  
 * Both planes of mask information 
 * are effectively AND-ed together to form the blockage plane for
 * that type of contact.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints into Blockage planes in RouteTypes
 *
 * ----------------------------------------------------------------------------
 */

mzBuildMaskDataBlocks(buildArea)
    Rect *buildArea;	/* Area over which blockage planes will be built */
{
    Rect searchArea;

    /* search area  = build area + context */
    searchArea.r_xbot = buildArea->r_xbot - mzContextRadius;
    searchArea.r_ybot = buildArea->r_ybot - mzContextRadius;
    searchArea.r_xtop = buildArea->r_xtop + mzContextRadius;
    searchArea.r_ytop = buildArea->r_ytop + mzContextRadius;

    /* Build the blockage planes on all layers, ignore unexpanded subcells
     * (unexpanded in reference window)
     */
    {
	int mzBuildBlockFunc();
	SearchContext scx;

	scx.scx_area = searchArea;
	scx.scx_trans = GeoIdentityTransform;
	scx.scx_use = mzRouteUse;

	(void) DBTreeSrTiles(
			     &scx, 
			     &DBAllButSpaceAndDRCBits, 
			     mzCellExpansionMask,
			     mzBuildBlockFunc, 
			     (ClientData) buildArea);
    }

    /* Add blocks at unexpanded subcells on all blockage planes 
     *
     * NOTE: A 0 expansion mask is special cased since 
     *     the mzrotuer interpets a 0 mask to mean all subcells are 
     *     expanded,
     *     while DBTreeSrCells() takes a 0 mask to mean all subcells are
     *     unexpanded.
     */
    if(mzCellExpansionMask != 0)
    {
	int mzBlockSubcellsFunc();
	SearchContext scx;

	scx.scx_area = searchArea;
	scx.scx_trans = GeoIdentityTransform;
	scx.scx_use = mzRouteUse;

	DBTreeSrCells(
		      &scx,
		      mzCellExpansionMask,
		      mzBlockSubcellsFunc,
		      (ClientData) buildArea);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzBuildBlockFunc --
 *
 * Filter function called via DBTreeSrTiles on behalf of mzBuildBlock()
 * above, for each solid tile in the area of interest.  Paints TT_BLOCKED
 * (TT_SAMENODE_BLOCKED if the tile is marked) areas on each of the planes 
 * affected by this tile.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Paints into Blockage Planes in RouteType structures.
 *
 * ----------------------------------------------------------------------------
 */

int
mzBuildBlockFunc(tile, cxp)

    register Tile *tile;
    register TreeContext *cxp;
{
    register SearchContext *scx = cxp->tc_scx;
    register Rect *buildArea = (Rect *) (cxp->tc_filter->tf_arg);
    Rect r, rDest;

    /* Transform to result coordinates */
    TITORECT(tile, &r);
    GEOCLIP(&r, &scx->scx_area);
    GEOTRANSRECT(&scx->scx_trans, &r, &rDest);

    if((int)(tile->ti_client) == MINFINITY)
    /* tile not part of dest node, paint normal blocks onto affected
     * planes.
     * ( area is bloated by appropriate spacing on each affected plane)
     */
    {
	mzPaintBlock(&rDest, TiGetType(tile), buildArea);
    }
    else
    /* tile is part of dest node, paint samenode block areas */
    {
	mzPaintSameNodeBlock(&rDest, TiGetType(tile), buildArea);
    }

    return (0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzBlockSubcellsFunc --
 *
 * Filter function called via DBTreeSrTiles on behalf of mzBuildBlock()
 * above, for each unexpanded subcell in the area of interest, 
 * a "blocked" area (TT_BLOCKED) is painted on each blockage plane for
 * the bounding box of the subcell, bloated by the maximum design rule
 * spacing on that plane.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Paints into Blockage Planes in RouteType structures.
 *
 * ----------------------------------------------------------------------------
 */

int
mzBlockSubcellsFunc(scx, cdarg)
    register SearchContext *scx;
    register ClientData cdarg;
{
    Rect r, rDest;
    register Rect *buildArea = (Rect *) cdarg;

    /* Transform bounding box to result coords */
    r = scx->scx_use->cu_def->cd_bbox;
    GEOTRANSRECT(&scx->scx_trans, &r, &rDest);

    if((int)(scx->scx_use->cu_client) == MINFINITY)
    /* cell over part of dest node, paint normal blocks onto affected
     * planes.
     * (area is bloated by appropriate spacing on each affected plane)
     */
    {
	mzPaintBlock(&rDest, TT_SUBCELL, buildArea);
    }
    else
    /* cell is over part of dest node, paint samenode blocks */
    {
	mzPaintSameNodeBlock(&rDest, TT_SUBCELL, buildArea);
    }

    return (0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzPaintBlock --
 *
 * Add TT_BLOCKED paint appropriate to data tile of given location
 * and type to blockage planes of all relevant routeTypes.
 *
 * The blockage tiles painted are bloated versions of the data tiles.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies blockage planes in RouteType structures.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzPaintBlock(r, type, buildArea)
    register Rect *r;
    register TileType type;
    register Rect *buildArea;
{
    register RouteType *rT;

    /* process routeTypes  */
    for (rT=mzActiveRTs; rT!=NULL; rT=rT->rt_nextActive)
    {
	Rect rblock;

	/* if there is a constraint between RouteType and type, 
	 * paint a block 
	 */
	if (rT->rt_bloatBot[type]>=0)
	{
	    /* Compute blockage rectangle */
	    int bot = rT->rt_bloatBot[type];
	    int top = rT->rt_bloatTop[type];
	    rblock.r_xbot = r->r_xbot - bot;
	    rblock.r_ybot = r->r_ybot - bot;
	    rblock.r_xtop = r->r_xtop + top;
	    rblock.r_ytop = r->r_ytop + top;

	    /* clip to build area */
	    GEOCLIP(&rblock, buildArea);

	    /* and paint it */
	    {
		DBPaintPlane(rT->rt_hBlock, 
			     &rblock, 
			     mzBlockPaintTbl[TT_BLOCKED],
			     (PaintUndoInfo *) NULL);
		DBPaintPlaneVert(rT->rt_vBlock, 
				 &rblock, 
				 mzBlockPaintTbl[TT_BLOCKED],
				 (PaintUndoInfo *) NULL);
	    }

	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzPaintSameNodeBlock --
 *
 * Add TT_SAMENODE_BLOCK paint appropriate to data tile of given location
 * and type to blockage planes of all relevant routeTypes.
 *
 * The blockage tiles painted are bloated versions of the data tiles.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies blockage planes in RouteType structures.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzPaintSameNodeBlock(r, type, buildArea)
    register Rect *r;
    register TileType type;
    register Rect *buildArea;
{
    register RouteType *rT;

    /* process routeTypes  */
    for (rT=mzActiveRTs; rT!=NULL; rT=rT->rt_nextActive)
    {
	Rect rblock;

	/* if there is a constraint between RouteType and type, 
	 * paint a block 
	 */
	if (rT->rt_bloatBot[type]>=0)
	{
	    /* Compute blockage rectangle */
	    int bot = rT->rt_bloatBot[type];
	    int top = rT->rt_bloatTop[type];
	    rblock.r_xbot = r->r_xbot - bot;
	    rblock.r_ybot = r->r_ybot - bot;
	    rblock.r_xtop = r->r_xtop + top;
	    rblock.r_ytop = r->r_ytop + top;

	    /* clip to build area */
	    GEOCLIP(&rblock, buildArea);

	    /* and paint it */
	    {
		DBPaintPlane(rT->rt_hBlock, 
			     &rblock, 
			     mzBlockPaintTbl[TT_SAMENODE_BLOCK],
			     (PaintUndoInfo *) NULL);
		DBPaintPlaneVert(rT->rt_vBlock, 
				 &rblock, 
				 mzBlockPaintTbl[TT_SAMENODE_BLOCK],
				 (PaintUndoInfo *) NULL);
	    }

	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzBuildFenceBlocks -- 
 *
 * Blocks regions of fence parity opposite of the destination-point.
 * (Fence boundaries can not be crossed).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blockage Planes modified.
 *
 * ----------------------------------------------------------------------------
 */

mzBuildFenceBlocks(buildArea)
    register Rect *buildArea;		/* Area over which planes modified */
{
    int mzBuildFenceBlocksFunc();
    Rect searchArea;


    /* search area  = build area + context */
    searchArea.r_xbot = buildArea->r_xbot - mzContextRadius;
    searchArea.r_ybot = buildArea->r_ybot - mzContextRadius;
    searchArea.r_xtop = buildArea->r_xtop + mzContextRadius;
    searchArea.r_ytop = buildArea->r_ytop + mzContextRadius;

    /* If route inside fence, gen blocks at space tiles.
     * if route outside fence, gen blocks at fence tiles.
     */
    if(mzInsideFence)
    {
	DBSrPaintArea(NULL,	/* no hint tile */
		      mzHFencePlane, 
		      &searchArea, 
		      &DBSpaceBits,
		      mzBuildFenceBlocksFunc, 
		      (ClientData) buildArea);
    }
    else
    {
	DBSrPaintArea(NULL,	/* no hint tile */
		      mzHFencePlane, 
		      &searchArea, 
		      &DBAllButSpaceBits,
		      mzBuildFenceBlocksFunc, 
		      (ClientData) buildArea);
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzBuildFenceBlocksFunc --
 *
 * Called by DBSrPaintArea for tile in given area on fence plane where routing
 * is prohibited.  These areas are blocked,
 * adjusting for wire width on each blockage plane.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Paints into blockage planes.
 *
 * ----------------------------------------------------------------------------
 */

int
mzBuildFenceBlocksFunc(tile, buildArea)
    register Tile *tile;
    register Rect *buildArea; /* clip to this area before painting */
{
    register RouteType *rT;
    register int d;
    Rect r, rAdjusted; 

    /* Get boundary of tile */
    TITORECT(tile, &r);

    for (rT=mzActiveRTs; rT!=NULL; rT=rT->rt_nextActive)
    {
	/* adjust to compensate for wire width */
	d = rT->rt_effWidth - 1;
	rAdjusted.r_xbot = r.r_xbot - d;
	rAdjusted.r_xtop = r.r_xtop;
	rAdjusted.r_ybot = r.r_ybot - d;
	rAdjusted.r_ytop = r.r_ytop;

	/* clip to area being generated. */
	GEOCLIP(&rAdjusted, buildArea);

	/* Paint into blockage planes */
	DBPaintPlane(rT->rt_hBlock, 
		     &rAdjusted, 
		     mzBlockPaintTbl[TT_BLOCKED], 
		     (PaintUndoInfo *) NULL);
	DBPaintPlaneVert(rT->rt_vBlock, 
			 &rAdjusted, 
			 mzBlockPaintTbl[TT_BLOCKED], 
			 (PaintUndoInfo *) NULL);
    }
   
    /* return 0 - to continue search */
    return(0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendBlockBoundsR --
 *
 * Generate blockage information around given rect.
 * Info required for radius of twice the bounds-increment around the rect.
 * The gen area is broken down into areas which have not already been gened.
 * Blockage info is generated for these subregions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints into Blockage planes in RouteTypes
 *	Updates bounds plane.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzExtendBlockBoundsR(rect)
    Rect *rect;
{
    Rect area;
    TileTypeBitMask genMask;
    /* Generate twice the required bounds increment, so we don't have
     * to regenerate as soon as we move from the center of the newly
     * generated region. 
     */
    int genBoundsIncrement = mzBoundsIncrement * 2;
    int mzExtendBlockFunc();

    /* keep stats */
    mzBlockGenCalls++;

    /* mark area about rect in which blockage plane required to be present */
    area.r_xbot = rect->r_xbot - genBoundsIncrement;
    area.r_ybot = rect->r_ybot - genBoundsIncrement;
    area.r_xtop = rect->r_xtop + genBoundsIncrement;
    area.r_ytop = rect->r_ytop + genBoundsIncrement;


    DBPaintPlane(mzHBoundsPlane, 
	    &area,
	    mzBoundsPaintTbl[TT_GENBLOCK],
	    (PaintUndoInfo *) NULL);

    /* Generate blockage planes under each GENBLOCK tile = regions in
     * new area where blockage planes not previously generated 
    */
    TTMaskZero(&genMask);
    TTMaskSetType(&genMask,TT_GENBLOCK);
    DBSrPaintArea(NULL,         /* no hint tile */ 
       mzHBoundsPlane,
       &area,
       &genMask,
       mzExtendBlockFunc,
       (ClientData) NULL);

    /* Paint area INBOUNDS in both bounds planes 
     *(blockage planes now generated here) 
     */
    DBPaintPlane(mzHBoundsPlane, 
	    &area,
	    mzBoundsPaintTbl[TT_INBOUNDS],
	    (PaintUndoInfo *) NULL);
    DBPaintPlaneVert(mzVBoundsPlane, 
	    &area,
	    mzBoundsPaintTbl[TT_INBOUNDS],
	    (PaintUndoInfo *) NULL);

    return;
}

#define RECT_AREA(r) \
    ((double)(r.r_xtop - r.r_xbot)*(double)(r.r_ytop - r.r_ybot))


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendBlockFunc --
 *
 * Called by DBSrPaintArea for rectangles where blockage info must be 
 * generated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints into Blockage planes in RouteTypes
 *
 * ----------------------------------------------------------------------------
 */
    /*ARGSUSED*/
int
mzExtendBlockFunc(tile, cdarg)
    register Tile *tile;
    ClientData cdarg;
{
    Rect area;

    /* Get location of tile */
    TITORECT(tile, &area);

    /* Don't actually generate blocks outside of user supplied
     * route area hint.
     */
    if(mzBoundsHint)
    {
	GEOCLIP(&area,mzBoundsHint);
	if((area.r_xbot > area.r_xtop) || (area.r_ybot > area.r_ytop))
	/*no overlap just return */
	{
	    return 0;
	}
    }

    /* Grow clip area by 2 units to take care of boundary conditions. */
    area.r_xbot -= 2;
    area.r_xtop += 2;
    area.r_ybot -= 2;
    area.r_ytop += 2;

    mzBuildMaskDataBlocks(&area);
    mzBuildFenceBlocks(&area);

    /* keep stats */
    mzBlockGenArea += RECT_AREA(area);

    /* return 0 to continue search */
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzExtendBlockBounds --
 *
 * Generate blockage information around given point.
 * Info required for radius of twice the bounds-increment around the point.
 * The gen area is broken down into areas which have not already been gened.
 * Blockage info is generated for these subregions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints into Blockage planes in RouteTypes
 *	Updates bounds plane.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzExtendBlockBounds(point)
    Point *point;
{
    Rect rect;
    
    rect.r_ll = *point;
    rect.r_ur = *point;

    mzExtendBlockBoundsR(&rect);

    return;
}

/* 
 * This struc is used to store generated walks, since it is necessary
 * to defer painting walks until all are generated.
 */
typedef struct
{
    RouteType * w_rT;
    Rect 	w_rect;
    TileType	w_type;
} Walk;

/* global used to store walks prior to painting them */
List *mzWalkList;


/*
 * ----------------------------------------------------------------------------
 *
 * mzBuildDestAreaBlocks --
 *
 * Generate blockage information around all dest areas, and do special dest
 * area processing (generation of dest area blocks and walks).  In addition
 * dest area alignment coords are added to the alignment structures.
 *
 * TT_DEST_AREA and TT_*_WALK regions are generated from the special dest area
 * cell.  Dest areas are regions to connect to and walks are regions blocked
 * only by dest nodes and directly adjacent to a dest area.  Walks are 
 * routed through by special termination code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies blockage planes.
 *	Modifies alignment strucs.
 *	Updates bounds plane.
 *
 * ----------------------------------------------------------------------------
 */

Void
mzBuildDestAreaBlocks()
{

    /* initialize global walk list */
    mzWalkList = NULL;

    /* Compute bounding box for dest areas cell */
    DBReComputeBbox(mzDestAreasUse->cu_def);

    /* Trim top and right edge of dest areas to make sure routes terminate
     * entirely inside dest area.
     */
    {
	RouteLayer *rL;

	for(rL=mzActiveRLs; rL!=NULL; rL=rL->rl_nextActive)
	{
	    mzTrimDestAreas(rL);
	}
    }

    /* Process dest areas in dest area cell one by one.  
     *     - generates normal blockage info.
     *     - paints dest area into appropriate blockage planes.
     *     - generates alignments.
     *     - generates list of walks.
     *     
     * Walks are not actually painted yet because existing 
     * walks can interfere with the generation of new ones.
     */
    {
	int mzDestAreaFunc();
	SearchContext scx;

	scx.scx_area = mzBoundingRect;
	scx.scx_trans = GeoIdentityTransform;
	scx.scx_use = mzDestAreasUse;
	
	/* clip area to bounding box to avoid overflow during transforms */
	GEOCLIP(&(scx.scx_area),&(mzDestAreasUse->cu_def->cd_bbox));

	/* process, one dest area at a time */
	(void) DBTreeSrTiles(
			     &scx, 
			     &DBAllButSpaceAndDRCBits, 
			     0, 
			     mzDestAreaFunc, 
			     (ClientData) NULL);
    }

    /* Actually paint walks into blockage planes
     * (and dealloc walk list)
     */
    {
	List *l;

	for(l = mzWalkList; l!= NULL; l=LIST_TAIL(l))
	{
	    Walk *walk = (Walk *) LIST_FIRST(l);

		
	    DBPaintPlane(walk->w_rT->rt_hBlock, 
			 &(walk->w_rect),
			 mzBlockPaintTbl[walk->w_type],
			 (PaintUndoInfo *) NULL);

	    DBPaintPlaneVert(walk->w_rT->rt_vBlock, 
			     &(walk->w_rect),
			     mzBlockPaintTbl[walk->w_type],
			     (PaintUndoInfo *) NULL);
	}

	ListDeallocC(mzWalkList);
    }

    /* Finally generate CONTACT_WALKS for all dest areas. 
     *
     * CONTACT_WALK(s) are painted only directly above and below
     * actual dest areas where contacts are possible.
     */
    {
	int mzCWalksFunc();
	SearchContext scx;

	scx.scx_area = mzBoundingRect;
	scx.scx_trans = GeoIdentityTransform;
	scx.scx_use = mzDestAreasUse;
	
	/* clip area to bounding box to avoid overflow during transforms */
	GEOCLIP(&(scx.scx_area),&(mzDestAreasUse->cu_def->cd_bbox));

	/* process, one dest area at a time */
	(void) DBTreeSrTiles(
			     &scx, 
			     &DBAllButSpaceAndDRCBits, 
			     0, 
			     mzCWalksFunc, 
			     (ClientData) NULL);
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzDestAreaFunc --
 *
 * Pregen blockage info around dest area, and then do special dest area 
 * processing.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Generates blockage info.
 *
 * Results:
 *	Returns 0 always.
 *
 * ----------------------------------------------------------------------------
 */

int
mzDestAreaFunc(tile, cxp)
    register Tile *tile;
    register TreeContext *cxp;
{
    register SearchContext *scx = cxp->tc_scx;
    TileType type = TiGetType(tile);
    Rect r, rect;
    RouteType *rT;

    /* Transform to result coordinates */
    TITORECT(tile, &r);
    GEOTRANSRECT(&scx->scx_trans, &r, &rect);

    /* gen normal blockage info around this area */
    mzExtendBlockBoundsR(&rect);
    
    /* find route type for this dest area */
    {
	rT = mzActiveRTs;
	while ((rT->rt_tileType != type) && (rT!=NULL))
	{
	    rT = rT->rt_nextActive;
	}
	ASSERT(rT!=NULL,"mzDestAreaFunc");
    }
	
    /* paint dest area into blockage planes of appropriate route type */
    {
	DBPaintPlane(rT->rt_hBlock, 
		     &rect, 
		     mzBlockPaintTbl[TT_DEST_AREA],
		     (PaintUndoInfo *) NULL);
	DBPaintPlaneVert(rT->rt_vBlock, 
			 &rect,
			 mzBlockPaintTbl[TT_DEST_AREA],
			 (PaintUndoInfo *) NULL);
    }

     /* Gen alignments and walks for dest areas.
      *
      * Since the above dest area may be partially blocked, the blockage
      * planes are searched for DEST_AREA tiles underneath
      * the dest area painted above to build lists of dest tiles.
      *
      * walks are accumulated into a list for painting after ALL walks
      * have been calculated.
      *
      */
    {
	int mzHWalksFunc();
	int mzVWalksFunc();
	TileTypeBitMask destAreaMask;
	
	TTMaskSetOnlyType(&destAreaMask, TT_DEST_AREA);

	DBSrPaintArea(NULL,	/* no hint tile */
		      rT->rt_hBlock,
		      &rect, 
		      &destAreaMask,
		      mzHWalksFunc, 
		      (ClientData) rT);

	DBSrPaintArea(NULL,	/* no hint tile */
		      rT->rt_vBlock,
		      &rect, 
		      &destAreaMask,
		      mzVWalksFunc, 
		      (ClientData) rT);
    }

    /* continue with next dest area */
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzHWalksFunc --
 *
 * Generate walks to top and bottom, and alignment y-coords for TT_DESTAREA
 * (Walks are added to list for painting after search completes.)
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Adds to walks list and alignment structure.
 *
 * ----------------------------------------------------------------------------
 */

int
mzHWalksFunc(tile, cdarg)
    register Tile *tile;
    ClientData cdarg;
{
    RouteType *rT = (RouteType *) cdarg;

    /* Add dest area coordinates to dest alignment structure */
    {
	mzNLInsert(&mzXAlignNL, LEFT(tile));
	mzNLInsert(&mzXAlignNL, RIGHT(tile));
    }

    /* compute LEFT_WALK(s) */
    {
	Walk *walk;
	Tile *tLeft = tile->ti_bl;
	
	/* Build walks for blocks to left of tile */
	while(BOTTOM(tLeft)<TOP(tile))
	{

	    if(TiGetType(tLeft)==TT_SAMENODE_BLOCK)
	    {
		MALLOC(Walk *, walk, sizeof(Walk));
		walk->w_rT = rT; 
		walk->w_type = TT_LEFT_WALK;
		walk->w_rect.r_ybot = MAX(BOTTOM(tile),BOTTOM(tLeft));
		walk->w_rect.r_ytop = MIN(TOP(tile),TOP(tLeft));
		walk->w_rect.r_xtop = RIGHT(tLeft);
		walk->w_rect.r_xbot = MAX(LEFT(tLeft),
					   RIGHT(tLeft)-mzMaxWalkLength);
		LIST_ADD(walk, mzWalkList);
	    }
	    
	    /* move to next tile up */
	    tLeft = tLeft->ti_rt;
	}
    }

    /* compute RIGHT_WALK */
    {
	Walk *walk;
	Tile *tRight = tile->ti_tr;

	/* Build walks for blocks to right of tile */
	while(TOP(tRight)>BOTTOM(tile))
	{
	   if(TiGetType(tRight)==TT_SAMENODE_BLOCK)
	   {
	       MALLOC(Walk *, walk, sizeof(Walk));
	       walk->w_rT = rT; 
	       walk->w_type = TT_RIGHT_WALK;
	       walk->w_rect.r_ybot = MAX(BOTTOM(tile),BOTTOM(tRight));
	       walk->w_rect.r_ytop = MIN(TOP(tile),TOP(tRight));
	       walk->w_rect.r_xbot = LEFT(tRight);
	       walk->w_rect.r_xtop = MIN(RIGHT(tRight), 
					  LEFT(tRight)+mzMaxWalkLength);
	       LIST_ADD(walk, mzWalkList);
	   }

	   /* move to next tile down */
	   tRight = tRight->ti_lb;
       }
    }
   
    /* return 0 - to continue search */
    return(0);
}



/*
 * ----------------------------------------------------------------------------
 *
 * mzVWalksFunc --
 *
 * Generate walks to top and bottom, and alignment y-coords for TT_DESTAREA
 * (Walks are added to list for painting after search completes.)
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Adds to walks list and alignment structure.
 *
 * ----------------------------------------------------------------------------
 */

int
mzVWalksFunc(tile, cdarg)
    register Tile *tile;
    ClientData cdarg;
{
    RouteType  *rT = (RouteType *) cdarg;

    /* Add dest area coordinates to dest alignment structure */
    {
	mzNLInsert(&mzYAlignNL, BOTTOM(tile));
	mzNLInsert(&mzYAlignNL, TOP(tile));
    }

    /* compute BOTTOM_WALK */
    {
	Walk *walk;
	Tile *tBelow = tile->ti_lb;
	
	/* Build walks for blocks to below tile */
	while(LEFT(tBelow)<RIGHT(tile))
	{
	    if(TiGetType(tBelow)==TT_SAMENODE_BLOCK)
	    {
		MALLOC(Walk *, walk, sizeof(Walk));
		walk->w_rT = rT;
		walk->w_type = TT_BOTTOM_WALK;
		walk->w_rect.r_xbot = MAX(LEFT(tile),LEFT(tBelow));
		walk->w_rect.r_xtop = MIN(RIGHT(tile),RIGHT(tBelow));
		walk->w_rect.r_ytop = TOP(tBelow);
		walk->w_rect.r_ybot = MAX(BOTTOM(tBelow),
					   TOP(tBelow)-mzMaxWalkLength);
		LIST_ADD(walk, mzWalkList);
	    }

	    /* move to next tile to left */
	    tBelow = tBelow->ti_tr;
	}
    }

    /* compute TOP_WALK */
    {
	Walk *walk;
	Tile *tAbove = tile->ti_rt;
	
	
	/* Build walks for blocks above tile */
	while(RIGHT(tAbove)>LEFT(tile))
	{
	    if(TiGetType(tAbove)==TT_SAMENODE_BLOCK)
	    {
		MALLOC(Walk *, walk, sizeof(Walk));
		walk->w_rT = rT;
		walk->w_type = TT_TOP_WALK;
		walk->w_rect.r_xbot = MAX(LEFT(tile),LEFT(tAbove));;
		walk->w_rect.r_xtop = MIN(RIGHT(tile),RIGHT(tAbove));
		walk->w_rect.r_ybot = BOTTOM(tAbove);
		walk->w_rect.r_ytop = MIN(TOP(tAbove), 
					   BOTTOM(tAbove)+mzMaxWalkLength);
		LIST_ADD(walk, mzWalkList);
	    }

	    /* move to next tile to right */
	    tAbove = tAbove->ti_bl;
	}
    }
   
    /* return 0 - to continue search */
    return(0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzCWalksFunc --
 *
 * Paint contact walks.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Generates blockage info.
 *
 * Results:
 *	Returns 0 always.
 *
 * ----------------------------------------------------------------------------
 */

int
mzCWalksFunc(tile, cxp)
    register Tile *tile;
    register TreeContext *cxp;
{
    register SearchContext *scx = cxp->tc_scx;
    TileType type = TiGetType(tile);
    Rect r, rect;
    RouteType *rT;

    /* Transform to result coordinates */
    TITORECT(tile, &r);
    GEOTRANSRECT(&scx->scx_trans, &r, &rect);
    
    /* find route type for this dest area */
    {
	rT = mzActiveRTs;
	while ((rT->rt_tileType != type) && (rT!=NULL))
	{
	    rT = rT->rt_nextActive;
	}
	ASSERT(rT!=NULL,"mzDestAreaFunc");
    }

    /* Generate CONTACT_WALK(s) above and below dest areas.
     *
     * CONTACT_WALK(s) are painted only directly above and below
     * actual dest areas where contacts are possible.
     */
    {
	int mzCWalksFunc2();
	TileTypeBitMask destAreaMask;
	
	TTMaskSetOnlyType(&destAreaMask, TT_DEST_AREA);

	DBSrPaintArea(NULL,	/* no hint tile */
		      rT->rt_hBlock,
		      &rect, 
		      &destAreaMask,
		      mzCWalksFunc2, 
		      (ClientData) rT);
    }

    /* continue with next dest area */
    return 0;
}

/* 
 *  Struc to pass data between mzCWalksFunc2 and mzCWalksFunc3
 */
typedef struct walkContactFuncData
{
    Rect		*wd_bounds;	/* dest area bounds */
    RouteLayer 		*wd_rL;		/* Route layer of walk_contact */
} WalkContactFuncData;


/*
 * ----------------------------------------------------------------------------
 *
 * mzCWalksFunc2 --
 *
 * Search dest area for regions where contacts are ok, and paint
 * CONTACT_WALK(s) there in adjacent layers.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Paints into adjacent blocakge planes.
 *
 * ----------------------------------------------------------------------------
 */

int
mzCWalksFunc2(tile, cdarg)
    register Tile *tile;
    ClientData cdarg;
{
    RouteType *rT = (RouteType *) cdarg; /* RouteType of this dest area */
    RouteContact *rC;	
    Rect rect;

    /* set rect to boundary of this dest area */
    TITORECT(tile, &rect);

    /* process contact types that can connect to this dest area */
    for(rC=mzRouteContacts; rC!=NULL; rC=rC->rc_next)
    {
	RouteLayer *rLOther = NULL; 

	/* skip inactive contact types */
	if(!(rC->rc_routeType.rt_active)) continue;

	/* Find layer connecting to dest area FROM */
	if(&(rC->rc_rLayer1->rl_routeType) == rT)
	{
	    rLOther = rC->rc_rLayer2;
	}
	else if(&(rC->rc_rLayer2->rl_routeType) == rT)
	{
	    rLOther = rC->rc_rLayer1;
	}

	/* If current contact type (RC) permits connection to dest area, 
	 * insert CONTACT_WALK(s) into blockage planes for other layer.
	 */
	if(rLOther)
	{
	    /* process contact-OK tiles in blockage plane for this contact */
	    {
		int mzCWalksFunc3();
		WalkContactFuncData wD;
		TileTypeBitMask contactOKMask;

		TTMaskSetOnlyType(&contactOKMask, TT_SPACE);
		TTMaskSetType(&contactOKMask, TT_SAMENODE_BLOCK);

		wD.wd_bounds = &rect; 
		wD.wd_rL = rLOther;

		/* search for OK places for contact, and paint them */
		DBSrPaintArea(NULL,	/* no hint tile */
			      rC->rc_routeType.rt_hBlock,
			      &rect, 
			      &contactOKMask,
			      mzCWalksFunc3, 
			      (ClientData) &wD);
	    }
	}
    }
    
    /* return 0 - to continue search */
    return(0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * mzCWalksFunc3 --
 *
 * Called for tiles on contact plane where contact is OK that overlap a
 * dest area.  We paint a CONTACT_WALK on the blockage planes for the
 * other routeLayer, the one the contact connects to the dest area.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Paints into blockage planes for "other" route layer.
 *
 * ----------------------------------------------------------------------------
 */

int
mzCWalksFunc3(tile, cdarg)
    register Tile *tile;
    ClientData cdarg;
{
    WalkContactFuncData *wD = (WalkContactFuncData *) cdarg;
    Rect rect;

    /* rect = tile clipped dest area bounds. */
    TITORECT(tile, &rect);
    GEOCLIP(&rect, wD->wd_bounds);

    /* paint contact_walk into blockage planes */
    {
	DBPaintPlane(wD->wd_rL->rl_routeType.rt_hBlock, 
		     &rect, 
		     mzBlockPaintTbl[TT_CONTACT_WALK],
		     (PaintUndoInfo *) NULL);
	DBPaintPlaneVert(wD->wd_rL->rl_routeType.rt_vBlock, 
			 &rect,
			 mzBlockPaintTbl[TT_CONTACT_WALK],
			 (PaintUndoInfo *) NULL);
    }

    /* return 0 - to continue search */
    return(0);
}

