/* selDisplay.c -
 *
 *	This file provides routines for displaying the current
 *	selection on the screen.
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
static char rcsid[]="$Header: selDisplay.c,v 6.0 90/08/28 18:56:41 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "textio.h"
#include "signals.h"

/* The current selection is displayed by displaying the outline of
 * shapes in one cell as an overlay (using the higlight facilities)
 * on top of another cell.  The variables below are used to remember
 * these two cells.
 */

static CellUse *selDisUse = NULL;	/* Name of cell whose contents are
					 * highlighted.
					 */
static CellDef *selDisRoot = NULL;	/* Name of a root cell in a window,
					 * on top of whose contents selDisUse
					 * is highlighted.  (i.e. determines
					 * windows in which selection is
					 * displayed).  NULL means no current
					 * selection.
					 */

/* The following variable is shared between SelRedisplay and the search
 * functions that it invokes.  It points to the plane indicating which
 * highlight areas must be redrawn.
 */

static Plane *selRedisplayPlane;

/*
 * ----------------------------------------------------------------------------
 *
 * SelRedisplay --
 *
 * 	This procedure is called by the highlight code to redraw
 *	the selection highlights.  The caller must have locked
 *	the window already.  Only the highlight code should invoke
 *	this procedure.  Other clients should always call the highlight
 *	procedures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Highlights are redrawn, if there is a selection to display
 *	and if it overlaps any non-space tiles in plane.
 *
 * ----------------------------------------------------------------------------
 */

Void
SelRedisplay(window, plane)
    Window *window;		/* Window in which to redisplay. */
    Plane *plane;		/* Non-space tiles on this plane indicate
				 * which areas must have their highlights
				 * redrawn.
				 */
{
    int i, labelSize;
    CellDef *displayDef;
    Label *label;
    Rect planeArea, screenArea;
    SearchContext scx;
    DBWclientRec *crec = (DBWclientRec *) window->w_clientData;
    extern int selRedisplayFunc();	/* Forward declaration. */
    extern int selRedisplayCellFunc();	/* Forward declaration. */
    extern int selAlways1();		/* Forward declaration. */

    /* Make sure that we've got something to show in the area
     * being redisplayed.
     */
    
    if (((CellUse *) (window->w_surfaceID))->cu_def != selDisRoot) return;
    displayDef = selDisUse->cu_def;
    if (!DBBoundPlane(plane, &planeArea)) return;
    if (!GEO_OVERLAP(&displayDef->cd_bbox, &planeArea)) return;

    /* Redisplay the information on the paint planes. */

    GrSetStuff(STYLE_DRAWBOX);
    selRedisplayPlane = plane;
    for (i = PL_SELECTBASE; i < DBNumPlanes; i += 1)
    {
	(void) DBSrPaintArea((Tile *) NULL, displayDef->cd_planes[i],
		&planeArea, &DBAllTypeBits, selRedisplayFunc,
		(ClientData) window);
    }

    /* Redisplay all of the labels in the selection. */

    labelSize = crec->dbw_labelSize;
    if (labelSize < GR_TEXT_SMALL) labelSize = GR_TEXT_SMALL;
    for (label = displayDef->cd_labels; label != NULL; label = label->lab_next)
    {
	Rect larger;

	/* See if the label needs to be redisplayed (make sure we do the
	 * search with a non-null area, or it will never return "yes").
	 */

	larger = label->lab_rect;
	if (larger.r_xbot == larger.r_xtop)
	    larger.r_xtop += 1;
	if (larger.r_ybot == larger.r_ytop)
	    larger.r_ytop += 1;
	if (!DBSrPaintArea((Tile *) NULL, plane, &larger, &DBAllButSpaceBits,
		selAlways1, (ClientData) NULL))
	    continue;
	WindSurfaceToScreen(window, &label->lab_rect, &screenArea);
	DBWDrawLabel(label->lab_text, &screenArea, label->lab_pos,
		STYLE_OUTLINEHIGHLIGHTS, labelSize, &crec->dbw_expandAmounts);
	if (SigInterruptPending) break;
    }

    /* Redisplay all of the subcells in the selection.  Change the
     * clipping rectangle to full-screen or else the cell names won't
     * get displayed properly.  The search function will clip itself.
     * This is the same hack that's in the display module.
     */

    GrClipTo(&GrScreenRect);
    scx.scx_use = selDisUse;
    scx.scx_area = planeArea;
    scx.scx_trans = GeoIdentityTransform;
    (void) DBCellSrArea(&scx, selRedisplayCellFunc, (ClientData) window);
}

/* Function used to see if an area in the selection touches an area
 * that's to be redisplayed:  it just returns 1 always.
 */

int
selAlways1()
{
    return 1;
}

/* Redisplay function for selected paint:  draw lines to outline
 * material.  Only draw lines on boundaries between different
 * kinds of material.
 */

