/*
 * DBWdisplay.c --
 *
 * This file contains code for redisplaying layout on the display.
 * It saves up information about what is to be redisplayed, then
 * does all of the redisplay at a convenient time.
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
static char rcsid[] = "$Header: DBWdisplay.c,v 6.0 90/08/28 18:11:19 mayo Exp $";
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
#include "utils.h"
#include "dbwtech.h"
#include "styles.h"
#include "main.h"
#include "tech.h"
#include "signals.h"
#include "router.h"

/* The following variable is exported to the rest of the world.
 * It is read from the "styletype" line of the technology file,
 * and defines the class of display styles file expected for
 * this technology.  It defaults as below;  almost all technologies
 * can use the default.
 */

char *DBWStyleType = "std";

/* a round-up sort of integer division */
#define	ceilDiv(n,d)	( ((n) < 0)  ? -( ((-(n))+(d)-1) / d)  \
	: ((n)+(d)-1) / d )

static bool debugit = FALSE;

/* The following statics are just for convenience in talking between
 * the top-level redisplay routine and the lower-level redisplay
 * routines invoked during the recursive search.
 */

static Window *dbwWindow;	/* Current window being redisplayed. */
static bool disWasPale;		/* TRUE if last rect was displayed pale */
static Rect rootClip;		/* Clip area for root area that is being
				 * redisplayed.
				 */
static Rect windClip;		/* Clip area for entire window. */
static int disStyle;		/* Display style. */
static Rect dbwWatchArea;	/* Area used to clip tiles being "watched" so
				 * that infinities don't cause arithmetic
				 * overflow.  In coords of cell being watched.
				 */
static Transform dbwWatchTrans;	/* Transform to root coords for watch tiles. */
static int dbwWatchDemo;	/* TRUE means use "demo" style for watch
				 * tile display.
				 */
static int  dbwSeeTypes;	/* TRUE means use tile type instead of 
				   pointer value for watch display.
				*/
static Rect dbwMinBBox;		/* If bounding boxes aren't at least this
				 * large, don't bother displaying name and
				 * id.  The ur point of the box gives the
				 * minimum allowed dimensions.
				 */
static int dbwLabelSize;	/* Size to use for label drawing. */
static Rect *dbwExpandAmounts;	/* Box to accumulate total sizes of labels.
				 * Always refers to crec->dbw_expandAmounts
				 * in window being redrawn.
				 */

/* The stuff below is shared between the top-level and action
 * routines for redisplay.  It is used to identify the edit cell
 * so it can be displayed differently.
 */

static CellDef *editDef;
static Transform editTrans;	/* Contains transform from edit cell to
				 * screen coordinates.  If edit cell isn't
				 * in this window, editDef is NULL.
				 */
static bool dbwAllSame;		/* Means don't display the edit cell
				 * differently after all.
				 */

static bool dbwIsLocked;	/* Is window already locked */
static Window *dbwLockW;	/* Window to lock */
static bool dbwNeedStyle;	/* Do we need to set the display style? */

/* Search functions, all of which are used before definition: */

extern int dbwTileFunc(), dbwWatchFunc(), dbwLabelFunc();
extern int dbwPaintFunc(), dbwBBoxFunc();
extern int dbwWindowFunc(), dbwChangedFunc();


