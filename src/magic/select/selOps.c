/*
 * selOps.c --
 *
 * This file contains top-level procedures to manipulate the selection,
 * e.g. to delete it, move it, etc.
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
static char rcsid[]="$Header: selOps.c,v 6.0 90/08/28 18:56:48 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "select.h"
#include "selInt.h"
#include "textio.h"
#include "undo.h"
#include "plow.h"
#include "malloc.h"
#include "drc.h"

/* The following variables are shared between SelectStretch and the
 * search functions that it causes to be invoked.
 */

static int selStretchX, selStretchY;	/* Stretch distances.  Only one should
					 * ever be non-zero.
					 */
static TileType selStretchType;		/* Type of material being stretched. */

/* The following structure type is used to build up a list of areas
 * to be painted.  It's used to save information while a search of
 * the edit cell is in progress:  can't do the paints until the
 * search has finished.
 */

typedef struct stretchArea
{
    Rect sa_area;			/* Area to be painted. */
    TileType sa_type;			/* Type of material to paint. */
    struct stretchArea *sa_next;	/* Next element in list. */
} StretchArea;

static StretchArea *selStretchList;	/* List of areas to paint. */

/*
 * ----------------------------------------------------------------------------
 *
 * SelectDelete --
 *
 * 	Delete everything in the edit cell that's selected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is removed from the edit cell.  If there's selected
 *	stuff that isn't in the edit cell, the user is warned.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectDelete(msg)
    char *msg;		/* Some information to print in error messages.
			 * For example, if called as part of a move procedure,
			 * supply "moved".  This will appear in messages of
			 * the form "only edit cell information was moved".
			 */
{
    bool nonEdit;
    Rect editArea;

    extern int selDelPaintFunc(), selDelCellFunc(), selDelLabelFunc();

    (void) SelEnumPaint(&DBAllButSpaceAndDRCBits, TRUE, &nonEdit,
	    selDelPaintFunc, (ClientData) NULL);
    if (nonEdit)
    {
	TxError("You selected paint outside the edit cell.  Only\n");
	TxError("    the paint in the edit cell was %s.\n", msg);
    }
    (void) SelEnumCells(TRUE, &nonEdit, (SearchContext *) NULL,
	    selDelCellFunc, (ClientData) NULL);
    if (nonEdit)
    {
	TxError("You selected one or more subcells that aren't children\n");
	TxError("    of the edit cell.  Only those in the edit cell were\n");
	TxError("    %s.\n", msg);
    }
    (void) SelEnumLabels(&DBAllTypeBits, TRUE, &nonEdit,
	    selDelLabelFunc, (ClientData) NULL);
    if (nonEdit)
    {
	TxError("You selected one or more labels that aren't in the\n");
	TxError("    edit cell.  Only the label(s) in the edit cell\n");
	TxError("    were %s.\n", msg);
    }

    DBReComputeBbox(EditCellUse->cu_def);
    GeoTransRect(&RootToEditTransform, &SelectDef->cd_bbox, &editArea);
    DBWAreaChanged(EditCellUse->cu_def, &editArea, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKPAINT, &editArea);
    SelectClear();
}

/* Search function to delete paint. */

int
selDelPaintFunc(rect, type)
    Rect *rect;			/* Area of paint, in root coords. */
    TileType type;		/* Type of paint to delete. */
{
    Rect editRect;

    GeoTransRect(&RootToEditTransform, rect, &editRect);
    DBErase(EditCellUse->cu_def, &editRect, type);
    return 0;
}

/* Search function to delete subcell uses. */

    /* ARGSUSED */
int
selDelCellFunc(selUse, use)
    CellUse *selUse;		/* Not used. */
    CellUse *use;		/* What to delete. */
{
    DBUnLinkCell(use, use->cu_parent);
    DBDeleteCell(use);
    (void) DBCellDeleteUse(use);
    return 0;
}


/* Search function to delete labels.  Delete any label at the right
 * place with the right name, regardless of layer attachment, because
 * the selection can differ from the edit cell in this regard. */

