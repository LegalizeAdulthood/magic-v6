/*
 * selEnum.c --
 *
 * This file contains routines to enumerate various pieces of the
 * selection, e.g. find all subcells that are in the selection and
 * also in the edit cell.  The procedures here are used as basic
 * building blocks for the selection commands like copy or delete.
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
static char rcsid[]="$Header: selEnum.c,v 6.0 90/08/28 18:56:43 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "main.h"
#include "select.h"
#include "selInt.h"
#include "malloc.h"
#include "textio.h"

/* Structure passed from top-level enumeration procedures to lower-level
 * ones:
 */

struct searg
{
    int (*sea_func)();		/* Client function to call. */
    ClientData sea_cdarg;	/* Client data to pass to sea_func. */
    bool sea_editOnly;		/* Only consider stuff that's in edit cell. */
    bool *sea_nonEdit;		/* Word to set if non-edit stuff is found. */
    int sea_plane;		/* Index of plane currently being searched. */
    TileType sea_type;		/* Type of current piece of selected paint. */
    LinkedRect *sea_rectList;	/* List of rectangles found in edit cell. */
    CellUse *sea_use;		/* Use that we're looking for an identical
				 * copy of in the layout.
				 */
    CellUse *sea_foundUse;	/* Use that was found to match sea_use, or else
				 * NULL.  Also used to hold use for foundLabel.
				 */
    Transform sea_foundTrans;	/* Transform from coords of foundUse to root. */
    Label *sea_label;		/* Label that we're trying to match in the
				 * layout.
				 */
    Label *sea_foundLabel;	/* Matching label that was found, or NULL.
				 * If non_NULL, foundUse and foundTrans
				 * describe its containing cell.
				 */
};

/*
 * ----------------------------------------------------------------------------
 *
 * SelEnumPaint --
 *
 * 	Find all selected paint, and call the client's procedure for
 *	all the areas of paint that are found.  Only consider paint
 *	on "layers", and if "editOnly" is TRUE, then only consider
 *	paint that it is the edit cell.  The client procedure must
 *	be of the form
 *
 *	int
 *	func(rect, type, clientData)
 *	    Rect *rect;
 *	    TileType type;
 *	    ClientData clientData;
 *	{
 *	}
 *
 *	The rect and type parameters identify the paint that was found,
 *	in root coordinates, and clientData is just the clientData
 *	argument passed to this	procedure.  Func should normally return
 *	0.  If it returns a non-zero return value, then the search
 *	will be aborted.
 *
 * Results:
 *	Returns 0 if the search finished normally.  Returns 1 if the
 *	search was aborted.
 *
 * Side effects:
 *	If foundNonEdit is non-NULL, its target is set to indicate
 *	whether there was selected paint from outside the edit cell.
 *	Otherwise, the only side effects are those of func.
 *
 * ----------------------------------------------------------------------------
 */

int
SelEnumPaint(layers, editOnly, foundNonEdit, func, clientData)
    TileTypeBitMask *layers;	/* Mask layers to find. */
    bool editOnly;		/* TRUE means only find material that is
				 * both selected and in the edit cell.
				 */
    bool *foundNonEdit;		/* If non-NULL, this word is set to TRUE
				 * if there's selected paint that's not in
				 * the edit cell, FALSE otherwise.
				 */
    int (*func)();		/* Function to call for paint that's found. */
    ClientData clientData;	/* Argument to pass through to func. */
{
    int plane;
    struct searg arg;
    extern int selEnumPFunc1();	/* Forward declaration. */

    arg.sea_func = func;
    arg.sea_cdarg = clientData;
    arg.sea_editOnly = editOnly;
    arg.sea_nonEdit = foundNonEdit;
    if (foundNonEdit != NULL) *foundNonEdit = FALSE;

    /* First, find all the paint in the selection that has the right
     * layers.
     */
    
    for (plane = PL_SELECTBASE; plane < DBNumPlanes; plane++)
    {
	arg.sea_plane = plane;
	if (DBSrPaintArea((Tile *) NULL, SelectDef->cd_planes[plane],
		&TiPlaneRect, layers, selEnumPFunc1,
		(ClientData) &arg) != 0)
	    return 1;
    }
    return 0;
}

