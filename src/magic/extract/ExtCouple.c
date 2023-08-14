/*
 * ExtCouple.c --
 *
 * Circuit extraction.
 * Extraction of coupling capacitance.
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
static char rcsid[] = "$Header: ExtCouple.c,v 6.0 90/08/28 18:15:08 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "extract.h"
#include "extractInt.h"

/* --------------------- Data local to this file ---------------------- */

/* Pointer to hash table currently being updated with coupling capacitance */
HashTable *extCoupleHashPtr;

/* Clipping area for coupling searches */
Rect *extCoupleSearchArea;

/* Current list of sidewall capacitance rules */
EdgeCap *extCoupleList;
EdgeCap *extOverlapList;

/* Def being processed */
CellDef *extOverlapDef;

/* Forward procedure declarations */
int extBasicOverlap(), extBasicCouple();
int extAddOverlap(), extAddCouple();
int extSideLeft(), extSideRight(), extSideBottom(), extSideTop();
int extSideOverlap();

/*
 * ----------------------------------------------------------------------------
 *
 * extFindCoupling --
 *
 * Find the coupling capacitances in the cell def.  Such capacitances
 * arise from three causes:
 *
 *	Overlap.  When two tiles on different planes overlap, they
 *		  may have a coupling capacitance proportional to
 *		  their areas.  If this is so, we subtract the substrate
 *		  capacitance of the overlapped type, and add the overlap
 *		  capacitance to the coupling hash table.
 *
 *	Sidewall. When tiles on the same plane are adjacent, they may
 *		  have a coupling capacitance proportional to the
 *		  length of their edges, divided by the distance between
 *		  them.  In this case, we just add the sidewall coupling
 *		  capacitance to the hash table.
 *
 *	Sidewall
 *	overlap.  When the edge of a tile on one plane overlaps a tile
 *		  on a different plane, the two tiles may have a coupling
 *		  capacitance proportional to the length of the overlapping
 *		  edge.  In this case we add the coupling capacitance to the
 *		  hash table.  (We may want to deduct the perimeter capacitance
 *		  to substrate?).
 *
 * Requires that ExtFindRegions has been run on 'def' to label all its
 * tiles with NodeRegions.  Also requires that the HashTable 'table'
 * has been initialized by the caller.
 *
 * If 'clipArea' is non-NULL, search for overlap capacitance only inside
 * the area *clipArea.  Search for sidewall capacitance only from tiles
 * inside *clipArea, although this capacitance may be to tiles outside
 * *clipArea.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When done, the HashTable 'table' will have been filled
 *	in with an entry for each pair of nodes having coupling
 *	capacitance.  Each entry will have a two-word key organized
 *	as an CoupleKey struct, with ck_1 and ck_2 pointing to the
 *	coupled nodes.  The value of the hash entry will be the
 *	coupling capacitance between that pair of nodes.
 *
 * ----------------------------------------------------------------------------
 */

extFindCoupling(def, table, clipArea)
    CellDef *def;
    HashTable *table;
    Rect *clipArea;
{
    Rect *searchArea;
    int pNum;

    extCoupleHashPtr = table;
    extCoupleSearchArea = clipArea;
    searchArea = clipArea ? clipArea : &TiPlaneRect;
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
	if (PlaneMaskHasPlane(ExtCurStyle->exts_overlapPlanes, pNum))
	    (void) DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
			searchArea, &ExtCurStyle->exts_overlapTypes[pNum],
			extBasicOverlap, (ClientData) def);
	if (PlaneMaskHasPlane(ExtCurStyle->exts_sidePlanes, pNum))
	    (void) DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
			searchArea, &ExtCurStyle->exts_sideTypes[pNum],
			extBasicCouple, (ClientData) def);
    }

}

/*
 * ----------------------------------------------------------------------------
 *
 * extOutputCoupling --
 *
 * Output the coupling capacitance table built up by extFindCoupling().
 * Each entry in the hash table is a capacitance between the pair of
 * nodes identified by he->h_key, an CoupleKey struct.
 *
 * ExtFindRegions and ExtLabelRegions should have been called prior
 * to this procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See the comments above.
 *
 * ----------------------------------------------------------------------------
 */

