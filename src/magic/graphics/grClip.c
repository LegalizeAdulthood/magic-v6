/* grClip.c -
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
 *
 * This file contains additional functions to manipulate a
 * color display.  Included here are rectangle clipping and
 * drawing routines.
 */


#ifndef lint
static char rcsid[]="$Header: grClip.c,v 6.0 90/08/28 18:40:40 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "malloc.h"

/* Library imports: */

extern int sscanf();

/* Forward declaration: */

extern bool GrDisjoint();

/* The following rectangle defines the size of the cross drawn for
 * zero-size rectangles.  This must be all on one line to keep
 * lintpick happy!
 */

global Rect GrCrossRect = {-GR_CROSSSIZE, -GR_CROSSSIZE, GR_CROSSSIZE, GR_CROSSSIZE};

global int GrNumClipBoxes = 0;	/* for benchmarking */

global int grCurDStyle;

/* A rectangle that is one square of the grid */
static Rect *grGridRect;



/*
 * ----------------------------------------------------------------------------
 * GrSetStuff --
 *
 *	Set up current drawing style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are changed.  If anything is drawn, it will appear in the
 *	specified style.
 * ----------------------------------------------------------------------------
 */

/* Current state for rectangle and text drawing */
static int grCurWMask, grCurColor, grCurOutline, grCurStipple, grCurFill;

/* Has the device driver been informed of the above?  We only inform it
 * when we actually need to draw something -- this makes it harmless (in terms
 * of graphics bandwidth) to call GrSetStuff extra times.
 */
bool grDriverInformed = TRUE;

void
GrSetStuff(style)
    int style;
{
    grCurDStyle = style;
    grCurWMask = grStyleTable[style].mask;
    grCurColor = grStyleTable[style].color;
    grCurOutline = grStyleTable[style].outline;
    grCurStipple = grStyleTable[style].stipple;
    grCurFill = grStyleTable[style].fill;
    grDriverInformed = FALSE;
}

/*---------------------------------------------------------------------------
 * grInformDriver:
 *
 *	Inform the driver about the last GrSetStuff call.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */

void
grInformDriver()
{
    /* Now let the device drivers know */
    (*grSetWMandCPtr)(grCurWMask, grCurColor);
    (*grSetLineStylePtr)(grCurOutline);
    (*grSetStipplePtr)(grCurStipple);
    grDriverInformed = TRUE;
}


/*
 * ----------------------------------------------------------------------------
 * grClipAgainst --
 *
 *	Clip a linked list of rectangles against a single rectangle.  This
 *	may result in the list getting longer or shorter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The original list may change.
 * ----------------------------------------------------------------------------
 */

grClipAgainst(startllr, clip)
    LinkedRect **startllr;	/* A pointer to the pointer that heads 
				 * the list .
				 */
    Rect *clip;		  	/* The rectangle to clip against */
{
    extern bool grClipAddFunc();	/* forward declaration */
    LinkedRect **llr, *lr;

    for (llr = startllr; *llr != (LinkedRect *) NULL; /*nop*/ )
    {
	if ( GEO_TOUCH(&(*llr)->r_r, clip) )
	{
	    lr = *llr;
	    *llr = lr->r_next;
	    /* this will modify the list that we are traversing! */
	    (void) GrDisjoint(&lr->r_r, clip, grClipAddFunc, 
		    (ClientData) &llr);
	    FREE( (char *) lr);
	}
	else
	    llr = &((*llr)->r_next);
    }
}


/* Add a box to our linked list, and advance
 * our pointer into the list
 */

bool grClipAddFunc(box, cd)  
    Rect *box;
    ClientData cd;
{
    LinkedRect ***lllr, *lr;

    lllr = (LinkedRect ***) cd;

    MALLOC(LinkedRect *, lr, sizeof (LinkedRect));
    lr->r_r = *box;
    lr->r_next = **lllr;
    **lllr = lr;
    *lllr = &lr->r_next;
}


