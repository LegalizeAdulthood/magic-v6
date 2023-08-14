/*
 * DBpaint2.c --
 *
 * More paint and erase primitives
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
static char rcsid[] = "$Header: DBpaint2.c,v 6.0 90/08/28 18:10:08 mayo Exp $";
#endif  not lint

#include <sys/types.h>
#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"

/*
 * ----------------------------------------------------------------------------
 * DBPaint --
 *
 * Paint a rectangular area with a specific tile type.
 * All paint tile planes in cellDef are painted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies potentially all paint tile planes in cellDef.
 * ----------------------------------------------------------------------------
 */

void
DBPaint (cellDef, rect, type)
    CellDef  * cellDef;		/* CellDef to modify */
    Rect     * rect;		/* Area to paint */
    TileType   type;		/* Type of tile to be painted */
{
    int pNum;
    PaintUndoInfo ui;

    cellDef->cd_flags |= CDMODIFIED|CDGETNEWSTAMP;
    ui.pu_def = cellDef;
    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (DBPaintOnPlane(type, pNum))
	{
	    ui.pu_pNum = pNum;
	    DBPaintPlane(cellDef->cd_planes[pNum], rect,
			DBStdPaintTbl(type, pNum), &ui);
	}
}

/*
 * ----------------------------------------------------------------------------
 * DBErase --
 *
 * Erase a specific tile type from a rectangular area.
 * The plane in which tiles of the given type reside is modified
 * in cellDef.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies potentially all paint tile planes in cellDef.
 * ----------------------------------------------------------------------------
 */

void
DBErase (cellDef, rect, type)
    CellDef  * cellDef;		/* Cell to modify */
    Rect     * rect;		/* Area to paint */
    TileType   type;		/* Type of tile to be painted */
{
    int pNum;
    PaintUndoInfo ui;

    cellDef->cd_flags |= CDMODIFIED|CDGETNEWSTAMP;
    ui.pu_def = cellDef;
    if (type == TT_SPACE)
    {
	/*
	 * Erasing space is the same as erasing everything under
	 * the rectangle.
	 */
	for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	{
	    ui.pu_pNum = pNum;
	    DBPaintPlane(cellDef->cd_planes[pNum], rect,
			DBStdPaintTbl(TT_SPACE, pNum), &ui);
	}
    }
    else
    {
	/*
	 * Ordinary type is being erased.
	 * Generate the erase on all planes in cellDef.
	 */
	for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	    if (DBEraseOnPlane(type, pNum))
	    {
		ui.pu_pNum = pNum;
		DBPaintPlane(cellDef->cd_planes[pNum], rect,
			    DBStdEraseTbl(type, pNum), &ui);
	    }
    }
}

/*
 * ----------------------------------------------------------------------------
 * DBPaintMask --
 *
 * Paint a rectangular area with all tile types specified in the
 * mask supplied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies potentially all paint tile planes in cellDef.
 * ----------------------------------------------------------------------------
 */

void
DBPaintMask(cellDef, rect, mask)
    CellDef	*cellDef;	/* CellDef to modify */
    Rect	*rect;		/* Area to paint */
    TileTypeBitMask *mask;	/* Mask of types to be erased */
{
    TileType t;

    for (t = TT_SPACE + 1; t < DBNumTypes; t++)
	if (TTMaskHasType(mask, t))
	    DBPaint(cellDef, rect, t);
}

/*
 * ----------------------------------------------------------------------------
 * DBEraseMask --
 *
 * Erase a rectangular area with all tile types specified in the
 * mask supplied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies potentially all paint tile planes in cellDef.
 * ----------------------------------------------------------------------------
 */

void
DBEraseMask(cellDef, rect, mask)
    CellDef	*cellDef;	/* CellDef to modify */
    Rect	*rect;		/* Area to erase */
    TileTypeBitMask *mask;	/* Mask of types to be erased */
{
    TileType t;

    for (t = TT_SPACE + 1; t < DBNumTypes; t++)
	if (TTMaskHasType(mask, t))
	    DBErase(cellDef, rect, t);
}
