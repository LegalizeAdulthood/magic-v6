/*
 * DBundo.c --
 *
 * Interface to the undo package for the database.
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
static char rcsid[] = "$Header: DBundo.c,v 6.0 90/08/28 18:10:34 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "malloc.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "undo.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "utils.h"
#include "drc.h"

/***
 *** Identifiers for each of the clients defined here
 ***/
UndoType dbUndoIDPaint;
UndoType dbUndoIDPutLabel, dbUndoIDEraseLabel;
UndoType dbUndoIDOpenCell, dbUndoIDCloseCell;
UndoType dbUndoIDCellUse;

/***
 *** Functions to play events forward/backward.
 ***/
    /* Paint */
void dbUndoPaintForw(), dbUndoPaintBack();

    /* Labels */
void dbUndoLabelForw(), dbUndoLabelBack();

    /* Change in edit cell def */
void dbUndoOpenCell(), dbUndoCloseCell();

    /* Cell uses */
void dbUndoCellForw(), dbUndoCellBack();

/***
 *** Functions invoked at beginning and end
 *** of an undo/redo command.
 ***/
void dbUndoInit();
CellUse *findUse();

/***
 *** The following points to the CellDef specified in the most
 *** recent database undo operation.  If, when recording the undo
 *** information for a new database operation, the cell def being
 *** modified is different from dbUndoLastCell, we record a special
 *** record on the undo list.
 ***
 *** This strategy "differentially encodes" changes in the cell def
 *** affected during the course of undo.
 ***/
CellDef *dbUndoLastCell;

/*
 * Redisplay for undoing database changes:
 * As we play the undo log backwards or forwards, we keep track
 * of a bounding rectangle, dbUndoAreaChanged for the area changed.
 * We rely on the fact that most database operations are over a
 * compact local area, so keeping around a single rectangular area
 * isn't too bad a compromise.
 *
 * When the edit cell changes, though, we need to call the redisplay
 * package with what we've accumulated, recompute the bounding box of
 * the old edit cell, and then start from scratch again.  The cell def
 * we will pass to the redisplay package is dbUndoLastCell.
 *
 * The flag dbUndoUndid records whether there have been any undo
 * events processed since the last time redisplay and bounding box
 * recomputation were done.
 */
Rect dbUndoAreaChanged;
bool dbUndoUndid;

/*
 * ----------------------------------------------------------------------------
 *
 * DBUndoInit --
 *
 * Initialize the database part of the undo package.
 * Makes the functions contained in here known to the
 * undo module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the undo package.
 *
 * ----------------------------------------------------------------------------
 */

