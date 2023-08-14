/*
 * DRCtech.c --
 *
 * Technology initialization for the DRC module.
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
static char rcsid[] = "$Header: DRCtech.c,v 6.0 90/08/28 18:12:35 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "utils.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"
#include "tech.h"
#include "textio.h"
#include "malloc.h"

/*
 * Largest DRC interaction radius in the given technology
 */
global int TechHalo;

/*
 * List of rules applicable for each possible pair of edges.
 */
global DRCCookie      * DRCRulesTbl [TT_MAXTYPES] [TT_MAXTYPES];

/* The paint table defined below is used when yanking paint for subcell
 * interaction checks.  It turns some kinds of overlaps into automatic
 * errors.
 */

global PaintResultType DRCPaintTable[NP][NT][NT];

/* The mask below defines tile types that are not permitted to overlap
 * themselves across cells unless the overlap is exact (each cell
 * contains exactly the same material).
 */

global TileTypeBitMask DRCExactOverlapTypes;

/* The following variable can be set to zero to turn off
 * any optimizations of design rule lists.
 */

global int DRCRuleOptimization = TRUE;

/* The following variables count how many rules were specified by
 * the technology file and how many edge rules were optimized away.
 */

static int drcRulesSpecified = 0;
static int drcRulesOptimized = 0;

/*
 * Forward declarations.
 */
int drcWidth(), drcSpacing(), drcEdge(), drcNoOverlap(), drcExactOverlap();
int drcStepSize();

/*
 * ----------------------------------------------------------------------------
 * DRCTechInit --
 *
 * Initialize the technology-specific variables for the DRC module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears out all the DRC tables.
 * ----------------------------------------------------------------------------
 */

