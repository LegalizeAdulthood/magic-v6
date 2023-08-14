/*
 * DBtechcontact.c --
 *
 * Management of contacts.
 * This file makes a distinction between the following two terms:
 *
 *  Layer	-- Logical type as specified in "types" section of .tech
 *		   file.  A layer may consist of many TileTypes, as is the
 *		   case when it is a contact.
 *  Type	-- TileType stored in a tile
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
static char rcsid[] = "$Header: DBtcontact.c,v 6.0 90/08/28 18:10:11 mayo Exp $";
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

/* Set of types "belonging" to each type */
TileTypeBitMask DBLayerTypeMaskTbl[NT];

/* Filled in after contact image types have been generated */
TileTypeBitMask DBPlaneTypes[PL_MAXTYPES];
int DBTypePlaneMaskTbl[NT];
TileTypeBitMask DBContactBits;
TileTypeBitMask DBImageBits;

/* --------------------- Data local to this file ---------------------- */

/* Table of the properties of all layers */
LayerInfo dbLayerInfo[NT];

/* Array of pointers to the entries in above table for contacts only */
LayerInfo *dbContactInfo[NT];
int dbNumContacts;
int dbNumImages;

/* Forward declarations */
int dbTechMatchResiduesFunc();
int dbTechCombineResiduesFunc();
int dbTechReturn1();
struct dbCombineInfotype
{
     Residues	ci_residue;
     TileType	ci_type;
} dbCombineInfo;

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechInitContact --
 *
 * Mark all types as being non-contacts initially.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes dbLayerInfo.
 *	Also marks each type in DBLayerTypeMaskTbl
