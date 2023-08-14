/* CIFsee.c -
 *
 *	This file provides procedures for displaying CIF layers on
 *	the screen using the highlight facilities.
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
static char rcsid[] = "$Header: CIFsee.c,v 6.0 90/08/28 18:05:21 mayo Exp $";
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
#include "CIFint.h"
#include "textio.h"
#include "undo.h"

/* The following variable holds the CellDef into which feedback
 * is to be placed for displaying CIF.
 */

static CellDef *cifSeeDef;


/*
 * ----------------------------------------------------------------------------
 *
 * cifSeeFunc --
 *
 * 	Called once for each tile that is to be displayed as feedback.
 *	This procedure enters the tile as feedback.  Note: the caller
 *	must arrange for cifSeeDef to contain a pointer to the cell
 *	def where feedback is to be displayed.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	A new feedback area is created over the tile.  The parameter
 *	"text" is associated with the feedback.
 * ----------------------------------------------------------------------------
 */

int
cifSeeFunc(tile, text)
    Tile *tile;			/* Tile to be entered as feedback. */
    char *text;			/* Explanation for the feedback. */
{
    Rect area;
    TiToRect(tile, &area);
    DBWFeedbackAdd(&area, text, cifSeeDef, CIFCurStyle->cs_scaleFactor,
	STYLE_PALEHIGHLIGHTS);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSeeLayer --
 *
 * 	Generates CIF over a given area of a given cell, then
 *	highlights a particular CIF layer on the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Highlight information is drawn on the screen.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSeeLayer(rootDef, area, layer)
    CellDef *rootDef;		/* Cell for which to generate CIF.  Must be
				 * the rootDef of a window.
				 */
    Rect *area;			/* Area in which to generate CIF. */
    char *layer;		/* CIF layer to highlight on the screen. */
{
    int oldCount, i;
    char msg[100];
    SearchContext scx;
    TileTypeBitMask mask;

    /* Make sure the desired layer exists. */

    if (!CIFNameToMask(layer, &mask))
	return;

    /* Flatten the area and generate CIF for it. */

    CIFErrorDef = rootDef;
    CIFInitCells();
    UndoDisable();
    CIFDummyUse->cu_def = rootDef;
    GEO_EXPAND(area, CIFCurStyle->cs_radius, &scx.scx_area);
    scx.scx_use = CIFDummyUse;
    scx.scx_trans = GeoIdentityTransform;
    (void) DBTreeSrTiles(&scx, &DBAllButSpaceAndDRCBits, 0,
	cifHierCopyFunc, (ClientData) CIFComponentDef);
    oldCount = DBWFeedbackCount;
    CIFGen(CIFComponentDef, area, CIFPlanes, &DBAllTypeBits, TRUE, TRUE);
    DBCellClearDef(CIFComponentDef);

    /* Report any errors that occurred. */

    if (DBWFeedbackCount != oldCount)
    {
	TxPrintf("%d problems occurred.  See feedback entries.\n",
	    DBWFeedbackCount-oldCount);
    }

    /* Display the chosen layer. */

    (void) sprintf(msg, "CIF layer \"%s\"", layer);
    cifSeeDef = rootDef;
    for (i = 0; i < CIFCurStyle->cs_nLayers; i++)
    {
	if (!TTMaskHasType(&mask, i)) continue;
	(void) DBSrPaintArea((Tile *) NULL, CIFPlanes[i], &TiPlaneRect,
	    &CIFSolidBits, cifSeeFunc, (ClientData) msg);
    }
    UndoEnable();
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSeeHierLayer --
 *
 * 	This procedure is similar to CIFSeeLayer except that it only
 *	generates hierarchical interaction information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	CIF information is highlighed on the screen.  If arrays is
 *	TRUE, then CIF that stems from array interactions is displayed.
 *	if subcells is TRUE, then CIF stemming from subcell interactions
 *	is displayed.  If both are TRUE, then both are displayed.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSeeHierLayer(rootDef, area, layer, arrays, subcells)
    CellDef *rootDef;		/* Def in which to compute CIF.  Must be
				 * the root definition of a window.
				 */
    Rect *area;			/* Area in which to generate CIF. */
    char *layer;		/* CIF layer to be highlighted. */
    bool arrays;		/* TRUE means show array interactions. */
    bool subcells;		/* TRUE means show subcell interactions. */
{
    int i, oldCount;
    char msg[100];
    TileTypeBitMask mask;

    /* Check out the CIF layer name. */

    if (!CIFNameToMask(layer, &mask)) return;

    CIFErrorDef = rootDef;
    oldCount = DBWFeedbackCount;
    CIFClearPlanes(CIFPlanes);
    if (subcells)
	CIFGenSubcells(rootDef, area, CIFPlanes);
    if (arrays)
	CIFGenArrays(rootDef, area, CIFPlanes);
    
    /* Report any errors that occurred. */

    if (DBWFeedbackCount != oldCount)
    {
	TxPrintf("%d problems occurred.  See feedback entries.\n",
	    DBWFeedbackCount - oldCount);
    }
    
    (void) sprintf(msg, "CIF layer \"%s\"", layer);
    cifSeeDef = rootDef;
    for (i = 0; i < CIFCurStyle->cs_nLayers; i++)
    {
	if (!TTMaskHasType(&mask, i)) continue;
	(void) DBSrPaintArea((Tile *) NULL, CIFPlanes[i], &TiPlaneRect,
	    &CIFSolidBits, cifSeeFunc, (ClientData) msg);
    }
}