/* Search function invoked for each piece of paint on the right layers
 * in the select cell.  Collect all of the sub-areas of this piece that
 * are also in the edit cell, then call the client function for each
 * one of them.  It's important to collect the pieces first, then call
 * the client:  if we call the client while the edit cell seardh is
 * underway, the client might trash the tile plane underneath us.
 */

int
selEnumPFunc1(tile, arg)
    Tile *tile;			/* Tile of matching type. */
    struct searg *arg;		/* Describes the current search. */
{
    Rect rect, editRect, rootRect;
    extern int selEnumPFunc2();	/* Forward declaration. */

    TiToRect(tile, &rect);
    arg->sea_type = TiGetType(tile);

    /* If this tile is a contact's secondary image, ignore it:  the
     * primary image will take care of everything (otherwise when
     * the second image is processed, the contact might not be there
     * anymore).
     */

    if (arg->sea_type >= DBNumUserLayers) return 0;

    /* If the paint doesn't have to be in the edit cell, life's pretty
     * simple:  just call the client and quit.
     */
    
    if (!arg->sea_editOnly)
    {
	if ((*arg->sea_func)(&rect, arg->sea_type, arg->sea_cdarg) != 0)
	    return 1;
	return 0;
    }
    
    /* Find the stuff that's in the edit cell. */

    GeoTransRect(&RootToEditTransform, &rect, &editRect);
    arg->sea_rectList = NULL;
    (void) DBSrPaintArea((Tile *) NULL,
	    EditCellUse->cu_def->cd_planes[arg->sea_plane],
	    &editRect, &DBAllTypeBits, selEnumPFunc2,
	    (ClientData) arg);
    
    /* Call the client for each rectangle found. */

    while (arg->sea_rectList != NULL)
    {
	GeoTransRect(&EditToRootTransform, &arg->sea_rectList->r_r, &rootRect);
	GeoClip(&rootRect, &rect);
	if ((*arg->sea_func)(&rootRect, arg->sea_type, arg->sea_cdarg) != 0)
	    return 1;
	freeMagic((char *) arg->sea_rectList);
	arg->sea_rectList = arg->sea_rectList->r_next;
    }
    return 0;
}

/* Second-level paint search function:  save around (in edit coords)
 * each tile that has the same type as requested in arg.  Record if
 * any wrong-type tiles are found.
 */

int
selEnumPFunc2(tile, arg)
    Tile *tile;			/* Tile found in the edit cell. */
    struct searg *arg;		/* Describes our search. */
{
    LinkedRect *lr;

    if (arg->sea_type != TiGetType(tile))
    {
	if (arg->sea_nonEdit != NULL) *(arg->sea_nonEdit) = TRUE;
	return 0;
    }

