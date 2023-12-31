/*
 * ExtYank.c --
 *
 * Circuit extraction.
 * Hierarchical yanking of paint and labels.
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
static char rcsid[] = "$Header: ExtYank.c,v 6.0 90/08/28 18:15:38 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <math.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "textio.h"
#include "debug.h"
#include "styles.h"
#include "extract.h"
#include "extractInt.h"

Rect extSubcellArea;		/* Area of currently processed subcell, clipped
				 * to area of interaction, in parent coords.
				 */

/* Forward declarations of filter functions */
int extHierYankFunc();
int extHierLabelFunc();

/*
 * ----------------------------------------------------------------------------
 *
 * extHierCopyLabels --
 *
 * Copy the label list from sourceDef to targetDef, prepending
 * it to any labels already in targetDef.  Does not change
 * sourceDef's label list.
 *
 * Labels are copied in order, so the first label on sourceDef's
 * list becomes the first label on targetDef's list.  THIS IS
 * CRITICAL TO INSURE THAT HIERARCHICAL ADJUSTMENTS CAN BE MADE
 * PROPERLY; SEE extSubtreeAdjustInit() FOR AN EXPLANATION.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

extHierCopyLabels(sourceDef, targetDef)
    CellDef *sourceDef, *targetDef;
{
    register Label *lab, *newlab, *firstLab, *lastLab;
    unsigned n;

    firstLab = lastLab = (Label *) NULL;
    for (lab = sourceDef->cd_labels; lab; lab = lab->lab_next)
    {
	n = sizeof (Label) + strlen(lab->lab_text) - sizeof lab->lab_text + 1;
	MALLOC(Label *, newlab, n);
	bcopy((char *) lab, (char *) newlab, (int) n);

	if (lastLab == NULL) lastLab = firstLab = newlab;
	else lastLab->lab_next = newlab, lastLab = newlab;
    }

    if (lastLab)
    {
	lastLab->lab_next = targetDef->cd_labels;
	targetDef->cd_labels = firstLab;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierFreeLabels --
 *
 * Free all the labels from 'def'.
 * Leaves the label lis of 'def' pointing to NULL.
 *
 * This procedure exists mainly so we can trace the freeing
 * of labels for debugging.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

extHierFreeLabels(def)
    CellDef *def;
{
    register Label *lab;

    for (lab = def->cd_labels; lab; lab = lab->lab_next)
	FREE((char *) lab);
    def->cd_labels = (Label *) NULL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierYankFunc --
 *
 * Filter function normally called by DBArraySr to yank hierarchically the
 * paint and labels from 'use'.  Called for each array element.  Also called
 * during array extraction.
 *
 * Expects hy->hy_area to be the area, in use->cu_parent coordinates,
 * to be yanked.  What we yank will be transformed to parent coordinates
 * and placed in the cell hy->hy_target.  If hy->hy_prefix is TRUE, we
 * will prepend all the labels we yank with the use id of this array
 * element; otherwise, the labels have no prefix.
 *
 * WARNING:
 *	Only node-name labels are yanked; attributes are not.
 *	The hierarchical extraction code depends on this fact.
 *
 * Results:
 *	Returns 0 to cause DBArraySr to keep going.
 *
 * Side effects:
 *	Adds paint and new labels to 'hy->hy_target'.
 *
 * ----------------------------------------------------------------------------
 */

extHierYankFunc(use, trans, x, y, hy)
    CellUse *use;	/* Use that is the root of the subtree being yanked */
    Transform *trans;	/* Transform from coordinates of use->cu_def to those
			 * in use->cu_parent, for the array element (x, y).
			 */
    int x, y;		/* Indices of this array element */
    HierYank *hy;	/* See comments in procedure header */
{
    char labelBuf[4096];
    TerminalPath tpath;
    SearchContext scx;
    Transform tinv;

    /*
     * Want scx.scx_area to be the area in coordinates of use->cu_def
     * but hy->hy_area is in coordinates of use->cu_parent.
     */
    GEOINVERTTRANS(trans, &tinv);
    GEOTRANSRECT(&tinv, hy->hy_area, &scx.scx_area);
    GEOCLIP(&scx.scx_area, &use->cu_def->cd_bbox);

    scx.scx_use = use;
    scx.scx_trans = *trans;
    scx.scx_x = x;
    scx.scx_y = y;

    /* Yank the paint */
    DBCellCopyAllPaint(&scx, &DBAllButSpaceBits, 0, hy->hy_target);

    /* Yank the labels */
    tpath.tp_next = tpath.tp_first = labelBuf;
    tpath.tp_last = &labelBuf[sizeof labelBuf - 2];
    if (hy->hy_prefix)
    {
	tpath.tp_next = DBPrintUseId(&scx, labelBuf, sizeof labelBuf - 3);
	*tpath.tp_next++ = '/';
    }
    *tpath.tp_next = '\0';
    (void) DBTreeSrLabels(&scx, &DBAllButSpaceBits, 0, &tpath, extHierLabelFunc,
		(ClientData) hy->hy_target->cu_def);

    return (0);
}

extHierLabelFunc(scx, label, tpath, targetDef)
    SearchContext *scx;
    Label *label;
    TerminalPath *tpath;
    CellDef *targetDef;
{
    register char *srcp, *dstp;
    register Label *newlab;
    int len;

    /* Reject if the label falls over space */
    if (label->lab_type == TT_SPACE)
	return (0);

    /* Reject if not a node label */
    if (!extLabType(label->lab_text, LABTYPE_NAME))
	return (0);

    /* Determine size of new label with hierarchical name */
    for (srcp = label->lab_text; *srcp++; )
	/* Nothing */;
    len = srcp - label->lab_text;
    for (srcp = tpath->tp_first; *srcp++; )
	/* Nothing */;
    len += srcp - tpath->tp_first + 1;

    /* Allocate new label at correct location */
    MALLOC(Label *, newlab, (unsigned) (sizeof (Label) + len - 4));
    GeoTransRect(&scx->scx_trans, &label->lab_rect, &newlab->lab_rect);
    newlab->lab_pos = GeoTransPos(&scx->scx_trans, label->lab_pos);
    newlab->lab_type = label->lab_type;
    dstp = newlab->lab_text;
    for (srcp = tpath->tp_first; *dstp++ = *srcp++; )
	/* Nothing */;
    for (--dstp, srcp = label->lab_text; *dstp++ = *srcp++; )
	/* Nothing */;

    newlab->lab_next = targetDef->cd_labels;
    targetDef->cd_labels = newlab;

    return (0);
}
