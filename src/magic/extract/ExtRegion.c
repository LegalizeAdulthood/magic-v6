/*
 * ExtRegion.c --
 *
 * Circuit extraction.
 * This file contains the code to trace out connected Regions
 * in a layout, and to build up or tear down lists of Regions.
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
static char rcsid[] = "$Header: ExtRegion.c,v 6.0 90/08/28 18:15:25 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "textio.h"
#include "debug.h"
#include "extract.h"
#include "extractInt.h"
#include "signals.h"

/*
 * ----------------------------------------------------------------------------
 *
 * ExtFindRegions --
 *
 * Find all the connected geometrical regions in a given area of a CellDef
 * that will correspond to nodes or devices in the extracted circuit.
 * Two procedures are supplied by the caller, 'first' and 'each'.
 *
 * The function 'first' must be non-NULL.  It is called for each tile
 * tile found in the region.  It must return a pointer to a Region
 * struct (or one of the client forms of a Region struct; see the
 * comments in extractInt.h).
 *
 *	Region *
 *	(*first)(tile, arg)
 *	    Tile *tile;		/# Tile is on plane arg->fra_pNum #/
 *	    FindRegion *arg;
 *	{
 *	}
 *
 * If the function 'each' is non-NULL, it is applied once to each tile found
 * in the region:
 *
 *	(*each)(tile, planeNum, arg)
 *	    Tile *tile;
 *	    int planeNum;	/# May be different than arg->fra_pNum #/
 *	    FindRegion *arg;
 *	{
 *	}
 *
 * Results:
 *	Returns a pointer to the first element in the linked list
 *	of Region structures for this CellDef.  The Region structs
 *	may in fact contain more than the basic Region struct; this
 *	will depend on what the function 'first' allocates.
 *
 * Side effects:
 *	Each non-space tile has its ti_client field left pointing
 *	to a Region structure that describes the region that tile
 *	belongs to.
 *
 * Non-interruptible.  It is the caller's responsibility to check
 * for interrupts.
 *	
 * ----------------------------------------------------------------------------
 */