    lr = (LinkedRect *) mallocMagic(sizeof(LinkedRect));
    TiToRect(tile, &lr->r_r);
    lr->r_next = arg->sea_rectList;
    arg->sea_rectList = lr;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelEnumCells --
 *
 * 	Call a client-supplied procedure for each selected subcell.
 *	If "editOnly" is TRUE, then only consider selected subcells
 *	that are children of the edit cell.  The client procedure
 *	must be of the form
 *
 *	int
 *	func(selUse, realUse, transform, clientData)
 *	    CellUse *selUse;
 *	    CellUse *realUse;
 *	    Transform *transform;
 *	    ClientData clientData;
 *	{
 *	}
 *
 *	SelUse is a pointer to a cellUse that's in the selection cell.
 *	RealUse is a pointer to the corresponding cell that's part of
 *	the layout.  Transform is a transform from the coordinates of
 *	RealUse to root coordinates.  If the cell is an array, only one
 *	call is made for the entire array, and transform is the transform
 *	for the root element of the array (array[xlo, ylo]).  Func should
 *	normally return 0.  If it returns a non-zero return value, then
 *	the search will be aborted.
 *
 * Results:
 *	Returns 0 if the search finished normally.  Returns 1 if the
 *	search was aborted.
 *
 * Side effects:
 *	If foundNonEdit is non-NULL, its target is set to indicate
 *	whether there were selected cells that weren't children of
 *	the edit cell. 	Otherwise, the only side effects are those
 *	of func.
 *
 * ----------------------------------------------------------------------------
 */

int
SelEnumCells(editOnly, foundNonEdit, scx, func, clientData)
    bool editOnly;		/* TRUE means only find material that is
				 * both selected and in the edit cell.
				 */
    bool *foundNonEdit;		/* If non-NULL, this word is set to TRUE
				 * if there are one or more selected cells
				 * that aren't children of the edit cell,
				 * FALSE otherwise.
				 */
    SearchContext *scx;		/* Most clients will provide a NULL value
				 * here, in which case all the subcells in
				 * the selection are enumerated.  If this
				 * is non-NULL, it describes a different
				 * area in which to enumerate subcells.  This
				 * feature is intended primarily for internal
				 * use within this module.
				 */
    int (*func)();		/* Function to call for subcells found. */
    ClientData clientData;	/* Argument to pass through to func. */
{
    struct searg arg;
    SearchContext scx2;
    extern int selEnumCFunc1();	/* Forward reference. */

    arg.sea_func = func;
    arg.sea_cdarg = clientData;
    arg.sea_editOnly = editOnly;
    arg.sea_nonEdit = foundNonEdit;
    if (foundNonEdit != NULL) *foundNonEdit = FALSE;

    /* Find all the subcells that are in the selection. */

    if (scx != NULL)
	scx2 = *scx;
    else
    {
	scx2.scx_use = SelectUse;
	scx2.scx_area = TiPlaneRect;
	scx2.scx_trans = GeoIdentityTransform;
    }
    if (DBCellSrArea(&scx2, selEnumCFunc1, (ClientData) &arg) != 0)
	return 1;
    return 0;
}

/* The first-level search function:  called for each subcell in the
 * selection.
 */

int
selEnumCFunc1(scx, arg)
    SearchContext *scx;		/* Describes cell that was found. */
    struct searg *arg;		/* Describes our search. */
{
    SearchContext scx2;
    extern int selEnumCFunc2();	/* Forward reference. */
    CellUse dummy;

    /* If this cell is the top-level one in its window, we have to
     * handle it specially:  just look for any use that's a top-level
     * use, then call the client for it.
     */
    
    if (scx->scx_use->cu_def == SelectRootDef)
    {
	CellUse *parent;

	/* A root use can't ever be a child of the edit cell. */

	if (arg->sea_editOnly)
	{
	    if (arg->sea_nonEdit != NULL) *(arg->sea_nonEdit) = TRUE;
	    return 2;
	}
	
	/* Find a top-level use (one with no parent). */

	for (parent = SelectRootDef->cd_parents;
	     parent != NULL;
	     parent = parent->cu_nextuse)
	{
	    if (parent->cu_parent == NULL) break;
	}

	if (parent == NULL)
	{
	    TxError("Internal error:  couldn't find selected root cell %s.\n",
		SelectRootDef->cd_name);
	    return 2;
	}

	/* Call the client. */

	if ((*arg->sea_func)(scx->scx_use, parent, &GeoIdentityTransform,
		arg->sea_cdarg) != 0)
	    return 1;
	return 2;
    }

    /* This isn't a top-level cell.  Find the instance corresponding
     * to this one in the layout.  Only search a 1-unit square at the
     * cell's lower-left corner in order to cut down the work that
     * has to be done.  Unfortunately
     * we can't use DBTreeSrCells for this, because we don't want to
     * look at expanded/unexpanded information.
     */
    
