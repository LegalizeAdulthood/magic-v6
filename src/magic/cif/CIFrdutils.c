/* CIFreadutils.c -
 *
 *	This file contains routines that parse a file in CIF
 *	format.  This file contains the top-level routine for
 *	reading CIF files, plus a bunch of utility routines
 *	for skipping white space, parsing numbers and points, etc.
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
static char rcsid[] = "$Header: CIFrdutils.c,v 6.0 90/08/28 18:05:18 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "CIFint.h"
#include "CIFread.h"
#include "textio.h"
#include "signals.h"
#include "undo.h"
#include "malloc.h"

/* The following variables are used to provide one character of
 * lookahead.  cifParseLaAvail is TRUE if cifParseLaChar contains
 * a valid character, FALSE otherwise.  The PEEK and TAKE macros
 * are used to manipulate this stuff.
 */

bool cifParseLaAvail = FALSE;
int cifParseLaChar = EOF;

/* Below is a variable pointing to the CIF input file.  It's used
 * by the PEEK and TAKE macros.  The other stuff is used to keep
 * track of our location in the CIF file for error reporting
 * purposes.
 */

FILE *cifInputFile;
int cifLineNumber;		/* Number of current line. */

/* The variables used below hold general information about what
 * we're currently working on.
 */

int cifReadScale1;			/* Scale factor:  multiply by Scale1 */
int cifReadScale2;			/* then divide by Scale2. */
Plane *cifReadPlane;			/* Plane into which to paint material
					 * NULL means no layer command has
					 * been seen for the current cell.
					 */

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadError --
 *
 * 	This procedure is called to print out error messages during
 *	CIF file reading.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is printed.
 *
 * Note:
 *	You can add more arguments if three turns out not to be enough.
 *
 * ----------------------------------------------------------------------------
 */

    /* VARARGS1 */
void
CIFReadError(format, arg1, arg2, arg3)
    char *format;		/* Text format string. */
    char *arg1, *arg2, *arg3;	/* Additional arguments, if needed. */
{
    TxError("Error at line %d of CIF file: ", cifLineNumber);
    TxError(format, arg1, arg2, arg3);
}

