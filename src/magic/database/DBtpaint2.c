/*
 * DBtechpaint2.c --
 *
 * Default composition rules.
 * Pretty complicated, unfortunately, so it's in a separate file.
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
static char rcsid[] = "$Header: DBtpaint2.c,v 6.0 90/08/28 18:10:32 mayo Exp $";
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

#define	SETPAINT(have,paint,get) \
	if (IsDefaultPaint((have), (paint))) \
	    dbSetPaintEntry((have), (paint), DBPlane(have), (get)); \
	else
#define	SETERASE(have,erase,get) \
	if (IsDefaultErase((have), (erase))) \
	    dbSetEraseEntry((have), (erase), DBPlane(have), (get)); \
	else

int dbTechBreakPaint();
int breakEraseAbove(), breakEraseBelow();

LayerInfo *dbTechLpImage;
LayerInfo *dbTechLpPaint;

/*
 * ----------------------------------------------------------------------------
 *
 * DBTechFinalCompose --
 *
 * Process all the contact erase/compose rules saved by DBTechAddCompose
 * when it was reading in the "compose" section of a technology file.
 * Also sets up the default paint/erase rules for contacts.
 *
 * Since by the end of this section we've processed all the painting
 * rules, we initialize the tables that say which planes get affected
 * by painting/erasing a given type.
 *
 * There's a great deal of work done here, so it's broken up into a
 * number of separate procedures, each of which implements a single
 * operation or default rule.  Most of the work deals with painting
 * and erasing contacts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the paint/erase tables.
 *	Initializes DBTypePaintPlanesTbl[] and DBTypeErasePlanesTbl[].
 *
 * ----------------------------------------------------------------------------
 */