    scx2.scx_use = &dummy;
    dummy.cu_def = SelectRootDef;
    GeoTransRect(&scx->scx_use->cu_transform, &scx->scx_use->cu_def->cd_bbox,
	    &scx2.scx_area);
    scx2.scx_area.r_xtop = scx2.scx_area.r_xbot + 1;
    scx2.scx_area.r_ytop = scx2.scx_area.r_ybot + 1;
    scx2.scx_trans = GeoIdentityTransform;
    arg->sea_use = scx->scx_use;
    arg->sea_foundUse = NULL;
    (void) DBCellSrArea(&scx2, selEnumCFunc2, (ClientData) arg);
    if (arg->sea_foundUse == NULL)
    {
	TxError("Internal error:  couldn't find selected cell %s.\n",
	    arg->sea_use->cu_id);
	return 2;
    }

    /* See whether the cell is a child of the edit cell and
     * call the client's procedure if everything's OK.  We do the
     * call here rather than in selEnumCFunc2 because the client
     * could modify the edit cell in a way that would cause the
     * search in progress to core dump.  By the time we get back
     * here, the search is complete so there's no danger.
     */

    if ((arg->sea_editOnly)
	    && (arg->sea_foundUse->cu_parent != EditCellUse->cu_def))
    {
	if (arg->sea_nonEdit != NULL) *(arg->sea_nonEdit) = TRUE;
	return 2;
    }
    if ((*arg->sea_func)(scx->scx_use, arg->sea_foundUse,
	    &arg->sea_foundTrans, arg->sea_cdarg) != 0)
	return 1;
    return 2;
}

/* Second-level cell search function:  called for each cell in the
 * tree of SelectRootDef that touches the lower-left corner of
 * the subcell in the selection that we're trying to match.  If
 * this use is for the same subcell, and has the same transformation
 * and array structure, then remember the cell use for the caller
 * and abort the search.
 */

int
selEnumCFunc2(scx, arg)
    SearchContext *scx;		/* Describes child of edit cell. */
    struct searg *arg;		/* Describes what we're looking for. */
{
    CellUse *use, *selUse;

    use = scx->scx_use;
    selUse = arg->sea_use;
    if (use->cu_def != selUse->cu_def) goto checkChildren;
    if ((scx->scx_trans.t_a != selUse->cu_transform.t_a)
	    || (scx->scx_trans.t_b != selUse->cu_transform.t_b)
	    || (scx->scx_trans.t_c != selUse->cu_transform.t_c)
	    || (scx->scx_trans.t_d != selUse->cu_transform.t_d)
	    || (scx->scx_trans.t_e != selUse->cu_transform.t_e)
	    || (scx->scx_trans.t_f != selUse->cu_transform.t_f))
	goto checkChildren;
    if ((use->cu_array.ar_xlo != selUse->cu_array.ar_xlo)
	    || (use->cu_array.ar_ylo != selUse->cu_array.ar_ylo)
	    || (use->cu_array.ar_xhi != selUse->cu_array.ar_xhi)
	    || (use->cu_array.ar_yhi != selUse->cu_array.ar_yhi)
	    || (use->cu_array.ar_xsep != selUse->cu_array.ar_xsep)
	    || (use->cu_array.ar_ysep != selUse->cu_array.ar_ysep))
	goto checkChildren;
    
    arg->sea_foundUse = use;
    arg->sea_foundTrans = scx->scx_trans;
    return 1;

    /* This cell didn't match... see if any of its children do. */

