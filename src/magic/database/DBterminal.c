/*
 * DBterminal.c --
 *
 * Support for finding "terminals" for routing.
 * This is a tentative stab at the database utilities required;
 * it may well change.
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
static char rcsid[] = "$Header: DBterminal.c,v 6.0 90/08/28 18:10:22 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "tech.h"
#include "textio.h"

/*
 * ----------------------------------------------------------------------------
 *
 * DBFindXTerminal --
 *
 * Find a horizontally extending terminal.  Such a terminal can be
 * found either at the top or the bottom of a cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets *prect to be a degenerate rectangle (one with zero height)
 *	whose endpoints mark the extent of all material on the same plane
 *	as the tile supplied, extending in a horizontal line through the
 *	specified point, and which is entirely of types specified by the
 *	supplied TileTypeBitMask.
 *
 *	The point supplied must lie on either the top or the bottom of
 *	the tile given.
 *
 * ----------------------------------------------------------------------------
 */

void
DBFindXTerminal(startTile, plane, edgePoint, mask, prect)
    Tile *startTile;		/* Tile having edgePoint lying either on its
				 * top or its bottom edge.
				 */
    Plane *plane;		/* Tile plane containing startTile */
    Point *edgePoint;		/* Point which must lie on the top or bottom
				 * edge of startTile.  The Y coordinate of
				 * this point determines the Y coordinate of
				 * the line searched for compatible material.
				 */
    TileTypeBitMask *mask;	/* Mask specifying all tile types which are
				 * considered to comprise the terminal.
				 */
    Rect *prect;		/* Set to be the degenerate rectangle
				 * specifying the extent of the terminal.
				 * The top and bottom Y coordinates will be
				 * the same as the Y coordinate of edgePoint.
				 */
{
    Point herePoint;
    int x;
    register Tile *hereTile;

    ASSERT(edgePoint->p_y==TOP(startTile) || edgePoint->p_y==BOTTOM(startTile),
		"DBFindXTerminal");

    herePoint.p_x = edgePoint->p_x;
    if (edgePoint->p_y == BOTTOM(startTile))
	herePoint.p_y = edgePoint->p_y;
    else
	herePoint.p_y = edgePoint->p_y - 1;

    prect->r_ybot = prect->r_ytop = edgePoint->p_y;
    startTile = TiSrPoint(startTile, plane, &herePoint);

    /* Find leftmost extent */
    hereTile = startTile;
    while (x = herePoint.p_x, TTMaskHasType(mask, TiGetType(hereTile)))
    {
	herePoint.p_x = LEFT(hereTile) - 1;
	hereTile = TiSrPoint(hereTile, plane, &herePoint);
    }
    prect->r_xbot = x+1;

    /* Find rightmost extent */
    hereTile = startTile;
    while (x = herePoint.p_x, TTMaskHasType(mask, TiGetType(hereTile)))
    {
	herePoint.p_x = RIGHT(hereTile);
	hereTile = TiSrPoint(hereTile, plane, &herePoint);
    }
    prect->r_xtop = x;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBFindYTerminal --
 *
 * Find a vertically extending terminal.  Such a terminal can be
 * found either at the left or the right of a cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets *prect to be a degenerate rectangle (one with zero width)
 *	whose endpoints mark the extent of all material on the same plane
 *	as the tile supplied, extending in a vertical line through the
 *	specified point, and which is entirely of types specified by the
 *	supplied TileTypeBitMask.
 *
 *	The point supplied must lie on either the left or the right of
 *	the tile given.
 *
 * ----------------------------------------------------------------------------
 */

void
DBFindYTerminal(startTile, plane, edgePoint, mask, prect)
    Tile *startTile;		/* Tile having edgePoint lying either on its
				 * left or its right edge.
				 */
    Plane *plane;		/* Tile plane containing startTile */
    Point *edgePoint;		/* Point which must lie on the left or right
				 * edge of startTile.  The X coordinate of this
				 * point determines the X coordinate of the
				 * line searched for compatible material.
				 */
    TileTypeBitMask *mask;	/* Mask specifying all tile types which are
				 * considered to comprise the terminal.
				 */
    Rect *prect;		/* Set to be the degenerate rectangle
				 * specifying the extent of the terminal.
				 * The left and right X coordinates will be
				 * the same as the X coordinate of edgePoint.
				 */
{
    Point herePoint;
    int y;
    register Tile *hereTile;

    ASSERT(edgePoint->p_x==LEFT(startTile) || edgePoint->p_x==RIGHT(startTile),
		"DBFindYTerminal");

    herePoint.p_y = edgePoint->p_y;
    if (edgePoint->p_x == LEFT(startTile))
	herePoint.p_x = edgePoint->p_x;
    else
	herePoint.p_x = edgePoint->p_x - 1;

    startTile = TiSrPoint(startTile, plane, &herePoint);
    prect->r_xbot = prect->r_xtop = edgePoint->p_x;

    /* Find topmost extent */
    hereTile = startTile;
    while (y = herePoint.p_y, TTMaskHasType(mask, TiGetType(hereTile)))
    {
	herePoint.p_y = TOP(hereTile);
	hereTile = TiSrPoint(hereTile, plane, &herePoint);
    }
    prect->r_ytop = y;

    /* Find bottommost extent */
    hereTile = startTile;
    while (y = herePoint.p_y, TTMaskHasType(mask, TiGetType(hereTile)))
    {
	herePoint.p_y = BOTTOM(hereTile) - 1;
	hereTile = TiSrPoint(hereTile, plane, &herePoint);
    }
    prect->r_ybot = y+1;
}
