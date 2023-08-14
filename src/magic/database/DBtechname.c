/*
 * DBtechname.c --
 *
 * Mapping between tile types and their names.
 * WARNING: with the exception of DB*TechName{Type,Plane}() and
 * DB*ShortName(), * the procedures in this file MUST be called
 * after DBTechFinalType() has been called.
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
static char rcsid[] = "$Header: DBtechname.c,v 6.0 90/08/28 18:10:17 mayo Exp $";
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

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechNameType --
 *
 * Map from a type name into a type number.  If the type name has
 * the form "<type>/<plane>" and <type> is a contact, then the
 * type returned is the image of the contact on <plane>.  Of
 * course, in this case, <type> must have an image on <plane>.
 *
 * Results:
 *	Type number.  A value of -2 indicates that the type name was
 *	unknown; -1 indicates that it was ambiguous.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

TileType
DBTechNameType(typename)
    char *typename;	/* The name of the type */
{
    char *slash;
    TileType type;
    int plane, i;

    slash = index(typename, '/');
    if (slash != NULL) *slash = 0;
    type = (TileType) dbTechNameLookup(typename, &dbTypeNameLists);
    if (slash == NULL) return type;
    *slash = '/';
    if (type < 0) return type;

    /* There's a plane qualification.  Locate the image. */

    plane = (int) dbTechNameLookup(slash+1, &dbPlaneNameLists);
    if (plane < 0) return -2;
    if (DBPlane(type) == plane) return type;

    for (i = DBNumUserLayers; i < DBNumTypes; i++)
    {
	if (TTMaskHasType(&DBLayerTypeMaskTbl[type], i)
		&& (DBTypePlaneTbl[i] == plane))
	{
	    return (TileType) i;
	}
    }
    return -2;
}

/*
 *-------------------------------------------------------------------------
 *
 * The following returns a bitmask with the appropriate types set for the
 *	typename supplied.  This is useful when searching for plane-qualified
 *	images, where there may be more than one that fits the bill.
 *
 * Results: returns the first type found
 *
 * Side Effects: sets bitmask with the appropriate types.
 *
 *-------------------------------------------------------------------------
 */