grObsBox(r)
    Rect *r;
{
    LinkedRect *ob;
    LinkedRect *ar;
    LinkedRect **areas;

    MALLOC(LinkedRect *, ar, sizeof (LinkedRect));
    ar->r_r = *r;
    ar->r_next = NULL;
    areas = &ar;

    /* clip against obscuring areas */
    for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
    {
	if ( GEO_TOUCH(r, &(ob->r_r)) )
	    grClipAgainst(areas, &(ob->r_r));
    }

    while (*areas != NULL)
    {
	LinkedRect *oldarea;
	if (grCurFill == GR_STGRID)
	    (void) (*grDrawGridPtr)(grGridRect, grCurOutline, &((*areas)->r_r));
	else
	    (*grFillRectPtr)(&((*areas)->r_r));
	oldarea = *areas;
	*areas = (*areas)->r_next;
	FREE( (char *) oldarea );
    }
}


/*---------------------------------------------------------
 * grClipPoints:
 *	This routine computes the 0, 1, or 2 intersection points
 *	between a line and a box.
 *
 * Results:
 *	False if the line is completely outside of the box.
 *
 * Side Effects:
 *---------------------------------------------------------
 */

bool
grClipPoints(line, box, p1, p1OK, p2, p2OK)
    Rect *line;		/* Actually a line from line->r_ll to
			 * line->r_ur.  It is assumed that r_ll is to
			 * the left of r_ur, but we don't assume that
			 * r_ll is below r_ur.
			 */
    Rect *box;		/* A box to check intersections with */
    Point *p1, *p2;	/* To be filled in with 0, 1, or 2 points
			 * that are on the border of the box as well as
			 * on the line.
			 */
    bool *p1OK, *p2OK;	/* Says if the point was filled in */
{
    int tmp, delx, dely;
    bool delyneg;
    int x1, x2, y1, y2;
    bool ok1, ok2;

    if (p1OK != NULL) *p1OK = FALSE;
    ok1 = FALSE;
    if (p2OK != NULL) *p2OK = FALSE;
    ok2 = FALSE;

    x1 = line->r_xbot;
    x2 = line->r_xtop;
    y1 = line->r_ybot;
    y2 = line->r_ytop;

    delx = x2-x1;
    dely = y2-y1;

    /* We have to be careful because of machine-dependent problems
     * with rounding during division by negative numbers.
     */

    if (dely<0)
    {
	dely = -dely;
	delyneg = TRUE;
    }
    else 
	delyneg = FALSE;
    /* we know that delx is nonnegative if this is a real (non-empty) line */
    if (delx < 0) return FALSE;

    if (x1 < box->r_xbot)
    {
	if (delx == 0) return FALSE;
	tmp = (((box->r_xbot-x1)*dely) + (delx>>1))/delx;
	if (delyneg) y1 -= tmp;
	else y1 += tmp;
	x1 = box->r_xbot;
    }
    else 
	if (x1 > box->r_xtop) return FALSE;

    if (x2 > box->r_xtop)
    {
	if (delx == 0) return FALSE;
	tmp = ((x2-box->r_xtop)*dely + (delx>>1))/delx;
	if (delyneg) y2 += tmp;
	else y2 -= tmp;
	x2 = box->r_xtop;
    }
    else 
	if (x2 < box->r_xbot) return FALSE;

    if (y2 > y1)
    {
	if (y1 < box->r_ybot)
	{
	    x1 += (((box->r_ybot-y1)*delx) + (dely>>1))/dely;
	    y1 = box->r_ybot;
	}
	else if (y1 > box->r_ytop) return FALSE;
	if (y2 > box->r_ytop)
	{
	    x2 -= (((y2 - box->r_ytop)*delx) + (dely>>1))/dely;
	    y2 = box->r_ytop;
	}
	else if (y2 < box->r_ybot) return FALSE;
    }
    else
    {
	if (y1 > box->r_ytop)
	{
	    if (dely == 0) return FALSE;
	    x1 += (((y1-box->r_ytop)*delx) + (dely>>1))/dely;
	    y1 = box->r_ytop;
	}
	else if (y1 < box->r_ybot) return FALSE;
	if (y2 < box->r_ybot)
	{
	    if (dely == 0) return FALSE;
	    x2 -= (((box->r_ybot-y2)*delx) + (dely>>1))/dely;
	    y2 = box->r_ybot;
	}
	else if (y2 > box->r_ytop) return FALSE;
    }

    if ( (x1 == box->r_xbot) || (y1 == box->r_ybot) || (y1 == box->r_ytop) )
    {
	if (p1 != NULL)
	{
	    p1->p_x = x1;
	    p1->p_y = y1;
	}
	if (p1OK != NULL) *p1OK = TRUE;
	ok1 = TRUE;
    }
    if ( (x2 == box->r_xtop) || (y2 == box->r_ybot) || (y2 == box->r_ytop) )
    {
	if (p2 != NULL)
	{
	    p2->p_x = x2;
	    p2->p_y = y2;
	}
	if (p2OK != NULL) *p2OK = TRUE;
	ok2 = TRUE;
    }
    /* is part of the line in the box? */
    return ok1 || ok2 || 
	    ((x1 >= box->r_xbot) && (x1 <= box->r_xtop) && (y1 >= box->r_ybot)
	    && (y1 <= box->r_ytop));
}


