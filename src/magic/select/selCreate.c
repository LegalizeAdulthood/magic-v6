/* selCreate.c -
 *
 *	This file provides routines to make selections by copying
 *	things into a special cell named "__SELECT__".
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
static char rcsid[]="$Header: selCreate.c,v 6.0 90/08/28 18:56:34 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "undo.h"
#include "commands.h"
#include "selInt.h"
#include "drc.h"
#include "main.h"
#include "signals.h"

/* Two cells worth of information are kept around by the selection
 * module.  SelectDef and SelectUse are for the cells whose contents
 * are the current selection.  Select2Def and Select2Use provide a
 * temporary working space for procedures that manipulate the selection.
 * for example, Select2Def is used to hold nets or regions while they
 * are being extracted by SelectRegion or SelectNet.  Once completely
 * extracted, information is copied to SelectDef.  Changes to
 * SelectDef are undo-able and redo-able (so that the undo package
 * can deal with selection changes), but changes to Select2Def are
 * not undo-able (undoing is always disabled when the cell is modified).
 */

global CellDef *SelectDef, *Select2Def;
global CellUse *SelectUse, *Select2Use;

/* The CellDef below points to the definition FROM which the selection
 * is extracted.  This is the root definition of a window.  Everything
 * in the selection must have come from the same place, so we clear the
 * selection whenever the user tries to select from a new hierarchy.
 */

CellDef *SelectRootDef = NULL;

/* The CellUse below is the last use selected by SelectUse.  It is
 * kept around to support the "replace" feature of SelectUse.
 *
 * Procedures which deselect a cell must reset this to null if they
 * happen to deselect this usage.  (Danger Will Robinson)
 */