/*
 * ----------------------------------------------------------------------------
 * DBWredisplay --
 *
 * 	This procedure does the dirty of redisplaying information
 *	on the graphics screen.  When it is done the given area of the
 *	window will be correctly displayed, including the grid, any
 *	watched tile planes, and tools.
 *
 *	This procedure locks windows as it goes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is drawn on the display in the given area.
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
Void
DBWredisplay(w, rootArea, clipArea)
    Window *w;			/* Window some of whose contents are to be
				 * redisplayed.
				 */
    Rect *rootArea;		/* The area that must be redisplayed, in
				 * root cell coordinates.
				 */
    Rect *clipArea;		/* The screen area that we should clip to 
				 */
{
    int i;
    TileTypeBitMask *mask;
    SearchContext scontext;
    Rect largerArea, labelArea;
    int bitMask, lambdasPerPixel, pixelsPerLambda;
    DBWclientRec *crec;
    CellDef *cellDef;
    TileTypeBitMask layers;

    /*
    TxPrintf("Root area (%d, %d) (%d, %d) redisplay.\n", 
	    rootArea->r_xbot, rootArea->r_ybot,
	    rootArea->r_xtop, rootArea->r_ytop);
    */

    GrLock(w, TRUE);

    /* Round up the redisplay area by 1 pixel on all sides.  This
     * is needed because TiSrArea won't return tiles that touch
     * the area without overlapping it.  Without the round-up, there
     * will be occasional (in fact, frequent), one-pixel wide slivers.
     */
    
    largerArea = *rootArea;
    largerArea.r_xbot -= 1;
    largerArea.r_ybot -= 1;
    largerArea.r_xtop += 1;
    largerArea.r_ytop += 1;

    crec = ((DBWclientRec *) w->w_clientData);
    cellDef = ((CellUse *)w->w_surfaceID)->cu_def;

    if ((crec->dbw_origin.p_x != w->w_origin.p_x)
	|| (crec->dbw_origin.p_y != w->w_origin.p_y)
	|| (crec->dbw_scale != w->w_scale)
	|| (crec->dbw_surfaceArea.r_xbot != w->w_surfaceArea.r_xbot)
	|| (crec->dbw_surfaceArea.r_ybot != w->w_surfaceArea.r_ybot)
	|| (crec->dbw_surfaceArea.r_xtop != w->w_surfaceArea.r_xtop)
	|| (crec->dbw_surfaceArea.r_ytop != w->w_surfaceArea.r_ytop))
    {
	/* The window has changed size or position or scale,
	 * so update measurements on label size in this window.
	 * Also, pick a size for labels based on the scale in
	 * half the window:  the idea is to make the labels about
	 * half the height of typical wires.  
	 */
	int halfWireWidth;
	Rect text;

	halfWireWidth = MAX(RtrMetalWidth, RtrPolyWidth);
	halfWireWidth = (halfWireWidth * w->w_scale)>>13;
	for (i = GR_TEXT_XLARGE; i >= GR_TEXT_SMALL; i -= 1)
	{
	    GrLabelSize("B", GEO_CENTER, i, &text);
	    if (halfWireWidth > (text.r_ytop - text.r_ybot)) break;
	}
	if (i < GR_TEXT_SMALL) i = GR_TEXT_SMALL;
	if ((3*halfWireWidth) <= (text.r_ytop - text.r_ybot))
	    crec->dbw_labelSize = -1;
	else crec->dbw_labelSize = i;

	crec->dbw_expandAmounts.r_xbot = 0;
	crec->dbw_expandAmounts.r_ybot = 0;
	crec->dbw_expandAmounts.r_xtop = 0;
	crec->dbw_expandAmounts.r_ytop = 0;

	crec->dbw_origin = w->w_origin;
	crec->dbw_scale = w->w_scale;
	crec->dbw_surfaceArea = w->w_surfaceArea;
    }

    /* Labels can stick out a long way from the point they are
     * anchored to.  In the code that draws labels we record the
     * size (in pixels) of the largest label in the window.  Here
     * we use that information to increase the size of the search
     * area for labels enough to be sure we'll see any label that
     * falls partially or completely in the area we're redisplaying.
     * Note:  we have to also include the size of the label cross
     * in the area to be redisplayed.
     */

    pixelsPerLambda = w->w_scale/4096;
    lambdasPerPixel = (4096 / w->w_scale) + 1; 
    labelArea = largerArea;
    (void) GeoInclude(&GrCrossRect, &crec->dbw_expandAmounts);
    if (pixelsPerLambda > 0)
    {
	labelArea.r_xtop -= 
		ceilDiv(crec->dbw_expandAmounts.r_xbot, pixelsPerLambda);
	labelArea.r_ytop -= 
		ceilDiv(crec->dbw_expandAmounts.r_ybot, pixelsPerLambda);
	labelArea.r_xbot -= 
		ceilDiv(crec->dbw_expandAmounts.r_xtop, pixelsPerLambda);
	labelArea.r_ybot -= 
		ceilDiv(crec->dbw_expandAmounts.r_ytop, pixelsPerLambda);
    }
    else
    {
	labelArea.r_xtop -= crec->dbw_expandAmounts.r_xbot * lambdasPerPixel;
	labelArea.r_ytop -= crec->dbw_expandAmounts.r_ybot * lambdasPerPixel;
	labelArea.r_xbot -= crec->dbw_expandAmounts.r_xtop * lambdasPerPixel;
	labelArea.r_ybot -= crec->dbw_expandAmounts.r_ytop * lambdasPerPixel;
    }

    /**
    TxPrintf("Need to expand area by (%d %d) (%d %d) pixels.\n",
	crec->dbw_expandAmounts.r_xbot,
	crec->dbw_expandAmounts.r_ybot,
	crec->dbw_expandAmounts.r_xtop,
	crec->dbw_expandAmounts.r_ytop);
    **/

    /* Set the clipping rectangle to only contain the area being displayed. */

    dbwWindow = w;
    windClip = w->w_screenArea;
    WindSurfaceToScreen(w, rootArea, &rootClip);
    GrClipTo(&rootClip);

    scontext.scx_area = largerArea;
    scontext.scx_use = ((CellUse *)w->w_surfaceID);
    scontext.scx_trans = GeoIdentityTransform;
    bitMask = crec->dbw_bitmask;

    /* See if this window contains the edit cell.  If so, remember
     * information about the edit cell so we can identify it during
     * the action routines.
     */
    
    if (cellDef == EditRootDef)
    {
        editDef = EditCellUse->cu_def;
	editTrans = EditToRootTransform;
    }
    else editDef = NULL;
    if (crec->dbw_flags & DBW_ALLSAME) dbwAllSame = TRUE;
    else dbwAllSame = FALSE;

    /* Empty the area of all previous info. */

    GrClipBox(&rootClip, STYLE_ERASEALL);

    /* Go through all of the tile display styles.  For each
     * style, if there are tiles that include that style, then
     * find and display all the tiles.
     *
     * We use the fast version of GrClipBox in the filter
     * function, GrFastBox.  To use it requires that we set
     * the style before the call, which we do inside of
     * dbwPaintFunc() on the first box displayed for each
     * style.
     *
     * Unlock the window before the following loop to avoid
     * holding the lock too long.
     */
    GrUnlock(w);
    dbwLockW = w;
    dbwIsLocked = FALSE;
    for (i=0;  i<MAXTILESTYLES;  i++)
    {
	mask = DBWStyleToTypes(i);
	TTMaskAndMask3(&layers, mask, &crec->dbw_visibleLayers);
	if (!TTMaskIsZero(&layers))
	{
	    /*
	     * To avoid holding a lock for too long, we don't set it
	     * until we actually see something to display, and we
	     * clear it immediately after we've displayed a style
	     * that had something in it.
	     */
	    disStyle = i;
	    disWasPale = FALSE;		/* Using normal solid style */
	    dbwNeedStyle = TRUE;	/* Haven't set style yet */
	    (void) DBTreeSrTiles(&scontext, &layers, bitMask,
		    dbwPaintFunc, (ClientData) NULL);
	    if (dbwIsLocked)
	    {
		GrUnlock(w);
		dbwIsLocked = FALSE;
	    }
	}
    }

    GrLock(w, TRUE);
    GrClipTo(&rootClip);

    /* Now, find any labels that are visible and display them.
     * Label text is only displayed for certain ranges of magnification:
     * If a window is too zoomed-out, then text redisplay takes too
     * long and just clutters up the screen anyway.  For displaying
     * labels, we set the clip area to the whole screen (otherwise
     * new labels will be clipped to just the paint area).  This only
     * works because (a) graphics clips down to the window's actual
     * displayed area anyway, and (b) labels are opaque layers on top
     * of everything else (otherwise there might be color mixing
     * problems).
     */

    if (crec->dbw_flags & DBW_SEELABELS)
    {
	scontext.scx_area = labelArea;

	/* make sure that we are searching an area, not just a point */
	scontext.scx_area.r_xtop = MAX(scontext.scx_area.r_xtop, 
		scontext.scx_area.r_xbot + 1);
	scontext.scx_area.r_ytop = MAX(scontext.scx_area.r_ytop, 
		scontext.scx_area.r_ybot + 1);
	dbwLabelSize = crec->dbw_labelSize;
	dbwExpandAmounts = &crec->dbw_expandAmounts;
	GrClipTo(&GrScreenRect);
 
        /* Set style information beforehand */
        GrSetStuff(STYLE_LABEL);
	(void) DBTreeSrLabels(&scontext, &DBAllTypeBits, bitMask,
		(TerminalPath *) NULL, dbwLabelFunc, 
		(ClientData) NULL);
	GrClipTo(&rootClip);
    }
    
    /* Next, display the bounding boxes that are visible.  Before doing
     * this, calculate the area occupied by the text "BBB".  A cell won't
     * get its id or name displayed unless its bbox is at least this
     * large.
     */

    GrLabelSize("BBB", GEO_CENTER, GR_TEXT_SMALL, &dbwMinBBox);
    dbwMinBBox.r_xtop -= dbwMinBBox.r_xbot;
    dbwMinBBox.r_ytop -= dbwMinBBox.r_ybot;

    /* Redisplay cell names as if the whole window were visible.
     * This must be done since we slide the names around to fit
     * into the clip area.  Set the style beforehand for speed.
     */

    GrClipTo(&GrScreenRect);	/* Will be cut down to window size by
				 * the graphics module.
				 */
    scontext.scx_area = largerArea;
    GrSetStuff(STYLE_BBOX);
    (void) DBTreeSrCells(&scontext, bitMask, dbwBBoxFunc, (ClientData)NULL);
    GrClipTo(&rootClip);


    /* Now redisplay the grid.  This code is a bit tricky because
     * it makes sure that the origin ends up on a grid line.
     */

    if (crec->dbw_flags & DBW_GRID)
    {
	int width, height;
	Rect gridRect;

	/* Compute a grid template rectangle, in screen coordinates, that
	 * is near the lower-left corner of the window.
	 */

	gridRect.r_ll = w->w_origin;
	width = crec->dbw_gridRect.r_xtop - crec->dbw_gridRect.r_xbot;
	height = crec->dbw_gridRect.r_ytop - crec->dbw_gridRect.r_ybot;
	gridRect.r_xbot -= w->w_scale *
	    ((w->w_surfaceArea.r_xbot - crec->dbw_gridRect.r_xbot) % width);
	gridRect.r_ybot -= w->w_scale *
	    ((w->w_surfaceArea.r_ybot - crec->dbw_gridRect.r_ybot) % height);
	gridRect.r_xtop = gridRect.r_xbot + w->w_scale*width;
	gridRect.r_ytop = gridRect.r_ybot + w->w_scale*height;
	GrClipBox(&gridRect, STYLE_GRID);
    
	/* Redisplay a little square around the origin for the edit cell
	 * (if the edit cell is in this window).  Make the origin 4 pixels
	 * across, but don't display it unless this is less than two lambda
	 * units.  That way, we always know how much to redisplay (in lambda
	 * units), when the edit cell changes.
	 */
	
	if (editDef != NULL)
	{
	    Rect r, r2;
	    r.r_xbot = r.r_ybot = -1;
	    r.r_xtop = r.r_ytop = 1;
	    WindSurfaceToScreen(w, &r, &r2);
	    if ((r2.r_xtop - r2.r_xbot) >= 4)
	    {
		GeoTransRect(&EditToRootTransform, &GeoNullRect, &r2);
		WindSurfaceToScreen(w, &r2, &r);
		r.r_xbot -= 2;
		r.r_xtop += 2;
		r.r_ybot -= 2;
		r.r_ytop += 2;
		GrClipBox(&r, STYLE_ORIGIN);
	    }
	}
    }

    /* If there is a tile plane being "watched", redisplay
     * its structure.
     */
    
    if (crec->dbw_watchPlane >= 0)
    {
	Transform toCell;

	GeoInvertTrans(&crec->dbw_watchTrans, &toCell);
	GeoTransRect(&toCell, &w->w_surfaceArea, &dbwWatchArea);
	dbwWatchArea.r_xbot -= lambdasPerPixel;
	dbwWatchArea.r_xtop += lambdasPerPixel;
	dbwWatchArea.r_ybot -= lambdasPerPixel;
	dbwWatchArea.r_ytop += lambdasPerPixel;
	dbwWatchTrans = crec->dbw_watchTrans;
        dbwWatchDemo = ((crec->dbw_flags & DBW_WATCHDEMO) != 0);
	dbwSeeTypes = ((crec->dbw_flags & DBW_SEETYPES) != 0);
	(void) TiSrArea((Tile *) NULL,
	    crec->dbw_watchDef->cd_planes[crec->dbw_watchPlane],
	    &dbwWatchArea, dbwTileFunc, (ClientData) NULL);
    }

    /* Record information so that the highlight manager will redisplay
     * highlights over the area we just redisplayed.
     */

    DBPaintPlane(crec->dbw_hlRedraw, &labelArea,
	    DBStdPaintTbl(TT_ERROR_P, PL_DRC_ERROR),
	    (PaintUndoInfo *) NULL);

    /* display a debugging box which shows which area should be redrawn */
    if (debugit)
    {
	Rect r;
	WindSurfaceToScreen(w, &largerArea, &r);
	(void) GrClipBox(&r, STYLE_LABEL);
    }

#ifdef	notdef
    /* finally, display the plowing debugging information */
    cmdPlowDisplayEdges(w, rootArea, clipArea);
#endif	notdef

    GrUnlock(w);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbwPaintFunc --
 *
 * 	Invoked by database searching routines during redisplay to
 *	draw a tile on the screen.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Clips and draws a paint tile.
 *
 * ----------------------------------------------------------------------------
 */
int
dbwPaintFunc(tile, cxp)
    register Tile *tile;	/* Tile to be redisplayed. */
    TreeContext *cxp;		/* From DBTreeSrTiles */
{
    Rect r, r2;
    register SearchContext *scx = cxp->tc_scx;
			/* Contains pointer to use containing def
			 * in which tile appears, and transform to
			 * screen coordinates.
			 */

    if (!dbwIsLocked)
    {
	GrLock(dbwLockW, TRUE);
	GrClipTo(&rootClip);
	dbwIsLocked = TRUE;
    }
    if (dbwNeedStyle)
    {
	GrSetStuff(disStyle);
	dbwNeedStyle = FALSE;
    }

    /* If this isn't the edit cell, add 64 to the display style
     * to be used.
     */
    
    if (!dbwAllSame && ((editDef != scx->scx_use->cu_def)
	|| (scx->scx_trans.t_a != editTrans.t_a)
	|| (scx->scx_trans.t_b != editTrans.t_b)
	|| (scx->scx_trans.t_c != editTrans.t_c)
	|| (scx->scx_trans.t_d != editTrans.t_d)
	|| (scx->scx_trans.t_e != editTrans.t_e)
	|| (scx->scx_trans.t_f != editTrans.t_f)))
    {
	if (!disWasPale)
	{
	    GrSetStuff(disStyle+64);
	    disWasPale = TRUE;
	}
    }
    else
    {
	if (disWasPale)
	{
	    GrSetStuff(disStyle);
	    disWasPale = FALSE;
	}
    }

    r.r_xbot = LEFT(tile);
    r.r_ybot = BOTTOM(tile);
    r.r_xtop = RIGHT(tile);
    r.r_ytop = TOP(tile);
    GeoTransRect(&scx->scx_trans, &r, &r2);
    WindSurfaceToScreen(dbwWindow, &r2, &r);
    GrFastBox(&r);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBWDrawLabel --
 *
 * 	This procedure does all the work of actually drawing labels
 * 	on the screen.  It is invoked by label redisplay code and also
 *	by the selection redisplay code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The label given by text, rect, and pos is drawn on the screen
 *	in style "style" and size "labelSize".  The caller must have set
 *	up all of the clipping information for the window in which the
 *	label is to be drawn.  This procedure updates sizeBox to be
 *	large enough to hold the total pixel area occupied by the label
 *	(assuming it were drawn at (0,0)).
 *
 * ----------------------------------------------------------------------------
 */

void
DBWDrawLabel(text, rect, pos, style, labelSize, sizeBox)
    char *text;			/* Text to be displayed. */
    Rect *rect;			/* Area of label's box/line/cross, in screen
				 * coordinates.
				 */
    int pos;			/* Position of text relative to rect (e.g.
				 * GEO_NORTH) in screen coordinates.
				 */
    int style;                 /* Style to use for redisplay; if -1 then
                                * this has already been set by the caller
                                * and we shouldn't call GrSetStuff.
                                */

    int labelSize;		/* Size to use for drawing labels.  If < 0 then
				 * no text is drawn:  only the box.
				 */
    Rect *sizeBox;		/* Expanded if necessary to include the
				 * screen area of the text for this label.
				 */
{
    Point p;
    Rect location;

    if (style < 0) GrFastBox(rect);
    else GrClipBox(rect, style);
    if (labelSize < 0) return;

    switch (pos)
    {
	case GEO_CENTER:
	    p.p_x = (rect->r_xbot + rect->r_xtop)/2;
	    p.p_y = (rect->r_ybot + rect->r_ytop)/2;
	    break;
	case GEO_NORTH:
	    p.p_x = (rect->r_xbot + rect->r_xtop)/2;
	    p.p_y = rect->r_ytop;
	    break;
	case GEO_NORTHEAST:
	    p = rect->r_ur;
	    break;
	case GEO_EAST:
	    p.p_x = rect->r_xtop;
	    p.p_y = (rect->r_ybot + rect->r_ytop)/2;
	    break;
	case GEO_SOUTHEAST:
	    p.p_x = rect->r_xtop;
	    p.p_y = rect->r_ybot;
	    break;
	case GEO_SOUTH:
	    p.p_x = (rect->r_xbot + rect->r_xtop)/2;
	    p.p_y = rect->r_ybot;
	    break;
	case GEO_SOUTHWEST:
	    p = rect->r_ll;
	    break;
	case GEO_WEST:
	    p.p_x = rect->r_xbot;
	    p.p_y = (rect->r_ybot + rect->r_ytop)/2;
	    break;
	case GEO_NORTHWEST:
	    p.p_x = rect->r_xbot;
	    p.p_y = rect->r_ytop;
	    break;
    }

    if (GrPutText(text, style, &p, pos, labelSize,
	    FALSE, &GrScreenRect, &location) )
    {
	sizeBox->r_xbot = MIN(sizeBox->r_xbot, location.r_xbot - p.p_x);
	sizeBox->r_ybot = MIN(sizeBox->r_ybot, location.r_ybot - p.p_y);
	sizeBox->r_xtop = MAX(sizeBox->r_xtop, location.r_xtop - p.p_x);
	sizeBox->r_ytop = MAX(sizeBox->r_ytop, location.r_ytop - p.p_y);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * dbwLabelFunc --
 *
 * 	Called by database searching routines during redisplay.  It
 *	displays a label on the screen.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	The label is clipped and drawn on the screen.
 *
 * ----------------------------------------------------------------------------
 */
    /*ARGSUSED*/
int
dbwLabelFunc(scx, label, tpath)
    SearchContext *scx;		/* Contains pointer to use containing def in
				 * which label appears, and transform to
				 * screen coordinates.
				 */
    Label *label;		/* Label to be displayed. */
    TerminalPath *tpath;	/* Contains pointer to full pathname of label */
{
    Rect labRect, tmp;
    int screenPos;

    screenPos = GeoTransPos(&scx->scx_trans, label->lab_pos);
    GeoTransRect(&scx->scx_trans, &label->lab_rect, &tmp);
    WindSurfaceToScreen(dbwWindow, &tmp, &labRect);
    if (!GEO_TOUCH(&labRect, &dbwWindow->w_screenArea)) return 0;
    DBWDrawLabel(label->lab_text, &labRect, screenPos, -1,
	dbwLabelSize, dbwExpandAmounts);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbwBBoxFunc --
 *
 * 	Called by database searching routines during redisplay.
 *      The caller should have already set the style information.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	The bounding box of the cell is clipped and drawn on the screen,
 *	along with the name and id of the celluse.
 *
 * ----------------------------------------------------------------------------
 */
int
dbwBBoxFunc(scx)
    SearchContext *scx;		/* Describes context of cell. */
{
    Rect r, r2;
    char idName[100];
    Point p;
    CellUse *cellUse;

    cellUse = scx->scx_use;
    GeoTransRect(&scx->scx_trans, &cellUse->cu_def->cd_bbox, &r2);
    WindSurfaceToScreen(dbwWindow, &r2, &r);
    GrFastBox(&r);

    /* Don't futz around with text if the bbox is tiny. */

    if (((r.r_xtop-r.r_xbot) < dbwMinBBox.r_xtop)
	|| ((r.r_ytop-r.r_ybot) < dbwMinBBox.r_ytop)) return 0;

    p.p_x = (r.r_xbot + r.r_xtop)/2;
    p.p_y = (r.r_ybot + 2*r.r_ytop)/3;
    GeoClip(&r, &windClip);
    GrPutText(cellUse->cu_def->cd_name, -1, &p,
	GEO_CENTER, GR_TEXT_LARGE, TRUE, &r, (Rect *) NULL);
    (void) DBPrintUseId(scx, idName, 100);
    p.p_y = (2*r.r_ybot + r.r_ytop)/3;
    GrPutText(idName, -1, &p, GEO_CENTER,
        GR_TEXT_LARGE, TRUE, &r, (Rect *) NULL);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbwTileFunc --
 *
 * 	Called by the database search routines, from DBWredisplay
 *	during redisplay.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Displays a tile's bounding box and corner stitches.
 *
 * ----------------------------------------------------------------------------
 */
int
dbwTileFunc(tile)
    Tile *tile;				/* A tile to be redisplayed. */
{
    Rect r, r2;
    int xoffset, yoffset;
    Point p;
    Point pLL, pUR;
    Tile *stitch;
    char string[20];
    int i, pos;

    TiToRect(tile, &r2);

    /* Some of the tiles have "infinite sizes", which we have to clip
     * in order to avoid numerical problems.
     */

    GeoClip(&r2, &dbwWatchArea);
    pLL = r2.r_ll;
    pUR = r2.r_ur;
    GeoTransRect(&dbwWatchTrans, &r2, &r);
    WindSurfaceToScreen(dbwWindow, &r, &r2);

    /* Draw the tile, then put up text for the tile's address and
     * stitches.
     */

    (void) GrClipBox(&r2, STYLE_DRAWTILE);
    GeoTransPoint(&dbwWatchTrans, &pLL, &p);
    WindPointToScreen(dbwWindow, &p, &pLL);
    GeoTransPoint(&dbwWatchTrans, &pUR, &p);
    WindPointToScreen(dbwWindow, &p, &pUR);
    GeoClipPoint(&pLL, &rootClip);
    GeoClipPoint(&pUR, &rootClip);

    
    if (dbwSeeTypes)
    {
    	 (void) sprintf(string, "%s",DBTypeShortName(TiGetType(tile)));
    }
    else
    {
    	 (void) sprintf(string, "%x", tile);
    }
    
    GeoClip(&r2, &rootClip);
    p.p_x = (r2.r_xbot + r2.r_xtop)/2;
    p.p_y = (r2.r_ybot + r2.r_ytop)/2;
    if (!dbwWatchDemo || dbwSeeTypes)
	GrPutText(string, STYLE_DRAWTILE, &p, GEO_CENTER,
	    GR_TEXT_LARGE, FALSE, &r2, (Rect *) NULL);
    
#define	OFFSET	12

    for (i=0;  i<4;  i++)
    {
	xoffset = 0;
	yoffset = 0;
	switch (i)
	{
	    case 0:
		stitch = BL(tile);
		p = pLL;
		yoffset = OFFSET;
		pos = GEO_NORTHEAST;
		break;
	    case 1:
		stitch = LB(tile);
		p = pLL;
		xoffset = OFFSET;
		pos = GEO_NORTHEAST;
		break;
	    case 2:
		stitch = RT(tile);
		p = pUR;
		xoffset = -OFFSET;
		pos = GEO_SOUTHWEST;
		break;
	    case 3:
		stitch = TR(tile);
		p = pUR;
		yoffset = -OFFSET;
		pos = GEO_SOUTHWEST;
		break;
	}

	pos = GeoTransPos(&dbwWatchTrans, pos);

	if (dbwWatchTrans.t_a == 0)
	{
	    /* a rotation by 90 or 270 */
	    int temp;
	    temp = xoffset;
	    xoffset = yoffset;
	    yoffset = temp;
	}

	if ( (dbwWatchTrans.t_a < 0) || (dbwWatchTrans.t_b < 0) )
	{
	    /* mirror in x */
	    xoffset = -xoffset;
	}

	if ( (dbwWatchTrans.t_d < 0) || (dbwWatchTrans.t_e < 0) )
	{
	    /* mirror in y */
	    yoffset = -yoffset;
	}

	p.p_x += xoffset;
	p.p_y += yoffset;
	if (dbwWatchDemo)
	{
	    Rect stick, head, head2;
	    stick.r_ll = p;
	    stick.r_ur = p;
#define TAIL	5
#define HEAD	9
	    switch (i)
	    {
		case 0:
		    stick.r_xbot -= HEAD;
		    stick.r_xtop += TAIL;
		    head = stick;
		    head.r_xbot++;
		    head.r_xtop = head.r_xbot;
		    head.r_ytop++;
		    head.r_ybot--;
		    head2 = head;
		    head2.r_xbot++;
		    head2.r_xtop++;
		    head2.r_ytop++;
		    head2.r_ybot--;
		    break;
		case 1:
		    stick.r_ybot -= HEAD;
		    stick.r_ytop += TAIL;
		    head = stick;
		    head.r_ybot++;
		    head.r_ytop = head.r_ybot;
		    head.r_xtop++;
		    head.r_xbot--;
		    head2 = head;
		    head2.r_xbot--;
		    head2.r_xtop++;
		    head2.r_ytop++;
		    head2.r_ybot++;
		    break;
		case 2:
		    stick.r_ybot -= TAIL;
		    stick.r_ytop += HEAD;
		    head = stick;
		    head.r_ytop--;
		    head.r_ybot = head.r_ytop;
		    head.r_xtop++;
		    head.r_xbot--;
		    head2 = head;
		    head2.r_xbot--;
		    head2.r_xtop++;
		    head2.r_ytop--;
		    head2.r_ybot--;
		    break;
		case 3:
		    stick.r_xbot -= TAIL;
		    stick.r_xtop += HEAD;
		    head = stick;
		    head.r_xtop--;
		    head.r_xbot = head.r_xtop;
		    head.r_ytop++;
		    head.r_ybot--;
		    head2 = head;
		    head2.r_xbot--;
		    head2.r_xtop--;
		    head2.r_ytop++;
		    head2.r_ybot--;
		    break;
	    }
	    GrClipBox(&stick, STYLE_LABEL);
	    GrClipBox(&head, STYLE_LABEL);
	    GrClipBox(&head2, STYLE_LABEL);
	}
	else if (!dbwSeeTypes)
	{
	    (void) sprintf(string, "%x", stitch);
	    GrPutText(string, STYLE_DRAWTILE, &p, pos,
		GR_TEXT_SMALL, FALSE, &r2, (Rect *) NULL);
	}
    }
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *	DBWAreaChanged --
 *
 * 	Invoked to remind us that a certain piece of a certain cell
 *	has been modified and must eventually be redisplayed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is remembered, so that when WindUpdate is called
 *	the area will be redisplayed in every window where it appears.
 *
 * ----------------------------------------------------------------------------
 */

static TileTypeBitMask *dbwLayersChanged;
				/* DBWAreaChanged's "layers" parameter. */

void
DBWAreaChanged(cellDef, defArea, expandMask, layers)
    CellDef *cellDef;		/* The cell definition that was modified. */
    Rect *defArea;		/* The area of the definition that changed. */
    int expandMask;		/* We're only interested these windows.
				 * Use DBW_ALLWINDOWS for all windows.
				 */
    TileTypeBitMask *layers;	/* Indicates which layers were modified.  If
				 * NULL, it means that labels were deleted
				 * from defArea in addition to paint.  We'll
				 * have to redisplay a larger area in order
				 * to fully erase labels that used to stick
				 * out from defArea.  NULL is used when
				 * subcells are unexpanded or moved for
				 * example.  NULL is the most inclusive option
				 * (does everything that &DBAllButSpaceBits
				 * does and more), and should be used whenever
				 * you're not sure what to do.  Note:  it's
				 * normally better to use DBWLabelChanged when
				 * individual labels are modified;  the NULL
				 * option is for when large numbers of labels
				 * have potentially been modified.
				 */
{
    Rect newArea;		/* Corresponding area in parent. */
    int newMask;		/* Windows we're interested in in parent. */
    int x, y, xlo, ylo;		/* Array indices. */
    int xhi, yhi;
    Rect tmp;
    CellUse *use;

    if ((defArea->r_xbot == defArea->r_xtop)
	|| (defArea->r_ybot == defArea->r_ytop)) return;

    /**
    TxPrintf("Cell %s, area (%d, %d) (%d, %d) changed, mask %d.\n", 
	    cellDef->cd_name,  defArea->r_xbot, defArea->r_ybot,
	    defArea->r_xtop, defArea->r_ytop, expandMask);
    **/

    /* Don't permit signals to interrupt us here. */

    SigDisableInterrupts();

    /* First, translate the area back up through the hierarchy to
     * cells that are roots of windows.
     */
    
    for (use = cellDef->cd_parents; use != NULL; use = use->cu_nextuse)
    {
	/* We're only interested in a use if it's expanded in one of
	 * the windows of expandMask.  Our new expand mask is the
	 * AND of the old one and the windows in which this use is
	 * expanded.
	 */
	
	newMask = expandMask & use->cu_expandMask;
	if (newMask == 0) continue;

	/* If this use has no parents, it might be the root of a window.
	 * If so, log the area in the window.
	 */

	if (use->cu_parent == NULL)
	{
	    dbwLayersChanged = layers;
	    (void) WindSearch((ClientData) DBWclientID, (ClientData) use,
		defArea, dbwChangedFunc, (ClientData) defArea);
	    continue;
	}

	/* This use isn't a root use.  If it isn't an array use, just
	 * translate the area back into the coordinates of the parent
	 * and invoke ourselves recursively.
	 */
	
	if ((use->cu_xlo == use->cu_xhi) && (use->cu_ylo == use->cu_yhi))
	{
	    GeoTransRect(&use->cu_transform, defArea, &newArea);
	    DBWAreaChanged(use->cu_parent, &newArea, newMask, layers);
	    continue;
	}

	/* This is an array.  If the area to be redisplayed is a
	 * substantial fraction of the total area of the array,
	 * just redisplay the whole array.  Otherwise redisplay
	 * the individual elements.
	 */

	if ((2*(defArea->r_xtop - defArea->r_xbot) >
	    (cellDef->cd_bbox.r_xtop - cellDef->cd_bbox.r_xbot))
	    || (2*(defArea->r_ytop - defArea->r_ybot) >
	    (cellDef->cd_bbox.r_ytop - cellDef->cd_bbox.r_ybot)))
	{
	    DBComputeArrayArea(defArea, use, use->cu_xlo,
		use->cu_ylo, &newArea);
	    DBComputeArrayArea(defArea, use, use->cu_xhi,
		use->cu_yhi, &tmp);
	    (void) GeoInclude(&newArea, &tmp);
	    GeoTransRect(&use->cu_transform, &tmp, &newArea);
	    DBWAreaChanged(use->cu_parent, &newArea, newMask, layers);
	}
	else
	{
	    if (use->cu_xlo > use->cu_xhi)
	    {
		xlo = use->cu_xhi; xhi = use->cu_xlo;
	    }
	    else
	    {
		xlo = use->cu_xlo; xhi = use->cu_xhi;
	    }
	    if (use->cu_ylo > use->cu_yhi)
	    {
		ylo = use->cu_yhi; yhi = use->cu_ylo;
	    }
	    else
	    {
		ylo = use->cu_ylo; yhi = use->cu_yhi;
	    }
	    for (y = ylo ; y <= yhi; y++)
		for (x = xlo; x <= xhi; x++)
		{
		    DBComputeArrayArea(defArea, use, x, y, &tmp);
		    GeoTransRect(&use->cu_transform, &tmp, &newArea);
		    DBWAreaChanged(use->cu_parent, &newArea, newMask, layers);
		}
	}
    }
    SigEnableInterrupts();
}


/*
 * ----------------------------------------------------------------------------
 *
 * dbwChangedFunc --
 *
 * 	This function is invoked by WindSearch under control of
 *	DBWAreaChanged.  It notifies the window manager to redisplay
 *	an area of a window.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	An area to be redisplayed is recorded.
 *
 * ----------------------------------------------------------------------------
 */

int
dbwChangedFunc(w, area)
    Window *w;			/* Window in which to record area. */
    Rect *area;			/* (Client data) Area to be redisplayed, in
				 * coordinates of the root definition.
				 */
{
    Rect screenArea;
    TileTypeBitMask tmp;
    DBWclientRec *crec = (DBWclientRec *) w->w_clientData;

    /* If none of the layers being redisplayed is visible in this
     * window, then there's no need to do anything.
     */
    
    if (dbwLayersChanged != NULL)
    {
	TTMaskAndMask3(&tmp, dbwLayersChanged, &crec->dbw_visibleLayers);
	if (TTMaskIsZero(&tmp)) return 0;
    }

    /* Compute screen area to redisplay, in pixels. */

    WindSurfaceToScreen(w, area, &screenArea);
    GeoClip(&screenArea, &w->w_screenArea);

    /* If labels are being redisplayed, expand the redisplay area
     * to account for labels that are rooted in the given area but
     * stick out past it.
     */
    
    if (dbwLayersChanged == NULL)
    {
	screenArea.r_xbot += crec->dbw_expandAmounts.r_xbot;
	screenArea.r_ybot += crec->dbw_expandAmounts.r_ybot;
	screenArea.r_xtop += crec->dbw_expandAmounts.r_xtop;
	screenArea.r_ytop += crec->dbw_expandAmounts.r_ytop;
    }

    /* If watching is enabled for this window, so sorry but the whole
     * thing will have to be redisplayed (even a small change could have
     * affected many many tiles.
     */

    if (crec->dbw_watchPlane >= 0) WindAreaChanged(w, (Rect *) NULL);
    else WindAreaChanged(w, &screenArea);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBWLabelChanged --
 *
 * 	This procedure is invoked when a label has been created or
 *	deleted.  It figures out which areas to redisplay in order
 *	to make sure that the label is completely redrawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list of areas to be redisplayed is modified.
 *
 * ----------------------------------------------------------------------------
 */


/* The following stuff is shared by the procedures below. */

static Rect boxArea;		/* Area of label box (not including text)
				 * in root cell coords.
				 */
static int rootPos;		/* Label position in root cell coords. */
extern int dbwLabelChangedFunc();  /* Function to call for each label. */

void
DBWLabelChanged(cellDef, text, area, pos, mask)
    CellDef *cellDef;		/* Cell definition containing label. */
    char *text;			/* The text of the label. */
    Rect *area;			/* The area of the label. */
    int pos;			/* Relative position of text to area.
    int mask;			/* Mask of windows where changes should be
				 * reflected (DBW_ALLWINDOWS selects all
				 * windows, and is usually the right value.)
				 */
{
    CellUse *use;
    Rect newArea, tmp;
    int newMask, newPos;
    int x, y, xlo, ylo, xhi, yhi;

    /* This procedure is basically the same as DBWAreaChanged, so
     * see that procedure for documentation on how this all works.
     */
    
    SigDisableInterrupts();
    for (use = cellDef->cd_parents; use != NULL; use = use->cu_nextuse)
    {
	newMask = mask & use->cu_expandMask;
	if (newMask == 0) continue;

	if (use->cu_parent == NULL)
	{
	    /* Got the root use for a window.  Find the relevant windows
	     * and do the rest of the processing on a per-window basis.
	     */

	    boxArea = *area;
	    rootPos = pos;
	    (void) WindSearch((ClientData) DBWclientID, (ClientData) use,
		(Rect *) NULL, dbwLabelChangedFunc, (ClientData) text);
	    continue;
	}

	if (use->cu_xlo > use->cu_xhi)
	{
	    xlo = use->cu_xhi;
	    xhi = use->cu_xlo;
	}
	else
	{
	    xlo = use->cu_xlo;
	    xhi = use->cu_xhi;
	}
	if (use->cu_ylo > use->cu_yhi)
	{
	    ylo = use->cu_yhi;
	    yhi = use->cu_ylo;
	}
	else
	{
	    ylo = use->cu_ylo;
	    yhi = use->cu_yhi;
	}
	for (y = ylo; y <= yhi; y++)
	    for (x = xlo; x <= xhi; x++)
	    {
		DBComputeArrayArea(area, use, x, y, &tmp);
		GeoTransRect(&use->cu_transform, &tmp, &newArea);
		newPos = GeoTransPos(&use->cu_transform, pos);
		DBWLabelChanged(use->cu_parent, text, &newArea,
		    newPos, newMask);
	    }
    }
    SigEnableInterrupts();
}

int
dbwLabelChangedFunc(w, text)
    Window *w;			/* Window in which label is displayed. */
    char *text;			/* Text of label.  BoxArea gives positioning
				 * point and pos gives relative location.
				 */
{
    Rect screenArea, textArea;
    int size;

    WindSurfaceToScreen(w, &boxArea, &screenArea);
    size = ((DBWclientRec *) w->w_clientData)->dbw_labelSize;
    if (size < GR_TEXT_SMALL)
	textArea = GrCrossRect;
    else
    {
	GrLabelSize(text, rootPos, size, &textArea);
	(void) GeoInclude(&GrCrossRect, &textArea);
    }
    screenArea.r_xbot += textArea.r_xbot;
    screenArea.r_ybot += textArea.r_ybot;
    screenArea.r_xtop += textArea.r_xtop;
    screenArea.r_ytop += textArea.r_ytop;
    WindAreaChanged(w, &screenArea);
    return 0;
}

/*
 * Technology initialization for the display module.
 */

global TileTypeBitMask	DBWStyleToTypesTbl[MAXTILESTYLES];

/*
 * ----------------------------------------------------------------------------
 * DBWTechInit --
 *
 * Initialize the display module's technology dependent variables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the display module's technology dependent information.
 * ----------------------------------------------------------------------------
 */

DBWTechInit()
{
    register i;

    for (i = 0; i < MAXTILESTYLES; i++)
	TTMaskZero(&DBWStyleToTypesTbl[i]);
}

/*
 * ----------------------------------------------------------------------------
 * DBWTechAddStyle --
 *
 * Add a new entry to the style tables.
 *
 * Results:
 *	TRUE if successful, FALSE on error.
 *
 * Side effects:
 *	Updates the display module's technology variables.
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBWTechAddStyle(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    TileType t;
    int style;
    static char styleType[50];

    if (argc != 2)
    {
	TechError("Badly formed line in \"style\" section\n");
	return TRUE;
    }

    if (strcmp(argv[0], "styletype") == 0)
    {
	(void) strncpy(styleType, argv[1], 49);
	styleType[49] = 0;
	DBWStyleType = styleType;
	return TRUE;
    }
    if ((t = DBTechNoisyNameType(argv[0])) < 0) return (FALSE);
    style = atoi(argv[1]);
    if (style >= MAXTILESTYLES)
    {
	TechError("Invalid style %d for tile type %s\n", argv[1], argv[0]);
	return TRUE;
    }

    TTMaskSetType(&DBWStyleToTypesTbl[style], t);
    return TRUE;
}