#define NEWAREA(lr,x1,y1,x2,y2)	{LinkedRect *tmp; \
    MALLOC(LinkedRect *, tmp, sizeof (LinkedRect)); \
    tmp->r_r.r_xbot = x1; tmp->r_r.r_xtop = x2; \
    tmp->r_r.r_ybot = y1; tmp->r_r.r_ytop = y2; tmp->r_next = lr; lr = tmp;}

/*---------------------------------------------------------
 * GrClipLine:
 *	GrClipLine will draw a line on the screen in the current
 *	style and clip stuff.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The line is drawn in the current style.
 *---------------------------------------------------------
 */

void
GrClipLine(x1, y1, x2, y2)
    int x1, y1, x2, y2;
{
    LinkedRect **ar;
    LinkedRect *ob;
    LinkedRect *areas;

    GR_CHECK_LOCK();
    if (!grDriverInformed) grInformDriver();

    /* we will pretend the the ll corner of a rectangle is the
     * left endpoint of a line, and the ur corner the right endpoint
     * of the line.
     */
    MALLOC(LinkedRect *, areas, sizeof (LinkedRect));
    areas->r_next = NULL;
    if (x1 < x2)
    {
	areas->r_r.r_xbot = x1;
	areas->r_r.r_ybot = y1;
	areas->r_r.r_xtop = x2;
	areas->r_r.r_ytop = y2;
    }
    else
    {
	areas->r_r.r_xtop = x1;
	areas->r_r.r_ytop = y1;
	areas->r_r.r_xbot = x2;
	areas->r_r.r_ybot = y2;
    }

    /* clip against the clip box */
    for (ar = &areas; *ar != NULL; )
    {
	Rect *l;
	Rect canonRect;
	l = &((*ar)->r_r);
	GeoCanonicalRect(l, &canonRect);
	if (!GEO_TOUCH(&canonRect, &grCurClip))
	{
	    /* line is totally outside of clip area */
	    goto deleteit;
	}
	else
	{
	    /* is there some intersection with clip area? */
	    if (!grClipPoints(l, &grCurClip, &(l->r_ll), (bool *) NULL,
		    &(l->r_ur), (bool*) NULL))
	    {
		/* no intersection */
		goto deleteit;
	    }

	    /* clip against obscuring areas */
	    for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
	    {
		Point p1, p2;
		Rect c;
		bool ok1, ok2;
		c = ob->r_r;
		c.r_xbot--;  c.r_ybot--;
		c.r_xtop++;  c.r_ytop++;
		if (grClipPoints(l, &c, &p1, &ok1, &p2, &ok2) &&
			!ok1 && !ok2)
		{
		    /* Line is not completely outside of the box,
		     * nor does it intersect it.
		     * Therefor, line is completely obscured.
		     */
		     goto deleteit;
		}

		if (ok1 && 
		   ( ((l->r_xbot == p1.p_x) && (l->r_ybot == p1.p_y)) ||
		     ((l->r_xtop == p1.p_x) && (l->r_ytop == p1.p_y)) ) )
		{
		    ok1 = FALSE;  /* do not split or clip at an endpoint */
		}
		if (ok2 && 
		   ( ((l->r_xbot == p2.p_x) && (l->r_ybot == p2.p_y)) ||
		     ((l->r_xtop == p2.p_x) && (l->r_ytop == p2.p_y)) ) )
		{
		    ok2 = FALSE;  /* do not split or clip at an endpoint */
		}

		if (ok1 ^ ok2)
		{
		    /* one segment to deal with */
		    if (ok1)
			l->r_ur = p1;
		    else
			l->r_ll = p2;
		}
		else if (ok1 && ok2)
		{
		    /* clip both sides */
		    LinkedRect *new;
		    MALLOC(LinkedRect *, new, sizeof (LinkedRect));
		    new->r_r.r_ur = l->r_ur;
		    new->r_r.r_ll = p2;
		    new->r_next = (*ar);
		    l->r_ur = p1;
		    (*ar) = new;
		}
	    }

	}

	ar = &((*ar)->r_next);
	continue;

deleteit: {
	    LinkedRect *reclaim;
	    reclaim = (*ar);
	    *ar = reclaim->r_next;
	    FREE( (char *) reclaim);
	}

    } /* for ar */


    /* draw the lines */
    while (areas != NULL)
    {
	LinkedRect *oldarea;
	(*grDrawLinePtr)(areas->r_r.r_xbot, areas->r_r.r_ybot,
		areas->r_r.r_xtop, areas->r_r.r_ytop);
	oldarea = areas;
	areas = areas->r_next;
	FREE( (char *) oldarea );
    }
}


