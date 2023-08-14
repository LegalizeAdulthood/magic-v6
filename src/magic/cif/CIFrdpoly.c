/* CIFreadpoly.c -
 *
 *	This file contains procedures that turn polygons into
 *	rectangles, as part of CIF file reading.
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
static char rcsid[] = "$Header: CIFrdpoly.c,v 6.0 90/08/28 18:05:12 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "CIFint.h"
#include "CIFread.h"
#include "malloc.h"

#define MAXPG 10000 	/* maximum # of points in polygon */
#define HEDGE 0		/* Horizontal edge */
#define REDGE 1		/* Rising edge */
#define FEDGE -1	/* Falling edge */

/*
 * ----------------------------------------------------------------------------
 *
 * cifLowX --
 *
 * 	This is a comparison procedure called by qsort.
 *
 * Results:
 *	1 if a.x > b.x,
 *     -1 if a.x < b.x,
 *	0 otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int 
cifLowX(a, b)
    CIFPath **a, **b;
{
    register Point *p, *q;

    p = &(*a)->cifp_point;
    q = &(*b)->cifp_point;
    if (p->p_x < q->p_x)
	return (-1);
    if (p->p_x > q->p_x)
	return (1);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifLowY --
 *
 * 	This is another comparison procedure called by qsort.
 *
 * Results:
 *	1 if a.y > b.y
 *     -1 if a.y < b.y
 *	0 otherwise
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int 
cifLowY(a, b)
    register Point **a, **b;
{
    if ((*a)->p_y < (*b)->p_y)
	return (-1);
    if ((*a)->p_y > (*b)->p_y)
	return (1);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifOrient --
 *
 * 	This procedure assigns a direction to each of the edges in a
 *	polygon.
 *
 * Results:
 *	TRUE is returned if all of the edges are horizontal or vertical,
 *	FALSE is returned otherwise.  If FALSE is returned, not all of
 *	the directions will have been filled in.
 *
 * Side effects:
 *	The parameter dir is filled in with the directions, which are
 *	each one of HEDGE, REDGE, or FEDGE.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifOrient(edges, nedges, dir)
    CIFPath *edges[];		/* Array of edges to be categorized. */
    int dir[];			/* Array to hold directions. */
    int nedges;			/* Size of arrays. */
{
    register Point *p, *q;
    int n;

    for (n = 0; n < nedges; n++)
    {
	/* note - path list should close on itself */
	p = &edges[n]->cifp_point;
	q = &edges[n]->cifp_next->cifp_point;
	if (p->p_y == q->p_y)
	{
	    /* note - point may connect to itself here */
	    dir[n] = HEDGE;
	    continue;
	}
	if (p->p_x == q->p_x)
	{
	    if (p->p_y < q->p_y)
	    {
		dir[n] = REDGE;
		continue;
	    }
	    if (p->p_y > q->p_y)
	    {
		dir[n] = FEDGE;
		continue;
	    }
	    /* Point connects to itself */
	    dir[n] = HEDGE;
	    continue;
	}
	/* It's not Manhattan, folks. */
	return (FALSE);
    }
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifCross --
 *
 * 	This procedure is used to see if an edge crosses a particular
 *	area.
 *
 * Results:
 *	TRUE is returned if edge is vertical and if it crosses the
 *	y-range defined by ybot and ytop.  FALSE is returned otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifCross(edge, dir, ybot, ytop)
    register CIFPath *edge;	/* Pointer to first of 2 path points in edge */
    int dir;			/* Direction of edge */
    int ybot, ytop;		/* Range of interest */
{
    int ebot, etop;

    switch (dir)
    {
	case REDGE:
	    ebot = edge->cifp_point.p_y;
	    etop = edge->cifp_next->cifp_point.p_y;
	    return (ebot <= ybot && etop >= ytop);

	case FEDGE:
	    ebot = edge->cifp_next->cifp_point.p_y;
	    etop = edge->cifp_point.p_y;
	    return (ebot <= ybot && etop >= ytop);

    }

    return (FALSE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFPolyToRects --
 *
 * 	Converts a manhattan polygon (specified as a path) into a
 *	linked list of rectangles.
 *
 * Results:
 *	The return value is a linked list of rectangles, or NULL if
 *	something went wrong.
 *
 * Side effects:
 *	Memory is allocated to hold the list of rectangles.  It is
 *	the caller's responsibility to free up the memory.
 *
 * ----------------------------------------------------------------------------
 */

LinkedRect *
CIFPolyToRects(path)
    CIFPath *path;		/* Path describing a polygon. */
{
    int npts = 0, n, dir[MAXPG], curr, wrapno;
    int xbot, xtop, ybot, ytop;
    Point *pts[MAXPG];
    CIFPath *p, *edges[MAXPG], *tail = 0;
    LinkedRect *rex = 0, *new;

    for (p = path; p; p = p->cifp_next, npts += 1)
    {
	if (npts >= MAXPG)
	{
	    CIFReadError("polygon with more than %d points.\n", MAXPG );
	    goto done;
	}
	pts[npts] = &(p->cifp_point);
	edges[npts] = p;
    }

    if (npts < 4)
    {
	    CIFReadError("polygon with fewer than 4 points.\n" );
	    goto done;
    }

    /* Close path list - don't worry, it's disconnected later. */

    tail = edges[npts-1];
    tail->cifp_next = path;

    /* Sort points by low y, edges by low x */

    qsort ((char *) pts, npts, (int) sizeof (Point *), cifLowY);
    qsort ((char *) edges, npts, (int) sizeof (CIFPath *), cifLowX);

    /* Find out which direction each edge points. */

    if (!cifOrient (edges, npts, dir))
    {
	    CIFReadError("non-manhattan polygon.\n" );
	    goto done;
    }

    /* Scan the polygon from bottom to top.  At each step, process
     * a minimum-sized y-range of the polygon (i.e. a range such that
     * there are no vertices inside the range).  Use wrap numbers
     * based on the edge orientations to determine how much of the
     * x-range for this y-range should contain material.
     */

    for (curr = 1; curr < npts; curr++)
    {
	/* Find the next minimum-sized y-range. */

	ybot = pts[curr-1]->p_y;
	while (ybot == pts[curr]->p_y)
	    if (++curr >= npts) goto done;
	ytop = pts[curr]->p_y;

	/* Process all the edges that cross the y-range, from left
	 * to right.
	 */

	for (wrapno=0, n=0; n < npts; n++)
	{
	    if (wrapno == 0) xbot = edges[n]->cifp_x;
	    if (!cifCross(edges[n], dir[n], ybot, ytop))
		    continue;
	    wrapno += dir[n] == REDGE ? 1 : -1;
	    if (wrapno == 0)
	    {
		xtop = edges[n]->cifp_point.p_x;
		if (xbot == xtop) continue;
		new = (LinkedRect *) mallocMagic(sizeof(LinkedRect));
		new->r_r.r_xbot = xbot;
		new->r_r.r_ybot = ybot;
		new->r_r.r_xtop = xtop;
		new->r_r.r_ytop = ytop;
		new->r_next = rex;
		rex = new;
	    }
	}
    }

    /* Normally, the loop exits directly to done, below.  It
     * only falls through here if the polygon has a degenerate
     * spike at its top (i.e. if there's only one point with
     * highest y-coordinate).
     */

done:
    /* Disconnect start of polygon from its tail. */

    if (tail)
	    tail->cifp_next = (CIFPath *) 0;
    return (rex);
}