Region *
ExtFindRegions(def, area, mask, connectsTo, uninit, first, each)
    CellDef *def;		/* Cell definition being searched */
    Rect *area;			/* Area to search initially for tiles */
    TileTypeBitMask *mask;	/* In the initial area search, only visit
				 * tiles whose types are in this mask.
				 */
    TileTypeBitMask *connectsTo;/* Connectivity table for determining regions.
				 * If t1 and t2 are the types of adjacent
				 * tiles, then t1 and t2 belong to the same
				 * region iff:
				 *	TTMaskHasType(&connectsTo[t1], t2)
				 *
				 * We assume that connectsTo[] is symmetric,
				 * so this is the same as:
				 *	TTMaskHasType(&connectsTo[t2], t1)
				 */
    ClientData uninit;		/* Contents of a ti_client field indicating
				 * that the tile has not yet been visited.
				 */
    Region * (*first)();	/* Applied to first tile in region */
    int (*each)();		/* Applied to each tile in region */
{
    FindRegion arg;
    int extRegionAreaFunc();

    ASSERT(first != NULL, "ExtFindRegions");
    arg.fra_connectsTo = connectsTo;
    arg.fra_def = def;
    arg.fra_uninit = uninit;
    arg.fra_first = first;
    arg.fra_each = each;
    arg.fra_region = (Region *) NULL;

    SigDisableInterrupts();
    for (arg.fra_pNum=PL_TECHDEPBASE; arg.fra_pNum<DBNumPlanes; arg.fra_pNum++)
	(void) DBSrPaintClient((Tile *) NULL, def->cd_planes[arg.fra_pNum],
		area, mask, uninit, extRegionAreaFunc, (ClientData) &arg);
    SigEnableInterrupts();

    return (arg.fra_region);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extRegionAreaFunc --
 *
 * Filter function called for each tile found during the area enumeration
 * in ExtFindRegions above.  Only tiles whose ti_client is not already
 * equal to arg->fra_uninit are visited.
 *
 * We call 'fra_first' to allocate a new region struct for it, and then
 * prepend it to the Region list (Region *) arg->fra_clientData.  We
 * then call ExtFindNeighbors to trace out recursively all the remaining
 * tiles in the region.
 *
 * Results:
 *	Always returns 0, to cause DBSrPaintClient to continue its search.
 *
 * Side effects:
 *	Allocates a new Region struct if the tile has not yet been visited.
 *	See also the comments for ExtFindNeighbors.
 *
 * ----------------------------------------------------------------------------
 */

extRegionAreaFunc(tile, arg)
    register Tile *tile;
    register FindRegion *arg;
{
    /* Allocate a new region */
    if (arg->fra_first)
	(void) (*arg->fra_first)(tile, arg);

    if (DebugIsSet(extDebugID, extDebAreaEnum))
	extShowTile(tile, "area enum", 0);

    /* Recursively visit all tiles surrounding this one that we connect to */
    (void) ExtFindNeighbors(tile, arg->fra_pNum, arg);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * ExtLabelRegions --
 *
 * Given a CellDef whose tiles have been set to point to LabRegions
 * by ExtFindRegions, walk down the label list and assign labels
 * to regions.  If the tile over which a label lies is still uninitialized
 * ie, points to extUnInit, we skip the label.
 *
 * A label is attached to the LabRegion for a tile if the label's
 * type and the tile's type are connected according to the table
 * 'connTo'.  This disambiguates the case where a label lies
 * on the boundary between two tiles of different types.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each LabRegion has labels added to its label list.
 *
 * ----------------------------------------------------------------------------
 */

ExtLabelRegions(def, connTo)
    register CellDef *def;		/* Cell definition being labelled */
    register TileTypeBitMask *connTo;	/* Connectivity table (see above) */
{
    static Point offsets[] = { { 0, 0 }, { 0, -1 }, { -1, -1 }, { -1, 0 } };
    register LabelList *ll;
    register Label *lab;
    register Tile *tp;
    LabRegion *reg;
    int quad, pNum;
    Point p;

    for (lab = def->cd_labels; lab; lab = lab->lab_next)
    {
	pNum = DBPlane(lab->lab_type);
	if (lab->lab_type == TT_SPACE || pNum < PL_TECHDEPBASE)
	    continue;
	for (quad = 0; quad < 4; quad++)
	{
	    /*
	     * Visit each of the four quadrants surrounding
	     * the lower-left corner of the label, searching
	     * for a tile whose type matches that of the label
	     * or connects to it.
	     */
	    p.p_x = lab->lab_rect.r_xbot + offsets[quad].p_x;
	    p.p_y = lab->lab_rect.r_ybot + offsets[quad].p_y;
	    tp = def->cd_planes[pNum]->pl_hint;
	    GOTOPOINT(tp, &p);
	    def->cd_planes[pNum]->pl_hint = tp;
	    if (extConnectsTo(TiGetType(tp), lab->lab_type, connTo)
		    && extHasRegion(tp, extUnInit))
	    {
		reg = (LabRegion *) extGetRegion(tp);
		MALLOC(LabelList *, ll, sizeof (LabelList));
		ll->ll_label = lab;
		ll->ll_next = reg->lreg_labels;
		ll->ll_attr = LL_NOATTR;
		reg->lreg_labels = ll;
		break;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * ExtLabelOneRegion --
 *
 * Same as ExtLabelRegion, but it only assigns labels to one particular
 * region.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The region has labels added to its label list.
 *
 * ----------------------------------------------------------------------------
 */
ExtLabelOneRegion(def, connTo, reg)
    register CellDef *def;		/* Cell definition being labelled */
    register TileTypeBitMask *connTo;	/* Connectivity table (see above) */
    NodeRegion  *reg;			/* The region whose labels we want */
{
    static Point offsets[] = { { 0, 0 }, { 0, -1 }, { -1, -1 }, { -1, 0 } };
    register LabelList *ll;
    register Label *lab;
    register Tile *tp;
    int quad, pNum;
    Point p;

    for (lab = def->cd_labels; lab; lab = lab->lab_next)
    {
	pNum = DBPlane(lab->lab_type);
	if (lab->lab_type == TT_SPACE || pNum < PL_TECHDEPBASE)
	    continue;
	for (quad = 0; quad < 4; quad++)
	{
	    /*
	     * Visit each of the four quadrants surrounding
	     * the lower-left corner of the label, searching
	     * for a tile whose type matches that of the label
	     * or connects to it.
	     */
	    p.p_x = lab->lab_rect.r_xbot + offsets[quad].p_x;
	    p.p_y = lab->lab_rect.r_ybot + offsets[quad].p_y;
	    tp = def->cd_planes[pNum]->pl_hint;
	    GOTOPOINT(tp, &p);
	    def->cd_planes[pNum]->pl_hint = tp;
	    if (extConnectsTo(TiGetType(tp), lab->lab_type, connTo)
		    && (NodeRegion *) extGetRegion(tp) == reg)
	    {
		MALLOC(LabelList *, ll, sizeof (LabelList));
		ll->ll_label = lab;
		ll->ll_next = reg->nreg_labels;
		ll->ll_attr = LL_NOATTR;
		reg->nreg_labels = ll;
		break;
	    }
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * ExtResetTiles --
 *
 * Given a CellDef whose tiles have been set to point to Regions
 * by ExtFindRegions, reset all the tiles to uninitialized.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All the non-space tiles in the CellDef have their ti_client
 *	fields set back to uninitialized.  Does not free the Region
 *	structs that these tiles point to; that must be done by
 *	ExtFreeRegions, ExtFreeLabRegions, or ExtFreeHierLabRegions.
 *
 * Non-interruptible.
 *
 * ----------------------------------------------------------------------------
 */

Void
ExtResetTiles(def, resetTo)
    register CellDef *def;
    ClientData resetTo;		/* New value for ti_client */
{
    register int pNum;

    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	DBResetTilePlane(def->cd_planes[pNum], resetTo);
}

/*
 * ----------------------------------------------------------------------------
 *
 * ExtFreeRegions --
 * ExtFreeLabRegions --
 * ExtFreeHierLabRegions --
 *
 * Free a list of Regions.
 * ExtFreeLabRegions also frees the LabelLists pointed to by lreg_labels.
 * ExtFreeHierLabRegions, in addition to freeing the LabelLists, frees
 * the labels they point to.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 * Non-interruptible.
 *
 * ----------------------------------------------------------------------------
 */

Void
ExtFreeRegions(regList)
    Region *regList;	/* List of regions to be freed */
{
    register Region *reg;

    for (reg = regList; reg; reg = reg->reg_next)
	FREE((char *) reg);
}

Void
ExtFreeLabRegions(regList)
    LabRegion *regList;	/* List of regions to be freed */
{
    register LabRegion *lreg;
    register LabelList *ll;

    for (lreg = regList; lreg; lreg = lreg->lreg_next)
    {
	for (ll = lreg->lreg_labels; ll; ll = ll->ll_next)
	    FREE((char *) ll);
	FREE((char *) lreg);
    }
}

Void
ExtFreeHierLabRegions(regList)
    Region *regList;	/* List of regions to be freed */
{
    register Region *reg;
    register LabelList *ll;

    for (reg = regList; reg; reg = reg->reg_next)
    {
	for (ll = ((LabRegion *)reg)->lreg_labels; ll; ll = ll->ll_next)
	{
	    FREE((char *) ll->ll_label);
	    FREE((char *) ll);
	}
	FREE((char *) reg);
    }
}