/*---------------------------------------------------------
 * GrFastBox:
 *	GrFastBox will draw a rectangle on the screen in the
 *	style set by the previous call to GrSetStuff.  It will also
 *	be clipped against the rectangles passed to GrSetStuff.
 *
 *	If GrClipBox is called between GrFastBox calls then GrSetStuff
 *	must be called to set the parameters back.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The rectangle is drawn in the style specified.
 *	The rectangle is clipped before being drawn.
 *---------------------------------------------------------
 */

void
GrFastBox(prect)
    Rect *prect;	/* The rectangle to be drawn, given in
				 * screen coordinates.
			         */
{
    register Rect *r;
    bool needClip, needObscure;
    LinkedRect *ob;

    GR_CHECK_LOCK();
    if (!grDriverInformed) grInformDriver();
    GrNumClipBoxes++;
    if (grCurFill == GR_STGRID)
    {
	r = &grCurClip;
	grGridRect = prect;
    }
    else
    {
	r = prect;
	if (!GEO_TOUCH(r, &grCurClip)) return;
    }

    /* Do a quick check to make the (very common) special case of
     * no clipping go fast.
     */
    needClip = !GEO_SURROUND(&grCurClip, r);
    needObscure = FALSE;
    for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
	needObscure |= GEO_TOUCH(r, &(ob->r_r));

    /* do solid areas */
    if ( (grCurFill == GR_STSOLID) || 
	(grCurFill == GR_STSTIPPLE) || (grCurFill == GR_STGRID) )
    {
	Rect clipr;
	/* We have a filled area to deal with */
	clipr = *r;
	if (needClip)
	    GeoClip(&clipr, &grCurClip);
	if (needObscure)
	    grObsBox(&clipr);
	else
	{
	    if (grCurFill == GR_STGRID)
		(void) (*grDrawGridPtr)(grGridRect, grCurOutline, &clipr);
	    else
		(void) (*grFillRectPtr)(&clipr);
	}
    } 

    /* return if rectangle is too small to see */
#define GR_THRESH	4
    if ((r->r_xtop - r->r_xbot < GR_THRESH) && 
	    (r->r_ytop - r->r_ybot < GR_THRESH) && (grCurFill != GR_STOUTLINE))
	return;

    /* draw outlines */
    if ( (grCurOutline != 0) && (grCurFill != GR_STGRID) )
    {
	if ( (grCurFill == GR_STOUTLINE) && (r->r_xbot == r->r_xtop) &&
		(r->r_ybot == r->r_ytop) )
	{

	    /* turn the outline into a cross */
	    if (needClip || needObscure) 
		goto clipit;
	    else
	    {
		bool crossClip, crossObscure;
		Rect crossBox;

		/* check the larger cross area for clipping */
		crossBox.r_xbot = r->r_xbot - GR_CROSSSIZE;
		crossBox.r_ybot = r->r_ybot - GR_CROSSSIZE;
		crossBox.r_xtop = r->r_xtop + GR_CROSSSIZE;
		crossBox.r_ytop = r->r_ytop + GR_CROSSSIZE;

		crossClip = !GEO_SURROUND(&grCurClip, &crossBox);
		crossObscure = FALSE;
		for (ob = grCurObscure; ob != NULL; ob = ob->r_next)
		    crossObscure |= GEO_TOUCH(&crossBox, &(ob->r_r));

		if (crossClip || crossObscure) 
		    goto clipit;
		else
		    goto noclipit;
	    }

	    clipit:
		GrClipLine(r->r_xbot, r->r_ybot - GR_CROSSSIZE,
		    r->r_xtop, r->r_ytop + GR_CROSSSIZE);
		GrClipLine(r->r_xbot - GR_CROSSSIZE, r->r_ybot,
		    r->r_xtop + GR_CROSSSIZE, r->r_ytop);
		goto endit;

	    noclipit:
		(*grDrawLinePtr)(r->r_xbot, r->r_ybot - GR_CROSSSIZE,
		    r->r_xtop, r->r_ytop + GR_CROSSSIZE);
		(*grDrawLinePtr)(r->r_xbot - GR_CROSSSIZE, r->r_ybot,
		    r->r_xtop + GR_CROSSSIZE, r->r_ytop);

	    endit:  ;
	} 
	else
	{
	    if (needClip || needObscure)
	    {
		GrClipLine(r->r_xbot, r->r_ytop, r->r_xtop, r->r_ytop);
		GrClipLine(r->r_xbot, r->r_ybot, r->r_xtop, r->r_ybot);
		GrClipLine(r->r_xbot, r->r_ybot, r->r_xbot, r->r_ytop);
		GrClipLine(r->r_xtop, r->r_ybot, r->r_xtop, r->r_ytop);
	    }
	    else
	    {
		(*grDrawLinePtr)(r->r_xbot, r->r_ytop, r->r_xtop, r->r_ytop);
		(*grDrawLinePtr)(r->r_xbot, r->r_ybot, r->r_xtop, r->r_ybot);
		(*grDrawLinePtr)(r->r_xbot, r->r_ybot, r->r_xbot, r->r_ytop);
		(*grDrawLinePtr)(r->r_xtop, r->r_ybot, r->r_xtop, r->r_ytop);
	    }
	}
    }

    /* do diagonal lines for contacts */
    if (grCurFill == GR_STCROSS)
    {
	if (needClip || needObscure)
	{
	    GrClipLine(r->r_xbot, r->r_ybot, r->r_xtop, r->r_ytop);
	    GrClipLine(r->r_xbot, r->r_ytop, r->r_xtop, r->r_ybot);
	}
	else
	{
	    (*grDrawLinePtr)(r->r_xbot, r->r_ybot, r->r_xtop, r->r_ytop);
	    (*grDrawLinePtr)(r->r_xbot, r->r_ytop, r->r_xtop, r->r_ybot);
	}
    }
}


