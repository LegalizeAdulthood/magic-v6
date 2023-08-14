/*
 * DBtechtype.c --
 *
 * Creation of tile and plane types and their names.
 * Lookup procedures are in DBtechname.c
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
static char rcsid[] = "$Header: DBtechtype.c,v 6.0 90/08/28 18:10:19 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#ifdef SYSV
#include <string.h>
#endif
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

    /* Types and their names */
int DBNumTypes;
char *DBTypeLongNameTbl[NT];
int DBTypePlaneTbl[NT];
NameList dbTypeNameLists;

    /* Planes and their names */
int DBNumPlanes;
char *DBPlaneLongNameTbl[PL_MAXTYPES];
NameList dbPlaneNameLists;

    /*
     * Sets of types.
     * These are generated after the "types" section of the
     * technology file has been read, but before any automatically
     * generated types (contact images) are created.
     */
int DBNumUserLayers;
TileTypeBitMask DBZeroTypeBits;
TileTypeBitMask DBAllTypeBits;
TileTypeBitMask DBBuiltinLayerBits;
TileTypeBitMask DBAllButSpaceBits;
TileTypeBitMask DBAllButSpaceAndDRCBits;
TileTypeBitMask DBSpaceBits;
TileTypeBitMask DBUserLayerBits;
TileTypeBitMask DBNonSpaceUserLayerBits;

/* Table of default, builtin planes */
DefaultPlane dbTechDefaultPlanes[] =
{
    PL_CELL,		"subcell",
    PL_DRC_ERROR,	"designRuleError",
    PL_DRC_CHECK,	"designRuleCheck",
    PL_M_HINT,		"mhint",
    PL_F_HINT,		"fhint",
    PL_R_HINT,		"rhint",
    0,			0,	0
};

/* Table of default, builtin types */
DefaultType dbTechDefaultTypes[] =
{
    TT_SPACE,		-1,		"space",		FALSE,
    TT_CHECKPAINT,	PL_DRC_CHECK,	"checkpaint,CP",	FALSE,
    TT_CHECKSUBCELL,	PL_DRC_CHECK,	"checksubcell,CS",	FALSE,
    TT_ERROR_P,		PL_DRC_ERROR,	"error_p,EP",		FALSE,
    TT_ERROR_S,		PL_DRC_ERROR,	"error_s,ES",		FALSE,
    TT_ERROR_PS,	PL_DRC_ERROR,	"error_ps,EPS",		FALSE,
    TT_MAGNET,		PL_M_HINT,	"magnet,mag",		TRUE,
    TT_FENCE,		PL_F_HINT,	"fence,f",		TRUE,
    TT_ROTATE,		PL_R_HINT,	"rotate,r",		TRUE,
    0,			0,		NULL,			0
};

