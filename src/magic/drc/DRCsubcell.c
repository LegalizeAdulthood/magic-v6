/*
 * DRCsubcell.c --
 *
 * This file provides the facilities for finding design-rule
 * violations that occur as a result of interactions between
 * subcells and either paint or other subcells.
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

#ifndef	lint
static char rcsid[] = "$Header: DRCsubcell.c,v 6.0 90/08/28 18:12:33 mayo Exp $";
#endif	not lint

#include <stdio.h>
#include <sys/types.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"
#include "windows.h"
#include "commands.h"
#include "undo.h"

/* The variables below are made owns so that they can be used to
 * pass information to the various search functions.
 */

static Rect drcSubIntArea;	/* Accumulates area of interactions. */
static CellDef *drcSubDef;	/* Cell definition we're checking. */
static int drcSubRadius;	/* Interaction radius. */
static CellUse *drcSubCurUse;	/* Holds current use when checking to see
				 * if more than one use in an area.
				 */
static Rect drcSubLookArea;	/* Area where we're looking for interactions */
static void (*drcSubFunc)();	/* Error function. */
static ClientData drcSubClientData;
				/* To be passed to error function. */

/* The cookie below is dummied up to provide an error message for
 * errors that occur because of inexact overlaps between subcells.
 */

static DRCCookie drcSubcellCookie = {
    0, { 0 }, { 0 }, (DRCCookie *) NULL,
    "This layer can't abut or partially overlap between subcells",
    0, 0, 0};


/*
 * ----------------------------------------------------------------------------
 *
 * drcFindOtherCells --
 *
 * 	This is a search function invoked when looking around a given
 *	cell for interactions.  If a cell is found other than drcSubCurUse,
 *	then it constitutes an interaction, and its area is included
 *	into the area parameter.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	The area parameter may be modified by including the area
 *	of the current tile.
 *
 * ----------------------------------------------------------------------------
 */