/*---------------------------------------------------------
 * GrClipBox:
 *	GrClipBox will draw a rectangle on the screen in one
 *	of several possible styles, except that the rectangle will
 *	be clipped against a list of obscurring rectangles.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The rectangle is drawn in the style specified.
 *	The rectangle is clipped before being drawn.
 *---------------------------------------------------------
 */

void
GrClipBox(prect, style)
    Rect *prect;		/* The rectangle to be drawn, given in
				 * screen coordinates.
			         */
    int style;			/* The style to be used in drawing it. */
{
    GrSetStuff(style);
    GrFastBox(prect);
}


/*
 * ----------------------------------------------------------------------------
 *	GrDisjoint --
 *
 * 	Clip a rectanglular area against a clipping box, applying the
 *	supplied procedure to each rectangular region in "area" which
 *	falls outside "clipbox".  This works in pixel space, where a
 *	rectangle is contains its lower x- and y-coordinates AND ALSO
 *	its upper coordinates.  This means that if the clipping box
 *	occupies a given pixel, the things being clipped must not occupy
 *	that pixel.  This procedure will NOT work in tile space.
 *
 *	The procedure should be of the form:
 *		bool func(box, cdarg)
 *			Rect	   * box;
 *			ClientData   cdarg;
 *
 * Results:
 *	Return TRUE unless the supplied function returns FALSE.
 *
 * Side effects:
 *	The side effects of the invoked procedure.
 * ----------------------------------------------------------------------------
 */