TileType
DBTechNameTypes(typename,bitmask)
    char *typename;	/* The name of the type */
    TileTypeBitMask	*bitmask;
{
    char *slash;
    TileType type,returntype;
    int plane, i;

    TTMaskZero(bitmask);
    slash = index(typename, '/');
    if (slash != NULL) *slash = 0;
    type = (TileType) dbTechNameLookup(typename, &dbTypeNameLists);
    if (slash == NULL)
    {
    	 TTMaskSetType(bitmask,type);
	 return type;
    } 
    *slash = '/';
    if (type < 0) return type;

    /* There's a plane qualification.  Locate the image. */

    plane = (int) dbTechNameLookup(slash+1, &dbPlaneNameLists);
    if (plane < 0) return -2;
    if (DBPlane(type) == plane)
    {
    	 TTMaskSetType(bitmask,type);
    	 return type;
    } 
    returntype = -2;
    for (i = DBNumUserLayers; i < DBNumTypes; i++)
    {
	if (TTMaskHasType(&DBLayerTypeMaskTbl[type], i)
		&& (DBTypePlaneTbl[i] == plane))
	{
	    TTMaskSetType(bitmask,i);
	    if (returntype == -2) returntype = i;
	}
    }
    return returntype;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechNoisyNameType --
 *
 * Map from a type name into a type number, complaining if the type
 * is unknown.
 *
 * Results:
 *	Type number.  A value of -2 indicates that the type name was
 *	unknown; -1 indicates that it was ambiguous.
 *
 * Side effects:
 *	Prints a diagnostic message if the type name is unknown.
 *
 * ----------------------------------------------------------------------------
 */

TileType
DBTechNoisyNameType(typename)
    char *typename;	/* The name of the type */
{
    TileType type;

    switch (type = DBTechNameType(typename))
    {
	case -1:
	    TechError("Ambiguous layer (type) name \"%s\"\n", typename);
	    break;
	case -2:
	    TechError("Unrecognized layer (type) name \"%s\"\n", typename);
	    break;
	default:
	    if (type < 0)
		TechError("Funny type \"%s\" returned %d\n", typename, type);
	    break;
    }

    return (type);
}
/*
 * ----------------------------------------------------------------------------
 *
 * DBTechNamePlane --
 *
 * Map from a plane name into a plane number.
 *
 * Results:
 *	Plane number.  A value of -2 indicates that the plane name was
 *	unknown; -1 indicates that it was ambiguous.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

DBTechNamePlane(planename)
    char *planename;	/* The name of the plane */
{
    return ((int) dbTechNameLookup(planename, &dbPlaneNameLists));
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechNoisyNamePlane --
 *
 * Map from a plane name into a plane number, complaining if the plane
 * is unknown.
 *
 * Results:
 *	Plane number.  A value of -2 indicates that the plane name was
 *	unknown; -1 indicates that it was ambiguous.
 *
 * Side effects:
 *	Prints a diagnostic message if the type name is unknown.
 *
 * ----------------------------------------------------------------------------
 */

int
DBTechNoisyNamePlane(planename)
    char *planename;	/* The name of the plane */
{
    int pNum;

    switch (pNum = DBTechNamePlane(planename))
    {
	case -1:
	    TechError("Ambiguous plane name \"%s\"\n", planename);
	    break;
	case -2:
	    TechError("Unrecognized plane name \"%s\"\n", planename);
	    break;
    }

    return (pNum);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTypeShortName --
 * DBPlaneShortName --
 *
 * Return the short name for a type or plane.
 * The short name is the "official abbreviation" for the type or plane,
 * identified by a leading '*' in the list of names in the technology
 * file.
 *
 * Results:
 *	Pointer to the primary short name for the given type or plane.
 *	If the type or plane has no official abbreviation, returns
 *	a pointer to the string "???".
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

char *
DBTypeShortName(type)
    TileType type;
{
    register NameList *tbl;

    for (tbl = dbTypeNameLists.sn_next;
	    tbl != &dbTypeNameLists;
	    tbl = tbl->sn_next)
    {
	if (tbl->sn_value == (ClientData) type && tbl->sn_primary)
	    return (tbl->sn_name);
    }

    if (DBTypeLongNameTbl[type])
	return (DBTypeLongNameTbl[type]);
    return ("???");
}

char *
DBPlaneShortName(pNum)
    int pNum;
{
    register NameList *tbl;

    for (tbl = dbPlaneNameLists.sn_next;
	    tbl != &dbPlaneNameLists;
	    tbl = tbl->sn_next)
    {
	if (tbl->sn_value == (ClientData) pNum && tbl->sn_primary)
	    return (tbl->sn_name);
    }

    if (DBPlaneLongNameTbl[pNum])
	return (DBPlaneLongNameTbl[pNum]);
    return ("???");
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechTypesToPlanes --
 *
 * Convert a TileTypeBitMask into a mask of the planes which may
 * contain tiles of that type.
 *
 * Results:
 *	A mask with bits set for those planes in which tiles of
 *	the types specified by the mask may reside.  The mask
 *	is guaranteed only to contain bits corresponding to
 *	paint tile planes.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

DBTechTypesToPlanes(mask)
    TileTypeBitMask *mask;
{
    TileType t;
    int planeMask, noCellMask;

    /* Space tiles are present in all planes but the cell plane */
    noCellMask = ~(PlaneNumToMaskBit(PL_CELL));
    if (TTMaskHasType(mask, TT_SPACE))
	return ((PlaneNumToMaskBit(DBNumPlanes)-1) & noCellMask);

    planeMask = 0;
    for (t = 0; t < DBNumTypes; t++)
	if (TTMaskHasType(mask, t))
	    planeMask |= DBTypePlaneMaskTbl[t];

    return (planeMask & noCellMask);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechPrintTypes --
 *
 * 	This routine prints out all the layer names for types defined
 *	in the current technology.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed.
 *
 * ----------------------------------------------------------------------------
 */

void
DBTechPrintTypes()
{
    TileType i;
    NameList *p;
    bool first;
    DefaultType *dtp;

    TxPrintf("Layer names are:\n");

    /* List technology independent types */
    for (i = TT_TECHDEPBASE; i < DBNumUserLayers; i++)
    {
	first = TRUE;
	for (p = dbTypeNameLists.sn_next; p != &dbTypeNameLists;
		p = p->sn_next)
	{
	    if (((TileType) p->sn_value) == i)
	    {
		if (first) TxPrintf("    %s", p->sn_name);
		else TxPrintf(" or %s", p->sn_name);
		first = FALSE;
	    }
	}
	if (!first) TxPrintf("\n");
    }

    /* List build in types that are normally painted by name */
    for (dtp = dbTechDefaultTypes; dtp->dt_names; dtp++)
    {
	if (dtp->dt_print)
	{
	    first = TRUE;
	    for (p = dbTypeNameLists.sn_next; p != &dbTypeNameLists;
		p = p->sn_next)
	    {
		if (((TileType) p->sn_value) == dtp->dt_type)
		{
		    if (first)
			TxPrintf("    %s", p->sn_name);
		    else 
			TxPrintf(" or %s", p->sn_name);
		    first = FALSE;
		}
	    }
	    if (!first) TxPrintf("\n");
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechNoisyNameMask --
 *
 *	Parses an argument string that selects a group of layers.
 *	The string may contain one or more layer names separated
 *	by commas.  The special layer name of "0" specifies no layer,
 *      it is used as a place holder, e.g., to specify a null
 *      layer list for the CornerTypes field in a drc edge-rule.
 *      In addition, a tilde may be used to indicate
 *	"all layers but", and parentheses may be used for grouping.
 *	Thus ~x means "all layers but x", and ~(x,y),z means "z plus
 *	everything except x and y)".  When contacts are specified,
 *	ALL images of the contact are automatically included, unless
 *	a specific plane is indicated in the layer specification
 *	using "/".  For example, x/foo refers to the image of contact
 *	"x" on plane "foo".  The layer specification may also follow
 *	a parenthesized group.  For example, ~(x,y)/foo refers to
 *	all layers on plane "foo" except "x" and "y".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Error messages are output if layers aren't understood.
 *	Sets the TileTypeBitMask 'mask' to all the layer names indicated.
 *
 * ----------------------------------------------------------------------------
 */

void
DBTechNoisyNameMask(layers, mask)
    char *layers;			/* String to be parsed. */
    TileTypeBitMask *mask;		/* Where to store the layer mask. */
{
    register char *p, *p2, c;
    TileTypeBitMask m2;            /* Each time around the loop, we will
                                         * form the mask for one section of
                                         * the layer string.
                                         */
    char save;
    bool allBut;

    TTMaskZero(mask);
    p = layers;
    while (TRUE)
    {
	TTMaskZero(&m2);

	c = *p;
	if (c == 0) break;

	/* Check for a tilde, and remember it in order to do negation. */

	if (c == '~')
	{
	    allBut = TRUE;
	    p += 1;
	    c = *p;
	}
	else allBut = FALSE;

	/* Check for parentheses.  If there's an open parenthesis,
	 * find the matching close parenthesis and recursively parse
	 * the string in-between.
	 */

	if (c == '(')
	{
	    int nesting = 0;

	    p += 1;
	    for (p2 = p; ; p2 += 1)
	    {
		if (*p2 == '(') nesting += 1;
		else if (*p2 == ')')
		{
		    nesting -= 1;
		    if (nesting < 0) break;
		}
		else if (*p2 == 0)
		{
		    TechError("Unmatched parenthesis in layer name \"%s\".\n",
			layers);
		    break;
		}
	    }
	    save = *p2;
	    *p2 = 0;
	    DBTechNoisyNameMask(p, &m2);
	    *p2 = save;
	    if (save == ')') p = p2 + 1;
	    else p = p2;
	}
	else
	{
	    TileType t;

	    /* No parenthesis, so just parse off a single name.  Layer
	     * name "0" corresponds to no layers at all.
	     */

	    for (p2 = p; ; p2++)
	    {
		c = *p2;
		if ((c == '/') || (c == ',') || (c == 0)) break;
	    }
	    if (p2 == p)
	    {
		TechError("Missing layer name in \"%s\".\n", layers);
	    }
	    else if (strcmp(p, "0") != 0)
	    {
		save = *p2;
		*p2 = 0;
		t = DBTechNoisyNameType(p);
		if (t >= 0) m2 = DBLayerTypeMaskTbl[t];
		*p2 = save;
	    }
	    p = p2;
	}

	/* Now negate the layers, if that is called for. */

	if (allBut) TTMaskCom(&m2);

	/* Restrict to a single plane, if that is called for. */

	if (*p == '/')
	{
	    int plane;

	    p2 = p+1;
	    while ((*p2 != 0) && (*p2 != ',')) p2 += 1;
	    save = *p2;
	    *p2 = 0;
    	    plane = DBTechNoisyNamePlane(p+1);
	    *p2 = save;
	    p = p2;
	    if (plane > 0) TTMaskAndMask(&m2, &DBPlaneTypes[plane]);
	}

	TTMaskSetMask(mask, &m2);
	while (*p == ',') p++;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechMinSetPlanes --
 *
 * Given a TileTypeBitMask, find the minimum set of planes that
 * contains one image for each of the tile types in the mask.
 * Also return a new tile type mask that contains the images
 * of the original tiles that fall in the result planes.
 *
 * WARNING: must be called AFTER all the database technology
 * initialization has completed.
 *
 * Results:
 *	A mask of planes.
 *
 * Side effects:
 *	The parameter newTypes is modified.
 *
 * ----------------------------------------------------------------------------
 */

int
DBTechMinSetPlanes(typeMask, newTypes)
    TileTypeBitMask typeMask;		/* Collection of tile types. */
    TileTypeBitMask *newTypes;		/* Filled in with new set of types
					 * equivalent to typeMask.
					 */
{
    int planes, okPlanes, i, j;
    TileTypeBitMask tempTypes;

    /*
     * First go through and find all planes for non-contact types.
     * These MUST be in the result.
     */
    planes = 0;
    TTMaskZero(newTypes);
    for (i = TT_SELECTBASE; i < DBNumTypes; i++)
    {
	if (!TTMaskHasType(&typeMask, i) || DBConnPlanes[i] != 0)
	    continue;
	TTMaskSetType(newTypes, i);
	planes |= PlaneNumToMaskBit(DBTypePlaneTbl[i]);
    }

    /*
     * Now go through each contact type.  If one of the contact's planes
     * is already necessary, use it.  Otherwise, add the plane for
     * the contact.
     */
    for (i = TT_SELECTBASE; i < DBNumTypes; i++)
    {
	if (!TTMaskHasType(&typeMask, i) || DBConnPlanes[i] == 0)
	    continue;
	okPlanes = DBConnPlanes[i] & planes;
	if (okPlanes != 0)
	{
	    for (j = PL_SELECTBASE; j < DBNumPlanes; j++)
		if (PlaneMaskHasPlane(okPlanes, j))
		{
		    TTMaskAndMask3(&tempTypes,
				&DBLayerTypeMaskTbl[i], &DBPlaneTypes[j]);
		    TTMaskSetMask(newTypes, &tempTypes);
		    break;
		}
	}
	else
	{
	    TTMaskSetType(newTypes, i);
	    planes |= PlaneNumToMaskBit(DBTypePlaneTbl[i]);
	}
    }

    return (planes);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechSubsetLayers --
 *
 * 	Eliminate all bits from one mask that aren't in another, and
 *	check to be sure that this elimination only occurs for contact
 *	types that will still have one image in the result.
 *
 * Results:
 *	TRUE is returned if the subsetting was successful.  Success
 *	means that for each layer in "src", the corresponding layer
 *	is in "mask" or else "src" contains another image of the bit
 *	that is in "mask".
 *
 * Side effects:
 *	The mask pointed to by "result" is modified to contain the
 *	subset of "src" that is in "mask".
 *
 * ----------------------------------------------------------------------------
 */

bool
DBTechSubsetLayers(src, mask, result)
    TileTypeBitMask src;		/* Original set of tile types. */
    TileTypeBitMask mask;		/* Only keep types in this mask. */
    TileTypeBitMask *result;		/* Store subset here. */
{
    TileTypeBitMask tmp,tmp2,clrmask;
    register int i;
    bool success = TRUE;

    TTMaskZero(result);
    TTMaskZero(&clrmask);
    for (i = 0; i < DBNumUserLayers; i++)
    {
	TTMaskAndMask3(&tmp, &src, &DBLayerTypeMaskTbl[i]);
	if (!TTMaskIsZero(&tmp))
	{
	     TTMaskAndMask3(&tmp2, &tmp,&mask);
	     if (!TTMaskIsZero(&tmp2))
	     {
	     	  TTMaskSetMask(&clrmask,&tmp);
	          TTMaskSetMask(result, &tmp2);
	     }
	}
    }
    if (TTMaskEqual(&clrmask,&src)) return TRUE; else return FALSE;
}
