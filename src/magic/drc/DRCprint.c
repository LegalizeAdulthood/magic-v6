/*
 * DRCPrint.c --
 *
 * Edge-based design rule checker
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

#ifndef	lint
static char rcsid[] = "$Header: DRCprint.c,v 6.0 90/08/28 18:12:32 mayo Exp $";
#endif	not lint

#include <stdio.h>
#include <sys/types.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"

extern char *maskToPrint();
extern char *DBTypeShortName();

/*
 * ----------------------------------------------------------------------------
 *
 * drcGetName --
 *
 * 	This is a utility procedure that returns a convenient name for
 *	a mask layer.
 *
 * Results:
 *	The result is the first 8 characters of the long name for
 *	the layer.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

char *
drcGetName(layer, string)
    int layer;
    char *string;		/* Used to hold name.  Must have length >= 8 */
{
    extern char *strncpy();
    (void) strncpy(string, DBTypeShortName(layer), 8);
    string[8] = '\0';
    if (layer == TT_SPACE) return "space";
    return string;
}


/*
 * ----------------------------------------------------------------------------
 * DRCPrintRulesTable --
 *
 *	Write compiled DRC rules table and adjacency matrix to the given file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

void
DRCPrintRulesTable (fp)
    FILE * fp;
{
    int		  i, j, k;
    DRCCookie	* dp;
    char buf1[20], buf2[20];
    int gotAny;
					/* print the rules table */
    for (i = 0; i < DBNumTypes; i++)
    {
	gotAny = FALSE;
	for (j = 0; j < DBNumTypes; j++)
	{
	    if (DRCRulesTbl [i][j] != (DRCCookie *) NULL)
	    {
		k = 1;
	        for (dp = DRCRulesTbl [i][j]; dp != (DRCCookie *) NULL;
			      dp = dp->drcc_next)
		{
		    gotAny = TRUE;
		    if (k == 1)
		    {
			(void) fprintf(fp,"%-8s %-8s  ",drcGetName(i, buf1),
			    drcGetName(j, buf2));
			k++;
		    }
		    else (void) fprintf(fp,"                   ");

		    (void) fprintf(fp,"%d x %d   %s\n",
			dp->drcc_dist, dp->drcc_cdist,
			maskToPrint(&dp->drcc_mask));
		    (void) fprintf(fp,"                           %s",
			maskToPrint(&dp->drcc_corner));
		    if (dp->drcc_flags &
			    (DRC_REVERSE|DRC_BOTHCORNERS|DRC_XPLANE))
			(void) fprintf(fp, "\n                          ");
		    if (dp->drcc_flags & DRC_REVERSE)
			(void) fprintf(fp," reverse");
		    if (dp->drcc_flags & DRC_BOTHCORNERS)
			(void) fprintf(fp," both-corners");
		    if (dp->drcc_flags & DRC_XPLANE)
			(void) fprintf(fp," cross-plane(%s)",
			    DBPlaneLongName(dp->drcc_plane));
		    (void) fprintf(fp,"\n");
	        }
	    }
	}
	if (gotAny) (void) fprintf(fp,"\n");
    }

    /* Print out overlaps that are illegal between subcells. */

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    if ((i == TT_ERROR_S) || (j == TT_ERROR_S)) continue;
	    if (DRCPaintTable[0][i][j] == TT_ERROR_S)
		(void) fprintf(fp, "Tile type %s can't overlap type %s.\n",
		    drcGetName(i, buf1), drcGetName(j, buf2));
	}
    }

    /* Print out tile types that must have exact overlaps. */

    if (!TTMaskIsZero(&DRCExactOverlapTypes))
    {
	(void) fprintf(fp, "Types that must overlap exactly: %s\n",
	    maskToPrint(&DRCExactOverlapTypes));
    }
}

char *
maskToPrint (mask)
    TileTypeBitMask *mask;
{
    int	i;
    int gotSome = FALSE;
    static char printchain[400];
    char buffer[20];

    if (TTMaskIsZero(mask))
	return "<none>";

    printchain[0] = '\0';

    for (i = 0; i < DBNumTypes; i++)
	if (TTMaskHasType(mask, i))
	{
	    if (gotSome) strcat(printchain, ",");
	    else gotSome = TRUE;
	    strcat(printchain, drcGetName(i, buffer));
	}

    return (printchain);
}
