/* CIFmain.c -
 *
 *	This file contains global information for the CIF module,
 *	such as performance statistics.
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
static char rcsid[] = "$Header: CIFmain.c,v 6.0 90/08/28 18:05:06 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "CIFint.h"
#include "textio.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"

/* The following points to a list of all the CIF output styles
 * currently understood:
 */

CIFStyle *CIFStyleList;

/* The current style being used for CIF output: */

CIFStyle *CIFCurStyle = NULL;

/* The following are statistics gathered at various points in
 * CIF processing.  There are two versions of each statistic:
 * a total number, and the number since stats were last printed.
 */

int CIFTileOps = 0;		/* Total tiles touched in geometrical
				 * operations.
				 */
int CIFHierTileOps = 0;		/* Tiles touched in geometrical operations
				 * as part of hierarchical processing.
				 */
int CIFRects = 0;		/* Total CIF rectangles output. */
int CIFHierRects = 0;		/* Rectangles stemming from interactions. */

static int cifTotalTileOps = 0;
static int cifTotalHierTileOps = 0;
static int cifTotalRects = 0;
static int cifTotalHierRects = 0;

/* This file provides several procedures for dealing with errors during
 * the CIF generation process.  Low-level CIF artwork-manipulation
 * procedures call CIFError without knowing what cell CIF is being
 * generated for, or what layer is being generated.  Higher-level
 * routines are responsible for recording that information in the
 * variables below so that CIFError can output meaningful diagnostics
 * using the feedback mechanism.
 */

global CellDef *CIFErrorDef;	/* Definition in which to record errors. */
global int CIFErrorLayer;	/* Index of CIF layer associated with errors.*/


/*
 * ----------------------------------------------------------------------------
 *
 * CIFPrintStats --
 *
 * 	This procedure prints out CIF statistics including both
 *	total values and counts since the last printing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Several messages are printed.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFPrintStats()
{
    TxPrintf("CIF statistics (recent/total):\n");
    cifTotalTileOps += CIFTileOps;
    TxPrintf("    Geometrical tile operations: %d/%d\n",
	CIFTileOps, cifTotalTileOps);
    CIFTileOps = 0;
    cifTotalHierTileOps += CIFHierTileOps;
    TxPrintf("    Tile operations for hierarchy: %d/%d\n",
	CIFHierTileOps, cifTotalHierTileOps);
    CIFHierTileOps = 0;
    cifTotalRects += CIFRects;
    TxPrintf("    CIF rectangles output: %d/%d\n",
	CIFRects, cifTotalRects);
    CIFRects = 0;
    cifTotalHierRects += CIFHierRects;
    TxPrintf("    CIF rectangles due to hierarchical interactions: %d/%d\n",
	CIFHierRects, cifTotalHierRects);
    CIFHierRects = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSetStyle --
 *
 * 	This procedure changes the current CIF output style to the one
 *	named by the parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current CIF style is changed.  If the name doesn't match,
 *	or is ambiguous, then a list of all CIF styles is output.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSetStyle(name)
    char *name;			/* Name of the new style.  If NULL, just
				 * print out the valid styles.
				 */
{
    CIFStyle *style, *match;
    int length;

    if (name == NULL) goto badStyle;
    length = strlen(name);
    match = NULL;
    for (style = CIFStyleList; style != NULL; style = style->cs_next)
    {
	if (strncmp(name, style->cs_name, length) == 0)
	{
	    if (match != NULL)
	    {
		TxError("CIF output style \"%s\" is ambiguous.\n", name);
		goto badStyle;
	    }
	    match = style;
	}
    }

    if (match != NULL)
    {
	CIFCurStyle = match;
	return;
    }

    TxError("\"%s\" is not one of the CIF output styles Magic knows.\n", name);
    badStyle:
    TxPrintf("The CIF output styles are: ");
    for (style = CIFStyleList; style != NULL; style = style->cs_next)
    {
	if (style == CIFStyleList)
	    TxPrintf("%s", style->cs_name);
	else TxPrintf(", %s", style->cs_name);
    }
    TxPrintf(".\n");
    if (CIFCurStyle != NULL)
	TxPrintf("The current style is \"%s\".\n", CIFCurStyle->cs_name);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFNameToMask --
 *
 * 	Finds the CIF planes for a given name.
 *
 * Results:
 *	TRUE if successful, FALSE if "name" failed to match any layers.
 *
 * Side effects:
 *	If there's no match, then an error message is output.
 *	The sets 'result' to be all types containing the CIF layer named
 *	"name".  The current CIF style is used for the lookup.
 * ----------------------------------------------------------------------------
 */

bool
CIFNameToMask(name, result)
    char *name;
    TileTypeBitMask *result;
{
    int i;

    TTMaskZero(result);
    for (i = 0;  i < CIFCurStyle->cs_nLayers; i+=1)
    {
	if (strcmp(name, CIFCurStyle->cs_layers[i]->cl_name) == 0)
	{
	    TTMaskSetType(result, i);
	}
    }

    if (!TTMaskEqual(result, &DBZeroTypeBits)) return (TRUE);

    TxError("CIF name \"%s\" doesn't exist in style \"%s\".\n", name,
	CIFCurStyle->cs_name);
    TxError("The valid CIF layer names are: ");
    for (i = 0; i < CIFCurStyle->cs_nLayers; i++)
    {
	if (i == 0)
	    TxError("%s", CIFCurStyle->cs_layers[i]->cl_name);
	else
	    TxError(", %s", CIFCurStyle->cs_layers[i]->cl_name);
    }
    TxError(".\n");
    return (FALSE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFError --
 *
 * 	This procedure is called by low-level CIF generation routines
 *	when a problem is encountered in generating CIF.  This procedure
 *	notes the problem using the feedback mechanism.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Feedback information is added.  The caller must have set CIFErrorDef
 *	to point to the cell definition that area refers to.  If CIFErrorDef
 *	is NULL, then errors are ignored.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFError(area, message)
    Rect *area;			/* Place in CIFErrorDef where there was a
				 * problem in generating CIFErrorLayer.
				 */
    char *message;		/* Short note about what went wrong. */
{
    char msg[200];

    if (CIFErrorDef == (NULL)) return;
    (void) sprintf(msg, "CIF error in cell %s, layer %s: %s",
	CIFErrorDef->cd_name, CIFCurStyle->cs_layers[CIFErrorLayer]->cl_name,
	message);
    DBWFeedbackAdd(area, msg, CIFErrorDef, CIFCurStyle->cs_scaleFactor,
	STYLE_PALEHIGHLIGHTS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFOutputScaleFactor --
 *
 * 	Returns current conversion factor between CIF units and
 *	Magic units.
 *
 * Results:
 *	The return value is the number of centimicrons per Magic
 *	unit in the current CIF output style.  If there is no
 *	known CIF output style, 1 is returned.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
CIFOutputScaleFactor()
{
    if (CIFCurStyle == NULL) return 1;
    return CIFCurStyle->cs_scaleFactor;
}