[] as consisting
 *	only of itself (no other images).
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechInitContact()
{
    register TileType t;
    register LayerInfo *lp;

    for (t = 0; t < TT_MAXTYPES; t++)
    {
	lp = &dbLayerInfo[t];
	lp->l_type = lp->l_residue = t;
	lp->l_rbelow = lp->l_rabove = TT_SPACE;
	lp->l_isContact = FALSE;
	lp->l_pmask = 0;
	lp->l_nresidues = 0;
	TTMaskSetOnlyType(&lp->l_tmask, t);
	TTMaskSetOnlyType(&DBLayerTypeMaskTbl[t], t);
    }

    dbNumContacts = dbNumImages = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechAddContact --
 *
 * Add the definition of a new contact type.
 * The syntax of each line in the "contact" section is:
 *
 *	contactType res1 res2 [res3]
 *
 * where res1, res2, and res3 are the residue types on the planes
 * connected by the contact.  At most three residue types may be
 * specified.  Furthermore, one must be on the home plane of the
 * contact (the plane specified in the "types" section for the type
 * contactType), and the others must be on adjacent planes.  Finally,
 * there can be only a single contactType that has the same residues.
 *
 * Results:
 *	FALSE on error, TRUE if successful.
 *
 * Side effects:
 *	Adds the definition of a new contact type.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
DBTechAddContact(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    TileType contactType,imageType;
    int nresidues, n;
    register LayerInfo *lp;
    Residues residues,imageRes;

    if (argc < 3 || argc > 4)
    {
	TechError("Line must contain type and 2 or 3 residues\n");
	return FALSE;
    }

    if ((contactType = DBTechNoisyNameType(*argv)) < 0)
	return FALSE;
    lp = &dbLayerInfo[contactType];

    /* Read the contact residues and check them for validity */
    nresidues = dbTechContactResidues(--argc, ++argv, contactType, &residues);
    if (nresidues < 0)
	return FALSE;

    /* Generate the images for this contact */
    n = 0;
    if (residues.r_below != TT_SPACE)
    {
	RESBUILD(&imageRes, TT_SPACE, residues.r_below, residues.r_residue);
	lp->l_images[n++] = dbTechSetImage(lp, &imageRes);
    }
    lp->l_images[n++] = lp->l_type;
    if (residues.r_above != TT_SPACE)
    {
	RESBUILD(&imageRes, residues.r_residue, residues.r_above, TT_SPACE);
	lp->l_images[n++] = dbTechSetImage(lp, &imageRes);
    }

    /* Has to come AFTER searching for matching images above */
    lp->l_isContact = TRUE;
    lp->l_nresidues = nresidues;
    lp->l_residues = residues;
    lp->l_pmask = dbTechResiduesToPlanes(&residues);

    /* Remember this as a paintable contact */
    dbContactInfo[dbNumContacts++] = lp;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechSetImage --
 *
 * Search for an existing type of contact whose residues match exactly
 * those in *rp.  If none can be found, create such a contact type.
 *
 * Results:
 *	Returns the TileType of the existing or created image type.
 *
 * Side effects:
 *	May create a new tile type.
 *
 * ----------------------------------------------------------------------------
 */

TileType
dbTechSetImage(lpContact, rp)
    LayerInfo *lpContact;
    Residues *rp;
{
    LayerInfo *lpImage;
    TileType imageType;
    int pNum;
    Residues	imageRes;

    /* Is there already a type with the desired residues? */
    imageType = dbTechMatchResidues(rp, TRUE,FALSE,TRUE);
    if (imageType != TT_SPACE)
	return imageType;

    /*
     * No: generate a new image type which only connects together
     * lp->l_residue and residueType.  
     */
    pNum = DBPlane(rp->r_residue);
    imageType = dbTechNewGeneratedType(lpContact->l_type, 0,pNum);
    lpImage = &dbLayerInfo[imageType];
    lpImage->l_isContact = TRUE;
    lpImage->l_nresidues = 2;
    lpImage->l_residues = *rp;
    lpImage->l_pmask = dbTechResiduesToPlanes(rp);

    /*
     * Mark l_images entries with a ridiculous value to cause a
     * crash if they're used as TileTypes.
     */
    lpImage->l_images[0] = (TileType) INFINITY;
    lpImage->l_images[1] = (TileType) INFINITY;

    /* need to look for other contacts that may share this residue;
	   these will have r_above equal to our r_below */
    if (rp->r_above == TT_SPACE)
    {
	RESBUILD(&imageRes, TT_SPACE , rp->r_residue, -1);
	RESBUILD(&dbCombineInfo.ci_residue,rp->r_below,rp->r_residue,TT_SPACE);
    }
    else
    {
	RESBUILD(&imageRes, -1 ,rp->r_residue , TT_SPACE);
	RESBUILD(&dbCombineInfo.ci_residue,TT_SPACE,rp->r_residue,rp->r_above);
    }
    dbCombineInfo.ci_type = lpContact->l_type;
    dbTechSrResidues(&imageRes,TRUE,dbTechCombineResiduesFunc,
	NULL,FALSE,TRUE);

    return lpImage->l_type;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechContactResidues --
 *
 * Process an argc/argv vector of up to three contact residue
 * type names, storing the residues in *rp and ensuring that:
 *	No residue is itself a contact
 *	Each residue is no more than one plane away from the
 *	    home plane of contactType.
 *	One of the residues is on the home plane of contactType.
 *	No other contact can already exist with the same set of residues.
 *
 * Results:
 *	Returns the number of residues in the contact,
 *	or -1 in the event of an error.
 *
 * Side effects:
 *	Fills in *rp.
 *
 * ----------------------------------------------------------------------------
 */

int
dbTechContactResidues(argc, argv, contactType, rp)
    int argc;
    char **argv;
    TileType contactType;
    Residues *rp;
{
    int homePlane, residuePlane, nresidues, pMask, res;
    TileType residueType, imageType,saveres;
    bool residueOnHome;
    TileType residueArray[3];
    char	**iargv=argv;

    rp->r_above = rp->r_below = rp->r_residue = TT_SPACE;
    nresidues = 0;
    pMask = 0;
    residueOnHome = FALSE;
    homePlane = DBPlane(contactType);
    for ( ; argc > 0; argc--, argv++)
    {
	if ((residueType = DBTechNoisyNameType(*argv)) < 0)
	    return -1;

	if (IsContact(residueType))
	{
	    TechError("Residue type %s is a contact itself\n",
		DBTypeLongName(residueType));
	    return -1;
	}
 
	/*
	 * Make sure the residue is on the same or an adjacent plane
	 * to the contact's home type.
	 */
	residuePlane = DBPlane(residueType);
	if (residuePlane < 0)
	{
	    TechError("Residue type %s doesn't have a home plane\n",
		DBTypeLongName(residueType));
	    return -1;
	}
	if (residuePlane < homePlane - 1 || residuePlane > homePlane + 1)
	{
	    TechError("Residue type %s isn't on the same plane as %s %s\n",
		DBTypeLongName(residueType), DBTypeLongName(contactType),
		"or adjacent to it");
	    return -1;
	}

	/* Enforce a single residue per plane */
	if (PlaneMaskHasPlane(pMask, residuePlane))
	{
	    TechError("Contact residues (%s) must be on different planes\n",
		DBTypeLongName(residueType));
	    return -1;
	}
	pMask |= PlaneNumToMaskBit(residuePlane);
	if (homePlane == residuePlane)
	    residueOnHome = TRUE;

	residueArray[nresidues++] = residueType;
    }

    if (!residueOnHome)
    {
	TechError("Contact type %s missing a residue on its home plane\n",
		DBTypeLongName(contactType));
	return -1;
    }

    /* Fill in the Residue structure we're returning our value in */
    for (res = 0; res < nresidues; res++)
    {
	residueType = residueArray[res];
	residuePlane = DBPlane(residueType);
	if (residuePlane == homePlane) rp->r_residue = residueType;
	else if (residuePlane == homePlane - 1) rp->r_below = residueType;
	else rp->r_above = residueType;


    }

    /*
     * See if there are any other contact types with identical residues;
     * if so, disallow contactType. 
     *
     * Since I modified this such that a user defined type is never used
     * as a residue, check only user types for duplicity; It's ok for 
     * one user-defined type and one derived type to have the same 
     * residues.
     *						dcs 4/25/88
     */
    imageType = dbTechMatchResidues(rp,TRUE,TRUE,FALSE);
    if (imageType != TT_SPACE)
    {
         TechError("Contact residues for %s identical to those for %s\n",
		DBTypeLongName(contactType),
		DBTypeLongName(imageType));
    		return -1;
    }
    return nresidues;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechMatchResidues --
 *
 * Find a type whose residues match *rp and return its LayerInfo struct.
 * The argument 'contactsOnly' is passed to dbTechSrResidues().
 *
 * Results:
 *	Returns the matching type, or TT_SPACE if none could be found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

TileType
dbTechMatchResidues(rp, contactsOnly,user,derived)
    Residues *rp;
    bool contactsOnly,user,derived;
{
    TileType type;

    if (dbTechSrResidues(rp, contactsOnly, dbTechMatchResiduesFunc,
		(ClientData) &type,user,derived))
    {
	return type;
    }

    return TT_SPACE;
}

int
dbTechMatchResiduesFunc(lp, pType)
    LayerInfo *lp;
    TileType *pType;
{
    *pType = lp->l_type;
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechSrResidues --
 *
 * Search for types whose residues match the template supplied.
 * A type matches the residue if all fields of its l_residues
 * structure match exactly, except for the fields for which rp's
 * value is -1 (a wildcard).  If 'contactsOnly' is TRUE, only
 * contact types are considered; otherwise, all types are matched
 * against the residue template.
 *
 * For each contact type found, calls the client procedure:
 *
 *	int
 *	(*proc)(lp, cdata)
 *	    LayerInfo *lp;	Contact type found
 *	    ClientData cdata;	Passed through from us
 *	{
 *	}
 *
 * The client procedure may return 0 to continue the search or 1 to
 * abort it.
 *
 * Results:
 *	Returns 0 if the client procedure returned 0's or was never
 *	called, or 1 if the client returned a 1 to abort the search.
 *
 * Side effects:
 *	Whatever the client procedure does.
 *
 * ----------------------------------------------------------------------------
 */

int
dbTechSrResidues(rp, contactsOnly, proc, cdata,user,derived)
    register Residues *rp;
    bool contactsOnly;
    int (*proc)();
    ClientData cdata;
    bool	user,derived;
{
    register LayerInfo *lp;
    TileType t;
    int	lowbound,highbound;
    
    lowbound = (user)?0:DBNumUserLayers;
    highbound = (derived)?DBNumTypes:DBNumUserLayers;

    for (t = lowbound; t < highbound; t++)
    {
	lp = &dbLayerInfo[t];
	if (!lp->l_isContact && contactsOnly)
	    continue;

	if (rp->r_above != -1 && rp->r_above != lp->l_rabove) continue;
	if (rp->r_below != -1 && rp->r_below != lp->l_rbelow) continue;
	if (rp->r_residue != -1 && rp->r_residue != lp->l_residue) continue;

	if ((*proc)(lp, cdata))
	    return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechFinalContact --
 *
 * Conclude reading the "contact" section of a technology file.
 * At this point, all tile types are known so we can call dbTechInitPaint()
 * to fill in the default paint/erase tables, and dbTechInitMasks() to fill
 * in the various exported TileTypeBitMasks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in the dbLayerInfo table for non-contacts.
 *	Sets DBLayerTypeMaskTbl to its final value.
 *	Initializes DBTypePlaneMaskTbl[] and DBPlaneTypes[].
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechFinalContact()
{
    register TileType primaryType, imageType;
    register LayerInfo *lp, *lpImage;
    int pNum, n;

    /* Finish building dbContactInfo for image types */
    dbNumImages = dbNumContacts;
    for (imageType = DBNumUserLayers; imageType < DBNumTypes; imageType++)
	if (IsContact(imageType))
	    dbContactInfo[dbNumImages++] = &dbLayerInfo[imageType];

    /* Fill in masks of contact and image types */
    TTMaskZero(&DBContactBits);
    TTMaskZero(&DBImageBits);
    for (primaryType = 0; primaryType < DBNumTypes; primaryType++)
    {
	if (IsContact(primaryType))
	{
	    TTMaskSetType(&DBImageBits, primaryType);
	    if (primaryType < DBNumUserLayers)
		TTMaskSetType(&DBContactBits, primaryType);
	}
    }

    for (primaryType = 0; primaryType < DBNumTypes; primaryType++)
    {
	lp = &dbLayerInfo[primaryType];
	if (!lp->l_isContact)
	{
	    /*
	     * Fill in layer info for non-contacts, since only the information
	     * for contacts was set by DBTechAddContact().
	     */
	    if (DBPlane(primaryType) > 0)
	    {
		lp->l_pmask = PlaneNumToMaskBit(DBPlane(primaryType));
		lp->l_nresidues = 1;
		lp->l_residue = lp->l_images[0] = primaryType;
	    }
	}
	else
	{
	    /*
	     * For each contact image, set l_tmask to be the mask of images
	     * on other planes to which this image is connected.
	     */
	    for (n = 0; n < dbNumImages; n++)
	    {
		lpImage = dbContactInfo[n];
		if (DBPlane(lpImage->l_type) == DBPlane(primaryType) - 1
			&& lpImage->l_residue == lp->l_rbelow
			&& lpImage->l_rabove == lp->l_residue 
			&& lpImage->l_type >= DBNumUserLayers)
		{
		    /* Image is on plane below primary type */
		    TTMaskSetType(&lp->l_tmask, lpImage->l_type);
		}
		else if (DBPlane(lpImage->l_type) == DBPlane(primaryType) + 1
			&& lpImage->l_residue == lp->l_rabove
			&& lpImage->l_rbelow == lp->l_residue
			&& lpImage->l_type >= DBNumUserLayers)
		{
		    /* Image is on plane above primary type */
		    TTMaskSetType(&lp->l_tmask, lpImage->l_type);
		}
	    }
	}

	/* DBLayerTypeMaskTbl is just an exported copy */
	DBLayerTypeMaskTbl[primaryType] = lp->l_tmask;
    }

    /*
     * Initialize the masks of planes on which each type appears.
     * It will contain all planes (except subcell) for space,
     * the home plane for each type up to DBNumTypes, and no
     * planes for undefined types.  Also update the mask of
     * types visible on each plane.
     */
    DBTypePlaneMaskTbl[TT_SPACE] = ~(PlaneNumToMaskBit(PL_CELL));
    for (primaryType = 0; primaryType < DBNumTypes; primaryType++)
    {
	pNum = DBTypePlaneTbl[primaryType];
	if (pNum > 0)
	{
	    DBTypePlaneMaskTbl[primaryType] = PlaneNumToMaskBit(pNum);
	    TTMaskSetType(&DBPlaneTypes[pNum], primaryType);
	}
    }
/*
    for (primaryType = TT_TECHDEPBASE; primaryType < DBNumTypes; primaryType++)
    {
    	 fprintf(stderr,"%d %s\n",primaryType,DBTypeShortName(primaryType));
    }
    for (pNum=PL_TECHDEPBASE;pNum <DBNumPlanes;pNum++)
    {
    	 fprintf(stderr,"%s %x %x\n",DBPlaneShortName(pNum),
 		DBPlaneTypes[pNum].tt_words[0],DBPlaneTypes[pNum].tt_words[1]);
    }
*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbImageOnPlane --
 *
 * Given a layer 'type' and a plane 'pNum', return the image
 * of 'type' on plane 'pNum'.  For a non-contact type, this is
 * just the type itself.  For a contact, it is the TileType
 * component of the contact whose home plane is 'pNum'.
 *
 * WARNING:
 *	It is the caller's responsibility to insure that 'type'
 *	actually has an image on plane 'pNum'.  This can most
 *	conveniently be done by insuring:
 *
 *		PlaneMaskHasPlane(PlaneMask(type), pNum)
 *
 * Results:
 *	Returns a TileType whose home plane is pNum.
 *
 * Side effects:
 *	May cause a niceabort() if 'type' does not have an
 *	image on plane 'pNum'.
 *
 * ----------------------------------------------------------------------------
 */

TileType
dbImageOnPlane(type, pNum)
    TileType type;
    register int pNum;
{
    register LayerInfo *lp = &dbLayerInfo[type];
    register contact;
    register p;

    if (!lp->l_isContact)
    {
	ASSERT(DBPlane(type) == pNum, "dbImageOnPlane");
	return (type);
    }

    for (p = 0; p < lp->l_nresidues; p++)
    {
	contact = lp->l_images[p];
	if (DBPlane(contact) == pNum)
	    return (contact);
    }

    TxError("Panic in dbImageOnPlane(): type %s has no image on plane %s\n",
		DBTypeLongName(type), DBPlaneLongName(pNum));
    niceabort();
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbImageOnPlane --
 *
 * Given a layer 'type' and a plane 'pNum', return the image
 * of 'type' on plane 'pNum'.  For a non-contact type, this is
 * just the type itself.  For a contact, it is the TileType
 * component of the contact whose home plane is 'pNum'.
 *
 * WARNING:
 *	It is the caller's responsibility to insure that 'type'
 *	actually has an image on plane 'pNum'.  This can most
 *	conveniently be done by insuring:
 *
 *		PlaneMaskHasPlane(PlaneMask(type), pNum)
 *
 * Results:
 *	Returns a TileType whose home plane is pNum.
 *
 * Side effects:
 *	May cause a niceabort() if 'type' does not have an
 *	image on plane 'pNum'.
 *
 * ----------------------------------------------------------------------------
 */

TileType
dbImageOnPlanes(type, pNum,lasttype)
    TileType type,lasttype;
    register int pNum;
{
    register LayerInfo *lp = &dbLayerInfo[type];
    int		tpNum = DBPlane(type);
    Residues	res;
    TileType	foundtype;
    int		dbImageSrFunc();
    
    if (tpNum == pNum) /* there are no residues to look for*/
    {
    	 return (type > lasttype)?type:-1;
    }
    else if (tpNum == pNum+1)
    {
    	 res.r_above = lp->l_residue;
	 res.r_residue = lp->l_rbelow;
	 res.r_below = -1;
	 return (dbTechSrResidues(&res,TRUE,dbImageSrFunc,
	     &lasttype,FALSE,TRUE))?lasttype:-1;
    }
    else if (tpNum == pNum-1)
    {
    	 res.r_below = lp->l_residue;
	 res.r_residue = lp->l_rabove;
	 res.r_above = -1;
	 return (dbTechSrResidues(&res,TRUE,dbImageSrFunc,
	     &lasttype,FALSE,TRUE))?lasttype:-1;
    }
    else
    {
    TxError("Panic in dbImageOnPlanes(): type %s has no image on plane %s\n",
		DBTypeLongName(type), DBPlaneLongName(pNum));
    niceabort();
    /*NOTREACHED*/
    }
}
int dbImageSrFunc(lp,lasttype)
	LayerInfo	*lp;
	TileType	*lasttype;

{
     if (lp->l_type > *lasttype)
     {
     	  *lasttype = lp->l_type;
	  return 1;
     }
     return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechResiduesToPlanes --
 *
 * Return a mask of the planes occupied by the residues in *rp.
 * Entries in *rp whose value is TT_SPACE or -1 are ignored.
 *
 * Results:
 *	Returns a mask of planes.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
dbTechResiduesToPlanes(rp)
    register Residues *rp;
{
    register int pmask;

    pmask = 0;
    if (rp->r_residue != TT_SPACE)
	pmask |= PlaneNumToMaskBit(DBPlane(rp->r_residue));
    if (rp->r_below != TT_SPACE)
	pmask |= PlaneNumToMaskBit(DBPlane(rp->r_below));
    if (rp->r_above != TT_SPACE)
	pmask |= PlaneNumToMaskBit(DBPlane(rp->r_above));

    return pmask;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechResiduesUnion --
 *
 * Take the "union" of the two Residue structures lp1->l_residues and
 * lp2->l_residues and store the result in *rpUnion.  The two Residues
 * must contain either the same type on each plane (r_below, r_residue,
 * r_above), or TT_SPACE; otherwise, they are considered "incompatible"
 * and we return FALSE.
 *
 * The result for each plane in the union is defined to be the non-space
 * of two types if one is non-space, or else TT_SPACE.  Values of -1 are
 * treated as TT_SPACE.
 *
 * Results:
 *	TRUE if successful, FALSE if the types were incompatible.
 *
 * Side effects:
 *	Modifies *rpUnion.
 *
 * ----------------------------------------------------------------------------
 */

#define	IGNOREWILD(t)	((t) == -1 ? TT_SPACE : (t))

bool
dbTechResiduesUnion(lp1, lp2, rpUnion)
    LayerInfo *lp1, *lp2;
    register Residues *rpUnion;
{
    register Residues *rp1 = &lp1->l_residues;
    register Residues *rp2 = &lp2->l_residues;

    if (IGNOREWILD(rp1->r_below) == TT_SPACE)
	rpUnion->r_below = IGNOREWILD(rp2->r_below);
    else if (IGNOREWILD(rp2->r_below) == TT_SPACE)
	rpUnion->r_below = IGNOREWILD(rp1->r_below);
    else if (rp1->r_below == rp2->r_below)
	rpUnion->r_below = rp1->r_below;
    else
	return FALSE;

    if (IGNOREWILD(rp1->r_residue) == TT_SPACE)
	rpUnion->r_residue = IGNOREWILD(rp2->r_residue);
    else if (IGNOREWILD(rp2->r_residue) == TT_SPACE)
	rpUnion->r_residue = IGNOREWILD(rp1->r_residue);
    else if (rp1->r_residue == rp2->r_residue)
	rpUnion->r_residue = rp1->r_residue;
    else
	return FALSE;

    if (IGNOREWILD(rp1->r_above) == TT_SPACE)
	rpUnion->r_above = IGNOREWILD(rp2->r_above);
    else if (IGNOREWILD(rp2->r_above) == TT_SPACE)
	rpUnion->r_above = IGNOREWILD(rp1->r_above);
    else if (rp1->r_above == rp2->r_above)
	rpUnion->r_above = rp1->r_above;
    else
	return FALSE;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechPrintImages --
 *
 * DEBUGGING.
 * Print a list of the contact types to which each possible contact image
 * belongs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbTechPrintImages()
{
    register LayerInfo *lpContact, *lpImage;
    int p, n, m;

    for (m = 0; m < dbNumImages; m++)
    {
	lpImage = dbContactInfo[m];
	TxPrintf("%s (on %s) image of ",
		DBTypeShortName(lpImage->l_type),
		DBPlaneShortName(DBPlane(lpImage->l_type)));
	for (n = 0; n < dbNumContacts; n++)
	{
	    lpContact = dbContactInfo[n];
	    for (p = 0; p < lpContact->l_nresidues; p++)
		if (lpContact->l_images[p] == lpImage->l_type)
		    TxPrintf("%s ", DBTypeShortName(lpContact->l_type));
	}
	TxPrintf("\n");
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * dbTechCombineResiduesFunc -- given a newly created residue, look for
 *	other contacts that have the same residue type on a plane. For each
 *	one found, create a new image to represent the union of the 2.
 *	
 *
 * Results: returns 0 to continue search
 *
 * Side Effects: may add new residue types.
 *
 *-------------------------------------------------------------------------
 */
dbTechCombineResiduesFunc(lp,junk)
	LayerInfo	*lp;
	ClientData	junk;

{
     Residues	residues2;
     TileType	imageType = -1;
     LayerInfo	*lpImage;
     
     residues2 = dbCombineInfo.ci_residue;
     /* first check to see if a tile of the requisite type exists */
     if (residues2.r_below == TT_SPACE)
     {
	  residues2.r_below = lp->l_residues.r_below;
	  if (dbTechSrResidues(&residues2,FALSE,dbTechReturn1,NULL,FALSE,TRUE) ==0)
	  {
	       imageType = dbTechNewGeneratedType(lp->l_type,
	       		 dbCombineInfo.ci_type,DBPlane(residues2.r_residue));

	  }
     }
     else if (residues2.r_above == TT_SPACE)
     {
	  residues2.r_above = lp->l_residues.r_above;
	  if (dbTechSrResidues(&residues2,FALSE,dbTechReturn1,NULL,FALSE,TRUE) ==0)
	  {
	       imageType = dbTechNewGeneratedType(lp->l_type,
	       		 dbCombineInfo.ci_type,DBPlane(residues2.r_residue));

	  }
     }
     if (imageType != -1)
     {
           lpImage = &dbLayerInfo[imageType];
           lpImage->l_isContact = TRUE;
           lpImage->l_nresidues = 3;
           lpImage->l_residues = residues2;
           lpImage->l_pmask = dbTechResiduesToPlanes(&residues2);
           lpImage->l_images[0] = (TileType) INFINITY;
           lpImage->l_images[1] = (TileType) INFINITY;
     }
     return 0;
     
}
/* the following always returns 1 to abort the search. */
dbTechReturn1(lp,dummy)
	LayerInfo	*lp;
	ClientData	dummy;

{
     return 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechGetContact --
 *
 * Given two tile types, determine the corresponding contact type.
 *
 * Results:
 *	Returns a contact type.
 *
 * Side effects:
 *	Prints stuff if it can't find a contact type.
 *
 * ----------------------------------------------------------------------------
 */

TileType
DBTechGetContact(type1, type2)
    TileType type1, type2;
{
    register int pmask;
    register LayerInfo *lp;
    register TileType t;

    pmask = PlaneNumToMaskBit(DBPlane(type1)) | PlaneNumToMaskBit(DBPlane(type2));
    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
    {
	lp = &dbLayerInfo[t];
	if (lp->l_isContact)
	    if ( lp->l_pmask == pmask )
		return t;
    }

    TxPrintf("No contact type for %d %d\n",type1,type2);
    return (TileType) 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechPrintContacts --
 *
 * DEBUGGING.
 * Print a list of the contact types to which each possible contact image
 * belongs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbTechPrintContacts()
{
    register LayerInfo *lpContact, *lpImage;
    register TileType t;
    int p, n, m;
    int pNum;

    for (m = 0; m < dbNumImages; m++)
    {
	lpImage = dbContactInfo[m];
	TxPrintf("%s (on %s) image of ",
		DBTypeShortName(lpImage->l_type),
		DBPlaneShortName(DBPlane(lpImage->l_type)));
	for (n = 0; n < dbNumContacts; n++)
	{
	    lpContact = dbContactInfo[n];
	    for (p = 0; p < lpContact->l_nresidues; p++)
		if (lpContact->l_images[p] == lpImage->l_type)
		    TxPrintf("%s ", DBTypeShortName(lpContact->l_type));
	}

	TxPrintf(" connects:");
	for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
	    if ( TTMaskHasType(&DBConnectTbl[lpImage->l_type], t) )
		TxPrintf(" %s", DBTypeShortName(t));

	TxPrintf(" planes:");
	for ( pNum = PL_TECHDEPBASE; pNum < PL_MAXTYPES; pNum++ )
	    if ( PlaneNumToMaskBit(pNum) & DBConnPlanes[lpImage->l_type] )
		TxPrintf(" %s", DBPlaneShortName(pNum));
	TxPrintf("\n");
    }
}