DBUndoInit()
{
    void (*nullProc)() = NULL;

    /* Paint: only one client is needed since paint/erase are inverses */
    dbUndoIDPaint = UndoAddClient(dbUndoInit, dbUndoCloseCell,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoPaintForw, dbUndoPaintBack, "paint");


    /* Labels */
    dbUndoIDPutLabel = UndoAddClient(nullProc, nullProc,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoLabelForw, dbUndoLabelBack, "put label");
    dbUndoIDEraseLabel = UndoAddClient(nullProc, nullProc,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoLabelBack, dbUndoLabelForw, "erase label");

    /*
     * Changes in the current target cell of undo for paint/erase/labels.
     * This client is used only inside this file.  Its purpose is
     * to let us save space and time in paint, erase and label undo
     * events.  We maintain dbUndoLastCell to be a pointer to the
     * CellDef last passed to the database undo package when recording
     * a paint, erase, or label undo event.  Only when this changes
     * is it necessary to record the fact on the undo list.  Hence
     * we avoid having to store the cell def affected with each paint,
     * erase, and label undo event.
     */
    dbUndoIDOpenCell = UndoAddClient(nullProc, nullProc,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoOpenCell, dbUndoCloseCell, "open cell");
    dbUndoIDCloseCell = UndoAddClient(nullProc, nullProc,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoCloseCell, dbUndoOpenCell, "close cell");

    /*
     * Celluses: one client is used for all purposes since we store
     * the action in the undo event.  (We let the undo client encode
     * this information for paint and labels only because there are
     * so many of them that saving space is important).
     */
    dbUndoIDCellUse = UndoAddClient(nullProc, nullProc,
			(UndoEvent *(*)()) NULL, (int (*)()) NULL,
			dbUndoCellForw, dbUndoCellBack, "modify cell use");

    dbUndoLastCell = (CellDef *) NULL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoInit --
 *
 * Initialize for playing undo events forward/backward for the
 * database module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the changed area.
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoInit()
{
    dbUndoUndid = FALSE;
    dbUndoAreaChanged.r_xbot = dbUndoAreaChanged.r_xtop = 0;
    dbUndoAreaChanged.r_ybot = dbUndoAreaChanged.r_ytop = 0;
}

/*
 * ============================================================================
 *
 *				PAINT
 *
 * ============================================================================
 */

/***
 *** The procedures to record paint undo events have been expanded
 *** in-line in DBPaintPlane() for speed.
 ***/

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoPaintForw --
 * dbUndoPaintBack --
 *
 * Play forward/backward a paint undo event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database.
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoPaintForw(up)
    register paintUE *up;
{
    DBPaintPlane(dbUndoLastCell->cd_planes[up->pue_plane], &up->pue_rect,
			DBStdEraseTbl(up->pue_oldtype, up->pue_plane),
			(PaintUndoInfo *) NULL);
    DBPaintPlane(dbUndoLastCell->cd_planes[up->pue_plane], &up->pue_rect,
			DBStdPaintTbl(up->pue_newtype, up->pue_plane),
			(PaintUndoInfo *) NULL);
    dbUndoUndid = TRUE;
    (void) GeoInclude(&up->pue_rect, &dbUndoAreaChanged);
    (void) DRCCheckThis(dbUndoLastCell, TT_CHECKPAINT, &up->pue_rect);
}

void
dbUndoPaintBack(up)
    register paintUE *up;
{
    DBPaintPlane(dbUndoLastCell->cd_planes[up->pue_plane], &up->pue_rect,
			DBStdEraseTbl(up->pue_newtype, up->pue_plane),
			(PaintUndoInfo *) NULL);
    DBPaintPlane(dbUndoLastCell->cd_planes[up->pue_plane], &up->pue_rect,
			DBStdPaintTbl(up->pue_oldtype, up->pue_plane),
			(PaintUndoInfo *) NULL);

    dbUndoUndid = TRUE;
    (void) GeoInclude(&up->pue_rect, &dbUndoAreaChanged);
    (void) DRCCheckThis(dbUndoLastCell, TT_CHECKPAINT, &up->pue_rect);
}

/*
 * ============================================================================
 *
 *				LABELS
 *
 * ============================================================================
 */

    typedef struct
    {
	Rect	 lue_rect;	/* Location of label */
	char	 lue_pos;	/* Relative position of label */
	char	 lue_type;	/* Type of tile labelled */
	char	 lue_text[4];	/* Text of label.  This is a place
				 * holder only; the actual structure
				 * is allocated to hold all the bytes
				 * in the label, plus the null byte.
				 */
    } labelUE;

    /*
     * labelSize(n) is the size of a labelUE large enough to hold
     * a string of n characters.  Space for the trailing NULL byte
     * is allocated automatically.
     */

#define	labelSize(n)	(sizeof (labelUE) - 3 + (n))

/*
 * ----------------------------------------------------------------------------
 *
 * DBUndoPutLabel --
 *
 * Record on the undo list the painting of a new label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the undo list.
 *
 * ----------------------------------------------------------------------------
 */

void
DBUndoPutLabel(cellDef, rect, pos, text, type)
    CellDef *cellDef;	/* CellDef being modified */
    register Rect *rect;/* Box definining label.  The lower left coordinate
			 * of the box determines the tile to which the label
			 * is attached, although it needn't be the physically
			 * lower left coordinate of the box.
			 */
    int pos;		/* Relative position to point */
    char *text;		/* Text of label */
    TileType type;	/* Type of tile being labelled */
{
    register labelUE *lup;

    if (!UndoIsEnabled())
	return;

    if (cellDef != dbUndoLastCell) dbUndoEdit(cellDef);
    lup = (labelUE *) UndoNewEvent(dbUndoIDPutLabel,
			(unsigned) labelSize(strlen(text)));
    if (lup == (labelUE *) NULL)
	return;

    lup->lue_rect = *rect;
    lup->lue_pos = pos;
    lup->lue_type = type;
    strcpy(lup->lue_text, text);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBUndoEraseLabel --
 *
 * Record on the undo list the erasing of an existing label
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the undo list.
 *
 * ----------------------------------------------------------------------------
 */

void
DBUndoEraseLabel(cellDef, rect, pos, text, type)
    CellDef *cellDef;	/* Cell being modified */
    register Rect *rect;/* Box definining label.  The lower left coordinate
			 * of the box determines the tile to which the label
			 * is attached, although it needn't be the physically
			 * lower left coordinate of the box.
			 */
    int pos;		/* Relative position to point */
    char *text;		/* Text of label */
    TileType type;	/* Type of tile being labelled */
{
    register labelUE *lup;

    if (!UndoIsEnabled())
	return;

    if (cellDef != dbUndoLastCell) dbUndoEdit(cellDef);
    lup = (labelUE *) UndoNewEvent(dbUndoIDEraseLabel,
			(unsigned) labelSize(strlen(text)));
    if (lup == (labelUE *) NULL)
	return;

    lup->lue_rect = *rect;
    lup->lue_pos = pos;
    lup->lue_type = type;
    strcpy(lup->lue_text, text);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoLabelForw --
 * dbUndoLabelBack --
 *
 * Play forward/backward a label undo event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database.
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoLabelForw(up)
    register labelUE *up;
{
    (void) DBPutLabel(dbUndoLastCell, &up->lue_rect, up->lue_pos,
	up->lue_text, up->lue_type);
    DBWLabelChanged(dbUndoLastCell, up->lue_text, &up->lue_rect,
	up->lue_pos, DBW_ALLWINDOWS);

    /*
     * Record that this cell def has changed, for bounding box
     * recomputation.  This is only necessary for labels attached
     * to space; labels attached to material will only appear or
     * disappear during undo/redo if the material to which they
     * were attached changes.
     */
    if (up->lue_type == TT_SPACE)
	dbUndoUndid = TRUE;
}

void
dbUndoLabelBack(up)
    register labelUE *up;
{
    (void) DBEraseLabelsByContent(dbUndoLastCell, &up->lue_rect,
		up->lue_pos, up->lue_type, up->lue_text);

    /*
     * Record that this cell def has changed, for bounding box
     * recomputing.  See the comments in dbUndoLabelForw above.
     */
    if (up->lue_type == TT_SPACE)
	dbUndoUndid = TRUE;
}

/*
 * ============================================================================
 *
 *				CELL MANIPULATION
 *
 * ============================================================================
 */

    typedef struct
    {
	/* Type of this event */
	int		 cue_action;

	/*
	 * The remainder contains a copy of the important
	 * information from the CellUse.
	 */
	unsigned	 cue_expandMask;
	unsigned	 cue_flags;
	Transform	 cue_transform;
	ArrayInfo	 cue_array;
	CellDef		*cue_def;
	CellDef		*cue_parent;
	Rect		 cue_bbox;
	char		 cue_id[4];
    } cellUE;

    /*
     * Compute the size of a cellUE, with sufficient space
     * at the end to store the use id.
     */
#define	cellSize(n)	(sizeof (cellUE) - 3 + (n))

/*
 * ----------------------------------------------------------------------------
 *
 * DBUndoCellUse --
 *
 * Record one of the following subcell actions:
 *	UNDO_CELL_PLACE		placement in a parent
 *	UNDO_CELL_DELETE	removal from a parent
 *	UNDO_CELL_CLRID		deleting the use id
 *	UNDO_CELL_SETID		setting the use id
 *
 * The last two, deleting and setting the use id, normally occur in
 * pairs except when the name is set for the first time.
 *
 * Because both the parent and child cell uses are stored
 * in the def, we don't bother to use or update dbUndoLastCell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the undo list.
 *
 * ----------------------------------------------------------------------------
 */

void
DBUndoCellUse(use, action)
    register CellUse *use;
    int action;
{
    register cellUE *up;

    up = (cellUE *) UndoNewEvent(dbUndoIDCellUse,
			(unsigned) cellSize(strlen(use->cu_id)));
    if (up == (cellUE *) NULL)
	return;

    up->cue_action = action;
    up->cue_transform = use->cu_transform;
    up->cue_array = use->cu_array;
    up->cue_def = use->cu_def;
    up->cue_parent = use->cu_parent;
    up->cue_flags = use->cu_flags;
    up->cue_expandMask = use->cu_expandMask;
    up->cue_bbox = use->cu_bbox;
    strcpy(up->cue_id, use->cu_id);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoCellForw --
 * dbUndoCellBack --
 *
 * Play a celluse undo event forward or backward.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the database.
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoCellForw(up)
    register cellUE *up;
{
    register CellUse *use;

    switch (up->cue_action)
    {
	case UNDO_CELL_PLACE:
	    use = DBCellNewUse(up->cue_def, (char *) NULL);
	    use->cu_transform = up->cue_transform;
	    use->cu_array = up->cue_array;
	    use->cu_flags = up->cue_flags;
	    use->cu_expandMask = up->cue_expandMask;
	    use->cu_bbox = up->cue_bbox;
	    use->cu_id = StrDup((char **) NULL, up->cue_id);
	    (void) DBLinkCell(use, up->cue_parent);
	    DBPlaceCell(use, up->cue_parent);
	    DBReComputeBbox(up->cue_parent);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox, DBW_ALLWINDOWS,
		(TileTypeBitMask *) NULL);
	    (void) DRCCheckThis(up->cue_parent, TT_CHECKSUBCELL, &up->cue_bbox);
	    break;
        case UNDO_CELL_DELETE:
	    use = findUse(up, TRUE);
	    DBUnLinkCell(use, up->cue_parent);
	    DBDeleteCell(use);
	    (void) DBCellDeleteUse(use);
	    DBReComputeBbox(up->cue_parent);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox, DBW_ALLWINDOWS,
		 (TileTypeBitMask *) NULL);
	    (void) DRCCheckThis(up->cue_parent, TT_CHECKSUBCELL, &up->cue_bbox);
	    break;
	/*
	 * We rely upon the fact that a UNDO_CELL_CLRID undo event is
	 * always followed immediately by a UNDO_CELL_SETID event.
	 * We also depend on the fact that no cell use ever has a
	 * null use id when it is linked into a parent def.
	 */
        case UNDO_CELL_SETID:
	    use = findUse(up, FALSE);	/* Find the one with the null id */
	    (void) DBReLinkCell(use, up->cue_id);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox,
			(int) ~use->cu_expandMask, &DBAllButSpaceBits);
	    break;
	/*
	 * The following is a hack.
	 * We clear out the use id of the cell so that
	 * findUse() will find it on the next time around,
	 * which should be when we process a UNDO_CELL_SETID
	 * event.
	 */
	case UNDO_CELL_CLRID:
	    use = findUse(up, TRUE);	/* Find it with current id */
	    DBUnLinkCell(use, up->cue_parent);
	    FREE(use->cu_id);
	    use->cu_id = (char *) NULL;
	    break;
    }
}

void
dbUndoCellBack(up)
    register cellUE *up;
{
    register CellUse *use;

    switch (up->cue_action)
    {
	case UNDO_CELL_DELETE:
	    use = DBCellNewUse(up->cue_def, (char *) NULL);
	    use->cu_transform = up->cue_transform;
	    use->cu_array = up->cue_array;
	    use->cu_flags = up->cue_flags;
	    use->cu_expandMask = up->cue_expandMask;
	    use->cu_bbox = up->cue_bbox;
	    use->cu_id = StrDup((char **) NULL, up->cue_id);
	    (void) DBLinkCell(use, up->cue_parent);
	    DBPlaceCell(use, up->cue_parent);
	    DBReComputeBbox(up->cue_parent);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox, DBW_ALLWINDOWS,
		(TileTypeBitMask *) NULL);
	    (void) DRCCheckThis(up->cue_parent, TT_CHECKSUBCELL, &up->cue_bbox);
	    break;
        case UNDO_CELL_PLACE:
	    use = findUse(up, TRUE);
	    DBUnLinkCell(use, up->cue_parent);
	    DBDeleteCell(use);
	    (void) DBCellDeleteUse(use);
	    DBReComputeBbox(up->cue_parent);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox, DBW_ALLWINDOWS,
		(TileTypeBitMask *) NULL);
	    (void) DRCCheckThis(up->cue_parent, TT_CHECKSUBCELL, &up->cue_bbox);
	    break;
	/*
	 * We rely upon the fact that a UNDO_CELL_CLRID undo event is
	 * always followed immediately by a UNDO_CELL_SETID event.
	 * We also depend on the fact that no cell use ever has a
	 * null use id when it is linked into a parent def.
	 */
        case UNDO_CELL_CLRID:
	    use = findUse(up, FALSE);	/* Find it with a NULL id */
	    (void) DBReLinkCell(use, up->cue_id);
	    DBWAreaChanged(up->cue_parent, &up->cue_bbox,
		(int) ~use->cu_expandMask, &DBAllButSpaceBits);
	    break;
	/*
	 * The following is a hack.
	 * We clear out the use id of the cell so that
	 * findUse() will find it on the next time around,
	 * which should be when we process a UNDO_CELL_SETID
	 * event.
	 */
	case UNDO_CELL_SETID:
	    use = findUse(up, TRUE);	/* Find it with current id */
	    DBUnLinkCell(use, up->cue_parent);
	    FREE(use->cu_id);
	    use->cu_id = (char *) NULL;
	    break;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * findUse --
 *
 * Find the cell use corresponding to the cellUE supplied.
 * If 'matchName' is FALSE, we search for a use with a null
 * use ID instead of one matching the use id of the undo event.
 * This is a clear hack that results from using two kinds of
 * undo event to record name changes.
 *
 * Results:
 *	Returns a pointer to a CellUse.
 *
 * Side effects:
 *	None.
 *	Aborts if it can't find the cell use.
 *
 * ----------------------------------------------------------------------------
 */
 
CellUse *
findUse(up, matchName)
    register cellUE *up;
    bool matchName;
{
    register CellUse *use;

    for (use = up->cue_def->cd_parents; use; use = use->cu_nextuse)
	if (use->cu_parent == up->cue_parent)
	{
	    if (matchName)
	    {
		if (strcmp(use->cu_id, up->cue_id) == 0)
		    return use;
	    }
	    else
	    {
		if (use->cu_id == (char *) NULL)
		    return use;
	    }
	}

    ASSERT(FALSE, "findUse: use == NULL");
    return (CellUse *) NULL;
}

/*
 * ============================================================================
 *
 *			    CHANGE IN "EDIT" CELL
 *
 * ============================================================================
 */

    typedef struct
    {
	char	 eue_name[4];	/* Name of cell def edited.  This is
				 * a place holder only, the actual
				 * structure is allocated to hold all
				 * the bytes in the def name, plus
				 * the null byte.
				 */
    } editUE;

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoEdit --
 *
 * Record a change in the cell currently being modified by database
 * operations.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the undo list.
 *	Sets dbUndoLastCell to the CellDef supplied.
 *
 * ----------------------------------------------------------------------------
 */

dbUndoEdit(new)
    register CellDef *new;
{
    register editUE *up;
    register CellDef *old = dbUndoLastCell;

    ASSERT(new != old, "dbUndoEdit");

    /*
     * The old cell def can be NULL, eg, when we're at the beginning
     * of the undo log.  If it is NULL, we don't want to create a close
     * record to close the old cell.
     */
    if (old)
    {
	up = (editUE *) UndoNewEvent(dbUndoIDCloseCell,
			(unsigned) strlen(old->cd_name) + 1);
	if (up == (editUE *) NULL)
	    return;
	strcpy(up->eue_name, old->cd_name);
    }

    up = (editUE *) UndoNewEvent(dbUndoIDOpenCell,
		(unsigned) strlen(new->cd_name) + 1);
    if (up == (editUE *) NULL)
	return;
    strcpy(up->eue_name, new->cd_name);
    dbUndoLastCell = new;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoOpenCell --
 *
 * Set dbUndoLastCell
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets dbUndoLastCell
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoOpenCell(eup)
    editUE *eup;
{
    CellDef *newDef;

    newDef = DBCellLookDef(eup->eue_name);
    ASSERT(newDef != (CellDef *) NULL, "dbUndoOpenCell");
    dbUndoLastCell = newDef;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbUndoCloseCell --
 *
 * If any undo events have been played for dbUndoLastCell,
 * recompute its bounding box and record the area of it to be
 * redisplayed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the bounding box on dbUndoLastCell and propagates this
 *	information to all uses of dbUndoLastCell.  Also, marks any area
 *	changed in dbUndoLastCell as needing redisplay.
 *
 *	Resets dbUndoDid to FALSE and dbUndoAreaChanged to an empty
 *	rectangle.
 *
 * ----------------------------------------------------------------------------
 */

void
dbUndoCloseCell()
{
    if (dbUndoUndid && dbUndoLastCell != NULL)
    {
	DBReComputeBbox(dbUndoLastCell);
	DBWAreaChanged(dbUndoLastCell, &dbUndoAreaChanged, DBW_ALLWINDOWS,
	    &DBAllButSpaceBits);
	dbUndoAreaChanged.r_xbot = dbUndoAreaChanged.r_xtop = 0;
	dbUndoAreaChanged.r_ybot = dbUndoAreaChanged.r_ytop = 0;
	dbUndoUndid = FALSE;
    }
}