/*
 * ----------------------------------------------------------------------------
 *
 *	CIFScaleCoord
 *
 * 	This procedure does rounding and division to convert from
 *	CIF units back into Magic units.
 *
 * Results:
 *	The result is the Magic unit equivalent to cifCoord.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
CIFScaleCoord(cifCoord)
    int cifCoord;			/* A coordinate in CIF units. */
{
    int result, scale;

    scale = cifCurReadStyle->crs_scaleFactor;

    /* Careful:  must round down a bit more for negative numbers, in
     * order to ensure that a point exactly halfway between Magic units
     * always gets rounded down, rather than towards zero (this would
     * result in different treatment of the same paint, depending on
     * where it is in the coordinate system.
     */

    if (cifCoord < 0)
	result = cifCoord - ((scale)>>1);
    else result = cifCoord + ((scale-1)>>1);
    return result/scale;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifIsBlank --
 *
 * 	Figures out whether a character qualifies as a blank in CIF.
 *	A blank is anything except a digit, an upper-case character,
 *	or the symbols "-", "(", "(", and ";".
 *
 * Results:
 *	Returns TRUE if ch is a CIF blank, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifIsBlank(ch)
    int	ch;
{

    if (  isdigit(ch) || isupper(ch)
	|| (ch == '-') || (ch == ';')
	|| (ch == '(') || (ch == ')')
	|| (ch == EOF))
    {
	return FALSE;
    }
    else return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSkipBlanks --
 *
 * 	This procedure skips over whitespace in the CIF file,
 *	keeping track of the line number and other information
 *	for error reporting.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Advances through the CIF file.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSkipBlanks()
{
    
    while (cifIsBlank(PEEK())) {
	if (TAKE() == '\n')
	{
	    cifLineNumber++;
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSkipSep --
 *
 * 	Skip over separators in the CIF file.  Blanks and upper-case
 *	characters are separators.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Advances through the CIF file.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSkipSep()
{
    int	ch;

    for (ch = PEEK() ; isupper(ch) || cifIsBlank(ch) ; ch = PEEK()) {
	if (TAKE() == '\n')
	{
	    cifLineNumber++;
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSkipToSemi --
 *
 * 	This procedure is called after errors.  It skips everything
 *	in the CIF file up to the next semi-colon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Advances through the CIF file.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSkipToSemi()
{
    int	ch;
    
    for (ch = PEEK() ; ((ch != ';') && (ch != EOF)) ; ch = PEEK()) {
	if (TAKE() == '\n')
	{
	    cifLineNumber++;
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSkipSemi --
 *
 * 	Skips a semi-colon, including blanks around the semi-colon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Advances through the CIF file.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSkipSemi()
{
    
    CIFSkipBlanks();
    if (PEEK() != ';') {
	CIFReadError("`;\' expected.\n");
	return;
    }
    TAKE();
    CIFSkipBlanks();
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseSInteger --
 *
 * 	This procedure parses a signed integer from the CIF file.
 *
 * Results:
 *	TRUE is returned if the parse completed without error,
 *	FALSE otherwise.
 *
 * Side effects:
 *	The integer pointed to by valuep is modified with the
 *	value of the signed integer.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseSInteger(valuep)
    int		*valuep;
{
    bool	is_signed;
    char	buffer[ BUFSIZ ];
    char	*bufferp;

    *valuep = 0;
    CIFSkipSep();
    if (PEEK() == '-')
    {
	TAKE();
	is_signed = TRUE;
    }
    else is_signed = FALSE;
    bufferp = &buffer[0];
    while (isdigit(PEEK()))
	*bufferp++ = TAKE();
    if (bufferp == &buffer[0])
	return FALSE;
    *bufferp = '\0';
    *valuep = atoi(&buffer[0]);
    if (is_signed)
	*valuep = -(*valuep);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseInteger --
 *
 * 	Parses a positive integer from the CIF file.
 *
 * Results:
 *	TRUE is returned if the parse was completed successfully,
 *	FALSE otherwise.
 *
 * Side effects:
 *	The value pointed to by valuep is modified to hold the integer.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseInteger(valuep)
    int *valuep;
{

    if (!CIFParseSInteger(valuep))
	return FALSE;
    if (*valuep < 0)
	CIFReadError("negative integer not permitted.\n");
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParsePoint --
 *
 * 	Parse a point from a CIF file.  A point is two integers
 *	separated by CIF separators.
 *
 * Results:
 *	TRUE is returned if the point was parsed correctly, otherwise
 *	FALSE is returned.
 *
 * Side effects:
 *	The parameter pointp is filled in with the coordinates of
 *	the point.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParsePoint(pointp)
    Point *pointp;
{

    pointp->p_x = 0;
    pointp->p_y = 0;
    if (!CIFParseSInteger(&pointp->p_x))
	return FALSE;
    pointp->p_x = (pointp->p_x*cifReadScale1)/cifReadScale2;
    if (!CIFParseSInteger(&pointp->p_y))
	return FALSE;
    pointp->p_y = (pointp->p_y*cifReadScale1)/cifReadScale2;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParsePath --
 *
 * 	This procedure parses a CIF path, which is sequence of
 *	one or more points.
 *
 *	If the path is non-Manhattan, we introduce additional
 *	stair-steps a minimum of cifCurReadStyle->crs_scaleFactor
 *	high and wide along each non-Manhattan segment.
 *
 * Results:
 *	TRUE is returned if the path was parsed successfully,
 *	FALSE otherwise.
 *
 * Side effects:
 *	Modifies the parameter pathheadpp to point to the path
 *	that is constructed.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParsePath(pathheadpp)
    CIFPath **pathheadpp;
{
    register CIFPath *pathtailp, *newpathp;
    bool nonManhattan = FALSE;
    CIFPath path;

    *pathheadpp = NULL;
    pathtailp = NULL;
    path.cifp_next = NULL;
    while (TRUE)
    {
	CIFSkipSep();
	if (PEEK() == ';')
	    break;

	if (!CIFParsePoint(&path.cifp_point))
	{
	    CIFFreePath(*pathheadpp);
	    return FALSE;
	}
	MALLOC(CIFPath *, newpathp, sizeof (CIFPath));
	*newpathp = path;
	if (*pathheadpp)
	{
	    /*
	     * Check that this segment is Manhattan.  If not, remember the
	     * fact and later introduce extra stair-steps to make the path
	     * Manhattan.  We don't do the stair-step introduction here for
	     * two reasons: first, the same code is also used by the Calma
	     * module, and second, it is important to know which side of
	     * the polygon is the outside when generating the stair steps.
	     */
	    if (pathtailp->cifp_x != newpathp->cifp_x
		    && pathtailp->cifp_y != (newpathp->cifp_y))
	    {
		if (!nonManhattan)
		    CIFReadError("non-Manhattan path; using stairstep.\n");
		nonManhattan = TRUE;
	    }
	    pathtailp->cifp_next = newpathp;
	}
	else *pathheadpp = newpathp;
	pathtailp = newpathp;
    }

    if (nonManhattan)
	CIFMakeManhattanPath(*pathheadpp);

    return (*pathheadpp != NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFMakeManhattanPath --
 *
 *	Convert a non-Manhattan path into a Manhattan one by adding
 *	additional points.  These points are added using a simple
 *	scan-conversion algorithm that generates a series of stair
 *	steps that are at least cifCurReadStyle->crs_scaleFactor
 *	units high and wide (but which may be higher or wider).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May insert additional points in the path.
 *
 * ----------------------------------------------------------------------------
 */

CIFMakeManhattanPath(pathHead)
    CIFPath *pathHead;
{
    register CIFPath *new, *next, *path;
    int xinit, xdiff, xincr, xlast, x;
    int yinit, ydiff, yincr, ylast, y;

    for (path = pathHead; path->cifp_next; path = path->cifp_next)
    {
	next = path->cifp_next;

	/* No work if this segment is Manhattan */
	if (path->cifp_x == next->cifp_x || path->cifp_y == next->cifp_y)
	    continue;

	/*
	 * The major loop will be over whichever difference (x or y)
	 * is the SMALLER of the two; for each iteration over the smaller
	 * dimension, we will add the number of units of the larger
	 * dimension per unit of the smaller dimension to the larger
	 * dimension.
	 */
	xdiff = next->cifp_x - path->cifp_x;
	ydiff = next->cifp_y - path->cifp_y;
	xinit = path->cifp_x;
	yinit = path->cifp_y;
	if (ABS(xdiff) > ABS(ydiff))
	{
	    /* Iterate over y, stopping before next->cifp_y */
	    yincr = cifCurReadStyle->crs_scaleFactor;
	    ylast = yinit;
	    if (ydiff < 0) yincr = -yincr;
	    for (y = yinit + yincr, x = xinit;
			(yincr > 0 && y < next->cifp_y)
		     || (yincr < 0 && y > next->cifp_y);
		    y += yincr)
	    {
		/* Move by one in y first */
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = x;
		new->cifp_y = y;
		path->cifp_next = new;
		path = new;

		/*
		 * Now move in x.
		 * Note that ((y - yinit) / ydiff) >= 0 always.
		 * Also, as long as y has not reached next->cifp_y,
		 * this quantity will be < 1, so x will range from
		 * path->cifp_x up to but not reaching next->cifp_x.
		 */
		x = xinit + (xdiff * (y - yinit)) / ydiff;
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = x;
		new->cifp_y = y;
		new->cifp_next = next;
		path->cifp_next = new;
		path = new;
		ylast = y;
	    }

	    /*
	     * The last y processed was short of next->cifp_y.
	     * If x was not yet at next->cifp_x, add one more point
	     * to bridge the gap.
	     */
	    if (x != next->cifp_x)
	    {
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = next->cifp_x;
		new->cifp_y = ylast;;
		new->cifp_next = next;
		path->cifp_next = new;
		path = new;
	    }
	}
	else
	{
	    /* Iterate over x, stopping before next->cifp_x */
	    xincr = cifCurReadStyle->crs_scaleFactor;
	    xlast = xinit;
	    if (xdiff < 0) xincr = -xincr;
	    for (x = xinit + xincr, y = yinit;
			(xincr > 0 && x < next->cifp_x)
		     || (xincr < 0 && x > next->cifp_x);
		x += xincr)
	    {
		/* Move by one in x first */
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = x;
		new->cifp_y = y;
		path->cifp_next = new;
		path = new;

		/*
		 * Now move in y.
		 * Note that ((x - xinit) / xdiff) >= 0 always.
		 * Also, as long as x has not reached next->cifp_x,
		 * this quantity will be < 1, so y will range from
		 * path->cifp_y up to but not reaching next->cifp_y.
		 */
		y = yinit + (ydiff * (x - xinit)) / xdiff;
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = x;
		new->cifp_y = y;
		new->cifp_next = next;
		path->cifp_next = new;
		path = new;
		xlast = x;
	    }

	    /*
	     * The last x processed was short of next->cifp_x.
	     * If y was not yet at next->cifp_y, add one more point
	     * to bridge the gap.
	     */
	    if (y != next->cifp_y)
	    {
		MALLOC(CIFPath *, new, sizeof (CIFPath));
		new->cifp_x = xlast;
		new->cifp_y = next->cifp_y;
		new->cifp_next = next;
		path->cifp_next = new;
		path = new;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFFreePath --
 *
 * 	This procedure frees up a path once it has been used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All the elements of path are returned to the storage allocator.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFFreePath(path)
    CIFPath *path;		/* Path to be freed. */
{
    while (path != NULL)
    {
	freeMagic((char *) path);
	path = path->cifp_next;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifCommandError --
 *
 * 	This procedure is called when unknown CIF commands are found
 *	in CIF files.  It skips the command and advances to the next
 *	command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

cifCommandError()
{
    CIFReadError("unknown command `%c'; ignored.\n" , PEEK());
    CIFSkipToSemi();
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseEnd --
 *
 * 	This procedure processes the "end" statement in a CIF file
 *	(it ignores it).
 *
 * Results:
 *	Always returns TRUE.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseEnd()
{
    TAKE();
    CIFSkipBlanks();
    if (PEEK() != EOF)
    {
	CIFReadError("End command isn't at end of file.\n");
	return FALSE;
    }
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseComment --
 *
 * 	This command skips over user comments in CIF files.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseComment()
{
    int		opens;
    int		ch;
    
	/*
	 *	take the '('
	 */
    TAKE();
    opens = 1;
    do
    {
	ch = TAKE();
	if (ch == '(')
	    opens++;
	else if (ch == ')')
	    opens--;
	else if (ch == '\n')
	{
	    cifLineNumber++;
	}
	else if (ch == EOF)
	{
	    CIFReadError("(comment) extends to end of file.\n");
	    return FALSE;
	}
    } while (opens > 0);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFDirectionToTrans --
 *
 * 	This procedure is used to convert from a direction vector
 *	to a Magic transformation.  The direction vector is a point
 *	giving a direction from the origin.  It better be along
 *	one of the axes.
 *
 * Results:
 *	The return value is the transformation corresponding to
 *	the direction, or the identity transform if the direction
 *	isn't along one of the axes.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

Transform *
CIFDirectionToTrans(point)
    Point *point;		/* Direction vector from origin. */
{
    if ((point->p_x != 0) && (point->p_y == 0))
    {
	if (point->p_x > 0)
	    return &GeoIdentityTransform;
	else return &Geo180Transform;
    }
    else if ((point->p_y != 0) && (point->p_x == 0))
    {
	if (point->p_y > 0)
	    return &Geo270Transform;
	else return &Geo90Transform;
    }
    CIFReadError("non-manhattan direction vector (%d, %d); ignored.\n",
	point->p_x, point->p_y);
    return &GeoIdentityTransform;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseTransform --
 *
 * 	This procedure is called to read in a transform from a
 *	CIF file.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	The parameter pointed to by transformp is modified to
 *	contain the transform indicated by the CIF file.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseTransform(transformp)
    Transform	*transformp;
{
    char	ch;
    Point	point;
    Transform	tmp;

    *transformp = GeoIdentityTransform;
    CIFSkipBlanks();
    for (ch = PEEK() ; ch != ';' ; ch = PEEK())
    {
	switch (ch)
	{
	    case 'T':
		    TAKE();
		    if (!CIFParsePoint(&point))
		    {
			CIFReadError("translation, but no point.\n");
			CIFSkipToSemi();
			return FALSE;
		    }
		    GeoTranslateTrans(transformp, point.p_x, point.p_y, &tmp);
		    *transformp = tmp;
		    break;
	    case 'M':
		    TAKE();
		    CIFSkipBlanks();
		    ch = PEEK();
		    if (ch == 'X')
		        GeoTransTrans(transformp, &GeoSidewaysTransform, &tmp);
		    else if (ch == 'Y')
		        GeoTransTrans(transformp, &GeoUpsideDownTransform,
				&tmp);
		    else
		    {
			CIFReadError("mirror, but not in X or Y.\n");
			CIFSkipToSemi();
			return FALSE;
		    }
		    TAKE();
		    *transformp = tmp;
		    break;
	    case 'R':
		    TAKE();
		    if (!CIFParseSInteger(&point.p_x) ||
			    !CIFParseSInteger(&point.p_y))
		    {
			CIFReadError("rotation, but no direction.\n");
			CIFSkipToSemi();
			return FALSE;
		    }
		    GeoTransTrans(transformp, CIFDirectionToTrans(&point),
			    &tmp);
		    *transformp = tmp;
		    break;
	    default:
		    CIFReadError("transformation expected.\n");
		    CIFSkipToSemi();
		    return FALSE;
	}
	CIFSkipBlanks();
    }

    /* Before returning, we must scale the transform into Magic units. */

    transformp->t_c = CIFScaleCoord(transformp->t_c);
    transformp->t_f = CIFScaleCoord(transformp->t_f);
    
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseCommand --
 *
 * 	Parse one CIF command and farm it out to a routine to handle
 *	that command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May modify the contents of cifReadCellDef by painting or adding
 *	new uses or labels.  May also create new CellDefs.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFReadFile(file)
FILE *file;			/* File from which to read CIF. */
{
    /* We will use 1-word CIF numbers as keys in this hash table */
    CIFReadCellInit(1);

    if (cifCurReadStyle == NULL)
    {
	TxError("Don't know how to read CIF:  nothing in tech file.\n");
	return;
    }
    TxPrintf("Warning: CIF reading is not undoable!  I hope that's OK.\n");
    UndoDisable();

    cifInputFile = file;
    cifReadScale1 = 1;
    cifReadScale2 = 1;
    cifParseLaAvail = FALSE;
    cifLineNumber = 1;
    cifReadPlane = (Plane *) NULL;
    cifCurLabelType = TT_SPACE;
    while (PEEK() != EOF)
    {
	if (SigInterruptPending) goto done;
	CIFSkipBlanks();
	switch (PEEK())
	{
	    case EOF:
		    break;
	    case ';':
		    break;
	    case 'B':
		    (void) CIFParseBox();
		    break;
	    case 'C':
		    (void) CIFParseCall();
		    break;
	    case 'D':
		    TAKE();
		    CIFSkipBlanks();
		    switch (PEEK())
		    {
			case 'D':
				    (void) CIFParseDelete();
				    break;
			case 'F':
				    (void) CIFParseFinish();
				    break;
			case 'S':
				    (void) CIFParseStart();
				    break;
			default:
				    cifCommandError();
				    break;
		    }
		    break;
	    case 'E':
		    (void) cifParseEnd();
		    goto done;
	    case 'L':
		    (void) CIFParseLayer();
		    break;
	    case 'P':
		    (void) CIFParsePoly();
		    break;
	    case 'R':
		    (void) CIFParseFlash();
		    break;
	    case 'W':
		    (void) CIFParseWire();
		    break;
	    case '(':
		    (void) cifParseComment();
		    break;
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		    (void) CIFParseUser();
		    break;
	    default:
		    cifCommandError();
		    break;
	}
	CIFSkipSemi();
    }

    CIFReadError("no \"End\" statement.\n");

    done:
    CIFReadCellCleanup();
    UndoEnable();
}