extOutputCoupling(table, outFile)
    HashTable *table;	/* Coupling capacitance hash table */
    FILE *outFile;	/* Output file */
{
    int cround = ExtCurStyle->exts_capScale / 2;
    register HashEntry *he;
    register CoupleKey *ck;
    HashSearch hs;
    char *text;
    int cap;

    HashStartSearch(&hs);
    while (he = HashNext(table, &hs))
    {
	cap = (((int) HashGetValue(he)) + cround) / ExtCurStyle->exts_capScale;
	if (cap == 0)
	    continue;

	ck = (CoupleKey *) he->h_key.h_words;
	text = extNodeName((LabRegion *) ck->ck_1);
	(void) fprintf(outFile, "cap \"%s\" ", text);
	text = extNodeName((LabRegion *) ck->ck_2);
	(void) fprintf(outFile, "\"%s\" %d\n", text, cap);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extBasicOverlap --
 *
 * Filter function for overlap capacitance.
 * Called for each tile that might have coupling capacitance
 * to another node because it overlaps a tile or tiles in that
 * node.  Causes an area search over the area of 'tile' in all
 * planes to which 'tile' has overlap capacitance, for any tiles
 * to which 'tile' has overlap capacitance.
 *
 * Results:
 *	Returns 0 to keep DBSrPaintArea() going.
 *
 * Side effects:
 *	See extAddOverlap().
 *
 * ----------------------------------------------------------------------------
 */

extBasicOverlap(tile, def)
    Tile *tile;
    CellDef *def;
{
    int thisType = TiGetType(tile), thisPlane = DBPlane(thisType);
    int pNum, pMask = ExtCurStyle->exts_overlapOtherPlanes[thisType];
    TileTypeBitMask *tMask = &ExtCurStyle->exts_overlapOtherTypes[thisType];
    Rect r;

    TITORECT(tile, &r);
    extOverlapDef = def;
    if (extCoupleSearchArea)
	GEOCLIP(&r, extCoupleSearchArea);

    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
	/* Skip if nothing interesting on the other plane */
	if (pNum == thisPlane || !PlaneMaskHasPlane(pMask, pNum))
	    continue;

	(void) DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum], &r, tMask,
		extAddOverlap, (ClientData) tile);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extAddOverlap --
 *
 * We are called for each tile that is overlapped by the tile passed to
 * extBasicOverlap() above (our argument 'tabove').  The intent is that
 * 'tbelow' actually shields 'tabove' from the substrate, so we should
 * replace node(tabove)'s capacitance to substrate with a capacitance
 * to node(tbelow) whose size is proportional to the area of the overlap.
 *
 * We check to insure that tabove is not shielded from tbelow by any
 * intervening material; if it is, we deduct the capacitance between
 * node(tabove) and node(tbelow) for the area of the overlap.
 *
 * Results:
 *	Returns 0 to keep DBSrPaintArea() going.
 *
 * Side effects:
 *	Updates the HashEntry with key node(tbelow), node(tabove)
 *	by adding the capacitance of the overlap if node(tbelow)
 *	and node(tabove) are different, and if they are not totally
 *	shielded by intervening material.  Also subtracts the capacitance
 *	to substrate from node(tabove) for the area of the overlap.
 *	If node(tbelow) and node(tabove) are the same, we do nothing.
 *
 * ----------------------------------------------------------------------------
 */

struct overlap
{
    Rect		 o_clip;
    int			 o_area;
    int			 o_pmask;
    TileTypeBitMask	 o_tmask;
};

extAddOverlap(tbelow, tabove)
    register Tile *tbelow, *tabove;
{
    int extSubtractOverlap(), extSubtractOverlap2();
    register NodeRegion *rabove, *rbelow;
    register HashEntry *he;
    struct overlap ov;
    TileType ta, tb;
    CoupleKey ck;
    int c, pNum;

    /* Do nothing if both tiles are connected */
    rabove = (NodeRegion *) extGetRegion(tabove);
    rbelow = (NodeRegion *) extGetRegion(tbelow);
    if (rabove == rbelow)
	return (0);

    /* Compute the area of overlap */
    ov.o_clip.r_xbot = MAX(LEFT(tbelow), LEFT(tabove));
    ov.o_clip.r_xtop = MIN(RIGHT(tbelow), RIGHT(tabove));
    ov.o_clip.r_ybot = MAX(BOTTOM(tbelow), BOTTOM(tabove));
    ov.o_clip.r_ytop = MIN(TOP(tbelow), TOP(tabove));
    if (extCoupleSearchArea)
	GEOCLIP(&ov.o_clip, extCoupleSearchArea);
    ov.o_area = (ov.o_clip.r_ytop - ov.o_clip.r_ybot)
	      * (ov.o_clip.r_xtop - ov.o_clip.r_xbot);
    ta = TiGetType(tabove);
    tb = TiGetType(tbelow);

    /*
     * Find whether rabove and rbelow are shielded by intervening material.
     * Deduct the area shielded from the area of the overlap, so we adjust
     * the overlap capacitance correspondingly.
     */
    if (ov.o_pmask = ExtCurStyle->exts_overlapShieldPlanes[ta][tb])
    {
	ov.o_tmask = ExtCurStyle->exts_overlapShieldTypes[ta][tb];
	for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	{
	    if (!PlaneMaskHasPlane(ov.o_pmask, pNum)) continue;
	    ov.o_pmask &= ~(PlaneNumToMaskBit(pNum));
	    if (ov.o_pmask == 0)
	    {
		(void) DBSrPaintArea((Tile *) NULL,
		    extOverlapDef->cd_planes[pNum], &ov.o_clip, &ov.o_tmask,
		    extSubtractOverlap, (ClientData) &ov);
	    }
	    else
	    {
		(void) DBSrPaintArea((Tile *) NULL,
		    extOverlapDef->cd_planes[pNum], &ov.o_clip, &DBAllTypeBits,
		    extSubtractOverlap2, (ClientData) &ov);
	    }
	    break;
	}
    }

    /* If any capacitance remains, add this record to the table */
    if (ov.o_area > 0)
    {
	/*
	 * Subtract the substrate capacitance from tabove's region due to
	 * the area of the overlap, minus any shielded area.  The shielded
	 * areas get handled later, when processing coupling between tabove
	 * and the shielding tile.  (Tabove was the overlapping tile, so it
	 * is shielded from the substrate by tbelow).
	 */
	rabove->nreg_cap -= ExtCurStyle->exts_areaCap[ta] * ov.o_area;

	/* Find the coupling hash record */
	if (rabove < rbelow) ck.ck_1 = rabove, ck.ck_2 = rbelow;
	else ck.ck_1 = rbelow, ck.ck_2 = rabove;
	he = HashFind(extCoupleHashPtr, (char *) &ck);

	/* Add the overlap capacitance to the table */
	c = ExtCurStyle->exts_overlapCap[ta][tb] * ov.o_area;
	c += (int) HashGetValue(he);
	HashSetValue(he, c);
    }

    return (0);
}

int
extSubtractOverlap(tile, ov)
    register Tile *tile;
    register struct overlap *ov;
{
    Rect r;
    int area;

    TITORECT(tile, &r);
    GEOCLIP(&r, &ov->o_clip);
    area = (r.r_xtop - r.r_xbot) * (r.r_ytop - r.r_ybot);
    if (area > 0)
	ov->o_area -= area;

    return (0);
}

int
extSubtractOverlap2(tile, ov)
    register Tile *tile;
    register struct overlap *ov;
{
    struct overlap ovnew;
    int area, pNum;
    Rect r;

    TITORECT(tile, &r);
    GEOCLIP(&r, &ov->o_clip);
    area = (r.r_xtop - r.r_xbot) * (r.r_ytop - r.r_ybot);
    if (area <= 0)
	return (0);

    /* This tile shields everything below */
    if (TTMaskHasType(&ov->o_tmask, TiGetType(tile)))
    {
	ov->o_area -= area;
	return (0);
    }

    /* Tile doesn't shield, so search next plane */
    ovnew = *ov;
    ovnew.o_clip = r;
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
	if (!PlaneMaskHasPlane(ovnew.o_pmask, pNum)) continue;
	ovnew.o_pmask &= ~(PlaneNumToMaskBit(pNum));
	if (ovnew.o_pmask == 0)
	{
	    (void) DBSrPaintArea((Tile *) NULL,
		extOverlapDef->cd_planes[pNum], &ovnew.o_clip, &ovnew.o_tmask,
		extSubtractOverlap, (ClientData) &ovnew);
	}
	else
	{
	    (void) DBSrPaintArea((Tile *) NULL,
		extOverlapDef->cd_planes[pNum], &ovnew.o_clip, &DBAllTypeBits,
		extSubtractOverlap2, (ClientData) &ovnew);
	}
	break;
    }
    ov->o_area = ovnew.o_area;

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extBasicCouple --
 *
 * Filter function for sidewall coupling capacitance.
 * Called for each tile that might have coupling capacitance
 * to another node because it is near tiles on the same plane,
 * or because its edge overlaps tiles on a different plane.
 *
 * Causes an area search over a halo surrounding each edge of
 * 'tile' for edges to which each edge has coupling capacitance
 * on this plane, and a search for tiles on different planes that
 * this edge overlaps.
 *
 * Results:
 *	Returns 0 to keep DBSrPaintArea() going.
 *
 * Side effects:
 *	See extAddCouple().
 *
 * ----------------------------------------------------------------------------
 */

extBasicCouple(tile, def)
    Tile *tile;
    CellDef *def;
{
    (void) extEnumTilePerim(tile, ExtCurStyle->exts_sideEdges[TiGetType(tile)],
			extAddCouple, (ClientData) def);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extAddCouple --
 *
 * Called for each segment along the boundary of the tile bp->b_inside
 * that might have coupling capacitance with its neighbors.
 * Causes an area search over a halo surrounding the boundary bp->b_segment
 * on the side outside bp->b_inside for edges to which this one has coupling
 * capacitance on this plane, and for tiles overlapping this edge on different
 * planes.
 *
 * Results:
 *	Returns 0 to keep DBSrPaintArea() going.
 *
 * Side effects:
 *	For each edge (tnear, tfar) we find that has coupling capacitance
 *	to us, update the HashEntry with key node(bp->b_inside), node(tfar)
 *	by adding the sidewall capacitance if node(bp->b_inside) and node(tfar)
 *	are different.  If node(bp->b_inside) and node(tfar) are the same, we
 *	do nothing.
 *
 *	For each tile tp we find on a different plane that overlaps this
 *	edge, update the HashEntry with key node(bp->b_inside), node(tp)
 *	by adding the sidewall overlap capacitance.  If node(bp->b_inside)
 *	and node(tp) are the same, do nothing.
 *
 * ----------------------------------------------------------------------------
 */

extAddCouple(bp, def)
    Boundary *bp;	/* Boundary being considered */
    CellDef *def;	/* Def containing this boundary */
{
    TileType tin = TiGetType(bp->b_inside), tout = TiGetType(bp->b_outside);
    int pNum, pMask, (*proc)();
    Boundary bpCopy;
    Rect r, ovr;

    extCoupleList = ExtCurStyle->exts_sideCoupleCap[tin][tout];
    extOverlapList = ExtCurStyle->exts_sideOverlapCap[tin][tout];
    if (extCoupleList == NULL && extOverlapList == NULL)
	return (0);

    /*
     * Clip the edge of interest to the area where we're searching
     * for coupling capacitance, if such an area has been specified.
     */
    if (extCoupleSearchArea)
    {
	bpCopy = *bp;
	bp = &bpCopy;
	GEOCLIP(&bp->b_segment, extCoupleSearchArea);
	if (GEO_RECTNULL(&bp->b_segment))
	    return (0);
    }
    r = ovr = bp->b_segment;

    switch (bp->b_direction)
    {
	case BD_UP:	/* Along left */
	    r.r_xbot -= ExtCurStyle->exts_sideCoupleHalo	;
	    ovr.r_xbot -= 1;
	    proc = extSideLeft;
	    break;
	case BD_DOWN:	/* Along right */
	    r.r_xtop += ExtCurStyle->exts_sideCoupleHalo	;
	    ovr.r_xtop += 1;
	    proc = extSideRight;
	    break;
	case BD_LEFT:	/* Along top */
	    r.r_ytop += ExtCurStyle->exts_sideCoupleHalo	;
	    ovr.r_ytop += 1;
	    proc = extSideTop;
	    break;
	case BD_RIGHT:	/* Along bottom */
	    r.r_ybot -= ExtCurStyle->exts_sideCoupleHalo	;
	    ovr.r_ybot -= 1;
	    proc = extSideBottom;
	    break;
    }

    if (extCoupleList)
	(void) DBSrPaintArea((Tile *) NULL, def->cd_planes[DBPlane(tin)],
		&r, &ExtCurStyle->exts_sideCoupleOtherEdges[tin][tout],
		proc, (ClientData) bp);

    if (extOverlapList)
    {
	pMask = ExtCurStyle->exts_sideOverlapOtherPlanes[tin][tout];
	for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	    if (PlaneMaskHasPlane(pMask, pNum))
	    {
		bp->b_plane = pNum;
		(void) DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
			&ovr, &ExtCurStyle->exts_sideOverlapOtherTypes[tin][tout],
			extSideOverlap, (ClientData) bp);
	    }
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideOverlap --
 *
 * The boundary 'bp' has been found to overlap the tile 'tp', which it
 * has coupling capacitance to.
 *
 * Results:
 *	Returns 0 to keep DBSrPaintArea() going.
 *
 * Side effects:
 *	Update the coupling capacitance between node(bp->t_inside) and
 *	node(tp) if the two nodes are different.  Does so by updating
 *	the value stored in the HashEntry keyed by the two nodes.
 *
 * ----------------------------------------------------------------------------
 */

extSideOverlap(tp, bp)
    register Tile *tp;		/* Overlapped tile */
    register Boundary *bp;	/* Overlapping edge */
{
    NodeRegion *rtp = (NodeRegion *) extGetRegion(tp);
    NodeRegion *rbp = (NodeRegion *) extGetRegion(bp->b_inside);
    register HashEntry *he;
    register EdgeCap *e;
    int length, cap;
    CoupleKey ck;

    if (rtp == rbp)
	return (0);

    if (bp->b_segment.r_xtop == bp->b_segment.r_xbot)
    {
	length = MIN(bp->b_segment.r_ytop, TOP(tp))
	       - MAX(bp->b_segment.r_ybot, BOTTOM(tp));
    }
    else
    {
	length = MIN(bp->b_segment.r_xtop, RIGHT(tp))
	       - MAX(bp->b_segment.r_xbot, LEFT(tp));
    }

    /* Is tp a space tile? If so, extGetRegion points to garbage;  	*/
    /* make terminal 2 point to ground					*/
    
    if (TiGetType(tp) == TT_SPACE)
    {
    	for (e = extOverlapList; e; e = e->ec_next)
	{
	     if (TTMaskHasType(&e->ec_near,TT_SPACE) && 
	     	  e->ec_plane == bp->b_plane)
	     {
	     	  rbp->nreg_cap += e->ec_cap*length;
	     }
	}
    }
    else
    {
    	  if (rtp < rbp) ck.ck_1 = rtp, ck.ck_2 = rbp;
    		else ck.ck_1 = rbp, ck.ck_2 = rtp;
    	  he = HashFind(extCoupleHashPtr, (char *) &ck);
    	  cap = (int) HashGetValue(he);
    	  for (e = extOverlapList; e; e = e->ec_next)
		if (TTMaskHasType(&e->ec_near, TiGetType(tp)))
	    		cap += e->ec_cap * length;
    	  HashSetValue(he, cap);
    }
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideLeft --
 *
 * Searching to the left of the boundary 'bp', we found the tile
 * 'tpfar' which may lie on the far side of an edge to which the
 * edge bp->b_inside | bp->b_outside has sidewall coupling capacitance.
 *
 * Walk along the right-hand side of 'tpfar' searching for such
 * edges, and recording their capacitance in the hash table
 * *extCoupleHashPtr.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	If node(tpfar) exists, and node(bp->b_inside) != node(tpfar),
 *	search along the inside edge of tpfar (the one closest to
 *	the boundary bp) for edges having capacitance with bp.  For
 *	each such edge found, update the entry in *extCoupleHashPtr
 *	identified by node(bp->b_inside) and node(tpfar) by adding
 *	the capacitance due to the adjacency of the pair of edges.
 *
 * ----------------------------------------------------------------------------
 */

extSideLeft(tpfar, bp)
    register Tile *tpfar;
    register Boundary *bp;
{
    NodeRegion *rinside = (NodeRegion *) extGetRegion(bp->b_inside);
    NodeRegion *rfar = (NodeRegion *) extGetRegion(tpfar);
    register Tile *tpnear;

    if (rfar != (NodeRegion *) extUnInit && rfar != rinside)
    {
	int sep = bp->b_segment.r_xbot - RIGHT(tpfar);
	int limit = MAX(bp->b_segment.r_ybot, BOTTOM(tpfar));
	int start = MIN(bp->b_segment.r_ytop, TOP(tpfar));

	for (tpnear = TR(tpfar); TOP(tpnear) > limit; tpnear = LB(tpnear))
	{
	    int overlap = MIN(TOP(tpnear), start) - MAX(BOTTOM(tpnear), limit);

	    if (overlap > 0)
		extSideCommon(rinside, rfar, tpnear, tpfar, overlap, sep);
	}
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideRight --
 *
 * Searching to the right of the boundary 'bp', we found the tile
 * 'tpfar' which may lie on the far side of an edge to which the
 * edge bp->b_inside | bp->b_outside has sidewall coupling capacitance.
 *
 * Walk along the left-hand side of 'tpfar' searching for such
 * edges, and recording their capacitance in the hash table
 * *extCoupleHashPtr.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See extSideLeft.
 *
 * ----------------------------------------------------------------------------
 */

extSideRight(tpfar, bp)
    register Tile *tpfar;
    register Boundary *bp;
{
    NodeRegion *rinside = (NodeRegion *) extGetRegion(bp->b_inside);
    NodeRegion *rfar = (NodeRegion *) extGetRegion(tpfar);
    register Tile *tpnear;

    if (rfar != (NodeRegion *) extUnInit && rfar != rinside)
    {
	int sep = LEFT(tpfar) - bp->b_segment.r_xtop;
	int limit = MIN(bp->b_segment.r_ytop, TOP(tpfar));
	int start = MAX(bp->b_segment.r_ybot, BOTTOM(tpfar));

	for (tpnear = BL(tpfar); BOTTOM(tpnear) < limit; tpnear = RT(tpnear))
	{
	    int overlap = MIN(TOP(tpnear), limit) - MAX(BOTTOM(tpnear), start);

	    if (overlap > 0)
		extSideCommon(rinside, rfar, tpnear, tpfar, overlap, sep);
	}
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideTop --
 *
 * Searching to the top of the boundary 'bp', we found the tile
 * 'tpfar' which may lie on the far side of an edge to which the
 * edge bp->b_inside | bp->b_outside has sidewall coupling capacitance.
 *
 * Walk along the bottom side of 'tpfar' searching for such
 * edges, and recording their capacitance in the hash table
 * *extCoupleHashPtr.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See extSideLeft.
 *
 * ----------------------------------------------------------------------------
 */

extSideTop(tpfar, bp)
    register Tile *tpfar;
    register Boundary *bp;
{
    NodeRegion *rinside = (NodeRegion *) extGetRegion(bp->b_inside);
    NodeRegion *rfar = (NodeRegion *) extGetRegion(tpfar);
    register Tile *tpnear;

    if (rfar != (NodeRegion *) extUnInit && rfar != rinside)
    {
	int sep = BOTTOM(tpfar) - bp->b_segment.r_ytop;
	int limit = MIN(bp->b_segment.r_xtop, RIGHT(tpfar));
	int start = MAX(bp->b_segment.r_xbot, LEFT(tpfar));

	for (tpnear = LB(tpfar); LEFT(tpnear) < limit; tpnear = TR(tpnear))
	{
	    int overlap = MIN(RIGHT(tpnear), limit) - MAX(LEFT(tpnear), start);

	    if (overlap > 0)
		extSideCommon(rinside, rfar, tpnear, tpfar, overlap, sep);
	}
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideBottom --
 *
 * Searching to the bottom of the boundary 'bp', we found the tile
 * 'tpfar' which may lie on the far side of an edge to which the
 * edge bp->b_inside | bp->b_outside has sidewall coupling capacitance.
 *
 * Walk along the top side of 'tpfar' searching for such
 * edges, and recording their capacitance in the hash table
 * *extCoupleHashPtr.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See extSideLeft.
 *
 * ----------------------------------------------------------------------------
 */

extSideBottom(tpfar, bp)
    register Tile *tpfar;
    register Boundary *bp;
{
    NodeRegion *rinside = (NodeRegion *) extGetRegion(bp->b_inside);
    NodeRegion *rfar = (NodeRegion *) extGetRegion(tpfar);
    register Tile *tpnear;

    if (rfar != (NodeRegion *) extUnInit && rfar != rinside)
    {
	int sep = bp->b_segment.r_ybot - TOP(tpfar);
	int limit = MAX(bp->b_segment.r_xbot, LEFT(tpfar));
	int start = MIN(bp->b_segment.r_xtop, RIGHT(tpfar));

	for (tpnear = RT(tpfar); RIGHT(tpnear) > limit; tpnear = BL(tpnear))
	{
	    int overlap = MIN(RIGHT(tpnear), start) - MAX(LEFT(tpnear), limit);

	    if (overlap > 0)
		extSideCommon(rinside, rfar, tpnear, tpfar, overlap, sep);
	}
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSideCommon --
 *
 * Perform the actual update to the hash table entry for
 * the regions 'rinside' and 'rfar'.  We assume that neither
 * 'rinside' nor 'rfar' are extUnInit, and further that they
 * are not equal.
 *
 * Walk along the rules in extCoupleList, applying the appropriate
 * amount of capacitance for an edge with tpnear on the close side
 * and tpfar on the remote side.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See extSideLeft.
 *
 * ----------------------------------------------------------------------------
 */

extSideCommon(rinside, rfar, tpnear, tpfar, overlap, sep)
    NodeRegion *rinside, *rfar;	/* Both must be valid */
    Tile *tpnear, *tpfar;	/* Tiles on near and far side of edge */
    int overlap, sep;		/* Overlap of this edge with original one,
				 * and distance between the two.
				 */
{
    TileType near = TiGetType(tpnear), far = TiGetType(tpfar);
    register HashEntry *he;
    register EdgeCap *e;
    CoupleKey ck;
    int cap;

    if (rinside < rfar) ck.ck_1 = rinside, ck.ck_2 = rfar;
    else ck.ck_1 = rfar, ck.ck_2 = rinside;
    he = HashFind(extCoupleHashPtr, (char *) &ck);

    cap = (int) HashGetValue(he);
    for (e = extCoupleList; e; e = e->ec_next)
	if (TTMaskHasType(&e->ec_near, near) && TTMaskHasType(&e->ec_far, far))
	    cap += (e->ec_cap * overlap) / sep;
    HashSetValue(he, cap);
}
