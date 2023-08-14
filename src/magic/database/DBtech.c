/*
 * DBtech.c --
 *
 * Technology initialization for the database module.
 * This file handles overall initialization, construction
 * of the general-purpose exported TileTypeBitMasks, and
 * the "connect" section.
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
static char rcsid[] = "$Header: DBtech.c,v 6.0 90/08/28 18:10:14 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "utils.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "tech.h"
#include "textio.h"
#include "malloc.h"

    /* Name of this technology */
char *DBTechName;

    /* Connectivity */
TileTypeBitMask	 DBConnectTbl[NT];
TileTypeBitMask	 DBNotConnectTbl[NT];
unsigned int	 DBConnPlanes[NT];
unsigned int	 DBAllConnPlanes[NT];

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechInit --
 *
 * Clear technology description information for database module.
 * CURRENTLY A NO-OP.  EVENTUALLY WILL CLEAR OUT ALL TECHNOLOGY
 * VARIABLES PRIOR TO REINITIALIZING A NEW TECHNOLOGY.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechInit()
{
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechSetTech --
 *
 * Set the name for the technology.
 *
 * Results:
 *	Returns FALSE if there were an improper number of
 *	tokens on the line.
 *
 * Side effects:
 *	Sets DBTechName to the name of the technology.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBTechSetTech(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    if (argc != 1)
    {
	TechError("Badly formed technology name\n");
	return FALSE;
    }
    (void) StrDup(&DBTechName, argv[0]);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechInitConnect --
 *
 * Initialize the connectivity tables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes DBConnectTbl[], DBConnPlanes[], and DBAllConnPlanes[].
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechInitConnect()
{
    register int i;

    for (i = 0; i < TT_MAXTYPES; i++)
    {
	TTMaskSetOnlyType(&DBConnectTbl[i], i);
	DBConnPlanes[i] = 0;
	DBAllConnPlanes[i] = 0;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechAddConnect --
 *
 * Add connectivity information.
 * Record the fact that material of the types in the comma-separated
 * list types1 connects to material of the types in the list types2.
 *
 * Results:
 *	TRUE if successful, FALSE on error
 *
 * Side effects:
 *	Updates DBConnectTbl[].
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBTechAddConnect(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    TileTypeBitMask types1, types2;
    register TileType t1, t2;

    if (argc != 2)
    {
	TechError("Line must contain exactly 2 lists of types\n");
	return FALSE;
    }

    DBTechNoisyNameMask(argv[0], &types1);
    DBTechNoisyNameMask(argv[1], &types2);
    for (t1 = 0; t1 < DBNumTypes; t1++)
	if (TTMaskHasType(&types1, t1))
	    for (t2 = 0; t2 < DBNumTypes; t2++)
		if (TTMaskHasType(&types2, t2))
		{
		    TTMaskSetType(&DBConnectTbl[t1], t2);
		    TTMaskSetType(&DBConnectTbl[t2], t1);
		}


    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechFinalConnect --
 *
 * Postprocessing for the connectivity information.
 * Modify DBConnectTbl[] so that:
 *
 *	(1) Any type connecting to one of the images of a contact
 *	    connects to all images of the contact.
 *	(2) Each image of a contact connects to the union of what
 *	    all the images connect to.
 *
 * Modify DBConnPlanes[] so that only types belonging to a contact
 * appear to connect to any plane other than their own.
 *
 * Constructs DBAllConnPlanes, which will be non-zero for those planes
 * to which each type connects, exclusive of that type's home plane and
 * those planes to which it connects as a contact.
 *
 * Create DBNotConnectTbl[], the complement of DBConnectTbl[].
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies DBConnPlanes[], DBAllConnPlanes[], and DBConnectTbl[]
 *	as above.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechFinalConnect()
{
    TileTypeBitMask saveConnect[TT_MAXTYPES], tmp, tmp2;
    register TileType base, s;
    register LayerInfo *lp;
    int n;

    for (s = 0; s < DBNumTypes; s++)
	DBConnPlanes[s] = 0;

    /*
     * Each contact type should connect to everything that each of
     * its connected images connects to, but only stuff that's on
     * the same plane as each connected image or the original contact
     * type (i.e, no transitive connections to contacts on other planes).
     * To ensure that we don't generate the transitive closure of the
     * connectivity table (we only want adjacency connectedness), we
     * make a copy of the original connectivity table and use it as
     * the source of image connectedness, instead of using DBConnectTbl[]
     * as well as updating it.
     */
    bcopy((char *) DBConnectTbl, (char *) saveConnect, sizeof saveConnect);
    for (n = 0; n < dbNumImages; n++)
    {
	lp = dbContactInfo[n];
	TTMaskSetMask(&DBConnectTbl[lp->l_type], &lp->l_tmask);
	for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    if (TTMaskHasType(&lp->l_tmask, s))
	    {
		tmp = DBPlaneTypes[DBPlane(lp->l_type)];
		TTMaskSetMask(&tmp, &DBPlaneTypes[DBPlane(s)]);
		TTMaskAndMask3(&tmp2, &saveConnect[s], &tmp);
		TTMaskSetMask(&DBConnectTbl[lp->l_type], &tmp2);
	    }

	/*
	 * DBConnPlanes[] is nonzero only for contact images, for which
	 * it shows the planes connected to an image, exclusive of the
	 * image's plane itself.
	 */
	DBConnPlanes[lp->l_type] = lp->l_pmask;
	DBConnPlanes[lp->l_type] &= ~(PlaneNumToMaskBit(DBPlane(lp->l_type)));
    }

    /* Make the connectivity matrix symmetric */
    for (base = TT_TECHDEPBASE; base < DBNumTypes; base++)
	for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    if (TTMaskHasType(&DBConnectTbl[base], s))
		TTMaskSetType(&DBConnectTbl[s], base);

    /* Construct DBNotConnectTbl[] to be the complement of DBConnectTbl[] */
    for (base = 0; base < TT_MAXTYPES; base++)
	TTMaskCom2(&DBNotConnectTbl[base], &DBConnectTbl[base]);

    /*
     * Now finally construct DBAllConnPlanes, which will be non-zero
     * for those planes to which each type 'base' connects, exclusive
     * of its home plane and those planes to which it connects as a
     * contact.
     */
    for (base = TT_TECHDEPBASE; base < DBNumTypes; base++)
    {
	DBAllConnPlanes[base] = DBTechTypesToPlanes(&DBConnectTbl[base]);
	DBAllConnPlanes[base] &= ~(PlaneNumToMaskBit(DBPlane(base)));
	DBAllConnPlanes[base] &= ~DBConnPlanes[base];
    }
}