global CellUse *selectLastUse = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * SelectInit --
 *
 * 	This procedure initializes the selection code by creating
 *	the selection cells.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The select cells are created if they don't already exist.
 *	Selection undo-ing is also initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectInit()
{
    static bool initialized = FALSE;

    if (initialized) return;
    else initialized = TRUE;

    /* Create the working cells used internally to this module to
     * hold selected information.  Don't allow any of this to be
     * undone, or else it could invalidate all the pointers we
     * keep around to the cells.
     */

    UndoDisable();
    SelectDef = DBCellLookDef("__SELECT__");
    if (SelectDef == (CellDef *) NULL)
    {
	SelectDef = DBCellNewDef("__SELECT__",(char *) NULL);
	ASSERT(SelectDef != (CellDef *) NULL, "SelectInit");
	DBCellSetAvail(SelectDef);
	SelectDef->cd_flags |= CDINTERNAL;
    }
    SelectUse = DBCellNewUse(SelectDef, (char *) NULL);
    DBSetTrans(SelectUse, &GeoIdentityTransform);
    SelectUse->cu_expandMask = -1;	/* This is always expanded. */

    Select2Def = DBCellLookDef("__SELECT2__");
    if (Select2Def == (CellDef *) NULL)
    {
	Select2Def = DBCellNewDef("__SELECT2__",(char *) NULL);
	ASSERT(Select2Def != (CellDef *) NULL, "SelectInit");
	DBCellSetAvail(Select2Def);
	Select2Def->cd_flags |= CDINTERNAL;
    }
    Select2Use = DBCellNewUse(Select2Def, (char *) NULL);
    DBSetTrans(Select2Use, &GeoIdentityTransform);
    Select2Use->cu_expandMask = -1;	/* This is always expanded. */
    UndoEnable();

    SelUndoInit();
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectClear --
 *
 * 	This procedure clears the current selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All information is removed from the select cell, and selection
 *	information is also taken off the screen.
 *
 * ----------------------------------------------------------------------------
 */

/* The variables below are used to record information about subcells that
 * must be cleared from the select cell.
 */

#define MAXUSES 30
static CellUse *(selDeleteUses[MAXUSES]);
static int selNDelete;

void
SelectClear()
{
    SearchContext scx;
    extern int selClearFunc();		/* Forward declaration. */

    if (SelectRootDef == NULL) return;

    scx.scx_area = SelectDef->cd_bbox;
    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);

    /* Erase all the paint from the select cell. */

    DBEraseMask(SelectDef, &TiPlaneRect, &DBAllButSpaceBits);

    /* Erase all of the labels from the select cell. */

    (void) DBEraseLabel(SelectDef, &TiPlaneRect, &DBAllTypeBits);

    /* Erase all of the subcells from the select cell.  This is a bit tricky,
     * because we can't erase the subcells while searching for them (it will
     * cause problems for the database).  The code below first grabs up a
     * few subcells, then deletes them, then grabs up a few more, then deletes
     * them, and so on until done.
     */

    scx.scx_use = SelectUse;
    scx.scx_trans = GeoIdentityTransform;
    while (TRUE)
    {
	int i;

	selNDelete = 0;
	(void) DBCellSrArea(&scx, selClearFunc, (ClientData) NULL);
	for (i = 0; i < selNDelete; i += 1)
	{
	    DBUnLinkCell(selDeleteUses[i], SelectDef);
	    DBDeleteCell(selDeleteUses[i]);
	    (void) DBCellDeleteUse(selDeleteUses[i]);
	}
	if (selNDelete < MAXUSES) break;
    }

    selectLastUse = NULL;

    /* Erase the selection from the screen. */

    SelRememberForUndo(FALSE, SelectRootDef, &scx.scx_area);
    DBWHLRedraw(SelectRootDef, &scx.scx_area, TRUE);
    DBReComputeBbox(SelectDef);
    DBWAreaChanged(SelectDef, &scx.scx_area, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
}

/* Search function to help clear subcells from the selection.  It just
 * records information about several subcells (up to MAXUSES).
 */

int
selClearFunc(scx)
    SearchContext *scx;		/* Describes a cell that was found. */
{
    selDeleteUses[selNDelete] = scx->scx_use;
    selNDelete += 1;
    if (selNDelete == MAXUSES) return 1;
    else return 2;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectArea --
 *
 * 	This procedure selects all information of given types that
 *	falls in a given area.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The indicated information is added to the select cell, and
 *	outlined on the screen.  Only information of particular
 *	types, and in expanded cells (according to xMask) is
 *	selected.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectArea(scx, types, xMask)
    SearchContext *scx;		/* Describes the area in which material
				 * is to be selected.  The resulting
				 * coordinates should map to the coordinates
				 * of EditRootDef.  The cell use should be
				 * the root of a window.
				 */
    TileTypeBitMask *types;	/* Indicates which layers to select.  Can
				 * include L_CELL and L_LABELS to select
				 * labels and unexpanded subcells.  If L_LABELS
				 * is specified then all labels touching the
				 * area are selected.  If L_LABELS isn't
				 * specified, then only labels attached to
				 * selected material are selected.
				 */
    int xMask;			/* Indicates window (or windows) where cells
				 * must be expanded for their contents to be
				 * considered.  0 means treat everything as
				 * expanded.
				 */
{
    Rect labelArea, cellArea;

    /* If the source definition is changing, clear the old selection. */

    if (SelectRootDef != scx->scx_use->cu_def)
    {
	if (SelectRootDef != NULL)
	    SelectClear();
	SelectRootDef = scx->scx_use->cu_def;
	SelSetDisplay(SelectUse, SelectRootDef);
    }

    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);

    /* Select paint. */

    (void) DBCellCopyAllPaint(scx, types, xMask, SelectUse);

    /* Select labels. */

    if (TTMaskHasType(types, L_LABEL))
        (void) DBCellCopyAllLabels(scx, &DBAllTypeBits, xMask,
		SelectUse, &labelArea);
    else (void) DBCellCopyAllLabels(scx, types, xMask, SelectUse, &labelArea);

    /* Select unexpanded cell uses. */

    if (TTMaskHasType(types, L_CELL))
        (void) DBCellCopyAllCells(scx, xMask, SelectUse, &cellArea);
    else
    {
	cellArea.r_xbot = 0;
	cellArea.r_xtop = -1;
    }

    /* Display the new selection. */

    (void) GeoIncludeAll(&scx->scx_area, &labelArea);
    (void) GeoIncludeAll(&cellArea, &labelArea);
    SelRememberForUndo(FALSE, SelectRootDef, &labelArea);
    DBReComputeBbox(SelectDef);
    DBWHLRedraw(SelectRootDef, &labelArea, TRUE);
    DBWAreaChanged(SelectDef, &SelectDef->cd_bbox, DBW_ALLWINDOWS,
	&DBAllButSpaceBits);
}

/*
 * ----------------------------------------------------------------------------
 *
 * selGetLabels --
 *
 * 	This is a local procedure used to extract all the labels
 *	associated with the paint in Select2Def.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Labels are added to SelectDef for all labels in the hierarchy
 *	under rootUse that are visible (according to xMask), and
 *	for which there is connected material in Select2Def.
 *
 * ----------------------------------------------------------------------------
 */

void
selGetLabels(rootUse, xMask)
    CellUse *rootUse;	/* Hierarchy to search for visible labels. */
    int xMask;		/* Indicates windows where cells must be expanded
			 * in order for their labels to be copied.
			 */
{
    SearchContext scx;

    /* This procedure duplicates much of the code of DBTreeSrLabels.
     * However, it is much more efficient in some cases because it
     * only checks a subtree if Select2Def contains some paint in
     * the area of the subtree.  If a very large L-shaped net has
     * been selected, this could drastically reduce the amount of
     * searching that must be done to find labels.
     */
    
    scx.scx_use = rootUse;
    scx.scx_area = Select2Def->cd_bbox;
    scx.scx_trans = GeoIdentityTransform;

    (void) selLabelCellFunc(&scx, xMask);
}

/* The procedure below does all the work of getting labels.  It's
 * called from above, and also indirectly by itself, through
 * DBCellSrArea.
 */

int
selLabelCellFunc(scx, xMask)
    SearchContext *scx;		/* Area and cell we're currently searching. */
    int xMask;			/* Indicates window where cell must be
				 * expanded.
				 */
{
    Label *lab;
    CellDef *def;
    int i;
    Rect rootArea;
    extern int selLabelPaintFunc();

    /* No point in checking this cell, its children, or its siblings
     * in an array if it isn't expanded.
     */

    if (!DBIsExpand(scx->scx_use, xMask)) return 2;

    /* Make sure that there's some material in Select2Def under
     * the area we're searching.  Otherwise there's no point
     * in checking for labels.
     */
    
    def = scx->scx_use->cu_def;
    GeoTransRect(&scx->scx_trans, &def->cd_bbox, &rootArea);
    for (i = PL_SELECTBASE; i < DBNumPlanes; i += 1)
    {
	if (DBSrPaintArea((Tile *) NULL, Select2Def->cd_planes[i], &rootArea,
	    &DBAllButSpaceAndDRCBits, selLabelPaintFunc,
	    (ClientData) NULL) != 0) goto checkCell;
    }
    return 0;

    /* Make sure that the cell is in memory. */

    checkCell:
    if ((def->cd_flags & CDAVAILABLE) == 0)
	if (!DBCellRead(def, (char *) NULL, TRUE)) return 2;

    /* For each label in the cell, see if there is connected paint
     * in Select2Def.  If so, add the label to SelectDef.  Ignore
     * space labels here, since they can't possibly be connected
     * to anything.
     */

    for (lab = def->cd_labels; lab != NULL; lab = lab->lab_next)
    {
	Rect area, searchArea;
	int newPos;

	if (lab->lab_type == TT_SPACE) continue;
	GeoTransRect(&scx->scx_trans, &lab->lab_rect, &area);
	newPos = GeoTransPos(&scx->scx_trans, lab->lab_pos);
	GEO_EXPAND(&area, 1, &searchArea);
	if (DBSrPaintArea((Tile *) NULL,
	    Select2Def->cd_planes[DBPlane(lab->lab_type)],
	    &searchArea, &DBConnectTbl[lab->lab_type], selLabelPaintFunc,
	    (ClientData) NULL) == 0) continue;
	DBEraseLabelsByContent(SelectDef, &area, newPos,
	    -1, lab->lab_text);
	(void) DBPutLabel(SelectDef, &area, newPos, lab->lab_text,
	    lab->lab_type);
    }

    /* Now check subcells of this cell. */

    (void) DBCellSrArea(scx, selLabelCellFunc, (ClientData) xMask);

    return 0;
}

/* The function below returns 1 whenever called.  It is used to detect
 * when there is paint of particular types under a particular area.
 */

int
selLabelPaintFunc()
{
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * selFindChunk --
 *
 * 	This is a recursive procedure to find the largest chunk of
 *	material in a particular area.  It locates a rectangular
 *	area of given materials whose minimum dimension is as
 *	large as possible, and whose maximum dimension is also as
 *	large as possible (but minimum dimension is more important).
 *	Furthermore, the chunk must lie within a particular area and
 *	must contain a given area.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
selFindChunk(plane, wrongTypes, searchArea, containedArea, bestMin,
	bestMax, bestChunk)
    Plane *plane;			/* Plane on which to hunt for chunk. */
    TileTypeBitMask *wrongTypes;	/* Types that are not allowed to be
					 * part of the chunk.
					 */
    Rect *searchArea;			/* Largest allowable size for the
					 * chunk.  Note:  don't overestimate
					 * this or the procedure will take a
					 * long time!  (it processes every
					 * tile in this area).
					 */
    Rect *containedArea;		/* The chunk returned must contain
					 * this entire area.
					 */
    int *bestMin;			/* Largest minimum dimension seen so
					 * far: skip any chunks that can't
					 * match this.  Updated by this
					 * procedure.
					 */
    int *bestMax;			/* Largest maximum dimension seen
					 * so far.
					 */
    Rect *bestChunk;			/* Filled in with largest chunk seen
					 * so far, if we find one better than
					 * bestMin and bestMax.
					 */
{
    Rect wrong, smaller;
    int min, max;
    extern int selChunkFunc();

    /* If the search area is already smaller than the chunk to beat,
     * there's no point in even examining this chunk.
     */

    min = searchArea->r_xtop - searchArea->r_xbot;
    max = searchArea->r_ytop - searchArea->r_ybot;
    if (min > max)
    {
	int tmp;
	tmp = min; min = max; max = tmp;
    }

    if (min < *bestMin) return;
    if ((min == *bestMin) && (max <= *bestMax)) return;

    /* At each stage, search the area that's left for material of the
     * wrong type.
     */

    if (DBSrPaintArea((Tile *) NULL, plane, searchArea, wrongTypes,
	    selChunkFunc, (ClientData) &wrong) == 0)
    {
	/* The area contains nothing but material of the right type,
	 * so it is now the "chunk to beat".
	 */

	*bestMin = min;
	*bestMax = max;
	*bestChunk = *searchArea;
	return;
    }

    if (SigInterruptPending)
	return;

    /* At this point the current search area contains some material of
     * the wrong type.  We have to reduce the search area to exclude this
     * material.  There are two ways that this can be done while still
     * producing areas that contain containedArea.  Try both of those,
     * and repeat the whole thing recursively on the smaller areas.
     */

    /* First, try reducing the x-range. */

    smaller = *searchArea;
    if (wrong.r_xbot >= containedArea->r_xtop)
	smaller.r_xtop = wrong.r_xbot;
    else if (wrong.r_xtop <= containedArea->r_xbot)
	smaller.r_xbot = wrong.r_xtop;
    else goto tryY;  /* Bad material overlaps containedArea in x. */
    selFindChunk(plane, wrongTypes, &smaller, containedArea,
	    bestMin, bestMax, bestChunk);
    

    /* Also try reducing the y-range to see if that works better. */

    tryY: smaller = *searchArea;
    if (wrong.r_ybot >= containedArea->r_ytop)
	smaller.r_ytop = wrong.r_ybot;
    else if (wrong.r_ytop <= containedArea->r_ybot)
	smaller.r_ybot = wrong.r_ytop;
    else return;  /* Bad material overlaps containedArea in y. */
    selFindChunk(plane, wrongTypes, &smaller, containedArea,
	    bestMin, bestMax, bestChunk);
}

/* This procedure is called for each tile of the wrong type in an
 * area that is supposed to contain only tiles of other types.  It
 * just returns the area of the wrong material and aborts the search.
 */

int
selChunkFunc(tile, rect)
    Tile *tile;			/* The offending tile. */
    Rect *rect;			/* Place to store the tile's area. */
{
    TiToRect(tile, rect);
    return 1;			/* Abort the search. */
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectChunk --
 *
 * 	This procedure selects a single rectangular chunk of 
 *	homogeneous material.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	More material is added to the select cell and displayed
 *	on the screen.  This procedure finds the largest rectangular
 *	chunk of material "type" that contains the area given in
 *	in scx.  The material need not all be in one cell, but it
 *	must all be in cells that are expanded according to "xMask".
 *	If pArea is given, the rectangle it points to is filled in
 *	with the area of the chunk that was selected.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectChunk(scx, type, xMask, pArea, less)
    SearchContext *scx;		/* Area to tree-search for material.  The
				 * transform must map to root coordinates
				 * of the edit cell.
				 */
    TileType type;		/* The type of material to be considered. */
    int xMask;			/* Indicates window (or windows) where cells
				 * must be expanded for their contents to be
				 * considered.  0 means treat everything as
				 * expanded.
				 */
    Rect *pArea;		/* If non-NULL, gets filled in with the area
				 * of the selection.
				 */
    bool less;
{
#define INITIALSIZE 10
    SearchContext newscx;
    TileTypeBitMask wrongTypes, typeMask;
    Rect bestChunk;
    int bestMin, bestMax, width, height;

    /* If the source definition is changing, clear the old selection. */

    if (SelectRootDef != scx->scx_use->cu_def)
    {
	if (SelectRootDef != NULL)
	    SelectClear();
	SelectRootDef = scx->scx_use->cu_def;
	SelSetDisplay(SelectUse, SelectRootDef);
    }

    /* The chunk is computed iteratively.  First extract a small
     * region (defined by INITIALSIZE) into Select2Def.  Then find
     * the largest chunk in the region.  If the chunk touches a
     * side of the region, then extract a larger region and try
     * again.  Keep making the region larger and larger until we
     * eventually find a region that completely contains the chunk
     * with space left over around the edges.
     */

    UndoDisable();
    TTMaskZero(&typeMask);
    TTMaskSetType(&typeMask, type);
    TTMaskCom2(&wrongTypes, &typeMask);

    newscx = *scx;
    GEO_EXPAND(&newscx.scx_area, INITIALSIZE, &newscx.scx_area);
    while (TRUE)
    {
	/* Extract a bunch of junk. */

	DBCellClearDef(Select2Def);
	DBCellCopyAllPaint(&newscx, &typeMask, xMask, Select2Use);

	/* Now find the best chunk in the area. */

	bestMin = bestMax = 0;
	bestChunk = GeoNullRect;
	selFindChunk(Select2Def->cd_planes[DBPlane(type)],
	    &wrongTypes, &newscx.scx_area, &scx->scx_area,
	    &bestMin, &bestMax, &bestChunk);
	if (GEO_RECTNULL(&bestChunk))
	{
	    /* No chunk was found, so return. */

	    UndoEnable();
	    if (pArea != NULL) *pArea = bestChunk;
	    return;
	}

	/* If the chunk is completely inside the area we yanked, then we're
	 * done.
	 */
	
	if (GEO_SURROUND_STRONG(&newscx.scx_area, &bestChunk)) break;

	/* The chunk extends to the edge of the area.  Anyplace that the
	 * chunk touches an edge, move that edge out by a factor of two.
	 * Anyplace it doesn't touch, move the edge in to be just one
	 * unit out from the chunk.
	 */
	
	width = newscx.scx_area.r_xtop - newscx.scx_area.r_xbot;
	height = newscx.scx_area.r_ytop - newscx.scx_area.r_ybot;

	if (bestChunk.r_xbot == newscx.scx_area.r_xbot)
	    newscx.scx_area.r_xbot -= width;
	else newscx.scx_area.r_xbot = bestChunk.r_xbot - 1;
	if (bestChunk.r_ybot == newscx.scx_area.r_ybot)
	    newscx.scx_area.r_ybot -= height;
	else newscx.scx_area.r_ybot= bestChunk.r_ybot - 1;
	if (bestChunk.r_xtop == newscx.scx_area.r_xtop)
	    newscx.scx_area.r_xtop += width;
	else newscx.scx_area.r_xtop = bestChunk.r_xtop + 1;
	if (bestChunk.r_ytop == newscx.scx_area.r_ytop)
	    newscx.scx_area.r_ytop += height;
	else newscx.scx_area.r_ytop = bestChunk.r_ytop + 1;
    }

    UndoEnable();
    if (less)
      {
	SelRemoveArea(&bestChunk, &typeMask);
      }
    else
      {
	newscx.scx_area = bestChunk;
	SelectArea(&newscx, &typeMask, xMask);
      }

    if (pArea != NULL) *pArea = bestChunk;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectRegion --
 *
 * 	Select an entire region of material, no matter what its
 *	shape.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure traces out the region consisting entirely
 *	of type "type", and selects all that material.  The search
 *	starts from "type" material under scx and continues outward
 *	to get all material in all cells connected to the area under
 *	scx by material of type "type".  If pArea is specified, then
 *	the rectangle that it points to is filled in with the bounding
 *	box of the region that was selected.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectRegion(scx, type, xMask, pArea, less)
    SearchContext *scx;		/* Area to tree-search for material.  The
				 * transform must map to EditRoot coordinates.
				 */
    TileType type;		/* The type of material to be considered. */
    int xMask;			/* Indicates window (or windows) where cells
				 * must be expanded for their contents to be
				 * considered.  0 means treat everything as
				 * expanded.
				 */
    Rect *pArea;		/* If non-NULL, points to rectangle to be
				 * filled in with region's bounding box.
				 */
    bool less;
{
    TileTypeBitMask connections[TT_MAXTYPES];
    int i;
    SearchContext scx2;

    /* If the source definition is changing, clear the old selection. */

    if (SelectRootDef != scx->scx_use->cu_def)
    {
	if (SelectRootDef != NULL)
	    SelectClear();
	SelectRootDef = scx->scx_use->cu_def;
	SelSetDisplay(SelectUse, SelectRootDef);
    }

    /* Set up a connection table that allows only a particular type
     * of material to be considered in the region.
     */

    for (i=0; i<DBNumTypes; i+=1)
	TTMaskZero(&connections[i]);
    TTMaskSetType(&connections[type], type);

    /* Clear out the temporary selection cell and yank all of the
     * connected paint into it.
     */

    UndoDisable();
    DBCellClearDef(Select2Def);
    DBTreeCopyConnect(scx, &connections[type], xMask, connections,
	    &TiPlaneRect, Select2Use);
    UndoEnable();

    /* Now transfer what we found into the main selection cell.  Pick
     * up all the labels that correspond to the selected material.
     */
    
    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);
    if (less)
      {
	(void) SelRemoveSel2();
      }
    else
      {
	scx2.scx_use = Select2Use;
	scx2.scx_area = Select2Def->cd_bbox;
	scx2.scx_trans = GeoIdentityTransform;
	DBCellCopyAllPaint(&scx2, &DBAllButSpaceAndDRCBits,
			   0, SelectUse);
	
	/* Grab relevant labels. */
	
	selGetLabels(scx->scx_use, xMask);
      }

    /* Display the new selection. */

    SelRememberForUndo(FALSE, SelectRootDef, &Select2Def->cd_bbox);
    DBReComputeBbox(SelectDef);
    DBWHLRedraw(SelectRootDef, &Select2Def->cd_bbox, TRUE);
    DBWAreaChanged(SelectDef, &Select2Def->cd_bbox, DBW_ALLWINDOWS,
	 &DBAllButSpaceBits);

    if (pArea != NULL) *pArea = Select2Def->cd_bbox;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectNet --
 *
 * 	This procedure selects an entire electrically-connected net.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Starting from material of type "type" under scx, this procedure
 *	finds and highlights all material in all expanded cells that
 *	is electrically-connected to the starting material through a
 *	chain of expanded cells.  If pArea is specified, then the
 *	rectangle that it points to is filled in with the bounding box
 *	of the net that was selected.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectNet(scx, type, xMask, pArea, less)
    SearchContext *scx;		/* Area to tree-search for material.  The
				 * transform must map to EditRoot coordinates.
				 */
    TileType type;		/* The type of material to be considered. */
    int xMask;			/* Indicates window (or windows) where cells
				 * must be expanded for their contents to be
				 * considered.  0 means treat everything as
				 * expanded.
				 */
    Rect *pArea;		/* If non-NULL, points to rectangle to be
				 * filled in with net's bounding box.
				 */
    bool less;
{
    TileTypeBitMask mask;
    SearchContext scx2;

    /* If the source definition is changing, clear the old selection. */

    if (SelectRootDef != scx->scx_use->cu_def)
    {
	if (SelectRootDef != NULL)
	    SelectClear();
	SelectRootDef = scx->scx_use->cu_def;
	SelSetDisplay(SelectUse, SelectRootDef);
    }

    TTMaskZero(&mask);
    TTMaskSetType(&mask, type);

    /* Clear out the temporary selection cell and yank all of the
     * connected paint into it.
     */

    UndoDisable();
    DBCellClearDef(Select2Def);
    DBTreeCopyConnect(scx, &mask, xMask, DBConnectTbl,
	    &TiPlaneRect, Select2Use);
    UndoEnable();

    /* Now transfer what we found into the main selection cell.  Pick
     * up all the labels that correspond to the selected material.
     */
    
    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);
    if (less)
      {
	(void) SelRemoveSel2();
      }
    else
      {
	scx2.scx_use = Select2Use;
	scx2.scx_area = Select2Def->cd_bbox;
	scx2.scx_trans = GeoIdentityTransform;
	DBCellCopyAllPaint(&scx2, &DBAllButSpaceAndDRCBits,
			   0, SelectUse);
	
	/* Grab relevant labels. */
	
	selGetLabels(scx->scx_use, xMask);
      }

    /* Display the newly-selected material. */

    SelRememberForUndo(FALSE, SelectRootDef, &Select2Def->cd_bbox);
    DBReComputeBbox(SelectDef);
    DBWHLRedraw(SelectRootDef, &Select2Def->cd_bbox, TRUE);
    DBWAreaChanged(SelectDef, &Select2Def->cd_bbox, DBW_ALLWINDOWS,
	&DBAllButSpaceBits);

    if (pArea != NULL) *pArea = Select2Def->cd_bbox;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectCell --
 *
 * 	Select a subcell by making a copy of it in the __SELECT__ cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given use is copied into the selection.  If replace is TRUE,
 *	then the last subcell to be selected via this procedure is
 *	deselected.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectCell(use, rootDef, trans, replace)
    CellUse *use;		/* Cell use to be selected. */
    CellDef *rootDef;		/* Root definition of window in which selection
				 * is being made.
				 */
    Transform *trans;		/* Transform from the coordinates of use's
				 * definition to the coordinates of rootDef.
				 */
    bool replace;		/* TRUE means deselect the last cell selected
				 * by this procedure, if it's still selected.
				 */
{
    CellUse *newUse;

    /* If the source definition is changing, clear the old selection. */

    if (SelectRootDef != rootDef)
    {
	if (SelectRootDef != NULL)
	    SelectClear();
	SelectRootDef = rootDef;
	SelSetDisplay(SelectUse, SelectRootDef);
    }

    /* Deselect the last cell selected, if requested. */

    if (replace && (selectLastUse != NULL))
    {
	Rect area;

	SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);
	area = selectLastUse->cu_bbox;
	DBUnLinkCell(selectLastUse, SelectDef);
	DBDeleteCell(selectLastUse);
	(void) DBCellDeleteUse(selectLastUse);
	SelRememberForUndo(FALSE, SelectRootDef, &area);
	DBWHLRedraw(SelectRootDef, &area, TRUE);
    }

    /* When creating a new use, try to re-use the id from the old
     * one.  Only create a new one if the old id can't be used.
     */

    newUse = DBCellNewUse(use->cu_def, (char *) use->cu_id);
    if (!DBLinkCell(newUse, SelectDef))
    {
	freeMagic((char *) newUse->cu_id);
	newUse->cu_id = NULL;
	(void) DBLinkCell(newUse, SelectDef);
    }

    DBSetArray(use, newUse);
    DBSetTrans(newUse, trans);
    newUse->cu_expandMask = use->cu_expandMask;

    /* If this cell is already selected, there's nothing more to do.
     * Since we didn't change the selection here, be sure NOT to remember
     * it for future deselection!
     */

    if (DBCellFindDup(newUse, SelectDef) != NULL)
    {
	DBUnLinkCell(newUse, SelectDef);
	(void) DBCellDeleteUse(newUse);
	selectLastUse = (CellUse *) NULL;
	return;
    }

    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);
    DBPlaceCell(newUse, SelectDef);
    selectLastUse = newUse;

    SelRememberForUndo(FALSE, SelectRootDef, &newUse->cu_bbox);
    DBReComputeBbox(SelectDef);
    DBWHLRedraw(SelectRootDef, &newUse->cu_bbox, TRUE);
    DBWAreaChanged(SelectDef, &newUse->cu_bbox, DBW_ALLWINDOWS,
	&DBAllButSpaceBits);
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectAndCopy2 --
 *
 * 	This is procedure is intended for use only within the selection
 *	module.  It takes what's in Select2Def, makes a copy of it in the
 *	edit cell, and makes the copy the selection.  It's used, for
 *	example, by the transformation and copying routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is augmented with what's in Select2Def.  The caller
 *	should normally have cleared the selection before calling us.
 *	The edit cell is modified to include everything that was in
 *	Select2Def.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectAndCopy2(newSourceDef)
    CellDef *newSourceDef;		/* The new selection is to be
					 * associated with this cell in the
					 * user's layout.
					 */
{
    SearchContext scx;
    Rect editArea, labelArea;
    int plane;
    extern int selACPaintFunc();	/* Forward reference. */
    extern int selACCellFunc();

    /* Just copy the information in Select2Def twice, once into the
     * edit cell and once into the main selection cell.
     */
    
    scx.scx_use = Select2Use;
    scx.scx_area = Select2Use->cu_bbox;
    scx.scx_trans = RootToEditTransform;
    (void) DBCellCopyAllPaint(&scx, &DBAllButSpaceAndDRCBits, -1, EditCellUse);
    (void) DBCellCopyAllLabels(&scx, &DBAllTypeBits, -1, EditCellUse,
	(Rect *) NULL);
    (void) DBCellCopyAllCells(&scx, -1, EditCellUse, (Rect *) NULL);
    GeoTransRect(&scx.scx_trans, &scx.scx_area, &editArea);
    DBAdjustLabels(EditCellUse->cu_def, &editArea);
    DBWAreaChanged(EditCellUse->cu_def, &editArea, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKPAINT, &editArea);
    DBReComputeBbox(EditCellUse->cu_def);

    SelectRootDef = newSourceDef;
    SelSetDisplay(SelectUse, SelectRootDef);

    SelRememberForUndo(TRUE, (CellDef *) NULL, (Rect *) NULL);
    scx.scx_trans = GeoIdentityTransform;

    /* In copying stuff into SelectUse, we have to be careful.  The problem
     * is that the stuff now in the edit cell may have switched layers.
     * (for example, Select2Def might have diff, which got painted
     * over poly in the edit cell to form transistor).  As a result, we
     * use Select2Def to figure out what areas of what planes to put into
     * SelectUse, but use the actual tile types from the edit cell.
     */
    
    for (plane = PL_SELECTBASE; plane < DBNumPlanes; plane++)
    {
	(void) DBSrPaintArea((Tile *) NULL, Select2Def->cd_planes[plane],
		&TiPlaneRect, &DBAllButSpaceAndDRCBits, selACPaintFunc,
		(ClientData) plane);
    }
    (void) DBCellCopyAllLabels(&scx, &DBAllTypeBits, -1, SelectUse,
	    &labelArea);

    /* We also have to be careful about copying subcells into the
     * main selection cell.  It might not have been possible to copy
     * a subcell into the edit cell (above), because the copying
     * would have formed a circularity.  In that case, we need to
     * drop that subcell from the new selection.  The code below just
     * copies those that are still in the edit cell.
     */

    (void) SelEnumCells(TRUE, (bool *) NULL, &scx, selACCellFunc,
	    (ClientData) NULL);

    DBReComputeBbox(SelectDef);
    
    /* A little hack here:  don't do explicit redisplay of the selection,
     * or record a very large redisplay area for undo-ing.  It's not
     * necessary since the layout redisplay also redisplays the highlights.
     * If we do it too, then we're just double-displaying and wasting
     * time.  (note: must record something for undo-ing in order to get
     * SelectRootDef set right... just don't pass a redisplay area).
     */

    SelRememberForUndo(FALSE, SelectRootDef, (Rect *) NULL);
    DBWAreaChanged(SelectDef, &SelectDef->cd_bbox, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
}

/* Utility function: for each tile, copy information over its area from
 * the given edit cell plane to SelectDef.  Always return 0 to keep the
 * search alive.
 */

int
selACPaintFunc(tile, plane)
    Tile *tile;			/* Tile in Select2Def. */
    int plane;			/* Index of plane this tile came from. */
{
    Rect area, editArea;
    int selACPaintFunc2();	/* Forward reference. */

    TiToRect(tile, &area);
    GeoTransRect(&RootToEditTransform, &area, &editArea);
    (void) DBSrPaintArea((Tile *) NULL, EditCellUse->cu_def->cd_planes[plane],
	    &editArea, &DBAllButSpaceAndDRCBits, selACPaintFunc2,
	    (ClientData) &editArea);
    return 0;
}

/* Second-level paint function:  just paint the overlap between
 * tile and editClip into SelectDef.
 */

int
selACPaintFunc2(tile, editClip)
    Tile *tile;			/* Tile in edit cell. */
    Rect *editClip;		/* Edit-cell area to clip to before painting
				 * into selection.
				 */
{
    Rect area, selArea;
    TileType type;

    TiToRect(tile, &area);
    GeoClip(&area, editClip);
    GeoTransRect(&EditToRootTransform, &area, &selArea);
    type = TiGetType(tile);
    if (type >= DBNumUserLayers)
    {
        register TileType primary,first;
        register typecount=0;

        for (primary = TT_SELECTBASE; primary < DBNumUserLayers; primary++)
        {
            if (TTMaskHasType(&DBLayerTypeMaskTbl[primary], type))
            {
                  typecount++;
                 first = primary;
            }
        }
        if (typecount == 1)
        {
             type = first;
        }
        else  /* have to see what's down there  */
        {

             for (primary = TT_SELECTBASE; primary < DBNumUserLayers; primary++)
             {
                  if (TTMaskHasType(&DBLayerTypeMaskTbl[primary], type))
                  {
                       int      planenum = DBPlane(primary);
                       Tile     *tp = EditCellUse->cu_def->cd_planes[planenum]->pl_hint;
                       GOTOPOINT(tp,&(tile->ti_ll));
                       if (TiGetType(tp) == primary)
                       {
                            type = primary;
                            break;
                       }
                  }
             }
        }
    }
    DBPaint(SelectDef, &selArea, type);
    return 0;
}

/* Cell search function:  invoked for each subcell in Select2Def that's
 * also in the edit cell.  Make a copy of the cell in SelectDef.
 */

int
selACCellFunc(selUse, realUse)
    CellUse *selUse;		/* Use to be copied into SelectDef.  This
				 * is the instance inside Select2Def.
				 */
    CellUse *realUse;		/* The cellUse (in the edit cell) corresponding
				 * to selUse.  We need this in order to use its
				 * instance id and expand mask in the selection.
				 */
{
    CellUse *newUse;

    newUse = DBCellNewUse(selUse->cu_def, realUse->cu_id);
    if (!DBLinkCell(newUse, SelectDef))
    {
	freeMagic((char *) newUse->cu_id);
	newUse->cu_id = NULL;
	(void) DBLinkCell(newUse, SelectDef);
    }
    newUse->cu_expandMask = realUse->cu_expandMask;
    DBSetArray(selUse, newUse);
    DBSetTrans(newUse, &selUse->cu_transform);
    DBPlaceCell(newUse, SelectDef);
    return 0;
}

