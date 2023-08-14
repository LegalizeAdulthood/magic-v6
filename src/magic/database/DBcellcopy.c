/*
 * DBcellcopy.c --
 *
 * Cell copying (yank and stuff)
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
static char rcsid[] = "$Header: DBcellcopy.c,v 6.0 90/08/28 18:09:32 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "malloc.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "textio.h"
#include "windows.h"
#include "dbwind.h"
#include "commands.h"

/*
 * The following variable points to the tables currently used for
 * painting.  The paint tables are occasionally switched, by clients
 * like the design-rule checker, by calling DBNewPaintTable.  This
 * paint table applies only to the routine in this module.
 */
static PaintResultType (*dbCurPaintTbl)[NT][NT] = DBPaintResultTbl;

/*
 * The following variable points to the version of DBPaintPlane used
 * for painting during yanks.  This is occasionally switched by clients
 * such as the design-rule checker that need to use, for example,
 * DBPaintPlaneMergeOnce instead of the standard version.
 */
static Void (*dbCurPaintPlane)() = DBPaintPlane;

    /* Structure passed to DBTreeSrTiles() */
struct copyAllArg
{
    TileTypeBitMask	*caa_mask;	/* Mask of tile types to be copied */
    Rect		 caa_rect;	/* Clipping rect in target coords */
    CellUse		*caa_targetUse;	/* Use to which tiles are copied */
    Rect		*caa_bbox;	/* Bbox of material copied (in
					 * targetUse coords).  Used only when
					 * copying cells.
					 */
};

    /* Structure passed to DBSrPaintArea() */
struct copyArg
{
    TileTypeBitMask	*ca_mask;	/* Mask of tile types to be copied */
    Rect		 ca_rect;	/* Clipping rect in source coords */
    CellUse		*ca_targetUse;	/* Use to which tiles are copied */
    Transform		*ca_trans;	/* Transform to target */
};

    /* Structure passed to DBTreeSrLabels to hold informatio about
     * copying labels.
     */