/* Forward declarations */
ClientData dbTechNameLookup();
char *dbTechNameAdd();
NameList *dbTechNameAddOne();

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechInitPlane --
 *
 * Initialize the default plane information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes DBNumPlanes to PL_TECHDEPBASE.
 *	Initializes DBPlaneLongNameTbl[] for builtin planes.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechInitPlane()
{
    register DefaultPlane *dpp;
    register char *cp;

    /* Tables of short names */
    dbPlaneNameLists.sn_next = &dbPlaneNameLists;
    dbPlaneNameLists.sn_prev = &dbPlaneNameLists;

    for (dpp = dbTechDefaultPlanes; dpp->dp_names; dpp++)
    {
	cp = dbTechNameAdd(dpp->dp_names, (ClientData) dpp->dp_plane,
			&dbPlaneNameLists);
	if (cp == NULL)
	{
	    TxError("DBTechInit: can't add plane names %s\n", dpp->dp_names);
	    niceabort();
	}
	DBPlaneLongNameTbl[dpp->dp_plane] = cp;
    }

    DBNumPlanes = PL_TECHDEPBASE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechInitType --
 *
 * Add the names and planes of the builtin types.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *	Initializes DBNumTypes to TT_TECHDEPBASE.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechInitType()
{
    register DefaultType *dtp;
/*###153 [lint] warning i unused in function DBTechInitType%%%*/
    register TileType i;
    register char *cp;

    /* Tables of short names */
    dbTypeNameLists.sn_next = &dbTypeNameLists;
    dbTypeNameLists.sn_prev = &dbTypeNameLists;

    /*
     * Add the type names to the list of known names, and set
     * the default plane for each type.
     */
    for (dtp = dbTechDefaultTypes; dtp->dt_names; dtp++)
    {
	cp = dbTechNameAdd(dtp->dt_names, (ClientData) dtp->dt_type,
			&dbTypeNameLists);
	if (cp == NULL)
	{
	    TxError("DBTechInit: can't add type names %s\n", dtp->dt_names);
	    niceabort();
	}
	DBTypeLongNameTbl[dtp->dt_type] = cp;
	DBTypePlaneTbl[dtp->dt_type] = dtp->dt_plane;
    }

    DBNumTypes = TT_TECHDEPBASE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechAddPlane --
 *
 * Define a tile plane type for the new technology.
 *
 * Results:
 *	TRUE if successful, FALSE on error
 *
 * Side effects:
 *	Updates the database technology variables.
 *	In particular, updates the number of known tile planes.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBTechAddPlane(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    char *cp;

    if (DBNumPlanes >= PL_MAXTYPES)
    {
	TechError("Too many tile planes (max=%d)\n", PL_MAXTYPES);
	return FALSE;
    }

    if (argc != 1)
    {
	TechError("Line must contain names for plane\n");
	return FALSE;
    }

    cp = dbTechNameAdd(argv[0], (ClientData) DBNumPlanes, &dbPlaneNameLists);
    if (cp == NULL)
	return FALSE;
    DBPlaneLongNameTbl[DBNumPlanes++] = cp;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechAddType --
 *
 * Define a tile type for the new technology.
 *
 * Results:
 *	TRUE if successful, FALSE on error
 *
 * Side effects:
 *	Updates the database technology variables.
 *	In particular, updates the number of known tile types.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBTechAddType(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    char *cp;
    int pNum;

    if (DBNumTypes >= TT_MAXTYPES-TT_RESERVEDTYPES)
    {
	TechError("Too many tile types (max=%d)\n",
		TT_MAXTYPES-TT_RESERVEDTYPES);
	return FALSE;
    }

    if (argc < 2)
    {
	TechError("Line must contain at least 2 fields\n");
	return TRUE;
    }

    cp = dbTechNameAdd(argv[1], (ClientData) DBNumTypes, &dbTypeNameLists);
    if (cp == NULL)
	return FALSE;
    pNum = DBTechNoisyNamePlane(argv[0]);
    if (pNum < 0)
	return FALSE;

    DBTypeLongNameTbl[DBNumTypes] = cp;
    DBTypePlaneTbl[DBNumTypes] = pNum;
    DBNumTypes++;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechNewGeneratedType --
 *
 * Create the name for a type generated as the image of a contact on
 * another plane.  The generated name is of the form 'type/plane',
 * where 'type' is the short name of the type 'base', and 'plane'
 * is the short name of the plane 'p'.  Install the new name and
 * return the new TileType.
 *
 * Results:
 *	Returns the new tile type, or -1 if none could be allocated.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

int
dbTechNewGeneratedType(base,base2,p)
    TileType base,base2;/* Base type of the contact being named */
    int p;		/* Plane on which the new image will reside */
{
    char buf[1024], *cp;

    if (DBNumTypes >= TT_MAXTYPES-TT_RESERVEDTYPES)
    {
	TechError("Too many types to generate a new contact.  Maximum=%d\n",
			TT_MAXTYPES-TT_RESERVEDTYPES);
	return (TileType) -1;
    }

    if (base2)
    {
         char	*backslash1;
	 char	*backslash2;
	 if (backslash1 = index(DBTypeShortName(base),'/')) *backslash1 = 0;
	 if (backslash2 = index(DBTypeShortName(base),'/')) *backslash2 = 0;
	 if (strcmp(DBTypeShortName(base),DBTypeShortName(base2)) >0)
	 {
	      (void) sprintf(buf, "%s+%s/%s", 
	 		DBTypeShortName(base),
	 		DBTypeShortName(base2),DBPlaneShortName(p));
	 }
	 else
	 {
	      (void) sprintf(buf, "%s+%s/%s", 
	 		DBTypeShortName(base2),
	 		DBTypeShortName(base),DBPlaneShortName(p));
	 }
	 if (backslash1) *backslash1 = '/';
	 if (backslash2) *backslash2 = '/';
    }
    else
    {
         (void) sprintf(buf, "%s/%s", 
	 	DBTypeShortName(base),DBPlaneShortName(p));
    }
    cp = dbTechNameAdd(buf, (ClientData) DBNumTypes, &dbTypeNameLists);
    if (cp == NULL)
    {
	TechError("Couldn't generate new image type %s\n", buf);
	return (TileType) -1;
    }

    DBTypeLongNameTbl[DBNumTypes] = cp;
    DBTypePlaneTbl[DBNumTypes] = p;

    return DBNumTypes++;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechFinalType --
 *
 * After processing the types and planes sections, compute the
 * various derived type and plane masks and tables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes DBNumUserLayers to be DBNumTypes at the time
 *	    this procedure is called, since none of the automatically
 *	    generated plane images have yet been created.
 *	Initializes the following bit masks:
 *		DBAllTypeBits
 *		DBSpaceBits
 *		DBBuiltinLayerBits
 *		DBAllButSpaceBits
 *		DBAllButSpaceAndDRCBits
 *		DBUserLayerBits
 *		DBNonSpaceUserLayerBits
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechFinalType()
{
    TileType i;
    int pNum;

    DBNumUserLayers = DBNumTypes;

    for (i = 0; i < TT_MAXTYPES; i++)
    {
	if (i >= TT_SELECTBASE)
	    TTMaskSetType(&DBAllButSpaceAndDRCBits, i);

	if (i < TT_TECHDEPBASE)
	    TTMaskSetType(&DBBuiltinLayerBits, i);
	else if (i < DBNumUserLayers)
	    TTMaskSetType(&DBUserLayerBits, i);
    }

    TTMaskCom2(&DBAllTypeBits, &DBZeroTypeBits);
    TTMaskSetOnlyType(&DBSpaceBits, TT_SPACE);
    TTMaskCom2(&DBAllButSpaceBits, &DBSpaceBits);
    TTMaskClearMask3(&DBNonSpaceUserLayerBits, &DBUserLayerBits, &DBSpaceBits);

    /* Space is visible on all planes but the subcell plane */
    TTMaskZero(&DBPlaneTypes[PL_CELL]);
    for (pNum = PL_PAINTBASE;  pNum < PL_MAXTYPES;  pNum++)
	TTMaskSetOnlyType(&DBPlaneTypes[pNum], TT_SPACE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechNameLookup --
 *
 * Lookup a type or plane name.
 * Case is significant.
 *
 * Results:
 *	Returns the ClientData associated with the given name.
 *	If the name was not found, we return -2; if it was ambiguous,
 *	we return -1.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ClientData
dbTechNameLookup(str, table)
    char *str;		/* The name to be looked up */
    NameList *table;	/* Table of names to search */
{
    /*
     * The search is carried out by using two pointers, one which moves
     * forward through list from its start, and one which moves backward
     * through table from its end.  The two pointers mark the range of
     * strings that match the portion of str that we have scanned.  When
     * all of the characters of str have been scanned, then the two
     * pointers better be identical, or one of the strings in the range
     * between the two pointers better match 'str' exactly.
     */
    register NameList *bot, *top;
    char currentchar;
    int indx;

    bot = table->sn_next;
    top = table->sn_prev;
    if (top == bot) return ((ClientData) -2);

    for (indx = 0; ; indx++)
    {
	/* Check for the end of string */
	currentchar = str[indx];
	if (currentchar == '\0')
	{
	    if (bot == top)
		return (bot->sn_value);

	    /*
	     * Several entries match this one up to the last character
	     * of the string.  If one is an exact match, we allow it;
	     * otherwise, we claim the string was ambiguous.
	     */
	    for ( ; bot != top; bot = bot->sn_next)
		if (bot->sn_name[indx] == '\0')
		    return (bot->sn_value);

	    return ((ClientData) -1);
	}

	/*
	 * Move bot up until the string it points to matches str in the
	 * indx'th position.  Make match refer to the indx of bot in table.
	 */
	while (bot->sn_name[indx] != currentchar)
	{
	    if (bot == top) return((ClientData) -2);
	    bot = bot->sn_next;
	}

	/* Move top down until it matches */
	while (top->sn_name[indx] != currentchar)
	{
	    if (bot == top) return((ClientData) -2);
	    top = top->sn_prev;
	}
    }

    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechNameAdd --
 *
 * Add several names to a name table.
 * The shortest name is marked as the standard "short" name
 * for this cdata value.
 *
 * Results:
 *	Returns a pointer to the first name added if successful,
 *	or NULL if not.  In this latter case, we print error messages.
 *
 * Side effects:
 *	Adds new entries to the table pointed to by *ptable, and
 *	modifies *ptable if necessary.
 *
 * ----------------------------------------------------------------------------
 */

char *
dbTechNameAdd(name, cdata, ptable)
    register char *name;	/* Comma-separated list of names to be added */
    ClientData cdata;		/* Value to be stored with each name above */
    NameList *ptable;		/* Table to which we will add names */
{
    register char *cp;
    char onename[BUFSIZ];
    char *first;
    int shortestLength, length;
    NameList *primary, *current;

    if (name == NULL)
	return (NULL);

    first = NULL;
    shortestLength = INFINITY;
    primary = NULL;
    while (*name)
    {
	if (*name == ',')
	{
	    name++;
	    continue;
	}
	for (cp = onename; *name && *name != ','; *cp++ = *name++)
	    /* Nothing */;
	*cp = '\0';
	if (*(cp = onename))
	{
	    if ((current = dbTechNameAddOne(cp, cdata, FALSE, ptable)) == NULL)
		return (NULL);
	    if (first == NULL)
		first = current->sn_name;
	    length = strlen(onename);
	    if (length < shortestLength)
		shortestLength = length, primary = current;
	}
    }

    if (primary)
	primary->sn_primary = TRUE;
    return (first);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechNameAddOne --
 *
 * Add a single name to the table.
 *
 * Results:
 *	Returns a pointer to the new entry if succesful,
 *	or NULL if the name was already in the table.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

NameList *
dbTechNameAddOne(name, cdata, isPrimary, ptable)
    register char *name;	/* Name to be added */
    ClientData cdata;		/* Client value associated with this name */
    bool isPrimary;		/* TRUE if this is the primary abbreviation */
    NameList *ptable;		/* Table of names to which we're adding this */
{
    register cmp;
    register NameList *tbl, *new;

    /* Sort the name into the existing list */
    for (tbl = ptable->sn_next ;tbl != ptable; tbl = tbl->sn_next)
	if ((cmp = strcmp(name, tbl->sn_name)) == 0)
	{
	    TechError("Duplicate name: %s\n", name);
	    return (NULL);
	}
	else if (cmp < 0)
	    break;

    /* Create a new name */
    MALLOC(NameList *, new, sizeof (NameList));
    new->sn_name = StrDup((char **) NULL, name);
    new->sn_value = cdata;
    new->sn_primary = isPrimary;

    /* Link this entry in to the list before 'tbl' */
    new->sn_next = tbl;
    new->sn_prev = tbl->sn_prev;
    tbl->sn_prev->sn_next = new;
    tbl->sn_prev = new;
    return (new);
}