bool
GrDisjoint(area, clipBox, func, cdarg)
    Rect	* area;
    Rect	* clipBox;
    bool 	(*func) ();
    ClientData	  cdarg;
{
    Rect 	  ok, rArea;
    bool	  result;

#define NULLBOX(R) ((R.r_xbot>R.r_xtop)||(R.r_ybot>R.r_ytop))

    ASSERT((area!=(Rect *) NULL), "GrDisjoint");
    if((clipBox==(Rect *) NULL)||(!GEO_TOUCH(area, clipBox)))
    {
    /* Since there is no overlap, all of "area" may be processed. */

	result= (*func)(area, cdarg);
	return(result);
    }

    /* Do the disjoint operation in four steps, one for each side
     * of clipBox.  In each step, divide the area being clipped
     * into one piece that is DEFINITELY outside clipBox, and one
     * piece left to check some more.
     */
    
    /* Top edge of clipBox: */

    rArea = *area;
    result = TRUE;
    if (clipBox->r_ytop < rArea.r_ytop)
    {
	ok = rArea;
	ok.r_ybot = clipBox->r_ytop + 1;
	rArea.r_ytop = clipBox->r_ytop;
	if (!(*func)(&ok, cdarg)) result = FALSE;
    }

    /* Bottom edge of clipBox: */

    if (clipBox->r_ybot > rArea.r_ybot)
    {
	ok = rArea;
	ok.r_ytop = clipBox->r_ybot - 1;
	rArea.r_ybot = clipBox->r_ybot;
	if (!(*func)(&ok, cdarg)) result = FALSE;
    }

    /* Right edge of clipBox: */

    if (clipBox->r_xtop < rArea.r_xtop)
    {
	ok = rArea;
	ok.r_xbot = clipBox->r_xtop + 1;
	rArea.r_xtop = clipBox->r_xtop;
	if (!(*func)(&ok, cdarg)) result = FALSE;
    }

    /* Left edge of clipBox: */

    if (clipBox->r_xbot > rArea.r_xbot)
    {
	ok = rArea;
	ok.r_xtop = clipBox->r_xbot - 1;
	rArea.r_xbot = clipBox->r_xbot;
	if (!(*func)(&ok, cdarg)) result = FALSE;
    }

    /* Just throw away what's left of the area being clipped, since
     * it overlaps the clipBox.
     */

    return result;
} /*GrDisjoint*/