Void
DRCTechInit()
{
    register int i, j, plane;
    register DRCCookie *dp;

    TechHalo = 0;
    drcRulesOptimized = 0;
    drcRulesSpecified = 0;
    TTMaskZero(&DRCExactOverlapTypes);

    /* Remove all old rules from the DRC rules table and put a dummy
     * rule at the front of each list.
     */

    for (i = 0; i < TT_MAXTYPES; i++)
    {
	for (j = 0; j < TT_MAXTYPES; j++)
	{
	    for (dp = DRCRulesTbl[i][j]; dp != NULL; dp = dp->drcc_next)
	    {
		(void) StrDup (&(dp->drcc_why), (char *) NULL);
		FREE( (char *) dp );
	    }
	    MALLOC(DRCCookie *, dp, sizeof (DRCCookie));
	    dp->drcc_dist = -1;
	    dp->drcc_next = (DRCCookie *) NULL;
	    DRCRulesTbl[i][j] = dp;
	}
    }

    /* Copy the default paint table into the DRC paint table.  The DRC
     * paint table will be modified as we read the drc section.  Also
     * make sure that the error layer is super-persistent (once it
     * appears, it can't be gotten rid of by painting).  Also, make
     * some crossings automatically illegal:  two layers can't cross
     * unless the result of painting one on top of the other is to
     * get one of the layers, and it doesn't matter which is painted
     * on top of which.
     */
    
    for (plane = 0; plane < DBNumPlanes; plane++)
    {
	/* printf("Plane is %s.\n", DBPlaneLongName(plane)); */
	for (i = 0; i < DBNumTypes; i++)
	    for (j = 0; j < DBNumTypes; j++)
	    {
		PaintResultType result = DBPaintResultTbl[plane][i][j];
		if ((i == TT_ERROR_S) || (j == TT_ERROR_S))
		    DRCPaintTable[plane][i][j] = TT_ERROR_S;
		else if ((i == TT_SPACE) || (j == TT_SPACE)
			|| (DBPlane(j) != plane)
			|| !DBPaintOnPlane(i, DBPlane(j)))
		    DRCPaintTable[plane][i][j] = result;
		else if ((!TTMaskHasType(&DBLayerTypeMaskTbl[i], result)
			&& !TTMaskHasType(&DBLayerTypeMaskTbl[j], result))
			|| ((result != DBPaintResultTbl[plane][j][i])
			&& (DBPlane(i) == plane)
			&& DBPaintOnPlane(j, DBPlane(i))))
		{
		    DRCPaintTable[plane][i][j] = TT_ERROR_S;
		    /* printf("Error: %s on %s, was %s\n", DBTypeShortName(i),
			    DBTypeShortName(j), DBTypeShortName(result)); */
		}
		else
		    DRCPaintTable[plane][i][j] = result;
	    }
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCTechAddRule --
 *
 * Add a new entry to the DRC table.
 *
 * Results:
 *	Always returns TRUE so that tech file read-in doesn't abort.
 *
 * Side effects:
 *	Updates the DRC technology variables.
 *
 * Organization:
 *	We select a procedure based on the first keyword (argv[0])
 *	and call it to do the work of implementing the rule.  Each
 *	such procedure is of the following form:
 *
 *	int
 *	proc(argc, argv)
 *	    int argc;
 *	    char *argv[];
 *	{
 *	}
 *
 * 	It returns the distance associated with the design rule,
 *	or -1 in the event of a fatal error that should cause
 *	DRCTechAddRule() to return FALSE (currently, none of them
 *	do, so we always return TRUE).  If there is no distance
 *	associated with the design rule, 0 is returned.
 *
 * ----------------------------------------------------------------------------
 */

#define ASSIGN(cookie,dist,next,mask,corner,why,cdist,flags,plane) \
	((cookie)->drcc_dist = dist, \
	(cookie)->drcc_next = next, \
	(cookie)->drcc_mask = mask, \
	(cookie)->drcc_corner = corner, \
	(cookie)->drcc_why = StrDup ((char **) NULL, why), \
	(cookie)->drcc_cdist = cdist, \
	(cookie)->drcc_flags = flags, \
	(cookie)->drcc_plane = plane)

	/* ARGSUSED */
bool
DRCTechAddRule(sectionName, argc, argv)
    char *sectionName;		/* Unused */
    int argc;
    char *argv[];
{
    int which, distance;
    char *fmt;
    static struct
    {
	char	*rk_keyword;	/* Initial keyword */
	int	 rk_minargs;	/* Min # arguments */
	int	 rk_maxargs;	/* Max # arguments */
	int    (*rk_proc)();	/* Procedure implementing this keyword */
	char	*rk_err;	/* Error message */
    } ruleKeys[] = {
	"edge",		 8,	9,	drcEdge,
    "layers1 layers2 distance okTypes cornerTypes cornerDistance why [plane]",
	"edge4way",	 8,	9,	drcEdge,
    "layers1 layers2 distance okTypes cornerTypes cornerDistance why [plane]",
	"exact_overlap", 2,	2,	drcExactOverlap,
    "layers",
	"no_overlap",	 3,	3,	drcNoOverlap,
    "layers1 layers2",
	"spacing",	 6,	6,	drcSpacing,
    "layers1 layers2 separation adjacency why",
	"stepsize",	 2,	2,	drcStepSize,
    "step_size",
	"width",	 4,	4,	drcWidth,
    "layers width why",
	0
    }, *rp;

    drcRulesSpecified += 1;

    which = LookupStruct(argv[0], (LookupTable *) ruleKeys, sizeof ruleKeys[0]);
    if (which < 0)
    {
	TechError("Bad DRC rule type \"%s\"\n", argv[0]);
	TxError("Valid rule types are:\n");
	for (fmt = "%s", rp = ruleKeys; rp->rk_keyword; rp++, fmt = ", %s")
	    TxError(fmt, rp->rk_keyword);
	TxError(".\n");
	return (TRUE);
    }
    rp = &ruleKeys[which];
    if (argc < rp->rk_minargs || argc > rp->rk_maxargs)
    {
	TechError("Rule type \"%s\" usage: %s %s\n",
		rp->rk_keyword, rp->rk_keyword, rp->rk_err);
	return (TRUE);
    }

    distance = (*rp->rk_proc)(argc, argv);
    if (distance < 0)
	return (FALSE);

    /* Update the halo to be the maximum distance of any design rule */
    if (distance > TechHalo)
	TechHalo = distance;

    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcWidth --
 *
 * Process a width rule.
 * This is of the form:
 *
 *	width layers distance why
 *
 * e.g,
 *
 *	width poly,pmc 2 "poly width must be at least 2"
 *
 * Results:
 *	Returns distance.
 *
 * Side effects:
 *	Updates the DRC technology variables.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
drcWidth(argc, argv)
    int argc;
    char *argv[];
{
    char *layers = argv[1];
    int distance = atoi(argv[2]);
    char *why = argv[3];
    TileTypeBitMask set, setC, tmp1;
    DRCCookie *dp, *dpnew;
    TileType i, j;
    int plane;

    DBTechNoisyNameMask(layers, &set);
    for (plane = PL_TECHDEPBASE; plane < DBNumPlanes; plane++)
	if (DBTechSubsetLayers(set, DBPlaneTypes[plane], &tmp1))
	    goto widthOK;
    TechError("All layers for \"width\" must be on same plane\n");
    return (0);

widthOK:
    set = tmp1;
    TTMaskCom2(&setC, &set);

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    /*
	     * Must have types in 'set' for at least 'distance'
	     * to the right of any edge between a type in '~set'
	     * and a type in 'set'.
	     */
	    if (SamePlane(i, j)
		    && TTMaskHasType(&setC, i) && TTMaskHasType(&set, j))
	    {
		/* find bucket preceding the new one we wish to insert */
		for (dp = DRCRulesTbl [i][j];
			 dp->drcc_next != (DRCCookie *) NULL &&
			 dp->drcc_next->drcc_dist < distance;
			     dp = dp->drcc_next)
		    ; /* null body */

		MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		ASSIGN(dpnew, distance, dp->drcc_next, set, set, why,
			    distance, DRC_FORWARD, plane);

		dp->drcc_next = dpnew;
	    }
	}
    }

    return (distance);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcSpacing --
 *
 * Process a spacing rule.
 * This is of the form:
 *
 *	spacing layers1 layers2 distance adjacency why
 *
 * e.g,
 *
 *	spacing metal,pmc/m,dmc/m metal,pmc/m,dmc/m 4 touching_ok \
 *		"metal spacing must be at least 4"
 *
 * Adjacency may be either "touching_ok" or "touching_illegal"
 * In the first case, no violation occurs when types in layers1 are
 * immediately adjacent to types in layers2.  In the second case,
 * such adjacency causes a violation.
 *
 * Results:
 *	Returns distance.
 *
 * Side effects:
 *	Updates the DRC technology variables.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
drcSpacing(argc, argv)
    int argc;
    char *argv[];
{
    char *layers1 = argv[1], *layers2 = argv[2];
    int distance = atoi(argv[3]);
    char *adjacency = argv[4];
    char *why = argv[5];
    TileTypeBitMask set1, set2, tmp1, tmp2, setR, setRreverse;
    int plane,curPlane, planes1, planes2;
    DRCCookie *dp, *dpnew;
    int needReverse = FALSE;
    TileType i, j;

    DBTechNoisyNameMask(layers1, &set1);
    planes1 = DBTechMinSetPlanes(set1, &set1);
    DBTechNoisyNameMask(layers2, &set2);
    planes2 = DBTechMinSetPlanes(set2, &set2);

    /* Look for plane that contains image of all types in both
     * set1 and set2.
     * NOTE:  plane set to this plane, or DBNumPlanes if none found.
     */
    for (plane = PL_TECHDEPBASE; plane < DBNumPlanes; plane++)
    {
	if (DBTechSubsetLayers(set1, DBPlaneTypes[plane], &tmp1)
		&& DBTechSubsetLayers(set2, DBPlaneTypes[plane], &tmp2))
	    break;
    }

    if (strcmp (adjacency, "touching_ok") == 0)
    {
	/* If touching is OK, everything must fall in the same plane. */
	if (plane == DBNumPlanes)
	{
	    TechError(
		"Spacing check with touching ok must all be in one plane.\n");
	    return (0);
	}

	/* In "touching_ok rules, spacing to set2  is be checked in FORWARD 
	 * direction at edges between set1 and  (setR = ~set1 AND ~set2).
	 *
	 * In addition, spacing to set1 is checked in FORWARD direction 
	 * at edges between set2 and (setRreverse = ~set1 AND ~set2).
	 *
	 * If set1 and set2 are different, above are checked in REVERSE as
	 * well as forward direction.  This is important since touching
	 * material frequently masks violations in one direction.
	 *
	 * setR and setRreverse are set appropriately below.
	 */

	tmp1 = set1;
	tmp2 = set2; 

	planes1 = planes2 = PlaneNumToMaskBit(plane);
	TTMaskCom(&tmp1);
	TTMaskCom(&tmp2);
	TTMaskAndMask(&tmp1, &tmp2);
	setR = tmp1;
	setRreverse = tmp1;

	/* If set1 = set2, set flag to check rules in both directions */
	if (!TTMaskEqual(&set1, &set2))
	    needReverse = TRUE;
    }
    else if (strcmp (adjacency, "touching_illegal") == 0)
    {
	/* If touching is illegal, set1 and set2 should not intersect 
	 * (adjacencies between types on the same plane would be missed)
	 */
	if (TTMaskIntersect(&set1, &set2))
	{
	    TechError(
		"Spacing check with touching illegal must be between non intersecting type lists.\n");
	    return (0);
	}

	/* In "touching_illegal rules, spacing to set2 will be checked
	 * in FORWARD 
	 * direction at edges between set1 and (setR=~set1). 
	 *
	 * In addition, spacing to set1 will be checked in FORWARD direction
	 * at edges between set2 and (setRreverse=  ~set2).
	 *
	 * setR and setRreverse are set appropriately below.
	 */
	TTMaskCom2(&setR, &set1);
	TTMaskCom2(&setRreverse, &set2);
    }
    else
    {
	TechError("Badly formed drc spacing line\n");
	return (0);
    }

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    if (i == j || !SamePlane(i, j)) continue;

	    /* LHS is an element of set1, RHS is an element of setR */
	    if (TTMaskHasType(&set1, i) && TTMaskHasType(&setR, j))
	    {
		/*
		 * Must not have 'set2' for 'distance' to the right of
		 * an edge between 'set1' and the types not in 'set1'
		 * (touching_illegal case) or in neither
		 * 'set1' nor 'set2' (touching_ok case).
		 */

		/* Find bucket preceding the new one we wish to insert */
		for (dp = DRCRulesTbl [i][j];
			 dp->drcc_next != (DRCCookie *) NULL &&
			 dp->drcc_next->drcc_dist < distance;
			     dp = dp->drcc_next)
		    ; /* null body */
		
		/* May have to insert several buckets on different planes */
		for (curPlane = PL_TECHDEPBASE; curPlane < DBNumPlanes; curPlane++)
		{
		    if (!PlaneMaskHasPlane(planes2, curPlane)) continue;
		    MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		    TTMaskClearMask3(&tmp1, &DBPlaneTypes[curPlane], &set2);
		    TTMaskAndMask3(&tmp2, &DBPlaneTypes[curPlane], &setR);
		    ASSIGN(dpnew, distance, dp->drcc_next,
			tmp1, tmp2, why, distance, DRC_FORWARD, curPlane);

		    if (i == TT_SPACE)
		    {
			if (curPlane != DBPlane(j))
			    dpnew->drcc_flags |= DRC_XPLANE;
		    }
		    else
		    {
			if (curPlane != DBPlane(i))
			    dpnew->drcc_flags |= DRC_XPLANE;
		    }
		    
		    if (needReverse)
		        dpnew->drcc_flags |= DRC_BOTHCORNERS;

		    dp->drcc_next = dpnew;

		    if (needReverse)
		    {
			/* Add check in reverse direction, 
			 * NOTE:  am assuming single plane rule here (since reverse
			 * rules only used with touching_ok which must be 
			 * single plane)
			 * so am not setting DRC_XPLANE as above.
			 * /
			 
			 /* find bucket preceding new one we wish to insert */
			 for (dp = DRCRulesTbl [j][i];
			          dp->drcc_next != (DRCCookie *) NULL &&
   			          dp->drcc_next->drcc_dist < distance;
			          dp = dp->drcc_next)
			     ; /* null body */
			 
			 MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
			 ASSIGN(dpnew,distance,dp->drcc_next,
				tmp1, tmp2, why, distance, 
				DRC_REVERSE|DRC_BOTHCORNERS, curPlane);
			 
			 dp->drcc_next = dpnew;
		     }
		}
	    }

	    /*
	     * Now, if set1 and set2 are distinct apply the rule for LHS in set1
	     * and RHS in set2.
	     */
	    if (TTMaskEqual(&set1, &set2)) continue;

	    /* LHS is an element of set2, RHS is an element of setRreverse */
	    if (TTMaskHasType(&set2, i) && TTMaskHasType(&setRreverse, j))
	    {
		/* Find bucket preceding the new one we wish to insert */
		for (dp = DRCRulesTbl [i][j];
			 dp->drcc_next != (DRCCookie *) NULL &&
			 dp->drcc_next->drcc_dist < distance;
			     dp = dp->drcc_next)
		    ; /* null body */

		for (curPlane = PL_TECHDEPBASE; curPlane < DBNumPlanes; curPlane++)
		{
		    if (!PlaneMaskHasPlane(planes1, curPlane)) continue;

		    MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		    TTMaskClearMask3(&tmp1,&DBPlaneTypes[curPlane],&set1);
		    TTMaskAndMask3(&tmp2,&DBPlaneTypes[curPlane],&setRreverse);
		    ASSIGN(dpnew, distance, dp->drcc_next,
			tmp1, tmp2, why, distance, DRC_FORWARD, curPlane);

		    if (i == TT_SPACE)
		    {
			if (curPlane != DBPlane(j))
			    dpnew->drcc_flags |= DRC_XPLANE;
		    }
		    else
		    {
			if (curPlane != DBPlane(i))
			    dpnew->drcc_flags |= DRC_XPLANE;
		    }

		    if (needReverse)
		        dpnew->drcc_flags |= DRC_BOTHCORNERS;

		    dp->drcc_next = dpnew;		    

		    if (needReverse)
		    {
			/* Add check in reverse direction, 
			 * NOTE:  am assuming single plane rule here (since reverse
			 * rules only used with touching_ok which must be 
			 * single plane)
			 * so am not setting DRC_XPLANE as above.
			 * /
			 
			 /* find bucket preceding new one we wish to insert */
			 for (dp = DRCRulesTbl [j][i];
			          dp->drcc_next != (DRCCookie *) NULL &&
   			          dp->drcc_next->drcc_dist < distance;
			          dp = dp->drcc_next)
			     ; /* null body */
			 
			 MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
			 ASSIGN(dpnew,distance,dp->drcc_next,
				tmp1, tmp2, why, distance, 
				DRC_REVERSE|DRC_BOTHCORNERS, curPlane);
			 
			 dp->drcc_next = dpnew;
		     }
		}
	    }

	    /* Finally, if multiplane rule then check that set2 types
	     * are not present just to right of edges with setR on LHS
	     * and set1 on RHS.  This check is necessary to make sure
	     * that a set1 rectangle doesn't coincide exactly with a
	     * set2 rectangle.  
	     * (This check added by Michael Arnold on 4/10/86.)
	     */

	    /* If not cross plane rule, check does not apply */
	    if (plane != DBNumPlanes) continue; 

	    /* LHS is an element of setR, RHS is an element of set1 */
	    if (TTMaskHasType(&setR, i) && TTMaskHasType(&set1, j))
	    {
		/*
		 * Must not have 'set2' for 'distance' to the right of
		 * an edge between the types not in set1 and set1.
		 * (is only checked for cross plane rules - these are
		 * all of type touching_illegal)
		 */

		/* Walk list to last check.  New checks ("cookies") go
		 * at end of list since we are checking for distance of
		 * 1 and the list is sorted in order of decreasing distance.
		 */
		for (dp = DRCRulesTbl [i][j];
		     dp->drcc_next != (DRCCookie *) NULL;
		     dp = dp->drcc_next)
		    ; /* null body */

		/* Insert one check for each plane involved in set2 */
		for (curPlane = PL_TECHDEPBASE; 
		    curPlane < DBNumPlanes; 
		    curPlane++)
		{
		    if (!PlaneMaskHasPlane(planes2, curPlane)) continue;

		    /* filter out checks that are not cross plane */
		    if (i == TT_SPACE) 
		    {
			if (curPlane == DBPlane(j))
			    continue;
		    }
		    else
		    {
			if (curPlane == DBPlane(i))
			    continue;
		    }

		    /* create new check and add it to list */
		    MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		    TTMaskClearMask3(&tmp1, &DBPlaneTypes[curPlane], &set2);
		    TTMaskZero(&tmp2);
		    ASSIGN(dpnew, 1, dp->drcc_next,
			tmp1, tmp2, why, distance, 
			DRC_FORWARD | DRC_XPLANE, curPlane);
		    dp->drcc_next = dpnew;
		}
	    }
	}
    }

    return (distance);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcEdge --
 *
 * Process a primitive edge rule.
 * This is of the form:
 *
 *	edge layers1 layers2 dist OKtypes cornerTypes cornerDist why [plane]
 * or	edge4way layers1 layers2 dist OKtypes cornerTypes cornerDist why [plane]
 *
 * e.g,
 *
 *	edge poly,pmc s 1 diff poly,pmc "poly-diff separation must be 2"
 *
 * An "edge" rule is applied only down and to the left.
 * An "edge4way" rule is applied in all four directions.
 *
 * Results:
 *	Returns greater of dist and cdist.
 *
 * Side effects:
 *	Updates the DRC technology variables.
 *
 * ----------------------------------------------------------------------------
 */

drcEdge(argc, argv)
    int argc;
    char *argv[];
{
    char *layers1 = argv[1], *layers2 = argv[2];
    int distance = atoi(argv[3]);
    char *okTypes = argv[4], *cornerTypes = argv[5];
    int cdist = atoi(argv[6]);
    char *why = argv[7];
    bool fourway = (strcmp(argv[0], "edge4way") == 0);
    TileTypeBitMask set1, set2, tmp1, tmp2, tmp3, setC, setM;
    DRCCookie *dp, *dpnew;
    int plane, checkPlane;
    TileType i, j;

    /*
     * Edge4way rules produce [j][i] entries as well as [i][j]
     * ones, and check both corners rather than just one corner.
     */
    DBTechNoisyNameMask(layers1, &set1);
    DBTechNoisyNameMask(layers2, &set2);

    /*
     * Make sure that all edges between the two sets can be
     * found on one plane.
     */
    for (plane = PL_TECHDEPBASE; plane < DBNumPlanes; plane++)
    {
	if (DBTechSubsetLayers(set1, DBPlaneTypes[plane], &tmp1)
	    && DBTechSubsetLayers(set2, DBPlaneTypes[plane], &tmp2))
	    goto edgeOK;
    }
    TechError("All edges in edge rule must lie in one plane.\n");
    return (0);

edgeOK:
    set1 = tmp1;
    set2 = tmp2;

    /* Give warning if types1 and types2 intersect */
    if(TTMaskIntersect(&set1,&set2))
    {
	TechError("Warning:  types1 and types2 have nonempty intersection.  DRC does not check edges with the same type on both sides.\n");
    }

    DBTechNoisyNameMask(cornerTypes, &tmp3);
    if (!DBTechSubsetLayers(tmp3, DBPlaneTypes[plane], &setC))
    {
	TechError("Corner types aren't in same plane as edges.\n");
	return (0);
    }

    checkPlane = plane;
    if (argc == 9)
    {
	checkPlane = DBTechNoisyNamePlane(argv[8]);
	if (checkPlane < 0)
	    return (0);
    }

    DBTechNoisyNameMask(okTypes, &setM);
    if (!DBTechSubsetLayers(setM, DBPlaneTypes[checkPlane], &setM))
    {
	TechError("OK types aren't all in the right plane.\n");
	return (0);
    }

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    if (TTMaskHasType(&set1, i) && TTMaskHasType(&set2, j))
	    {
		/* Find bucket preceding the new one we wish to insert */
		for (dp = DRCRulesTbl [i][j];
		    dp->drcc_next != (DRCCookie *) NULL &&
		    dp->drcc_next->drcc_dist < distance;
			dp = dp->drcc_next)
		    ; /* null body */

		MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		ASSIGN(dpnew, distance, dp->drcc_next,
		    setM, setC, why, cdist, DRC_FORWARD, checkPlane);
		if (fourway) dpnew->drcc_flags |= DRC_BOTHCORNERS;
		if (checkPlane != plane)
		    dpnew->drcc_flags |= DRC_XPLANE;
		dp->drcc_next = dpnew;

		if (fourway)
		{
		    /* find bucket preceding new one we wish to insert */
		    for (dp = DRCRulesTbl [j][i];
			dp->drcc_next != (DRCCookie *) NULL &&
			dp->drcc_next->drcc_dist < distance;
			    dp = dp->drcc_next)
			; /* null body */

		    MALLOC(DRCCookie *, dpnew, sizeof (DRCCookie));
		    ASSIGN(dpnew,distance,dp->drcc_next,
			    setM, setC, why, cdist, DRC_REVERSE, checkPlane);
		    dpnew->drcc_flags |= DRC_BOTHCORNERS;
		    if (checkPlane != plane)
			dpnew->drcc_flags |= DRC_XPLANE;
		    dp->drcc_next = dpnew;
		}
	    }
	}
    }

    return (MAX(distance, cdist));
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcNoOverlap --
 *
 * Process a no-overlap rule.
 * This is of the form:
 *
 *	no_overlap layers1 layers2
 *
 * e.g,
 *
 *	no_overlap poly m2contact
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	Updates the DRC technology variables.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
drcNoOverlap(argc, argv)
    int argc;
    char *argv[];
{
    char *layers1 = argv[1], *layers2 = argv[2];
    TileTypeBitMask set1, set2;
    TileType i, j;
    int plane;

    /*
     * Grab up two sets of tile types, and make sure that if
     * any type from one set is painted over any type from the
     * other, then an error results.
     */

    DBTechNoisyNameMask(layers1, &set1);
    DBTechNoisyNameMask(layers2, &set2);

    for (i = 0; i < DBNumTypes; i++)
	for (j = 0; j < DBNumTypes; j++)
	    if (TTMaskHasType(&set1, i) && TTMaskHasType(&set2, j))
		for (plane = 0; plane < DBNumPlanes; plane++)
		{
		    DRCPaintTable[plane][j][i] = TT_ERROR_S;
		    DRCPaintTable[plane][i][j] = TT_ERROR_S;
		}

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcExactOverlap --
 *
 * Process an exact overlap
 * This is of the form:
 *
 *	exact_overlap layers
 *
 * e.g,
 *
 *	exact_overlap pmc,dmc
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	Updates DRCExactOverlapTypes.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
drcExactOverlap(argc, argv)
    int argc;
    char *argv[];
{
    char *layers = argv[1];
    TileTypeBitMask set;
    int i;

    /*
     * Grab up a bunch of tile types, and remember these: tiles
     * of these types cannot overlap themselves in different cells
     * unless they overlap exactly.
     */

    DBTechNoisyNameMask(layers, &set);

    /*
     * Go through the types and eliminate all but the first image
     * of each contact.
     */

    for (i = 0; i < DBNumTypes; i++)
    {
	if (!TTMaskHasType(&set, i)) continue;
	TTMaskSetType(&DRCExactOverlapTypes, i);
	TTMaskClearMask(&set, &DBLayerTypeMaskTbl[i]);
    }
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * drcStepSize --
 *
 * Process a declaration of the step size.
 * This is of the form:
 *
 *	stepsize step_size
 *
 * e.g,
 *
 *	stepsize 1000
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	Updates DRCStepSize.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
drcStepSize(argc, argv)
    int argc;
    char *argv[];
{
    DRCStepSize = atoi(argv[1]);
    if (DRCStepSize <= 0)
    {
	TechError("Step size must be a positive integer.\n");
	DRCStepSize = 0;
    }
    else if (DRCStepSize < 16)
    {
	TechError("Warning: abnormally small DRC step size (%d)\n",
		DRCStepSize);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCTechFinal --
 *
 * Called after all lines of the drc section in the technology file have been
 * read.  The preliminary DRC Rules Table is pruned by removing rules covered
 * by other (longer distance) rules, and by removing the dummy rule at the
 * front of each list.  Where edges are completely illegal, the rule list is
 * pruned to a single rule.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May remove DRCCookies from the linked lists of the DRCRulesTbl.
 *
 * ----------------------------------------------------------------------------
 */

Void
DRCTechFinal()
{
    TileTypeBitMask tmpMask, nextMask;
    DRCCookie  *dummy, *dp, *next;
    DRCCookie **dpp, **dp2back;
    TileType i, j;


    /* Remove dummy buckets */
    for (i = 0; i < TT_MAXTYPES; i++)
    {
	for (j = 0; j < TT_MAXTYPES; j++)
	{
	    dpp = &( DRCRulesTbl [i][j]);
	    dummy = *dpp;
	    *dpp = dummy->drcc_next;
	    FREE ((char *) dummy); 	/* "why" string is null */
	}
    }

    if (!DRCRuleOptimization) return;

    /* Check for edges that are completely illegal.  Where this is the
     * case, eliminate all of the edge's rules except one.
     */
    
    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    DRCCookie *keep = NULL;
	    
	    for (dp = DRCRulesTbl[i][j]; dp != NULL; dp = dp->drcc_next)
	    {
		if (dp->drcc_flags & DRC_XPLANE) continue;
		if (dp->drcc_flags & DRC_REVERSE)
		{
		    if (TTMaskHasType(&dp->drcc_mask, i)) continue;
		}
		else if (TTMaskHasType(&dp->drcc_mask, j)) continue;
		keep = dp;
		goto illegalEdge;
	    }
	    continue;

	    /* This edge is illegal.  Throw away all rules except the one
	     * needed that is always violated.
	     */
	    
	    illegalEdge:
	    for (dp = DRCRulesTbl[i][j]; dp != NULL; dp = dp->drcc_next)
	    {
		if (dp == keep) continue;
		(void) StrDup(&(dp->drcc_why), (char *) NULL);
		FREE((char *) dp);
		drcRulesOptimized += 1;
	    }
	    DRCRulesTbl[i][j] = keep;
	    keep->drcc_next = NULL;
	    /* TxPrintf("Edge %s-%s is illegal.\n", DBTypeShortName(i),
		DBTypeShortName(j));
	    */
	}
    }

    /*
     * Remove any rule A "covered" by another rule B, i.e.,
     *		B's distance >= A's distance,
     *		B's corner distance >= A's corner distance,
     *		B's RHS type mask is a subset of A's RHS type mask, and
     *		B's corner mask == A's corner mask
     *		B's check plane == A's check plane
     *		either both A and B or neither is a REVERSE direction rule
     *		if A is BOTHCORNERS then B must be, too
     */

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    for (dp = DRCRulesTbl[i][j]; dp != NULL; dp = dp->drcc_next)
	    {
		/*
		 * Check following buckets to see if any is a superset.
		 */
		
		for (next = dp->drcc_next; next != NULL;
			next = next->drcc_next)
		{
		    tmpMask = nextMask = next->drcc_mask;
		    TTMaskAndMask(&tmpMask, &dp->drcc_mask);
		    if (!TTMaskEqual(&tmpMask, &nextMask)) continue;
		    if (!TTMaskEqual(&dp->drcc_corner, &next->drcc_corner))
			continue;
		    if (dp->drcc_dist > next->drcc_dist) continue;
		    if (dp->drcc_cdist > next->drcc_cdist) continue;
		    if (dp->drcc_plane != next->drcc_plane) continue;
		    if (dp->drcc_flags & DRC_REVERSE)
		    {
			if (!(next->drcc_flags & DRC_REVERSE)) continue;
		    }
		    else if (next->drcc_flags & DRC_REVERSE) continue;
		    if ((next->drcc_flags & DRC_BOTHCORNERS)
			    && (dp->drcc_flags & DRC_BOTHCORNERS) == 0)
			continue;

		    break;
		}

		if (next == NULL) continue;

		/* "dp" is a subset of "next".  Eliminate it. */

		/* TxPrintf("For edge %s-%s, \"%s\" covers \"%s\"\n",
		    DBTypeShortName(i), DBTypeShortName(j),
		    next->drcc_why, dp->drcc_why);
		*/
		dp2back = &(DRCRulesTbl [i][j]);
		while (*dp2back != dp)
		    dp2back = &(*dp2back)->drcc_next;
		*dp2back = dp->drcc_next;
		(void) StrDup (&(dp->drcc_why), (char *) NULL);
		FREE ((char *) dp);
		drcRulesOptimized += 1;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * DRCTechRuleStats --
 *
 * 	Print out some statistics about the design rule database.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A bunch of stuff gets printed on the terminal.
 *
 * ----------------------------------------------------------------------------
 */

void
DRCTechRuleStats()
{
#define MAXBIN 10
    int counts[MAXBIN+1];
    int edgeRules, overflow;
    int i, j;
    DRCCookie *dp;

    /* Count up the total number of edge rules, and histogram them
     * by the number of rules per edge.
     */
    
    edgeRules = 0;
    overflow = 0;
    for (i=0; i<=MAXBIN; i++) counts[i] = 0;

    for (i=0; i<DBNumTypes; i++)
	for (j=0; j<DBNumTypes; j++)
	{
	    int thisCount = 0;
	    for (dp = DRCRulesTbl[i][j]; dp != NULL; dp = dp->drcc_next)
		thisCount++;
	    edgeRules += thisCount;
	    if ((i != TT_SPACE) && (j != TT_SPACE) &&
		(DBPlane(i) != DBPlane(j))) continue;
	    if (thisCount <= MAXBIN) counts[thisCount] += 1;
	    else overflow += 1;
	}
    
    /* Print out the results. */

    TxPrintf("Total number of rules specifed in tech file: %d\n",
	drcRulesSpecified);
    TxPrintf("Edge rules optimized away: %d\n", drcRulesOptimized);
    TxPrintf("Edge rules left in database: %d\n", edgeRules);
    TxPrintf("Histogram of # edges vs. rules per edge:\n");
    for (i=0; i<=MAXBIN; i++)
    {
	TxPrintf("  %2d rules/edge: %d.\n", i, counts[i]);
    }
    TxPrintf(" >%2d rules/edge: %d.\n", MAXBIN, overflow);
}