struct copyLabelArg
{
    CellUse *cla_targetUse;		/* Use to which labels are copied. */
    Rect *cla_bbox;			/* If non-NULL, points to rectangle
					 * to be filled in with total area of
					 * all labels copied.
					 */
};

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyAllPaint --
 *
 * Copy paint from the tree rooted at scx->scx_use to the paint planes
 * of targetUse, transforming according to the transform in scx.
 * Only the types specified by typeMask are copied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the paint planes in targetUse.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyAllPaint(scx, mask, xMask, targetUse)
    SearchContext *scx;		/* Describes root cell to search, area to
				 * copy, transform from root cell to coords
				 * of targetUse.
				 */
    TileTypeBitMask *mask;	/* Types of tiles to be yanked/stuffed */
    int xMask;			/* Expansion state mask to be used in search */
    CellUse *targetUse;		/* Cell into which material is to be stuffed */
{
    struct copyAllArg arg;
    int dbCopyAllPaint();

    arg.caa_mask = mask;
    arg.caa_targetUse = targetUse;
    GEOTRANSRECT(&scx->scx_trans, &scx->scx_area, &arg.caa_rect);

    (void) DBTreeSrTiles(scx, mask, xMask, dbCopyAllPaint, (ClientData) &arg);
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyAllLabels --
 *
 * Copy labels from the tree rooted at scx->scx_use to targetUse,
 * transforming according to the transform in scx.  Only labels
 * attached to layers of the types specified by mask are copied.
 * The area to be copied is determined by GEO_LABEL_IN_AREA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Copies labels to targetUse, clipping against scx->scx_area.
 *	If pArea is given, store in it the bounding box of all the
 *	labels copied.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyAllLabels(scx, mask, xMask, targetUse, pArea)
    SearchContext *scx;		/* Describes root cell to search, area to
				 * copy, transform from root cell to coords
				 * of targetUse.
				 */
    TileTypeBitMask *mask;	/* Only labels of these types are copied */
    int xMask;			/* Expansion state mask to be used in search */
    CellUse *targetUse;		/* Cell into which labels are to be stuffed */
    Rect *pArea;		/* If non-NULL, points to a box that will be
				 * filled in with bbox (in targetUse coords)
				 * of all labels copied.  Will be degenerate
				 * if nothing was copied.
				 */
{
    int dbCopyAllLabels();
    struct copyLabelArg arg;

    /* DBTeeSrLabels finds all the labels that we want plus some more.
     * We'll filter out the ones that we don't need.
     */
    
    arg.cla_targetUse = targetUse;
    arg.cla_bbox = pArea;
    if (pArea != NULL)
    {
	pArea->r_xbot = 0;
	pArea->r_xtop = -1;
    }
    (void) DBTreeSrLabels(scx, mask, xMask, (TerminalPath *) 0,
			dbCopyAllLabels, (ClientData) &arg);
}

    /*ARGSUSED*/
int
dbCopyAllLabels(scx, lab, tpath, arg)
    register SearchContext *scx;
    register Label *lab;
    TerminalPath *tpath;
    struct copyLabelArg *arg;
{
    Rect labTargetRect;
    int targetPos;
    CellDef *def;

    def = arg->cla_targetUse->cu_def;
    if (!GEO_LABEL_IN_AREA(&lab->lab_rect, &(scx->scx_area))) return 0;
    GeoTransRect(&scx->scx_trans, &lab->lab_rect, &labTargetRect);
    targetPos = GeoTransPos(&scx->scx_trans, lab->lab_pos);

    /* Eliminate duplicate labels.  Don't pay any attention to layers
     * in deciding on duplicates:  if text and position match, it's a
     * duplicate.
     */

    (void) DBEraseLabelsByContent(def, &labTargetRect, targetPos,
        -1, lab->lab_text);
    (void) DBPutLabel(def, &labTargetRect, targetPos,
		lab->lab_text, lab->lab_type);
    if (arg->cla_bbox != NULL)
	(void) GeoIncludeAll(&labTargetRect, arg->cla_bbox);
    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyPaint --
 *
 * Copy paint from the paint planes of scx->scx_use to the paint planes
 * of targetUse, transforming according to the transform in scx.
 * Only the types specified by typeMask are copied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the paint planes in targetUse.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyPaint(scx, mask, xMask, targetUse)
    SearchContext *scx;		/* Describes cell to search, area to
				 * copy, transform from cell to coords
				 * of targetUse.
				 */
    TileTypeBitMask *mask;	/* Types of tiles to be yanked/stuffed */
    int xMask;			/* Expansion state mask to be used in search */
    CellUse *targetUse;		/* Cell into which material is to be stuffed */
{
    int pNum, planeMask;
    TreeContext cxp;
    TreeFilter filter;
    struct copyAllArg arg;
    int dbCopyAllPaint();

    if (!DBIsExpand(scx->scx_use, xMask))
	return;

    arg.caa_mask = mask;
    arg.caa_targetUse = targetUse;
    GeoTransRect(&scx->scx_trans, &scx->scx_area, &arg.caa_rect);

    /* Build dummy TreeContext */
    cxp.tc_scx = scx;
    cxp.tc_filter = &filter;
    filter.tf_arg = (ClientData) &arg;

    /* tf_func, tf_mask, tf_xmask, tf_planes, and tf_tpath are unneeded */

    planeMask = DBTechTypesToPlanes(mask);
    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (PlaneMaskHasPlane(planeMask, pNum))
	    (void) DBSrPaintArea((Tile *) NULL,
		scx->scx_use->cu_def->cd_planes[pNum], &scx->scx_area,
		mask, dbCopyAllPaint, (ClientData) &cxp);
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyLabels --
 *
 * Copy labels from scx->scx_use to targetUse, transforming according to
 * the transform in scx.  Only labels attached to layers of the types
 * specified by mask are copied.  If mask contains the L_LABEL bit, then
 * all labels are copied regardless of their layer.  The area copied is 
 * determined by GEO_LABEL_IN_AREA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the labels in targetUse.  If pArea is given, it will
 *	be filled in with the bounding box of all labels copied.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyLabels(scx, mask, xMask, targetUse, pArea)
    SearchContext *scx;		/* Describes root cell to search, area to
				 * copy, transform from root cell to coords
				 * of targetUse.
				 */
    TileTypeBitMask *mask;	/* Only labels of these types are copied */
    int xMask;			/* Expansion state mask to be used in search */
    CellUse *targetUse;		/* Cell into which labels are to be stuffed */
    Rect *pArea;		/* If non-NULL, points to rectangle to be
				 * filled in with bbox (in targetUse coords)
				 * of all labels copied.  Will be degenerate
				 * if no labels are copied.
				 */
{
    register Label *lab;
    CellDef *def = targetUse->cu_def;
    Rect labTargetRect;
    Rect *rect = &scx->scx_area;
    int targetPos;
    CellUse *sourceUse = scx->scx_use;

    if (pArea != NULL)
    {
	pArea->r_xbot = 0;
	pArea->r_xtop = -1;
    }

    if (!DBIsExpand(sourceUse, xMask))
	return;

    for (lab = sourceUse->cu_def->cd_labels; lab; lab = lab->lab_next)
	if (GEO_LABEL_IN_AREA(&lab->lab_rect, rect) &&
		(TTMaskHasType(mask, lab->lab_type)
		|| TTMaskHasType(mask, L_LABEL)))
	{
	    GeoTransRect(&scx->scx_trans, &lab->lab_rect, &labTargetRect);
	    targetPos = GeoTransPos(&scx->scx_trans, lab->lab_pos);

	    /* Eliminate duplicate labels.  Don't pay any attention to
	     * type when deciding on duplicates, since types can change
	     * later and then we'd have a duplicate.
	     */

	    (void) DBEraseLabelsByContent(def, &labTargetRect, targetPos,
		    -1, lab->lab_text);
	    (void) DBPutLabel(def, &labTargetRect, targetPos,
		    lab->lab_text, lab->lab_type);
	    if (pArea != NULL)
		(void) GeoIncludeAll(&labTargetRect, pArea);
	}
}

/***
 *** Filter function for paint
 ***/

int
dbCopyAllPaint(tile, cxp)
    register Tile *tile;	/* Pointer to tile to copy */
    TreeContext *cxp;		/* Context from DBTreeSrTiles */
{
    register SearchContext *scx = cxp->tc_scx;
    register struct copyAllArg *arg;
    Rect sourceRect, targetRect;
    PaintUndoInfo ui;
    CellDef *def;
    TileType type;
    int pNum;

    /*
     * Don't copy space tiles -- this copy is additive.
     * We should never get passed a space tile, though, because
     * the caller will be using DBSrPaintArea, so this is just
     * a sanity check.
     */
    if ((type = TiGetType(tile)) == TT_SPACE)
	return 0;

    arg = (struct copyAllArg *) cxp->tc_filter->tf_arg;

    /* Construct the rect for the tile in source coordinates */
    TITORECT(tile, &sourceRect);

    /* Transform to target coordinates */
    GEOTRANSRECT(&scx->scx_trans, &sourceRect, &targetRect);

    /* Clip against the target area */
    GEOCLIP(&targetRect, &arg->caa_rect);

    ui.pu_def = def = arg->caa_targetUse->cu_def;
    def->cd_flags |= CDMODIFIED|CDGETNEWSTAMP;
    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (DBPaintOnPlane(type, pNum))
	{
	    ui.pu_pNum = pNum;
	    (*dbCurPaintPlane)(def->cd_planes[pNum], &targetRect,
		dbCurPaintTbl[pNum][type], &ui);
	}

    return (0);
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyAllCells --
 *
 * Copy unexpanded subcells from the tree rooted at scx->scx_use
 * to the subcell plane of targetUse, transforming according to
 * the transform in scx.
 *
 * This effectively "flattens" a cell hierarchy in the sense that
 * all unexpanded subcells in a region (which would appear in the
 * display as bounding boxes) are copied into targetUse without
 * regard for their original location in the hierarchy of scx->scx_use.
 * If an array is unexpanded, it is copied as an array, not as a
 * collection of individual cells.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the cell plane in targetUse.  If pArea is given, it
 *	will be filled in with the total area of all cells copied.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyAllCells(scx, xMask, targetUse, pArea)
    SearchContext *scx;		/* Describes root cell to search, area to
				 * copy, transform from root cell to coords
				 * of targetUse.
				 */
    CellUse *targetUse;		/* Cell into which material is to be stuffed */
    int xMask;			/* Expansion state mask to be used in
				 * searching.  Cells not expanded according
				 * to this mask are copied.  To copy everything
				 * in the subtree under scx->scx_use without
				 * regard to expansion, pass a mask of 0.
				 */
    Rect *pArea;		/* If non-NULL, points to a rectangle to be
				 * filled in with bbox (in targetUse coords)
				 * of all cells copied.  Will be degenerate
				 * if nothing was copied.
				 */
{
    struct copyAllArg arg;
    int dbCellCopyCellsFunc();

    arg.caa_targetUse = targetUse;
    arg.caa_bbox = pArea;
    if (pArea != NULL)
    {
	pArea->r_xbot = 0;		/* Make bounding box empty initially. */
	pArea->r_xtop = -1;
    }
    GeoTransRect(&scx->scx_trans, &scx->scx_area, &arg.caa_rect);

    (void) DBTreeSrCells(scx, xMask, dbCellCopyCellsFunc, (ClientData) &arg);
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBCellCopyCells --
 *
 * Copy all subcells that are immediate children of scx->scx_use->cu_def
 * into the subcell plane of targetUse, transforming according to
 * the transform in scx.  Arrays are copied as arrays, not as a
 * collection of individual cells.  If a cell is already present in
 * targetUse that would be exactly duplicated by a new cell, the new
 * cell isn't copied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the cell plane in targetUse.  If pArea is given, it will
 *	be filled in with the bounding box of all cells copied.
 *
 *-----------------------------------------------------------------------------
 */

void
DBCellCopyCells(scx, targetUse, pArea)
    SearchContext *scx;		/* Describes root cell to search, area to
				 * copy, transform from coords of
				 * scx->scx_use->cu_def to coords of targetUse.
				 */
    CellUse *targetUse;		/* Cell into which material is to be stuffed */
    Rect *pArea;		/* If non-NULL, points to rectangle to be
				 * filled in with bbox (in targetUse coords)
				 * of all cells copied.  Will be degenerate
				 * if nothing was copied.
				 */
{
    struct copyAllArg arg;
    int dbCellCopyCellsFunc();

    arg.caa_targetUse = targetUse;
    arg.caa_bbox = pArea;
    if (pArea != NULL)
    {
	pArea->r_xbot = 0;
	pArea->r_xtop = -1;
    }
    GeoTransRect(&scx->scx_trans, &scx->scx_area, &arg.caa_rect);

    (void) DBCellSrArea(scx, dbCellCopyCellsFunc, (ClientData) &arg);
}

/*
 *-----------------------------------------------------------------------------
 *
 * dbCellCopyCellsFunc --
 *
 * Do the actual work of yanking cells for DBCellCopyAllCells() and
 * DBCellCopyCells() above.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the cell plane in arg->caa_targetUse->cu_def.
 *
 *-----------------------------------------------------------------------------
 */

int
dbCellCopyCellsFunc(scx, arg)
    register SearchContext *scx;	/* Pointer to search context containing
					 * ptr to cell use to be copied,
					 * and transform to the target def.
					 */
    register struct copyAllArg *arg;	/* Client data from caller */
{
    CellUse *use, *newUse;
    CellDef *def;
    int xsep, ysep, xbase, ybase;
    Transform newTrans;

    use = scx->scx_use;
    def = use->cu_def;

    /* Don't allow circular structures! */

    if (DBIsAncestor(def, arg->caa_targetUse->cu_def))
    {
	TxPrintf("Copying %s would create a circularity in the",
	    def->cd_name);
	TxPrintf(" cell hierarchy \n(%s is already its ancestor)",
	    arg->caa_targetUse->cu_def->cd_name);
	TxPrintf(" so cell not copied.\n");
	return 2;
    }

    /* When creating a new use, try to re-use the id from the old
     * one.  Only create a new one if the old id can't be used.
     */

    newUse = DBCellNewUse(def, (char *) use->cu_id);
    if (!DBLinkCell(newUse, arg->caa_targetUse->cu_def))
    {
	FREE((char *) newUse->cu_id);
	newUse->cu_id = NULL;
	(void) DBLinkCell(newUse, arg->caa_targetUse->cu_def);
    }
    newUse->cu_expandMask = use->cu_expandMask;

    /* The translation stuff is funny, since we got one element of
     * the array, but not necessarily the lower-left element.  To
     * get the transform for the array as a whole, subtract off fo
     * the index of the element.  The easiest way to see how this
     * works is to look at the code in dbCellSrFunc;  the stuff here
     * is the opposite.
     */

    if (use->cu_xlo > use->cu_xhi) xsep = -use->cu_xsep;
    else xsep = use->cu_xsep;
    if (use->cu_ylo > use->cu_yhi) ysep = -use->cu_ysep;
    else ysep = use->cu_ysep;
    xbase = xsep * (scx->scx_x - use->cu_xlo);
    ybase = ysep * (scx->scx_y - use->cu_ylo);
    GeoTransTranslate(-xbase, -ybase, &scx->scx_trans, &newTrans);
    DBSetArray(use, newUse);
    DBSetTrans(newUse, &newTrans);
    if (DBCellFindDup(newUse, arg->caa_targetUse->cu_def) != NULL)
    {
	if (!(arg->caa_targetUse->cu_def->cd_flags & CDINTERNAL))
	{
	    TxError("Cell \"%s\" would end up on top of an identical copy\n",
		newUse->cu_id);
	    TxError("    of itself.  I'm going to forget about the");
	    TxError(" new copy.\n");
	}
	DBUnLinkCell(newUse, arg->caa_targetUse->cu_def);
	(void) DBCellDeleteUse(newUse);
    }
    else
    {
	DBPlaceCell(newUse, arg->caa_targetUse->cu_def);
	if (arg->caa_bbox != NULL)
	    (void) GeoIncludeAll(&newUse->cu_bbox, arg->caa_bbox);
    }
    return 2;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBNewPaintTable --
 *
 * 	This procedure changes the paint table to be used by the
 *	DBCellCopyPaint and DBCellCopyAllPaint procedures.
 *
 * Results:
 *	The return value is the address of the paint table that used
 *	to be in effect.  It is up to the client to restore this
 *	value with another call to this procedure.
 *
 * Side effects:
 *	A new paint table takes effect.
 *
 * ----------------------------------------------------------------------------
 */

PaintResultType (*
DBNewPaintTable(newTable))[NT][NT]
    PaintResultType (*newTable)[NT][NT];  /* Address of new paint table. */
{
    PaintResultType (*oldTable)[NT][NT] = dbCurPaintTbl;
    dbCurPaintTbl = newTable;
    return oldTable;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBNewPaintPlane --
 *
 * 	This procedure changes the painting procedure to be used by the
 *	DBCellCopyPaint and DBCellCopyAllPaint procedures.
 *
 * Results:
 *	The return value is the address of the paint procedure that
 *	used to be in effect.  It is up to the client to restore this
 *	value with another call to this procedure.
 *
 * Side effects:
 *	A new paint procedure takes effect.
 *
 * ----------------------------------------------------------------------------
 */

VoidProc
DBNewPaintPlane(newProc)
    Void (*newProc)();		/* Address of new procedure */
{
    Void (*oldProc)() = dbCurPaintPlane;
    dbCurPaintPlane = newProc;
    return (oldProc);
}