int
drcFindOtherCells(tile, area)
    Tile *tile;			/* Tile in subcell plane. */
    Rect *area;			/* Area in which to include interactions. */
{
    Rect r;
    CellTileBody *ctbptr = (CellTileBody *) tile->ti_body;

    if (ctbptr == NULL) return 0;
    if ((ctbptr->ctb_use != drcSubCurUse) || (ctbptr->ctb_next != NULL))
    {
	TiToRect(tile, &r);
	(void) GeoInclude(&r, area);
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcSubcellTileFunc --
 *
 * 	Called by TiSrArea when looking for interactions in
 *	a given area.  It sees if this subcell tell participates
 *	in any interactions in the area we're rechecking.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	The area drcSubIntArea is modified to include interactions
 *	stemming from this subcell.
 *
 * ----------------------------------------------------------------------------
 */

int
drcSubcellTileFunc(tile)
    Tile *tile;			/* Subcell tile. */
{
    Rect area, haloArea, intArea;
    int i;
    CellTileBody *ctbptr = (CellTileBody *) tile->ti_body;

    if (ctbptr == NULL) return 0;

    /* To determine interactions, find the bounding box of
     * all paint and other subcells within one halo of this
     * subcell tile (and also within the original area where
     * we're recomputing errors).
     */

    TiToRect(tile, &area);
    GEO_EXPAND(&area, drcSubRadius, &haloArea);
    GeoClip(&haloArea, &drcSubLookArea);
    intArea = GeoNullRect;
    for (i = PL_TECHDEPBASE; i < DBNumPlanes; i++)
    {
	(void) DBSrPaintArea((Tile *) NULL, drcSubDef->cd_planes[i],
	    &haloArea, &DBAllButSpaceBits, drcIncludeArea,
	    (ClientData) &intArea);
    }
    drcSubCurUse = ctbptr->ctb_use;
    (void) TiSrArea((Tile *) NULL, drcSubDef->cd_planes[PL_CELL],
	&haloArea, drcFindOtherCells, (ClientData) &intArea);
    if (GEO_RECTNULL(&intArea)) return 0;

    GEO_EXPAND(&intArea, drcSubRadius, &intArea);
    GeoClip(&intArea, &haloArea);
    (void) GeoInclude(&intArea, &drcSubIntArea);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcAlwaysOne --
 *
 * 	This is a utility procedure that always returns 1 when it
 *	is called.  It aborts searches and notifies the invoker that
 *	an item was found during the search.
 *
 * Results:
 *	Always returns 1 to abort searches.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
drcAlwaysOne()
{
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcSubCheckPaint --
 *
 * 	This procedure is invoked once for each subcell in a
 *	particular interaction area.  It checks to see whether the
 *	subcell's subtree actually contains some paint in the potential
 *	interaction area.  As soon as the second such subcell is found,
 *	it aborts the search.
 *
 * Results:
 *	Returns 0 to keep the search alive, unless we've found the
 *	second subcell containing paint in the interaction area.
 *	When this occurs, the search is aborted by returning 1.
 *
 * Side effects:
 *	When the first use with paint is found, curUse is modified
 *	to contain its address.
 *
 * ----------------------------------------------------------------------------
 */

int
drcSubCheckPaint(scx, curUse)
    SearchContext *scx;		/* Contains information about the celluse
				 * that was found.
				 */
    CellUse **curUse;		/* Points to a celluse, or NULL, or -1.  -1
				 * means paint was found in the root cell,
				 * and non-NULL means some other celluse had
				 * paint in it.  If we find another celluse
				 * with paint, when this is non-NULL, it
				 * means there really are two cells with
				 * interacting paint, so we abort the
				 * search to tell the caller to really check
				 * this area.
				 */
{
    if (DBTreeSrTiles(scx, &DBAllButSpaceAndDRCBits, 0, drcAlwaysOne,
	(ClientData) NULL) != 0)
    {
	/* This subtree has stuff under the interaction area. */

	if (*curUse != NULL) return 1;
	*curUse = scx->scx_use;
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCFindInteractions --
 *
 * 	This procedure finds the bounding box of all subcell-subcell
 *	or subcell-paint interactions in a given area of a given cell.
 *
 * Results:
 *	Returns TRUE if there were any interactions in the given
 *	area, FALSE if there were none.
 *
 * Side effects:
 *	The parameter interaction is set to contain the bounding box
 *	of all places in area where one subcell comes within radius
 *	of another subcell, or where paint in def comes within radius
 *	of a subcell.  Interactions between elements of array are not
 *	considered here, but interactions between arrays and other
 *	things are considered.  This routine is a bit clever, in that
 *	it not only checks for bounding boxes interacting, but also
 *	makes sure the cells really contain material in the interaction
 *	area.
 * ----------------------------------------------------------------------------
 */

bool
DRCFindInteractions(def, area, radius, interaction)
    CellDef *def;		/* Cell to check for interactions. */
    Rect *area;			/* Area of def to check for interacting
				 * material.
				 */
    int radius;			/* How close two pieces of material must be
				 * to be considered interacting.  Two pieces
				 * radius apart do NOT interact, but if they're
				 * close than this they do.
				 */
    Rect *interaction;		/* Gets filled in with the bounding box of
				 * the interaction area, if any.  Doesn't
				 * have a defined value when FALSE is returned.
				 */
{
    int i;
    CellUse *use;
    SearchContext scx;

    drcSubDef = def;
    drcSubRadius = radius;
    DRCDummyUse->cu_def = def;

    /* Find all the interactions in the area and compute the
     * outer bounding box of all those interactions.  An interaction
     * exists whenever material in one cell approaches within radius
     * of material in another cell.  As a first approximation, assume
     * each cell has material everywhere within its bounding box.
     */

    drcSubIntArea = GeoNullRect;
    GEO_EXPAND(area, radius, &drcSubLookArea);
    (void) TiSrArea((Tile *) NULL, def->cd_planes[PL_CELL],
	&drcSubLookArea, drcSubcellTileFunc, (ClientData) NULL);
    
    /* If there seems to be an interaction area, make a second pass
     * to make sure there's more than one cell with paint in the
     * area.  This will save us a lot of work where two cells
     * have overlapping bounding boxes without overlapping paint.
     */
    
    if (GEO_RECTNULL(&drcSubIntArea)) return FALSE;
    use = NULL;
    for (i = PL_TECHDEPBASE; i < DBNumPlanes; i++)
    {
	if (DBSrPaintArea((Tile *) NULL, def->cd_planes[i],
	    &drcSubIntArea, &DBAllButSpaceBits, drcAlwaysOne,
	    (ClientData) NULL) != 0)
	{
	    use = (CellUse *) -1;
	    break;
	}
    }
    scx.scx_use = DRCDummyUse;
    scx.scx_trans = GeoIdentityTransform;
    scx.scx_area = drcSubIntArea;
    if (DBTreeSrCells(&scx, 0, drcSubCheckPaint, (ClientData) &use) == 0)
	return FALSE;

    /* OK, no more excuses, there's really an interaction area here. */
    
    *interaction = drcSubIntArea;
    GeoClip(interaction, area);
    if (GEO_RECTNULL(interaction)) return FALSE;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcExactOverlapCheck --
 *
 * 	This procedure is invoked to check for overlap violations.
 *	It is invoked by DBSrPaintArea from drcExactOverlapTile.
 *	Any tiles passed to this procedure must lie within
 *	arg->dCD_rect, or an error is reported.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	If an error occurs, the client error function is called and
 *	the error count is incremented.
 *
 * ----------------------------------------------------------------------------
 */

int
drcExactOverlapCheck(tile, arg)
    Tile *tile;			/* Tile to check. */
    struct drcClientData *arg;	/* How to detect and process errors. */
{
    Rect rect;

    TiToRect(tile, &rect);
    if (GEO_SURROUND(arg->dCD_rect, &rect)) return 0;

    GeoClip(&rect, arg->dCD_clip);
    if (GEO_RECTNULL(&rect)) return 0;

    (*(arg->dCD_function)) (arg->dCD_celldef, &rect, arg->dCD_cptr,
	arg->dCD_clientData);
    (*(arg->dCD_errors))++;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcExactOverlapTile --
 *
 * 	This procedure is invoked by DBTreeSrTiles for each tile
 *	in each constituent cell of a subcell interaction.  It
 *	makes sure that if this tile overlaps other tiles of the
 *	same type in other cells, then the overlaps are EXACT:
 *	each cell contains exactly the same information.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	If there are errors, the client error handling routine
 *	is invoked and the count in the drcClientData record is
 *	incremented.
 *
 * ----------------------------------------------------------------------------
 */

int
drcExactOverlapTile(tile, cxp)
    Tile *tile;			/* Tile that must overlap exactly. */
    TreeContext *cxp;		/* Tells how to translate out of subcell.
				 * The client data must be a drcClientData
				 * record, and the caller must have filled
				 * in the celldef, clip, errors, function,
				 * cptr, and clientData fields.
				 */
{
    struct drcClientData *arg;
    TileTypeBitMask typeMask;
    Rect r1, r2;
    int i;
    
    arg = (struct drcClientData *) cxp->tc_filter->tf_arg;
    TiToRect(tile, &r1);
    GeoTransRect(&(cxp->tc_scx->scx_trans), &r1, &r2);
    arg->dCD_rect = &r2;
    GEO_EXPAND(&r2, 1, &r1);
    for (i = PL_TECHDEPBASE; i < DBNumPlanes; i++)
    {
	TTMaskSetOnlyType(&typeMask, TiGetType(tile));
        (void) DBSrPaintArea((Tile *) NULL, DRCdef->cd_planes[i],
	    &r1, &typeMask, drcExactOverlapCheck, (ClientData) arg);
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCInteractionCheck --
 *
 * 	This is the top-level procedure that performs subcell interaction
 *	checks.  All interaction rule violations in area of def are
 *	found, and func is called for each one.
 *
 * Results:
 *	The number of errors found.
 *
 * Side effects:
 *	The procedure func is called for each violation found.  See
 *	the header for DRCBasicCheck for information about how func
 *	is called.  The violations passed to func are expressed in
 *	the coordinates of def.  Only violations stemming from
 *	interactions in def, as opposed to def's children, are reported.
 *
 * Design Note:
 *	This procedure is trickier than you think.  The problem is that
 *	DRC must be guaranteed to produce EXACTLY the same collection
 *	of errors in an area, no matter how the area is checked.  Checking
 *	it all as one big area should produce the same results as
 *	checking it in several smaller pieces.  Otherwise, "drc why"
 *	won't work correctly, and the error configuration will depend
 *	on how the chip was checked, which is intolerable.  This problem
 *	is solved here by dividing the world up into squares along a grid
 *	of dimension DRCStepSize aligned at the origin.  Interaction areas
 *	are always computed by considering everything inside one grid square
 *	at a time.  We may have to consider several grid squares in order
 *	to cover the area passed in by the client.
 * ----------------------------------------------------------------------------
 */

int
DRCInteractionCheck(def, area, func, cdarg)
    CellDef *def;		/* Definition in which to do check. */
    Rect *area;			/* Area in which all errors are to be found. */
    void (*func)();		/* Function to call for each error. */
    ClientData cdarg;		/* Extra info to be passed to func. */
{
    int oldTiles, count, x, y;
    Rect intArea, square;
    PaintResultType (*savedPaintTable)[NT][NT];
    Void (*savedPaintPlane)();
    struct drcClientData arg;
    SearchContext scx;

    drcSubFunc = func;
    drcSubClientData = cdarg;
    oldTiles = DRCstatTiles;
    count = 0;

    /* Divide the area to be checked up into squares.  Process each
     * square separately.
     */
    
    x = (area->r_xbot/DRCStepSize) * DRCStepSize;
    if (x > area->r_xbot) x -= DRCStepSize;
    y = (area->r_ybot/DRCStepSize) * DRCStepSize;
    if (y > area->r_ybot) y -= DRCStepSize;
    for (square.r_xbot = x; square.r_xbot < area->r_xtop;
	 square.r_xbot += DRCStepSize)
	for (square.r_ybot = y; square.r_ybot < area->r_ytop;
	     square.r_ybot += DRCStepSize)
	{
	    square.r_xtop = square.r_xbot + DRCStepSize;
	    square.r_ytop = square.r_ybot + DRCStepSize;

	    /* Find all the interactions in the square, and clip to the error
	     * area we're interested in. */

	    if (!DRCFindInteractions(def, &square, TechHalo, &intArea))
		continue;
	    GeoClip(&intArea, area);
    
	    /* Flatten the interaction area. */

	    DRCstatInteractions += 1;
	    GEO_EXPAND(&intArea, TechHalo, &scx.scx_area);
	    DRCDummyUse->cu_def = def;
	    scx.scx_use = DRCDummyUse;
	    scx.scx_trans = GeoIdentityTransform;
	    DBCellClearDef(DRCdef);
	    savedPaintTable = DBNewPaintTable(DRCPaintTable);
	    savedPaintPlane = DBNewPaintPlane(DBPaintPlaneMergeOnce);
	    (void) DBCellCopyAllPaint(&scx, &DBAllButSpaceBits, 0, DRCuse);
	    (void) DBNewPaintTable(savedPaintTable);
	    (void) DBNewPaintPlane(savedPaintPlane);

	    /* Run the basic checker over the interaction area. */

	    count += DRCBasicCheck(DRCdef, &scx.scx_area, &intArea,
		func, cdarg);
	    /* TxPrintf("Interaction area: (%d, %d) (%d %d)\n",
		intArea.r_xbot, intArea.r_ybot,
		intArea.r_xtop, intArea.r_ytop);
	    */

	    /* Check for illegal partial overlaps. */

	    scx.scx_use = DRCDummyUse;
	    scx.scx_area = intArea;
	    scx.scx_trans = GeoIdentityTransform;
	    arg.dCD_celldef = DRCdef;
	    arg.dCD_clip = &intArea;
	    arg.dCD_errors = &count;
	    arg.dCD_cptr = &drcSubcellCookie;
	    arg.dCD_function = func;
	    arg.dCD_clientData = cdarg;
	    (void) DBTreeSrTiles(&scx, &DRCExactOverlapTypes, 0,
		drcExactOverlapTile, (ClientData) &arg);
	}
    
    /* Update count of interaction tiles processed. */

    DRCstatIntTiles += DRCstatTiles - oldTiles;
    return count;
}

int drcFlatCount;
DRCFlatCheck(use, area)
    CellUse *use;
    Rect *area;
{
    int x, y;
    Rect chunk;
    SearchContext scx;
    void drcIncCount();
    PaintResultType (*savedPaintTable)[NT][NT];
    Void (*savedPaintPlane)();

    drcFlatCount = 0;
    UndoDisable();
    for (y = area->r_ybot; y < area->r_ytop;  y += 300)
    {
	for (x = area->r_xbot; x < area->r_xtop; x += 300)
	{
	    chunk.r_xbot = x;
	    chunk.r_ybot = y;
	    chunk.r_xtop = x+300;
	    chunk.r_ytop = y+300;
	    if (chunk.r_xtop > area->r_xtop) chunk.r_xtop = area->r_xtop;
	    if (chunk.r_ytop > area->r_ytop) chunk.r_ytop = area->r_ytop;
	    GEO_EXPAND(&chunk, TechHalo, &scx.scx_area);
	    scx.scx_use = use;
	    scx.scx_trans = GeoIdentityTransform;
	    DBCellClearDef(DRCdef);
	    savedPaintTable = DBNewPaintTable(DRCPaintTable);
	    savedPaintPlane = DBNewPaintPlane(DBPaintPlaneMergeOnce);
	    (void) DBCellCopyAllPaint(&scx, &DBAllButSpaceBits, 0, DRCuse);
	    (void) DBNewPaintTable(savedPaintTable);
	    (void) DBNewPaintPlane(savedPaintPlane);
	    (void) DRCBasicCheck(DRCdef, &scx.scx_area, &chunk,
		drcIncCount, (ClientData) NULL);
	}
    }
    TxPrintf("%d total errors found.\n", drcFlatCount);
    UndoEnable();
}

void
drcIncCount()
{
    drcFlatCount += 1;
}