    checkChildren:
    if (DBCellSrArea(scx, selEnumCFunc2, (ClientData) arg) != 0)
	return 1;
    else return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelEnumLabels --
 *
 * 	Find all selected labels, and call the client's procedure for
 *	each label found.  Only consider labels attached to "layers",
 *	and if "editOnly" is TRUE, then only consider labels that
 *	are in the edit cell.  The client procedure must be of the
 *	form
 *
 *	int
 *	func(label, cellDef, transform, clientData)
 *	    Label *label;
 *	    CellDef *cellDef;
 *	    Transform *transform;
 *	    ClientData clientData;
 *	{
 *	}
 *
 *	Label is a pointer to a selected label.  It refers to the label
 *	in cellDef, and transform gives the transform from that
 *	cell's coordinates to root coordinates.  ClientData is just
 *	the clientData argument passed to this procedure.  Func
 *	should normally return 0.  If it returns a non-zero return
 *	value, then the search will be aborted.
 *
 * Results:
 *	Returns 0 if the search finished normally.  Returns 1 if the
 *	search was aborted.
 *
 * Side effects:
 *	If foundNonEdit is non-NULL, its target is set to indicate
 *	whether there was at least one selected label that was not
 *	in the edit cell.  Otherwise, the only side effects are
 *	those of func.
 *
 * ----------------------------------------------------------------------------
 */

int
SelEnumLabels(layers, editOnly, foundNonEdit, func, clientData)
    TileTypeBitMask *layers;	/* Find labels on these layers. */
    bool editOnly;		/* TRUE means only find labels that are
				 * both selected and in the edit cell.
				 */
    bool *foundNonEdit;		/* If non-NULL, this word is set to TRUE
				 * if there are selected labels that aren't
				 * in the edit cell, FALSE otherwise.
				 */
    int (*func)();		/* Function to call for each label found. */
    ClientData clientData;	/* Argument to pass through to func. */
{
    register Label *selLabel;
    CellUse dummy;
    SearchContext scx;
    struct searg arg;
    extern int selEnumLFunc();	/* Forward reference. */

    if (foundNonEdit != NULL) *foundNonEdit = FALSE;

    /* First of all, search through all of the selected labels. */

    for (selLabel = SelectDef->cd_labels; selLabel != NULL;
	    selLabel = selLabel->lab_next)
    {
	if (!TTMaskHasType(layers, selLabel->lab_type)) continue;

	/* Find the label corresponding to this one in the design. */
	
	scx.scx_use = &dummy;
	dummy.cu_def = SelectRootDef;
	GEO_EXPAND(&selLabel->lab_rect, 1, &scx.scx_area);
	scx.scx_trans = GeoIdentityTransform;
	arg.sea_label = selLabel;
	arg.sea_foundLabel = NULL;
	(void) DBTreeSrLabels(&scx, &DBAllTypeBits, 0, (TerminalPath *) NULL,
	    selEnumLFunc, (ClientData) &arg);
	if (arg.sea_foundLabel == NULL)
	{
	    TxError("Internal error:  couldn't find selected label %s.\n",
		selLabel->lab_text);
	    continue;
	}

	/* If only edit-cell labels are wanted, check this label's
	 * parentage.
	 */
	
	if (editOnly && (arg.sea_foundUse->cu_def != EditCellUse->cu_def))
	{
	    if (foundNonEdit != NULL) *foundNonEdit = TRUE;
	    continue;
	}

	if ((*func)(arg.sea_foundLabel, arg.sea_foundUse->cu_def,
	    &arg.sea_foundTrans, clientData) != 0) return 1;
    }

    return 0;
}

/* Search function for label enumeration:  make sure that this label
 * matches the one we're looking for.  If it does, then record
 * information about it and return right away.
 */

	/* ARGSUSED */
int
selEnumLFunc(scx, label, tpath, arg)
    SearchContext *scx;		/* Describes current cell for search. */
    Label *label;		/* Describes label that is in right area
				 * and has right type.
				 */
    TerminalPath *tpath;	/* Ignored. */
    struct searg *arg;		/* Indicates what we're looking for. */
{
    Rect *want, got;

    GeoTransRect(&scx->scx_trans, &label->lab_rect, &got);
    want = &arg->sea_label->lab_rect;
    if (want->r_xbot != got.r_xbot) return 0;
    if (want->r_ybot != got.r_ybot) return 0;
    if (want->r_xtop != got.r_xtop) return 0;
    if (want->r_ytop != got.r_ytop) return 0;
    if (arg->sea_label->lab_pos
	!= GeoTransPos(&scx->scx_trans, label->lab_pos)) return 0;
    if (strcmp(label->lab_text, arg->sea_label->lab_text) != 0) return 0;

    arg->sea_foundLabel = label;
    arg->sea_foundUse = scx->scx_use;
    arg->sea_foundTrans = scx->scx_trans;
    return 1;
}