int
selDelLabelFunc(label)
    Label *label;		/* Label to delete. */
{
    DBEraseLabelsByContent(EditCellUse->cu_def, &label->lab_rect,
	    label->lab_pos, -1, label->lab_text);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectCopy --
 *
 * 	This procedure makes a copy of the selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is copied, with the copy being transformed by
 *	"transform" relative to the current selection.  The copy is
 *	made the new selection.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectCopy(transform)
    Transform *transform;	/* How to displace the copy relative
				 * to the original.  This displacement
				 * is given in root coordinates.
				 */
{
    SearchContext scx;

    /* Copy from SelectDef to Select2Def while transforming, then
     * let SelectAndCopy2 do the rest of the work.  Don't record
     * anything involving Select2Def for undo-ing.
     */

    UndoDisable();
    DBCellClearDef(Select2Def);
    scx.scx_use = SelectUse;
    scx.scx_area = SelectUse->cu_bbox;
    scx.scx_trans = *transform;
    (void) DBCellCopyAllPaint(&scx, &DBAllButSpaceAndDRCBits, -1, Select2Use);
    (void) DBCellCopyAllLabels(&scx, &DBAllTypeBits, -1, Select2Use,
	(Rect *) NULL);
    (void) DBCellCopyAllCells(&scx, -1, Select2Use, (Rect *) NULL);
    DBReComputeBbox(Select2Def);
    UndoEnable();

    SelectClear();
    SelectAndCopy2(EditRootDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * selTransTo2 --
 *
 * 	This local procedure makes a transformed copy of the selection
 *	in Select2Def, ignoring everything that's not in the edit cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Select2Def gets modified to hold the transformed selection.
 *	Error messages get printed if the selection contains any
 *	non-edit material.
 *
 * ----------------------------------------------------------------------------
 */

void
selTransTo2(transform)
    Transform *transform;	/* How to transform stuff before copying
				 * it to Select2Def.
				 */
{
    int selTransPaintFunc();	/* Forward references. */
    int selTransCellFunc();
    int selTransLabelFunc();

    UndoDisable();
    DBCellClearDef(Select2Def);
    (void) SelEnumPaint(&DBAllButSpaceAndDRCBits, TRUE, (bool *) NULL,
	    selTransPaintFunc, (ClientData) transform);
    (void) SelEnumCells(TRUE, (bool *) NULL, (SearchContext *) NULL,
	    selTransCellFunc, (ClientData) transform);
    (void) SelEnumLabels(&DBAllTypeBits, TRUE, (bool *) NULL,
	    selTransLabelFunc, (ClientData) transform);
    DBReComputeBbox(Select2Def);
    UndoEnable();
}

/* Search function to copy paint.  Always return 1 to keep the search alive. */

int
selTransPaintFunc(rect, type, transform)
    Rect *rect;			/* Area of paint. */
    TileType type;		/* Type of paint. */
    Transform *transform;	/* How to change coords before painting. */
{
    Rect new;

    GeoTransRect(transform, rect, &new);
    DBPaint(Select2Def, &new, type);
    return 0;
}

/* Search function to copy subcells.  Always return 1 to keep the
 * search alive.
 */

    /* ARGSUSED */
int
selTransCellFunc(selUse, realUse, realTrans, transform)
    CellUse *selUse;		/* Use from selection. */
    CellUse *realUse;		/* Corresponding use from layout (used to
				 * get id). */
    Transform *realTrans;	/* Transform for realUse (ignored). */
    Transform *transform;	/* How to change coords of selUse before
				 * copying.
				 */
{
    CellUse  *newUse;
    Transform newTrans;

    newUse = DBCellNewUse(selUse->cu_def, (char *) realUse->cu_id);
    if (!DBLinkCell(newUse, Select2Def))
    {
	freeMagic((char *) newUse->cu_id);
	newUse->cu_id = NULL;
	(void) DBLinkCell(newUse, Select2Def);
    }
    GeoTransTrans(&selUse->cu_transform, transform, &newTrans);
    DBSetArray(selUse, newUse);
    DBSetTrans(newUse, &newTrans);
    newUse->cu_expandMask = selUse->cu_expandMask;
    DBPlaceCell(newUse, Select2Def);

    return 0;
}

/* Search function to copy labels.  Return 0 always to avoid
 * aborting search.
 */

    /* ARGSUSED */
int
selTransLabelFunc(label, cellDef, defTransform, transform)
    Label *label;		/* Label to copy.  This points to label
				 * in cellDef.
				 */
    CellDef *cellDef;		/* Definition containing label in layout. */
    Transform *defTransform;	/* Transform from cellDef to root. */
    Transform *transform;	/* How to modify coords before copying to
				 * Select2Def.
				 */
{
    Rect rootArea, finalArea;
    int rootPos, finalPos;

    GeoTransRect(defTransform, &label->lab_rect, &rootArea);
    rootPos = GeoTransPos(defTransform, label->lab_pos);
    GeoTransRect(transform, &rootArea, &finalArea);
    finalPos = GeoTransPos(transform, rootPos);
    (void) DBPutLabel(Select2Def, &finalArea, finalPos, label->lab_text,
	    label->lab_type);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectTransform --
 *
 * 	This procedure modifies the selection by transforming
 *	it geometrically.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is modified and redisplayed.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectTransform(transform)
    Transform *transform;		/* How to displace the selection.
					 * The transform is in root (user-
					 * visible) coordinates.
					 */
{
    /* Copy from SelectDef to Select2Def, transforming along the way. */

    selTransTo2(transform);

    /* Now just delete the selection and recreate it from Select2Def,
     * copying into the edit cell along the way.
     */

    SelectDelete("modified");
    SelectAndCopy2(EditRootDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectExpand --
 *
 * 	Expand all of the selected cells that are unexpanded, and
 *	unexpand all of those that are expanded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the selected cells will become visible or
 *	invisible on the display in the indicated window(s).
 *
 * ----------------------------------------------------------------------------
 */

void
SelectExpand(mask)
    int mask;			/* Bits of this word indicate which
				 * windows the selected cells will be
				 * expanded in.
				 */
{
    extern int selExpandFunc();	/* Forward reference. */

    (void) SelEnumCells(FALSE, (bool *) NULL, (SearchContext *) NULL,
	    selExpandFunc, (ClientData) mask);
}

    /* ARGSUSED */
int
selExpandFunc(selUse, use, transform, mask)
    CellUse *selUse;		/* Use from selection. */
    CellUse *use;		/* Use to expand (in actual layout). */
    Transform *transform;	/* Not used. */
    int mask;			/* Windows in which to expand. */
{
    /* Don't change expansion status of root cell:  screws up
     * DBWAreaChanged (need to always have at least top-level
     * cell be expanded).
     */

    if (use->cu_parent == NULL)
    {
	TxError("Can't unexpand root cell of window.\n");
	return 0;
    }

    /* Be sure to modify the expansion bit in the selection as well as
     * the one in the layout in order to keep them consistend.
     */

    if (DBIsExpand(use, mask))
    {
	DBExpand(selUse, mask, FALSE);
	DBExpand(use, mask, FALSE);
	DBWAreaChanged(use->cu_parent, &use->cu_bbox, mask,
	    (TileTypeBitMask *) NULL);
    }
    else
    {
	DBExpand(selUse, mask, TRUE);
	DBExpand(use, mask, TRUE);
	DBWAreaChanged(use->cu_parent, &use->cu_bbox, mask, &DBAllButSpaceBits);
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectArray --
 *
 * 	Array everything in the selection.  Cells get turned into
 *	arrays, and paint and labels get replicated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The edit cell is modified in a big way.  It's also redisplayed.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectArray(arrayInfo)
    ArrayInfo *arrayInfo;	/* Describes desired shape of array, all in
				 * root coordinates.
				 */
{
    extern int selArrayPFunc(), selArrayCFunc(), selArrayLFunc();

    /* The way arraying is done is similar to moving:  make an
     * arrayed copy of everything in Select2Def, then delete the
     * selection, then copy everything back from Select2Def and
     * select it.
     */
    
    UndoDisable();
    DBCellClearDef(Select2Def);
    (void) SelEnumPaint(&DBAllButSpaceAndDRCBits, TRUE, (bool *) NULL,
	    selArrayPFunc, (ClientData) arrayInfo);
    (void) SelEnumCells(TRUE, (bool *) NULL, (SearchContext *) NULL,
	    selArrayCFunc, (ClientData) arrayInfo);
    (void) SelEnumLabels(&DBAllTypeBits, TRUE, (bool *) NULL,
	    selArrayLFunc, (ClientData) arrayInfo);
    DBReComputeBbox(Select2Def);
    UndoEnable();

    /* Now just delete the selection and recreate it from Select2Def,
     * copying into the edit cell along the way.
     */
    
    SelectDelete("arrayed");
    SelectAndCopy2(EditRootDef);
}

/* Search function for paint.  Just make many copies of the paint
 * into Select2Def.  Always return 0 to keep the search alive.
 */

int
selArrayPFunc(rect, type, arrayInfo)
    Rect *rect;			/* Rectangle to be arrayed. */
    TileType type;		/* Type of tile. */
    ArrayInfo *arrayInfo;	/* How to array. */
{
    int y, nx, ny;
    Rect current;

    nx = arrayInfo->ar_xhi - arrayInfo->ar_xlo;
    if (nx < 0) nx = -nx;
    ny = arrayInfo->ar_yhi - arrayInfo->ar_ylo;
    if (ny < 0) ny = -ny;

    current = *rect;
    for ( ; nx >= 0; nx -= 1)
    {
	current.r_ybot = rect->r_ybot;
	current.r_ytop = rect->r_ytop;
	for (y = ny; y >= 0; y -= 1)
	{
	    DBPaint(Select2Def, &current, type);
	    current.r_ybot += arrayInfo->ar_ysep;
	    current.r_ytop += arrayInfo->ar_ysep;
	}
	current.r_xbot += arrayInfo->ar_xsep;
	current.r_xtop += arrayInfo->ar_xsep;
    }
    return 0;
}

/* Search function for cells.  Just make an arrayed copy of
 * each subcell found.
 */

    /* ARGSUSED */
int
selArrayCFunc(selUse, use, transform, arrayInfo)
    CellUse *selUse;		/* Use from selection (not used). */
    CellUse *use;		/* Use to be copied and arrayed. */
    Transform *transform;	/* Transform from use->cu_def to root. */
    ArrayInfo *arrayInfo;	/* Array characteristics desired. */
{
    CellUse *newUse;
    Transform tinv, newTrans;
    Rect tmp, oldBbox;

    /* When creating a new use, try to re-use the id from the old
     * one.  Only create a new one if the old id can't be used.
     */

    newUse = DBCellNewUse(use->cu_def, (char *) use->cu_id);
    if (!DBLinkCell(newUse, Select2Def))
    {
	freeMagic((char *) newUse->cu_id);
	newUse->cu_id = NULL;
	(void) DBLinkCell(newUse, Select2Def);
    }
    newUse->cu_expandMask = use->cu_expandMask;

    DBSetTrans(newUse, transform);
    GeoInvertTrans(transform, &tinv);
    DBMakeArray(newUse, &tinv, arrayInfo->ar_xlo,
	arrayInfo->ar_ylo, arrayInfo->ar_xhi, arrayInfo->ar_yhi,
	arrayInfo->ar_xsep, arrayInfo->ar_ysep);
    
    /* Set the array's transform so that its lower-left corner is in
     * the same place that it used to be.
     */
    
    GeoInvertTrans(&use->cu_transform, &tinv);
    GeoTransRect(&tinv, &use->cu_bbox, &tmp);
    GeoTransRect(transform, &tmp, &oldBbox);
    GeoTranslateTrans(&newUse->cu_transform,
	    oldBbox.r_xbot - newUse->cu_bbox.r_xbot,
	    oldBbox.r_ybot - newUse->cu_bbox.r_ybot,
	    &newTrans);
    DBSetTrans(newUse, &newTrans);

    if (DBCellFindDup(newUse, Select2Def) != NULL)
    {
	DBUnLinkCell(newUse, Select2Def);
	(void) DBCellDeleteUse(newUse);
    }
    else DBPlaceCell(newUse, Select2Def);

    return 0;
}

/* Search function for labels.  Similar to paint search function. */

    /* ARGSUSED */
int
selArrayLFunc(label, def, transform, arrayInfo)
    Label *label;		/* Label to be copied and replicated. */
    CellDef *def;		/* Definition containing label. */
    Transform *transform;	/* Transform from coords of def to root. */
    ArrayInfo *arrayInfo;	/* How to replicate. */
{
    int y, nx, ny, rootPos;
    Rect original, current;

    nx = arrayInfo->ar_xhi - arrayInfo->ar_xlo;
    if (nx < 0) nx = -nx;
    ny = arrayInfo->ar_yhi - arrayInfo->ar_ylo;
    if (ny < 0) ny = -ny;

    GeoTransRect(transform, &label->lab_rect, &original);
    rootPos = GeoTransPos(transform, label->lab_pos);
    current = original;
    for ( ; nx >= 0; nx -= 1)
    {
	current.r_ybot = original.r_ybot;
	current.r_ytop = original.r_ytop;
	for (y = ny; y >= 0; y -= 1)
	{
	    /* Eliminate any duplicate labels.  Don't use type in comparing
	     * for duplicates, because the selection's type could change
	     * after it gets added to the edit cell.  Any label with
	     * the same text and position is considered a duplicate.
	     */
	    DBEraseLabelsByContent(Select2Def, &current, rootPos,
		-1, label->lab_text);
	    (void) DBPutLabel(Select2Def, &current, rootPos, label->lab_text,
		label->lab_type);
	    current.r_ybot += arrayInfo->ar_ysep;
	    current.r_ytop += arrayInfo->ar_ysep;
	}
	current.r_xbot += arrayInfo->ar_xsep;
	current.r_xtop += arrayInfo->ar_xsep;
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *	SelectStretch --
 *
 * 	Move the selection a given amount in x (or y).  While moving,
 *	erase everything that the selection passes over, and stretch
 *	material behind the selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The edit cell is modified.  The selection is also modified
 *	and redisplayed.
 * ----------------------------------------------------------------------------
 */

void
SelectStretch(x, y)
    int x;			/* Amount to move in the x-direction. */
    int y;			/* Amount to move in the y-direction.  Must
				 * be zero if x is non-zero.
				 */
{
    Transform transform;
    int plane;
    Rect modifiedArea, editModified;
    extern int selStretchEraseFunc(), selStretchFillFunc();
				/* Forward declarations. */

    if ((x == 0) && (y == 0)) return;

    /* First of all, copy from SelectDef to Select2Def, moving the
     * selection along the way.
     */
    
    GeoTranslateTrans(&GeoIdentityTransform, x, y, &transform);
    selTransTo2(&transform);

    /* We're going to modify not just the old selection area or the new
     * one, but everything in-between too.  Remember this and tell the
     * displayer and DRC about it later.
     */

    modifiedArea = Select2Def->cd_bbox;
    (void) GeoInclude(&SelectDef->cd_bbox, &modifiedArea);
    GeoTransRect(&RootToEditTransform, &modifiedArea, &editModified);

    /* Delete the selection itself. */

    SelectDelete("stretched");

    /* Next, delete all the material in front of each piece of paint in
     * the selection.
     */
    
    selStretchX = x;
    selStretchY = y;
    for (plane = PL_SELECTBASE; plane < DBNumPlanes; plane += 1)
    {
	(void) DBSrPaintArea((Tile *) NULL, Select2Def->cd_planes[plane],
		&TiPlaneRect, &DBAllButSpaceAndDRCBits, selStretchEraseFunc,
		(ClientData) NULL);
    }

    /* To achieve the stretch affect, fill in material behind the selection
     * everywhere that it used to touch other material in the edit cell.
     * This code first builds up a list of areas to paint, then paints them
     * (can't paint as we go because the new paint interacts with the
     * computation of what to stretch).
     */

    selStretchList = NULL;
    for (plane = PL_SELECTBASE; plane < DBNumPlanes; plane += 1)
    {
	(void) DBSrPaintArea((Tile *) NULL, Select2Def->cd_planes[plane],
		&TiPlaneRect, &DBAllButSpaceAndDRCBits, selStretchFillFunc,
		(ClientData) NULL);
    }

    /* Paint back the areas in the list. */

    while (selStretchList != NULL)
    {
	DBPaint(EditCellUse->cu_def, &selStretchList->sa_area,
		selStretchList->sa_type);
	freeMagic((char *) selStretchList);
	selStretchList = selStretchList->sa_next;
    }

    /* Paint the new translated selection back into the edit cell,
     * select it again, and tell DRC and display about what we
     * changed.
     */
    
    SelectAndCopy2(EditRootDef);
    DBWAreaChanged(EditCellUse->cu_def, &editModified, DBW_ALLWINDOWS,
	    (TileTypeBitMask *) NULL);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKPAINT, &editModified);
}

/*
 * ----------------------------------------------------------------------------
 *	selStretchEraseFunc --
 *
 * 	Called by DBSrPaintArea during stretching for each tile in the
 *	new selection.  Erase the area that the tile swept out as it
 *	moved.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	The edit cell is modified.
 * ----------------------------------------------------------------------------
 */

int
selStretchEraseFunc(tile)
    Tile *tile;			/* Tile being moved in a stretch operation. */
{
    Rect area, editArea;
    TileType type;

    type = TiGetType(tile);

    TiToRect(tile, &area);

    /* Compute the area that this tile swept out (the current area is
     * its location AFTER moving), and erase everything that was in
     * its path.
     */

    if (selStretchX > 0)
	area.r_xbot -= selStretchX;
    else area.r_xtop -= selStretchX;
    if (selStretchY > 0)
	area.r_ybot -= selStretchY;
    else area.r_ytop -= selStretchY;

    /* Translate into edit coords and erase all material on the
     * tile's plane.
     */
    
    GeoTransRect(&RootToEditTransform, &area, &editArea);
    DBEraseMask(EditCellUse->cu_def, &editArea, &DBPlaneTypes[DBPlane(type)]);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *	selStretchFillFunc --
 *
 * 	This function is invoked during stretching for each paint tile in
 *	the (new) selection.  It finds places in where the back-side of this
 *	tile borders space in the (new) selection, then looks for paint in
 *	the edit cell that borders the old location of the paint.  If the
 *	selection has been moved away from paint in the edit cell, additional
 *	material is filled in behind the selection.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	Modifies the edit cell by painting material.
 * ----------------------------------------------------------------------------
 */

int
selStretchFillFunc(tile)
    Tile *tile;			/* Tile in the old selection. */
{
    Rect area;
    extern int selStretchFillFunc2();

    selStretchType = TiGetType(tile);
    TiToRect(tile, &area);

    /* Check the material just behind this paint (in the sense of the
     * stretch direction) for space in the selection and non-space in
     * the edit cell.
     */
    
    if (selStretchX > 0)
    {
	area.r_xtop = area.r_xbot;
	area.r_xbot -= 1;
    }
    else if (selStretchX < 0)
    {
	area.r_xbot = area.r_xtop;
	area.r_xtop += 1;
    }
    else if (selStretchY > 0)
    {
	area.r_ytop = area.r_ybot;
	area.r_ybot -= 1;
    }
    else
    {
	area.r_ybot = area.r_ytop;
	area.r_ytop += 1;
    }

    /* The search functions invoked indirectly by the following procedure
     * call build up a list of areas to paint.
     */

    (void) DBSrPaintArea((Tile *) NULL,
	    Select2Def->cd_planes[DBPlane(selStretchType)], &area,
	    &DBSpaceBits, selStretchFillFunc2, (ClientData) &area);
    
    return 0;
}

/* Second-level filling search function:  find all of the edit material
 * that intersects areas where space borders a selected paint tile.
 */

int
selStretchFillFunc2(tile, area)
    Tile *tile;				/* Space tile that borders selected
					 * paint.
					 */
    Rect *area;				/* A one-unit wide strip along the
					 * border (i.e. the area in which
					 * we're interested in space).
					 */
{
    Rect spaceArea, editArea;
    extern int selStretchFillFunc3();

    TiToRect(tile, &spaceArea);

    /* Find out which portion of this space tile borders the selected
     * tile, transform it back to coords of the old selection and then
     * to edit coords, and find all the edit material that borders the
     * selected tile in this area.
     */

    GeoClip(&spaceArea, area);
    spaceArea.r_xbot -= selStretchX;
    spaceArea.r_xtop -= selStretchX;
    spaceArea.r_ybot -= selStretchY;
    spaceArea.r_ytop -= selStretchY;
    GeoTransRect(&RootToEditTransform, &spaceArea, &editArea);

    (void) DBSrPaintArea((Tile *) NULL,
	    EditCellUse->cu_def->cd_planes[DBPlane(selStretchType)],
	    &editArea, &DBAllButSpaceAndDRCBits, selStretchFillFunc3,
	    (ClientData) &spaceArea);

    return 0;
}

/* OK, now we've found a piece of material in the edit cell that is
 * right next to a piece of selected material that's about to move
 * away from it.  Stretch one or the other to fill the gap.  Use the
 * material that's moving as the stretch material unless it's a fixed-size
 * material and the other stuff is stretchable.
 */

int
selStretchFillFunc3(tile, area)
    Tile *tile;			/* Tile of edit material that's about to
				 * be left behind selection.
				 */
    Rect *area;			/* Border area we're interested in, in 
    				 * root coords.
				 */
{
    Rect editArea, rootArea;
    TileType type;
    register TileTypeBitMask *mask;
    StretchArea *sa;

    /* Compute the area to be painted. */

    TiToRect(tile, &editArea);
    GeoTransRect(&EditToRootTransform, &editArea, &rootArea);
    GeoClip(&rootArea, area);
    if (selStretchX > 0)
    {
	rootArea.r_xbot = rootArea.r_xtop;
	rootArea.r_xtop += selStretchX;
    }
    else if (selStretchX < 0)
    {
	rootArea.r_xtop = rootArea.r_xbot;
	rootArea.r_xbot += selStretchX;
    }
    else if (selStretchY > 0)
    {
	rootArea.r_ybot = rootArea.r_ytop;
	rootArea.r_ytop += selStretchY;
    }
    else
    {
	rootArea.r_ytop = rootArea.r_ybot;
	rootArea.r_ybot += selStretchY;
    }
    GeoTransRect(&RootToEditTransform, &rootArea, &editArea);

    /* Compute the material to be painted.  Be careful:  for contacts,
     * must use the master image.
     */
    
    type = TiGetType(tile);
#ifndef NO_PLOW
    if (TTMaskHasType(&PlowFixedTypes, type)
	    || !TTMaskHasType(&PlowFixedTypes, selStretchType))
	type = selStretchType;
#endif
    if (type >= DBNumUserLayers)
    {
	mask = &DBLayerTypeMaskTbl[type];
	for (type = TT_SELECTBASE; !TTMaskHasType(mask, type); type += 1)
	    /* Null loop body. */;
    }

    /* Save around the area we just found. */

    sa = (StretchArea *) mallocMagic(sizeof(StretchArea));
    sa->sa_area = editArea;
    sa->sa_type = type;
    sa->sa_next = selStretchList;
    selStretchList = sa;

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelectDump --
 *
 *      Copies an area of one cell into the edit cell, selecting the
 *	copy so that it can be manipulated later.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The edit cell is modified.
 *
 * ----------------------------------------------------------------------------
 */

void
SelectDump(scx)
    SearchContext *scx;			/* Describes the cell from which
					 * material is to be copied, the
					 * area to copy, and the transform
					 * to root coordinates in the edit
					 * cell's window.
					 */
{
    /* Copy from the source cell to Select2Def while transforming,
     * then let SelectandCopy2 do the rest of the work.  Don't
     * record any of the Select2Def changes for undo-ing.
     */

    UndoDisable();
    DBCellClearDef(Select2Def);
    (void) DBCellCopyAllPaint(scx, &DBAllButSpaceAndDRCBits, -1, Select2Use);
    (void) DBCellCopyAllLabels(scx, &DBAllTypeBits, -1, Select2Use,
	    (Rect *) NULL);
    (void) DBCellCopyAllCells(scx, -1, Select2Use, (Rect *) NULL);
    DBReComputeBbox(Select2Def);
    UndoEnable();

    SelectClear();
    SelectAndCopy2(EditRootDef);
}