int
selRedisplayFunc(tile, window)
    Tile *tile;			/* Tile to be drawn on highlight layer. */
    Window *window;		/* Window in which to redisplay. */
{
    Rect area, edge, screenEdge;
    register Tile *neighbor;

    TiToRect(tile, &area);

    if (!DBSrPaintArea((Tile *) NULL, selRedisplayPlane, &area,
	    &DBAllButSpaceBits, selAlways1, (ClientData) NULL))
	return 0;

    /* Go along the tile's bottom border, searching for tiles
     * of a different type along that border.  If the bottom of
     * the tile is at -infinity, then don't do anything.
     */
    
    if (area.r_ybot > TiPlaneRect.r_ybot)
    {
	edge.r_ybot = edge.r_ytop = area.r_ybot;
	for (neighbor = tile->ti_lb; LEFT(neighbor) < area.r_xtop;
		neighbor = neighbor->ti_tr)
	{
	    if (neighbor->ti_body == tile->ti_body) continue;
	    edge.r_xbot = LEFT(neighbor);
	    edge.r_xtop = RIGHT(neighbor);
	    if (edge.r_xbot < area.r_xbot) edge.r_xbot = area.r_xbot;
	    if (edge.r_xtop > area.r_xtop) edge.r_xtop = area.r_xtop;
	    WindSurfaceToScreen(window, &edge, &screenEdge);
	    GrClipLine(screenEdge.r_xbot, screenEdge.r_ybot,
		    screenEdge.r_xtop, screenEdge.r_ytop);
	}
    }

    /* Now go along the tile's left border, doing the same thing.   Ignore
     * edges that are at infinity.
     */

    if (area.r_xbot > TiPlaneRect.r_xbot)
    {
	edge.r_xbot = edge.r_xtop = area.r_xbot;
	for (neighbor = tile->ti_bl; BOTTOM(neighbor) < area.r_ytop;
		neighbor = neighbor->ti_rt)
	{
	    if (neighbor->ti_body == tile->ti_body) continue;
	    edge.r_ybot = BOTTOM(neighbor);
	    edge.r_ytop = TOP(neighbor);
	    if (edge.r_ybot < area.r_ybot) edge.r_ybot = area.r_ybot;
	    if (edge.r_ytop < area.r_ytop) edge.r_ytop = area.r_ytop;
	    WindSurfaceToScreen(window, &edge, &screenEdge);
	    GrClipLine(screenEdge.r_xbot, screenEdge.r_ybot,
		    screenEdge.r_xtop, screenEdge.r_ytop);
	}
    }

    return 0;			/* To keep the search from aborting. */
}

/* Redisplay function for cells:  do what the normal redisplay code does
 * in DBWdisplay.c, except draw in the highlight color.
 */

int
selRedisplayCellFunc(scx, window)
    SearchContext *scx;		/* Describes cell found. */
    Window *window;		/* Window in which to redisplay. */
{
    Rect tmp, screen;
    Point p;
    char idName[100];

    GeoTransRect(&scx->scx_trans, &scx->scx_use->cu_def->cd_bbox, &tmp);
    if (!DBSrPaintArea((Tile *) NULL, selRedisplayPlane, &tmp,
	    &DBAllButSpaceBits, selAlways1, (ClientData) NULL))
	return 0;
    WindSurfaceToScreen(window, &tmp, &screen);
    GrFastBox(&screen);

    /* Don't futz around with text if the bbox is tiny. */

    GrLabelSize("BBB", GEO_CENTER, GR_TEXT_SMALL, &tmp);
    if (((screen.r_xtop-screen.r_xbot) < tmp.r_xtop)
	|| ((screen.r_ytop-screen.r_ybot) < tmp.r_ytop)) return 0;

    p.p_x = (screen.r_xbot + screen.r_xtop)/2;
    p.p_y = (screen.r_ybot + 2*screen.r_ytop)/3;
    GeoClip(&screen, &window->w_screenArea);
    GrPutText(scx->scx_use->cu_def->cd_name, STYLE_SOLIDHIGHLIGHTS, &p,
	GEO_CENTER, GR_TEXT_LARGE, TRUE, &screen, (Rect *) NULL);
    (void) DBPrintUseId(scx, idName, 100);
    p.p_y = (2*screen.r_ybot + screen.r_ytop)/3;
    GrPutText(idName, STYLE_SOLIDHIGHLIGHTS, &p, GEO_CENTER,
        GR_TEXT_LARGE, TRUE, &screen, (Rect *) NULL);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelSetDisplay --
 *
 * 	This procedure is called to set up displaying of the selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure sets things up so that future calls to the
 *	highlight code will cause information in selectUse to be
 *	outlined on top of windows containing displayRoot.  This
 *	procedure should be called whenever either of the two
 *	cells changes.  Note:  this procedure does NOT actually
 *	redisplay anything.  The highlight procedures should be
 *	invoked to do the redisplay.
 *
 * ----------------------------------------------------------------------------
 */

void
SelSetDisplay(selectUse, displayRoot)
    CellUse *selectUse;		/* Cell whose contents are to be
				 * highlighted.
				 */
    CellDef *displayRoot;	/* Cell definition on top of whose contents
				 * the highlights are to be displayed.  Must
				 * be the root cell of a window.  May be NULL
				 * to turn off seletion displaying.
				 */
{
    static bool firstTime = TRUE;

    if (firstTime)
    {
	DBWHLAddClient(SelRedisplay);
	firstTime = FALSE;
    }
    selDisUse = selectUse;
    selDisRoot = displayRoot;
}
