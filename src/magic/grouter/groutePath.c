/*
 * groutePath.c -
 *
 * Global signal router code for low-level manipulation of GlPoints.
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
static char sccsid[] = "@(#)groutePoint.c	4.9 MAGIC (Berkeley) 12/6/85";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "doubleint.h"
#include "heap.h"
#include "tile.h"
#include "debug.h"
#include "database.h"
#include "gcr.h"
#include "windows.h"
#include "main.h"
#include "dbwind.h"
#include "signals.h"
#include "router.h"
#include "grouter.h"
#include "textio.h"
#include "malloc.h"

/* First, last, and current GlPages on list for allocating GlPoints */
GlPage *glPathFirstPage = NULL;
GlPage *glPathLastPage = NULL;
GlPage *glPathCurPage = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * glListAdd --
 *
 * Add a point to the head of the given linked list of points,
 * with a given cost.  To free the list when done, pass it to
 * glPathFreePerm() below.  Lists are linked using the gl_path
 * field of a GlPoint, just like paths, but the order of the
 * points is unimportant and the cost reflects the cost of
 * using a GlPoint as a starting point, rather than the cost
 * of reaching it via a particular path.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for the new point using mallocMagic(),
 *	rather than using the mechanism of glPathNew().  Hence the
 *	list built with glListAdd() persists through calls to
 *	glPathFreeTemp() and must be freed explicitly using glPathFreePerm().
 *
 * ----------------------------------------------------------------------------
 */

Void
glListAdd(list, pin, cost)
    register GlPoint **list;	/* Linked via gl_path pointers */
    GCRPin *pin;
    int cost;
{
    register GlPoint *newPt;

    MALLOC(GlPoint *, newPt, sizeof (GlPoint));
    newPt->gl_tile = (Tile *) NULL;
    newPt->gl_pin = pin;
    newPt->gl_cost = cost;
    newPt->gl_path = *list;
    *list = newPt;
}

/*
 * ----------------------------------------------------------------------------
 *
 * glListToHeap --
 *
 * Copy crossing points from the list of starting points for a net
 * into the global heap.  The heap cost of the copied point includes
 * the Manhattan distance from the point to the destination.  We
 * recompute the hint tiles for points when we copy them, since
 * the tile structure of glChanPlane may have changed since list
 * was built.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated.  Structs are added to the global heap.
 *
 * ----------------------------------------------------------------------------
 */

glListToHeap(list, destPt)
    GlPoint *list;	/* List of points linked via gl_path pointers */
    Point *destPt;
{
    register GlPoint *temp, *new;
    register GCRPin *pin;
    Tile *tp;
    int dist;

    for (temp = list; temp; temp = temp->gl_path)
    {
	/*
	 * Only copy to heap if this pin doesn't lie within a blocked
	 * region (a pin can lie in a blocked region when the density
	 * for a portion of a channel is reached, and the pin happens
	 * to lie along that channel portion).
	 */
	pin = temp->gl_pin;
	if (tp = glChanPinToTile((Tile *) NULL, pin))
	{
	    new = glPathNew(pin, temp->gl_cost, (GlPoint *) NULL);
	    new->gl_tile = tp;
	    dist = ABSDIFF(pin->gcr_point.p_x, destPt->p_x) +
		   ABSDIFF(pin->gcr_point.p_y, destPt->p_y) +
		   temp->gl_cost;
	    HeapAdd(&glMazeHeap, &dist, (char *) new);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * glPathCopyPerm --
 * glPathFreePerm --
 *
 * The first procedure copies a path into a mallocMagic()'d list.
 * All points from 'list' are copied; the copy of the point whose
 * gl_path field pointed to root has its gl_path field pointing
 * to NULL.
 *
 * The second procedure frees a list allocated in this fashion.
 *
 * Results:
 *	glPathCopyPerm returns a pointer to the new GlPoint path;
 *	glPathFreePerm returns nothing.
 *
 * Side effects:
 *	Memory is allocated for the new point for real,
 *	using mallocMagic(), rather than using the mechanism
 *	of glPathNew().  Hence the list built with glPathCopyPerm
 *	persists through calls to glPathFreeTemp() and must be
 *	freed explicitly using glPathFreePerm().
 *
 * ----------------------------------------------------------------------------
 */

GlPoint *
glPathCopyPerm(list)
    register GlPoint *list;
{
    register GlPoint *new, *prev, *first;

    for (first = prev = NULL; list; prev = new, list = list->gl_path)
    {
	MALLOC(GlPoint *, new, sizeof (GlPoint));
	*new = *list;
	if (first == NULL) first = new;
	if (prev) prev->gl_path = new;
    }

    if (prev) prev->gl_path = (GlPoint *) NULL;

    return first;
}

Void
glPathFreePerm(list)
    GlPoint *list;
{
    register GlPoint *p;

    for (p = list; p; p = p->gl_path)
	FREE((char *) p);
}

/*
 * ----------------------------------------------------------------------------
 *
 * glPathNew --
 *
 * Create a new GlPoint and link it to the GlPoint from which
 * it was visited.
 *
 * Results:
 *	Pointer to the newly allocated struct.
 *
 * Side effects:
 *	Memory is allocated.
 *
 * ----------------------------------------------------------------------------
 */

GlPoint *
glPathNew(pin, cost, prev)
    GCRPin *pin;	/* Pin at the crossing point (mustn't be NULL) */
    int	cost;		/* The cost to get here from source */
    GlPoint *prev;	/* Point from which the new one was visited */
{
    register GlPoint *result;

    ASSERT(pin != NULL, "glPathNew");
    if (glPathCurPage == NULL || glPathCurPage->glp_free >= POINTSPERSEG)
    {
	/* Skip to next page if this one is full */
	if (glPathCurPage && glPathCurPage->glp_free >= POINTSPERSEG)
	    glPathCurPage = glPathCurPage->glp_next;

	/* If out of pages, allocate a new one */
	if (glPathCurPage == NULL)
	{
	    MALLOC(GlPage *, glPathCurPage, sizeof (GlPage));
	    glPathCurPage->glp_next = (GlPage *) NULL;
	    glPathCurPage->glp_free = 0;
	    if (glPathLastPage == NULL)
	    {
		glPathFirstPage = glPathCurPage;
		glPathLastPage = glPathCurPage;
	    }
	    else
	    {
		glPathLastPage->glp_next = glPathCurPage;
		glPathLastPage = glPathCurPage;
	    }
	}
    }

    result = &glPathCurPage->glp_array[glPathCurPage->glp_free++];
    result->gl_path = prev;
    result->gl_cost = cost;
    result->gl_pin = pin;
    result->gl_tile = (Tile *) NULL;

    return result;
}

/*
 * ----------------------------------------------------------------------------
 *
 * glPathFreeTemp --
 *
 * Reset the temporary arena used for allocating GlPoints.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 * ----------------------------------------------------------------------------
 */

Void
glPathFreeTemp()
{
    register GlPage *gpage;

    for (gpage = glPathFirstPage; gpage; gpage = gpage->glp_next)
    {
	/* Mark page of RoutePaths as being free */
	gpage->glp_free = 0;

	/* Can stop after processing the last page used in this cycle */
	if (gpage == glPathCurPage)
	    break;
    }

    /* Start allocating again from the first page on the list */
    glPathCurPage = glPathFirstPage;
}