Void
DBTechFinalCompose()
{
    /* Default rules for painting/erasing contacts */
    dbComposePaintAllImages();
    dbComposeResidues();
    dbComposeContacts();

    /* Process rules saved from reading the "compose" section */
    dbComposeSavedRules();

    /* Build up exported tables */
    dbTechPaintErasePlanes();
    dbTechComponents();
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbTechPaintErasePlanes --
 *
 * Fill in the tables telling which planes get affected
 * by painting and erasing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in DBTypePaintPlanesTbl[] and DBTypeErasePlanesTbl[].
 *
 * ----------------------------------------------------------------------------
 */

Void
dbTechPaintErasePlanes()
{
    register TileType t, s;
    register int pNum;

    /* Space tiles are special: they may appear on any plane */
    DBTypePaintPlanesTbl[TT_SPACE] = ~(PlaneNumToMaskBit(PL_CELL));
    DBTypeErasePlanesTbl[TT_SPACE] = ~(PlaneNumToMaskBit(PL_CELL));

    /* Skip TT_SPACE */
    for (t = 1; t < DBNumTypes; t++)
    {
	DBTypePaintPlanesTbl[t] = DBTypeErasePlanesTbl[t] = 0;

	/* Contact images are never painted or erased directly */
	if (t >= DBNumUserLayers)
	    continue;

	for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	    for (s = 0; s < DBNumTypes; s++)
		if (DBPlane(s) == pNum || s == TT_SPACE)
		{
		    if (DBStdPaintEntry(s, t, pNum) != s)
			DBTypePaintPlanesTbl[t] |= PlaneNumToMaskBit(pNum);
		    if (DBStdEraseEntry(s, t, pNum) != s)
			DBTypeErasePlanesTbl[t] |= PlaneNumToMaskBit(pNum);
		}
    }
}

/* 
 * ----------------------------------------------------------------------------
 *
 * dbTechComponents --
 *
 * Fill in the table that tells for each tile type which
 * other types are "components" of it.
 *
 * Marks a tile type "s" as a component of a type "r" if:
 *	- s and r live on the same plane, and
 *	- the result of painting s on r is still r.
 *
 * Make a second pass and merge the information for all of the
 * images of each contact.  Each type that is a component of a
 * contact's image is also a component of the contact type.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbTechComponents()
{
    register LayerInfo *lp;
    register TileType image, paint;
    register int n, p;

    /* Components are those types that can be painted without effect */
    for (image = TT_TECHDEPBASE; image < DBNumTypes; image++)
    {
	TTMaskZero(&DBComponentTbl[image]);
	for (paint = TT_TECHDEPBASE; paint < DBNumUserLayers; paint++)
	    if (DBPlane(image) == DBPlane(paint)
		    && DBStdPaintEntry(image, paint, DBPlane(image)) == image)
		TTMaskSetType(&DBComponentTbl[image], paint);
    }

    /*
     * In addition, each type that is a component of a contact's image
     * is also a component of the primary type of the contact.
     */
    for (n = 0; n < dbNumContacts; n++)
    {
	lp = dbContactInfo[n];
	for (p = 0; p < lp->l_nresidues; p++)
	{
	    image = lp->l_images[p];
	    TTMaskSetMask(&DBComponentTbl[lp->l_type], &DBComponentTbl[image]);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbComposePaintAllImages --
 *
 * Painting the primary type of a contact layer paints its image
 * over all types on each image's plane.  (The only types that
 * should ever be painted are primary types.)
 *
 * This rule is called first because it may be overridden by later
 * rules, or by explicit composition rules.
 *
 * Only affects paint entries that haven't already been set to other
 * values by explicit paint rules.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies paint/erase tables.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbComposePaintAllImages()
{
    register TileType tImage, s;
    register LayerInfo *lp;
    TileType tPaint;
    int p, n;

    /* Iterate over primary types only */
    for (n = 0; n < dbNumContacts; n++)
    {
	lp = dbContactInfo[n];
	tPaint = lp->l_type;
	for (p = 0; p < lp->l_nresidues; p++)
	{
	    /*
	     * Painting the primary type paints the associated tImage
	     * on DBPlane(tImage).  Space is handled specially.
	     */
	    tImage = lp->l_images[p];
	    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
		if (DBPlane(s) == DBPlane(tImage))
		    SETPAINT(s, tPaint, tImage);
	    if (IsDefaultPaint(TT_SPACE, tPaint))
		dbSetPaintEntry(TT_SPACE, tPaint, DBPlane(tImage), tImage);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbComposeResidues --
 *
 * The behavior of a contact type when other, non-contact types are
 * painted over it or erased from it is derived from the behavior of
 * its residue types.
 *
 *  1.	If painting doesn't affect a contact's residue on a plane,
 *	it doesn't affect the contact's image on that plane either.
 *	This allows, for example, painting metal1 over a contact
 *	"containing" metal1 without breaking the contact.
 *
 *  2.	If painting or erasing a type affects a residue of a
 *	contact, the image's connectivity to adjacent planes
 *	is broken and the image is replaced by the result of
 *	painting or erasing over the residue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies paint/erase tables.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbComposeResidues()
{
    register LayerInfo *lp;
    register TileType s;
    register int n;

    /* Painting that doesn't affect the residue doesn't affect the contact */
    for (n = 0; n < dbNumImages; n++)
    {
	lp = dbContactInfo[n];
	for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    if (!PAINTAFFECTS(lp->l_residue, s))
		SETPAINT(lp->l_type, s, lp->l_type);
    }

    /*
     * Painting that does affect the residue breaks the contact.
     * This may also affect adjacent planes since they reflect
     * the presence of a contact on this one.
     */
    for (n = 0; n < dbNumImages; n++)
	for (s = TT_TECHDEPBASE; s < DBNumUserLayers; s++)
	    if (!IsContact(s))
		dbComposeBreak(dbContactInfo[n], s);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbComposeBreak --
 *
 * There are three ways that lp->l_type can be affected by
 * painting type s:
 *
 *	1. Painting s can affect lp->l_residue, in which case
 *	   the result of painting s over lp->l_type is the same
 *	   as painting s over lp->l_residue.  This has precedence
 *	   over all cases.
 *
 *	2. Painting s can affect lp->l_rabove.
 *	3. Painting s can affect lp->l_rbelow.
 *	   These two latter cases break the contact lp->l_type, and
 *	   replace it with a type whose connectivity to the level
 *	   above and/or below has been broken.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies paint/erase tables.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbComposeBreak(lp, s)
    register LayerInfo *lp;
    register TileType s;
{
    int pNum = DBPlane(lp->l_residue);
    Residues res;
    bool work;
    TileType	lasttype,result;
    LayerInfo	*lp2;


    if (PAINTAFFECTS(lp->l_residue, s))
	 SETPAINT(lp->l_type, s, DBStdPaintEntry(lp->l_residue, s, pNum));
    else
    {
	res = lp->l_residues;
	if (PAINTAFFECTS(lp->l_rabove, s))
	{
	     res.r_above = TT_SPACE;
	     result = (lp->l_type < DBNumUserLayers)?
	     	dbTechMatchResidues(&res, FALSE,TRUE,FALSE):
	     	dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
	     SETPAINT(lp->l_type, s, result);
	     if (result == TT_SPACE &&
	     	PlaneMaskHasPlane(PlaneMask(lp->l_type),pNum-1) &&
		lp->l_type < DBNumUserLayers)
	     {
	     	  lasttype = 0;
		  while ((lasttype=dbImageOnPlanes(lp->l_type,pNum-1,lasttype)) != -1)
		  {
		       lp2 = &dbLayerInfo[lasttype];
		       res = lp2->l_residues;
		       res.r_above = TT_SPACE;
	               result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
		       SETPAINT(lasttype,s,result);
		  }
	     }
	} 
	if (PAINTAFFECTS(lp->l_rbelow, s))
	{
	     res.r_below = TT_SPACE;
	     result = (lp->l_type < DBNumUserLayers)?
	     	dbTechMatchResidues(&res, FALSE,TRUE,FALSE):
	     	dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
	     SETPAINT(lp->l_type, s, result);
	     if (result == TT_SPACE &&
	     	PlaneMaskHasPlane(PlaneMask(lp->l_type),pNum+1)&&
		lp->l_type < DBNumUserLayers)
	     {
	     	  lasttype = 0;
		  while ((lasttype=dbImageOnPlanes(lp->l_type,pNum+1,lasttype)) != -1)
		  {
		       lp2 = &dbLayerInfo[lasttype];
		       res = lp2->l_residues;
		       res.r_below = TT_SPACE;
	               result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
		       SETPAINT(lasttype,s,result);
		  }
	     }
	     
	} 
    }

    if (ERASEAFFECTS(lp->l_residue, s))
	SETERASE(lp->l_type, s, DBStdEraseEntry(lp->l_residue, s, pNum));
    else
    {
	res = lp->l_residues;
	work = FALSE;
	if (ERASEAFFECTS(lp->l_rabove, s))
	{
	     res.r_above = TT_SPACE;
	     result = (lp->l_type < DBNumUserLayers)?
	     	dbTechMatchResidues(&res, FALSE,TRUE,FALSE):
	     	dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
	     SETERASE(lp->l_type, s, result);
	     if (result == TT_SPACE &&
	     	PlaneMaskHasPlane(PlaneMask(lp->l_type),pNum-1) &&
		lp->l_type < DBNumUserLayers)
	     {
	     	  lasttype = 0;
		  while ((lasttype=dbImageOnPlanes(lp->l_type,pNum-1,lasttype)) != -1)
		  {
		       lp2 = &dbLayerInfo[lasttype];
		       res = lp2->l_residues;
		       res.r_above = TT_SPACE;
	               result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
		       SETERASE(lasttype,s,result);
		  }
	     }
	} 
	if (ERASEAFFECTS(lp->l_rbelow, s)) 
	{
	     res.r_below = TT_SPACE;
	     result = (lp->l_type < DBNumUserLayers)?
	     	dbTechMatchResidues(&res, FALSE,TRUE,FALSE):
	     	dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
	     SETERASE(lp->l_type, s, result);
	     if (result == TT_SPACE &&
	     	PlaneMaskHasPlane(PlaneMask(lp->l_type),pNum+1) &&
		lp->l_type < DBNumUserLayers)
	     {
	     	  lasttype = 0;
		  while ((lasttype=dbImageOnPlanes(lp->l_type,pNum+1,lasttype)) != -1)
		  {
		       lp2 = &dbLayerInfo[lasttype];
		       res = lp2->l_residues;
		       res.r_below = TT_SPACE;
	               result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
		       SETERASE(lasttype,s,result);
		  }
	     }
	} 
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbComposeContacts --
 *
 * This procedure handles the rules for composition of contact types.
 * The idea is that each contact type specifies the directions that
 * it connects (up and/or down) along with the residues it connects
 * to in each direction.  We look at the results of painting each
 * type of contact in dbContactInfo[] over each possible contact
 * image type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies paint/erase tables.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbComposeContacts()
{
    register LayerInfo *lpImage, *lpPaint;
    register int n, m;

    for (m = 0; m < dbNumImages; m++)
    {
	lpImage = dbContactInfo[m];	/* Existing image */
	for (n = 0; n < dbNumContacts; n++)
	{
	    lpPaint = dbContactInfo[n];	/* Image being painted or erased */
	    dbComposePaintContact(lpImage, lpPaint);
	    dbComposeEraseContact(lpImage, lpPaint);
	}
    }
}

/*
 * dbComposePaintContact --
 *
 * Construct the painting rules for painting type lpPaint over
 * the contact image lpImage.
 */

Void
dbComposePaintContact(lpImage, lpPaint)
    register LayerInfo *lpImage, *lpPaint;
{
    int imagePNum = DBPlane(lpImage->l_type);
    int paintPNum = DBPlane(lpPaint->l_type);
    TileType result, paintImage;
    LayerInfo *lpPaintImage;
    Residues res;

    /*
     * If lpPaint has an image on imagePNum, we'll either
     * replace lpImage with a union type or with lpPaint's
     * image.
     */
    if (PlaneMaskHasPlane(lpPaint->l_pmask, imagePNum))
    {
	paintImage = dbImageOnPlane(lpPaint->l_type, imagePNum);
	lpPaintImage = &dbLayerInfo[paintImage];

	/*
	 * The types referred to by lpImage and lpPaintImage exist on the
	 * same plane, pImage.  Look at the residues of both types on
	 * this plane, and on the planes each image connects.  If they're
	 * compatible (identical, or if either is TT_SPACE meaning that the
	 * image doesn't connect to that plane), the result is the contact
	 * image type that connects the union of the residues.
	 */

	/* case 1- the image and the paint are on the same plane: */
	/* replace the image with the paint. */

	if (imagePNum == paintPNum) goto smashit;
	
	/* if we got here, we know the two don't line up */
	
	/* case 2- lpImage is a paintable contact --    */
	/*	here, we want to replace lpImage with   */
	/*	lpPaintImage.				*/
	
	if (lpImage->l_type < DBNumUserLayers ) goto smashit;
	
	/* case 3- OK, we're painting one of our images over and 
		existing image.  We fist have to see if there is
		a compatible residue with another type further up
		or lower down.
	*/

	res = lpPaintImage->l_residues;

	if (imagePNum == paintPNum+1)
	{
	     res.r_above =  lpImage->l_residues.r_above;
	}
	else
	{
	     res.r_below =  lpImage->l_residues.r_below;
	}
	result = dbTechMatchResidues(&res, TRUE,FALSE,TRUE);
	if (result != TT_SPACE)
	{
	     SETPAINT(lpImage->l_type, lpPaint->l_type, result);
	     return;
	}

smashit:
	/* No compatible image so replace with lpPaintImage->l_type */
	SETPAINT(lpImage->l_type, lpPaint->l_type, lpPaintImage->l_type);
	return;
    }

    /*
     * There's no image on imagePNum.
     * If lpPaint has an image on a plane adjacent to imagePNum,
     * then we look to see whether painting lpPaint on that plane
     * will cause the connection to this plane to be broken.
     */
    dbTechLpImage = lpImage;
    dbTechLpPaint = lpPaint;
    if (PlaneMaskHasPlane(lpPaint->l_pmask, imagePNum + 1)
	    && lpImage->l_rabove != TT_SPACE)
    {
	if (lpImage->l_type < DBNumUserLayers &&
	    lpPaint->l_rbelow != TT_SPACE)
	{
	     /* check to see if there is a compatible residue to connect
	        between lpImage and lpPaint.
	     */
	     res.r_residue = lpPaint->l_residues.r_below;
	     res.r_above   = lpPaint->l_residues.r_residue;
	     res.r_below   = lpImage->l_residues.r_residue;
	     if (dbTechMatchResidues(&res,TRUE,FALSE,TRUE) == TT_SPACE ||
	     	 lpImage->l_residues.r_above != lpPaint->l_residues.r_below)
	     
	     {
	     	  /* nothing compatible; have to terminate lpImage */
		  res = lpImage->l_residues;
		  res.r_above = TT_SPACE;
	          result = dbTechMatchResidues(&res, TRUE,TRUE,FALSE);
	          SETPAINT(lpImage->l_type, lpPaint->l_type, result);
		  if (result == TT_SPACE && lpImage->l_rbelow != TT_SPACE)
		  {
		       /* we may have to blast lpImage's lower image */
		       TileType	imageImage = dbImageOnPlane(lpImage->l_type, imagePNum-1);
 
		       res = dbLayerInfo[imageImage].l_residues;
		       res.r_above = TT_SPACE;
                       result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
                       SETPAINT(imageImage, lpPaint->l_type, result);
		  }
		  
	     }
	}
	else if (lpPaint->l_rbelow != TT_SPACE && 
				lpImage->l_type >= DBNumUserLayers)
	{
	     /* this type is an image, and we're already nuked the primary;
	     	time to bash this one, too.
	     */
	     res = lpImage->l_residues;
	     res.r_above = TT_SPACE;
	     result = dbTechMatchResidues(&res, TRUE,FALSE,TRUE);
	     SETPAINT(lpImage->l_type, lpPaint->l_type, result);
	}
	else if (lpImage->l_type < DBNumUserLayers)
	{
	     /* this is a two-residue contact on top of a 3 residue
	        contact. try to see if there is a compatible paintable
		contact of with lpImage->l_rabove = TT_SPACE */
	     res = lpImage->l_residues;
	     res.r_above = TT_SPACE;
	     result = dbTechMatchResidues(&res, TRUE,TRUE,FALSE);
	     SETPAINT(lpImage->l_type, lpPaint->l_type, result);
	     if (result == TT_SPACE && lpImage->l_rbelow != TT_SPACE)
	     {
	     	  /* have to zap the image, also */
		  TileType imageImage = dbImageOnPlane(lpImage->l_type, imagePNum-1);
 
		  res = dbLayerInfo[imageImage].l_residues;
		  res.r_above = TT_SPACE;
                  result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
                  SETPAINT(imageImage, lpPaint->l_type, result);
	     }
		
	}
    }
    else if (PlaneMaskHasPlane(lpPaint->l_pmask, imagePNum - 1)
	    && lpImage->l_rbelow != TT_SPACE)
    {
	if (lpImage->l_type < DBNumUserLayers &&
	    lpPaint->l_rabove != TT_SPACE)
	{
	     /* check to see if there is a compatible residue to connect
	        between lpImage and lpPaint.
	     */
	     res.r_residue = lpPaint->l_residues.r_above;
	     res.r_below   = lpPaint->l_residues.r_residue;
	     res.r_above   = lpImage->l_residues.r_residue;
	     if (dbTechMatchResidues(&res,TRUE,FALSE,TRUE) == TT_SPACE ||
	     	 lpImage->l_residues.r_below != lpPaint->l_residues.r_above)
	     
	     {
	     	  /* nothing compatible; have to terminate lpImage */
		  res = lpImage->l_residues;
		  res.r_below = TT_SPACE;
	          result = dbTechMatchResidues(&res, TRUE,TRUE,FALSE);
	          SETPAINT(lpImage->l_type, lpPaint->l_type, result);
		  if (result == TT_SPACE && lpImage->l_rabove != TT_SPACE)
		  {
		       /* we may have to blast lpImage's upper image */
		       TileType	imageImage = dbImageOnPlane(lpImage->l_type, imagePNum+1);
 
		       res = dbLayerInfo[imageImage].l_residues;
		       res.r_below = TT_SPACE;
                       result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
                       SETPAINT(imageImage, lpPaint->l_type, result);
		  }
		  
	     }
	}
	else if (lpPaint->l_rabove != TT_SPACE && 
				lpImage->l_type >= DBNumUserLayers)
	{
	     /* this type is an image, and we're already nuked the primary;
	     	time to bash this one, too.
	     */
	     res = lpImage->l_residues;
	     res.r_below = TT_SPACE;
	     result = dbTechMatchResidues(&res, TRUE,FALSE,TRUE);
	     SETPAINT(lpImage->l_type, lpPaint->l_type, result);
	}
	else if (lpImage->l_type < DBNumUserLayers)
	{
	     /* this is a two-residue contact on top of a 3 residue
	        contact. try to see if there is a compatible paintable
		contact of with lpImage->l_rbelow = TT_SPACE */
	     res = lpImage->l_residues;
	     res.r_below = TT_SPACE;
	     result = dbTechMatchResidues(&res, TRUE,TRUE,FALSE);
	     SETPAINT(lpImage->l_type, lpPaint->l_type, result);
	     if (result == TT_SPACE && lpImage->l_rabove != TT_SPACE)
	     {
	     	  /* have to zap the image, also */
		  TileType imageImage = dbImageOnPlane(lpImage->l_type, imagePNum+1);
 
		  res = dbLayerInfo[imageImage].l_residues;
		  res.r_below = TT_SPACE;
                  result = dbTechMatchResidues(&res, FALSE,FALSE,TRUE);
                  SETPAINT(imageImage, lpPaint->l_type, result);
	     }
	}
    }
}

/*
 * dbComposeEraseContact --
 *
 * Construct the erasing rules for erasing type lpErase from
 * the contact image lpImage.
 */

Void
dbComposeEraseContact(lpImage, lpErase)
    register LayerInfo *lpImage, *lpErase;
{
    int pImage = DBPlane(lpImage->l_type);
    Residues res, resErase;
    LayerInfo *lpEraseImage;
    TileType result;

    if (!PlaneMaskHasPlane(lpErase->l_pmask, pImage))
	return;

    /*
     * Only allow erasure if the residues of lpEraseImage are
     * compatible with those of lpImage, i.e, either identical
     * with lpImage's residues on those planes where lpImage
     * exists, or TT_SPACE.
     */
    lpEraseImage = &dbLayerInfo[dbImageOnPlane(lpErase->l_type, pImage)];
    if (!dbTechResiduesUnion(lpImage, lpEraseImage, &res))
	return;

    /*
     * Erasing lpErase from lpImage breaks the connection between
     * lpImage and the planes to which lpEraseImage connects.
     * Breaking a connection involves updating lpImage's tile
     * type and the types of any contacts on the connected planes
     * that connect to lpImage.
     */
    resErase = lpImage->l_residues;
    if (resErase.r_below != TT_SPACE && lpEraseImage->l_rbelow != TT_SPACE)
    {
	resErase.r_below = TT_SPACE;
	if (pImage < DBPlane(lpErase->l_type))
	{
	    RESBUILD(&res, -1, lpImage->l_rbelow, lpImage->l_residue);
	    (void) dbTechSrResidues(&res, FALSE, breakEraseBelow,
			    (ClientData) lpErase->l_type,TRUE,TRUE);
	}
    }

    if (resErase.r_above != TT_SPACE && lpEraseImage->l_rabove != TT_SPACE)
    {
	resErase.r_above = TT_SPACE;
	if (pImage > DBPlane(lpErase->l_type))
	{
	    RESBUILD(&res, lpImage->l_residue, lpImage->l_rabove, -1);
	    (void) dbTechSrResidues(&res, FALSE, breakEraseAbove,
			    (ClientData) lpErase->l_type,TRUE,TRUE);
	}
    }

    /* Set the result type for lpImage itself */
    result = dbTechMatchResidues(&resErase, TRUE,TRUE,TRUE);
    SETERASE(lpImage->l_type, lpErase->l_type, result);
}

int
breakEraseAbove(lp, type)
    register LayerInfo *lp;
    TileType type;
{
    Residues res;

    RESBUILD(&res, TT_SPACE, lp->l_residue, lp->l_rabove);
    SETERASE(lp->l_type, type, dbTechMatchResidues(&res, FALSE,TRUE,TRUE));
    return 0;
}

int
breakEraseBelow(lp, type)
    register LayerInfo *lp;
    TileType type;
{
    Residues res;

    RESBUILD(&res, lp->l_rbelow, lp->l_residue, TT_SPACE);
    SETERASE(lp->l_type, type, dbTechMatchResidues(&res, FALSE,TRUE,TRUE));
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbComposeSavedRules --
 *
 * Process all the contact compose/decompose rules saved
 * when the compose section of the tech file was read.
 * Each pair on the RHS of one of these rules must contain
 * exactly one contact type that spans the same set of planes
 * as the image type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies entries in the paint and erase tables.
 *
 * ----------------------------------------------------------------------------
 */

Void
dbComposeSavedRules()
{
    LayerInfo *lpContact;
    TileType imageType;
    TypePair *pair;
    Rule *rule;
    int n, p;

    for (n = 0; n < dbNumSavedRules; n++)
    {
	rule = &dbSavedRules[n];
	lpContact = &dbLayerInfo[rule->r_result];
	for (p = 0; p < lpContact->l_nresidues; p++)
	{
	    imageType = lpContact->l_images[p];
	    for (pair = rule->r_pairs;
		    pair < &rule->r_pairs[rule->r_npairs];
		    pair++)
	    {
		dbComposeDecompose(imageType, pair->rp_a, pair->rp_b);
		dbComposeDecompose(imageType, pair->rp_b, pair->rp_a);
		if (rule->r_ruleType == RULE_COMPOSE)
		{
		    dbComposeCompose(imageType, pair->rp_a, pair->rp_b);
		    dbComposeCompose(imageType, pair->rp_b, pair->rp_a);
		}
	    }
	}
    }
}

/*
 * dbComposeDecompose --
 *
 * Painting componentType over imageType is a no-op.
 * Erasing componentType from imageType gives either the image of
 * remainingType, if one exists on DBPlane(imageType), or else
 * gives the residue of imageType.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the paint/erase tables as described above.
 *	Indicates that these modifications are not default
 *	rules by setting the corresponding bits in the tables
 *	dbNotDefaultPaintTbl[] and dbNotDefaultEraseTbl[].
 */

Void
dbComposeDecompose(imageType, componentType, remainingType)
    TileType imageType;
    TileType componentType;
    TileType remainingType;
{
    int pNum = DBPlane(imageType);
    TileType resultType;

    /* Painting componentType is a no-op */
    dbSetPaintEntry(imageType, componentType, pNum, imageType);
    TTMaskSetType(&dbNotDefaultPaintTbl[imageType], componentType);

    /*
     * Erasing componentType gives remainingType or breaks
     * imageType's contact.
     */
    resultType = (PlaneMaskHasPlane(PlaneMask(remainingType), pNum))
			? dbImageOnPlane(remainingType, pNum)
			: dbLayerInfo[imageType].l_residue;
    dbSetEraseEntry(imageType, componentType, pNum, resultType);
    TTMaskSetType(&dbNotDefaultEraseTbl[imageType], componentType);
}

/*
 * dbComposeCompose --
 *
 * Painting paintType over existingType gives imageType.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the paint/erase tables as described above.
 *	Indicates that these modifications are not default
 *	rules by setting the corresponding bits in the tables
 *	dbNotDefaultPaintTbl[] and dbNotDefaultEraseTbl[].
 */

dbComposeCompose(imageType, existingType, paintType)
    TileType imageType;
    TileType existingType;
    TileType paintType;
{
    int pNum = DBPlane(imageType);
    TileType composeImage;

    if (PlaneMaskHasPlane(PlaneMask(existingType), pNum))
    {
	composeImage = dbImageOnPlane(existingType, pNum);
	dbSetPaintEntry(composeImage, paintType, pNum, imageType);
	TTMaskSetType(&dbNotDefaultPaintTbl[composeImage], paintType);
    }
}
