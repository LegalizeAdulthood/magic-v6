/*
 * PlowTech.c --
 *
 * Plowing.
 * Read the "drc" section of the technology file and construct the
 * design rules used by plowing.
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
static char rcsid[]="$Header: PlowTech.c,v 6.0 90/08/28 18:53:23 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "utils.h"
#include "malloc.h"
#include "plowInt.h"
#include "drc.h"

/* Imports from DRC */
extern char *maskToPrint();

/* Rule tables */
PlowRule *plowSpacingRulesTbl[TT_MAXTYPES][TT_MAXTYPES];
PlowRule *plowWidthRulesTbl[TT_MAXTYPES][TT_MAXTYPES];

/* Special type masks */
TileTypeBitMask PlowFixedTypes;		/* Fixed-width types (e.g, fets) */
TileTypeBitMask PlowContactTypes;	/* All types that are contacts */
TileTypeBitMask PlowCoveredTypes;	/* All types that can't be uncovered
					 * (i.e, have an edge slide past them)
					 */
TileTypeBitMask PlowDragTypes;		/* All types that drag along trailing
					 * minimum-width material when they
					 * move.
					 */

/*
 * Entry [t] of the following table is the maximum distance associated
 * with any design rules in a bucket with type 't' on the LHS.
 */
int plowMaxDist[TT_MAXTYPES];

/* Forward declarations */
Void plowEdgeRule(), plowWidthRule(), plowSpacingRule();
PlowRule *plowTechOptimizeRule();

/*
 * ----------------------------------------------------------------------------
 *
 * PlowDRCInit --
 *
 * Initialization before processing the "drc" section for plowing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the rules table.
 *
 * ----------------------------------------------------------------------------
 */

Void
PlowDRCInit()
{
    register int i, j;
    register PlowRule *pr;

    /* Remove all old rules from the plowing rules table */
    for (i = 0; i < TT_MAXTYPES; i++)
    {
	for (j = 0; j < TT_MAXTYPES; j++)
	{
	    for (pr = plowWidthRulesTbl[i][j]; pr; pr = pr->pr_next)
		FREE((char *) pr );
	    for (pr = plowSpacingRulesTbl[i][j]; pr; pr = pr->pr_next)
		FREE((char *) pr );
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlowDRCLine --
 *
 * Process a single line from the "drc" section.
 *
 * Results:
 *	TRUE always.
 *
 * Side effects:
 *	Adds rules to our plowing rule tables.
 *
 * Organization:
 *	We select a procedure based on the first keyword (argv[0])
 *	and call it to do the work of implementing the rule.  Each
 *	such procedure is of the following form:
 *
 *	Void
 *	proc(argc, argv)
 *	    int argc;
 *	    char *argv[];
 *	{
 *	}
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
PlowDRCLine(sectionName, argc, argv)
    char *sectionName;		/* Unused */
    int argc;
    char *argv[];
{
    int which;
    static struct
    {
	char	*rk_keyword;	/* Initial keyword */
	int	 rk_minargs;	/* Min # arguments */
	int	 rk_maxargs;	/* Max # arguments */
	int    (*rk_proc)();	/* Procedure implementing this keyword */
    } ruleKeys[] = {
	"edge",		 8,	9,	plowEdgeRule,
	"edge4way",	 8,	9,	plowEdgeRule,
	"spacing",	 6,	6,	plowSpacingRule,
	"width",	 4,	4,	plowWidthRule,
	0
    }, *rp;

    /*
     * Leave the job of printing error messages to the DRC tech file reader.
     * We only process a few of the various design-rule types here anyway.
     */
    which = LookupStruct(argv[0], (LookupTable *) ruleKeys, sizeof ruleKeys[0]);
    if (which >= 0)
    {
	rp = &ruleKeys[which];
	if (argc >= rp->rk_minargs && argc <= rp->rk_maxargs)
	    (*rp->rk_proc)(argc, argv);
    }

    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * plowWidthRule --
 *
 * Process a width rule.
 * This is of the form:
 *
 *	width layers distance why
 * e.g, width poly,pmc 2 "poly width must be at least 2"
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the plowing width rule table.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
plowWidthRule(argc, argv)
    int argc;
    char *argv[];
{
    char *layers = argv[1];
    int distance = atoi(argv[2]);
    TileTypeBitMask set, setC, tmp1;
    register PlowRule *pr;
    register TileType i, j;
    int pNum;

    /*
     * All layers in a width rule must be on the same plane;
     * DBTechSubsetLayers() below maps contacts to their proper images.
     */
    DBTechNoisyNameMask(layers, &set);
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	if (DBTechSubsetLayers(set, DBPlaneTypes[pNum], &tmp1))
	    break;
    if (pNum >= DBNumPlanes)
	return;
    set = tmp1;
    TTMaskCom2(&setC, &set);
    TTMaskAndMask(&setC, &DBPlaneTypes[pNum]);

    /*
     * Must have types in 'set' for at least 'distance' to the right of
     * any edge between a type in '~set' and a type in 'set'.
     */
    for (i = 0; i < DBNumTypes; i++)
    {
	if (TTMaskHasType(&setC, i))
	{
	    for (j = 0; j < DBNumTypes; j++)
	    {
		if (SamePlane(i, j) && TTMaskHasType(&set, j))
		{
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    pr->pr_dist = distance;
		    pr->pr_ltypes = setC;
		    pr->pr_oktypes = set;
		    pr->pr_pNum = pNum;
		    pr->pr_flags = PR_WIDTH;
		    pr->pr_next = plowWidthRulesTbl[i][j];
		    plowWidthRulesTbl[i][j] = pr;
		}
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * plowSpacingRule --
 *
 * Process a spacing rule.
 * This is of the form:
 *
 *	spacing layers1 layers2 distance adjacency why
 * e.g, spacing metal,pmc/m,dmc/m metal,pmc/m,dmc/m 4 touching_ok \
 *		"metal spacing must be at least 4"
 *
 * Adjacency may be either "touching_ok" or "touching_illegal".  In
 * the former case, no violation occurs when types in layers1 are
 * immediately adjacent to types in layers2.  In the second case,
 * such adjacency causes a violation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the plowing spacing rules.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
plowSpacingRule(argc, argv)
    int argc;
    char *argv[];
{
    char *layers1 = argv[1], *layers2 = argv[2];
    int distance = atoi(argv[3]);
    char *adjacency = argv[4];
    TileTypeBitMask set1, set2, tmp1, tmp2, setR, setRreverse;
    int pNum, planes1, planes2;
    register PlowRule *pr;
    register TileType i, j;

    DBTechNoisyNameMask(layers1, &set1);
    planes1 = DBTechMinSetPlanes(set1, &set1);
    DBTechNoisyNameMask(layers2, &set2);
    planes2 = DBTechMinSetPlanes(set2, &set2);

    if (strcmp (adjacency, "touching_ok") == 0)
    {
	/* If touching is OK, everything must fall in the same plane. */

	for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	    if (DBTechSubsetLayers(set1, DBPlaneTypes[pNum], &tmp1)
		    && DBTechSubsetLayers(set2, DBPlaneTypes[pNum], &tmp2))
		break;
	if (pNum >= DBNumPlanes)
	    return;

	/*
	 * Must not have 'set2' for 'distance' to the right of an edge between
	 * 'set1' and the types in neither 'set1' nor 'set2' (ie, 'setR').
	 */
	set1 = tmp1;
	set2 = tmp2;
	planes1 = planes2 = PlaneNumToMaskBit(pNum);
	TTMaskCom(&tmp1);
	TTMaskCom(&tmp2);
	TTMaskAndMask(&tmp1, &tmp2);
	TTMaskAndMask(&tmp2, &DBPlaneTypes[pNum]);
	setRreverse = setR = tmp1;
    }
    else if (strcmp (adjacency, "touching_illegal") == 0)
    {
	/*
	 * Must not have 'set2' for 'distance' to the right of an edge between
	 * 'set1' and the types not in 'set1' (ie, 'setR').
	 */
	TTMaskCom2(&setR, &set1);
	TTMaskCom2(&setRreverse, &set2);
    }
    else return;

    for (i = 0; i < DBNumTypes; i++)
    {
	for (j = 0; j < DBNumTypes; j++)
	{
	    if (i == j || !SamePlane(i, j)) continue;

	    /* LHS is an element of set1 and RHS is an element of setR */
	    if (TTMaskHasType(&set1, i) && TTMaskHasType(&setR, j))
	    {
		/* May have to insert several buckets on different planes */
		for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
		{
		    if (!PlaneMaskHasPlane(planes2, pNum))
			continue;
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    TTMaskClearMask3(&tmp1, &DBPlaneTypes[pNum], &set2);
		    TTMaskCom2(&tmp2, &setRreverse);
		    TTMaskAndMask3(&pr->pr_ltypes, &DBPlaneTypes[pNum], &tmp2);
		    pr->pr_oktypes = tmp1;
		    pr->pr_dist = distance;
		    pr->pr_pNum = pNum;
		    pr->pr_flags = 0;
		    pr->pr_next = plowSpacingRulesTbl[i][j];
		    plowSpacingRulesTbl[i][j] = pr;
		}
	    }

	    /* Also apply backwards, unless it would create duplicates */
	    if (TTMaskEqual(&set1, &set2)) continue;

	    /* LHS is an element of set2, RHS is an element of setRreverse */
	    if (TTMaskHasType(&set2, i) && TTMaskHasType(&setRreverse, j))
	    {
		/* May have to insert several buckets on different planes */
		for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
		{
		    if (!PlaneMaskHasPlane(planes1, pNum)) continue;
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    TTMaskClearMask3(&tmp1, &DBPlaneTypes[pNum], &set1);
		    TTMaskCom2(&tmp2, &setRreverse);
		    TTMaskAndMask3(&pr->pr_ltypes, &DBPlaneTypes[pNum], &tmp2);
		    pr->pr_oktypes = tmp1;
		    pr->pr_dist = distance;
		    pr->pr_pNum = pNum;
		    pr->pr_flags = 0;
		    pr->pr_next = plowSpacingRulesTbl[i][j];
		    plowSpacingRulesTbl[i][j] = pr;
		}
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * plowEdgeRule --
 *
 * Process a primitive edge rule.
 * This is of the form:
 *
 *	edge layers1 layers2 dist OKtypes cornerTypes cornerDist why [plane]
 * or	edge4way layers1 layers2 dist OKtypes cornerTypes cornerDist why [plane]
 * e.g, edge poly,pmc s 1 diff poly,pmc "poly-diff separation must be 2"
 *
 * An "edge" rule is applied only down and to the left.
 * An "edge4way" rule is applied in all four directions.
 *
 * For plowing, we consider edge rules to be spacing rules.
 * Ordinary "edge" rules can be handled exactly (taking the distance
 * to be the maximum of dist and cornerDist above), because they are
 * always applied in the proper direction.  Each edge rule produces
 * one normal spacing rule, and possibly an additional spacing rule
 * that is only applied in the penumbra (if cornerTypes and layers2
 * are different).
 *
 * An "edge4way" rule also requires a conservative approximation to
 * handle the case when it is being applied in the opposite direction.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the plowing spacing rules.
 *
 * ----------------------------------------------------------------------------
 */

plowEdgeRule(argc, argv)
    int argc;
    char *argv[];
{
    char *layers1 = argv[1], *layers2 = argv[2];
    int distance = atoi(argv[3]);
    char *okTypes = argv[4], *cornerTypes = argv[5];
    int cdist = atoi(argv[6]);
    TileTypeBitMask set1, set2, tmp1, tmp2, tmp3, setC, setM;
    TileTypeBitMask setOK, setLeft, setRight;
    int pNum, checkPlane, flags;
    bool needPenumbraOnly;
    bool isFourWay = (strcmp(argv[0], "edge4way") == 0);
    register PlowRule *pr;
    register TileType i, j;

    DBTechNoisyNameMask(layers1, &set1);
    DBTechNoisyNameMask(layers2, &set2);
    distance = MAX(distance, cdist);

    /* Make sure that all edges between the two sets exist on one plane */
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	if (DBTechSubsetLayers(set1, DBPlaneTypes[pNum], &tmp1)
		&& DBTechSubsetLayers(set2, DBPlaneTypes[pNum], &tmp2))
	    break;
    if (pNum >= DBNumPlanes)
	return;

    set1 = tmp1;
    set2 = tmp2;
    DBTechNoisyNameMask(cornerTypes, &tmp3);
    if (!DBTechSubsetLayers(tmp3, DBPlaneTypes[pNum], &setC))
	return;

    /* If an explicit check plane was specified, use it */
    checkPlane = pNum;
    if (argc == 9)
    {
	checkPlane = DBTechNamePlane(argv[8]);
	if (checkPlane < 0)
	    return;
    }

    /* Get the images of everything in okTypes on the check plane */
    DBTechNoisyNameMask(okTypes, &setM);
    if (!DBTechSubsetLayers(setM, DBPlaneTypes[checkPlane], &setM))
	return;

    needPenumbraOnly = !TTMaskEqual(&set2, &setC);
    TTMaskCom2(&setLeft, &setC);
    TTMaskAndMask(&setLeft, &DBPlaneTypes[pNum]);
    TTMaskCom2(&setRight, &set2);
    TTMaskAndMask(&setRight, &DBPlaneTypes[pNum]);
    flags = isFourWay ? PR_EDGE4WAY : PR_EDGE;
    for (i = 0; i < DBNumTypes; i++)
    {
	if (TTMaskHasType(&set1, i))
	{
	    for (j = 0; j < DBNumTypes; j++)
	    {
		if (TTMaskHasType(&set2, j))
		{
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    pr->pr_ltypes = setLeft;
		    pr->pr_oktypes = setM;
		    pr->pr_dist = distance;
		    pr->pr_pNum = checkPlane;
		    pr->pr_next = plowSpacingRulesTbl[i][j];
		    pr->pr_flags = flags;
		    plowSpacingRulesTbl[i][j] = pr;
		}

		if (needPenumbraOnly && TTMaskHasType(&setC, j))
		{
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    pr->pr_ltypes = setRight;
		    pr->pr_oktypes = setM;
		    pr->pr_dist = distance;
		    pr->pr_pNum = checkPlane;
		    pr->pr_next = plowSpacingRulesTbl[i][j];
		    pr->pr_flags = flags|PR_PENUMBRAONLY;
		    plowSpacingRulesTbl[i][j] = pr;
		}
	    }
	}
    }

    if (!isFourWay)
	return;

    /*
     * Four-way edge rules are applied by the design-rule checker
     * both forwards and backwards.  Since plowing can only look
     * forward, we need to approximate the backward rules with
     * a collection of forward rules.
     *
     * Suppose we have the following 4-way rule:
     *
     *   CORNER
     *	--------+
     *	  LEFT	|  RIGHT : OKTypes
     *
     * To check it in the following (backward) configuration, using
     * only rightward-looking rules,
     *
     *	       OKTypes : RIGHT	|  LEFT
     *				+--------
     *				  CORNER
     *
     * we generate the following forward rules (with the same distance):
     *
     *    ~t
     *	--------+
     *	   t	|  ~t : ~LEFT
     *
     * for each t in ~OKTypes.  In plowing terms, each rule will have LTYPES of
     * t and OKTYPES of ~LEFT.  In effect, this is creating a forward spacing
     * rule between each of the types ~OKTypes, and the materials in LEFT.
     * The edge is found on checkPlane, and checked on plane pNum.
     *
     * Because the corner and right-hand types for these rules are the same,
     * we don't need to generate any PR_PENUMBRAONLY rules.
     */

    setRight = setM;
    TTMaskCom2(&setLeft, &setM);
    TTMaskAndMask(&setLeft, &DBPlaneTypes[checkPlane]);
    TTMaskCom2(&setOK, &set1);
    TTMaskAndMask(&setOK, &DBPlaneTypes[pNum]);
    for (i = 0; i < DBNumTypes; i++)
    {
	if (TTMaskHasType(&setLeft, i))
	{
	    for (j = 0; j < DBNumTypes; j++)
	    {
		if (TTMaskHasType(&setRight, j))
		{
		    MALLOC(PlowRule *, pr, sizeof (PlowRule));
		    TTMaskSetOnlyType(&pr->pr_ltypes, i);
		    pr->pr_oktypes = setOK;
		    pr->pr_dist = distance;
		    pr->pr_pNum = pNum;
		    pr->pr_flags = flags|PR_EDGEBACK;
		    pr->pr_next = plowSpacingRulesTbl[i][j];
		    plowSpacingRulesTbl[i][j] = pr;
		}
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlowDRCFinal --
 *
 * Called after all lines of the drc section in the technology file have been
 * read.  The preliminary plowing rules tables are pruned by removing rules
 * covered by other (longer distance) rules.
 *
 * We also construct plowMaxDist[] to contain for entry 't' the maximum
 * distance associated with any plowing rule in a bucket with 't' on its
 * LHS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May remove PlowRules from the linked lists of the width and
 *	spacing rules tables.  Sets the values in plowMaxDist[].
 *
 * ----------------------------------------------------------------------------
 */

Void
PlowDRCFinal()
{
    register PlowRule *pr;
    register TileType i, j;

    for (i = 0; i < DBNumTypes; i++)
    {
	plowMaxDist[i] = 0;
	for (j = 0; j < DBNumTypes; j++)
	{
	    if (pr = plowWidthRulesTbl[i][j])
	    {
		pr = plowWidthRulesTbl[i][j] = plowTechOptimizeRule(pr);
		for ( ; pr; pr = pr->pr_next)
		    if (pr->pr_dist > plowMaxDist[i])
			plowMaxDist[i] = pr->pr_dist;
	    }
	    if (pr = plowSpacingRulesTbl[i][j])
	    {
		pr = plowSpacingRulesTbl[i][j] = plowTechOptimizeRule(pr);
		for ( ; pr; pr = pr->pr_next)
		    if (pr->pr_dist > plowMaxDist[i])
			plowMaxDist[i] = pr->pr_dist;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * plowTechOptimizeRule --
 *
 * Called to optimize the chain of rules in a single bin of either
 * the spacing or width rules tables.
 *
 * In general, we want to remove any rule A "covered" by another
 * rule B, i.e.,
 *
 *	B's distance >= A's distance,
 *	B's OKTypes is a subset of A's OKTypes, and
 *	B's Ltypes == A's Ltypes
 *	B's check plane == A's check plane
 *
 * Results:
 *	Returns a pointer to the new chain of rules for this bin.
 *
 * Side effects:
 *	May deallocate memory.
 *
 * ----------------------------------------------------------------------------
 */

PlowRule *
plowTechOptimizeRule(ruleList)
    PlowRule *ruleList;
{
    register PlowRule *pCand, *pCandLast, *pr;
    TileTypeBitMask tmpMask;

    /*
     * The pointer 'pCand' in the following loop will iterate over
     * candidates for deletion, and pCandLast will trail by one.
     */
    pCand = ruleList;
    pCandLast = (PlowRule *) NULL;
    while (pCand)
    {
	for (pr = ruleList; pr; pr = pr->pr_next)
	{
	    if (pr != pCand
		    && pr->pr_dist >= pCand->pr_dist
		    && pr->pr_flags == pCand->pr_flags
		    && pr->pr_pNum == pCand->pr_pNum
		    && TTMaskEqual(&pr->pr_ltypes, &pCand->pr_ltypes))
	    {
		/*
		 * Is pr->pr_oktypes a subset of pCand->pr_oktypes,
		 * i.e, is it more restrictive?
		 */
		TTMaskAndMask3(&tmpMask, &pr->pr_oktypes, &pCand->pr_oktypes);
		if (TTMaskEqual(&tmpMask, &pr->pr_oktypes))
		{
		    /*
		     * Delete pCand, and resume outer loop with the
		     * new values of pCand and pCandLast set below.
		     */
		    FREE((char *) pCand);
		    if (pCandLast)
			pCandLast->pr_next = pCand->pr_next;
		    else
			ruleList = pCand->pr_next;
		    pCand = pCand->pr_next;
		    goto next;
		}
	    }
	}

	/* Normal continuation: advance to next rule in bin */
	pCandLast = pCand, pCand = pCand->pr_next;

next:	;
    }

    return (ruleList);
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlowTechInit --
 *
 * Initialize the masks of types PlowFixedTypes, PlowCoveredTypes,
 * and PlowDragTypes to be empty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Zeroes PlowFixedTypes, PlowCoveredTypes, and PlowDragTypes.
 *
 * ----------------------------------------------------------------------------
 */

PlowTechInit()
{
    PlowFixedTypes = DBZeroTypeBits;
    PlowCoveredTypes = DBZeroTypeBits;
    PlowDragTypes = DBZeroTypeBits;
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlowTechLine --
 *
 * Process a line from the plowing section of a technology file.
 * Such a line is currently of the following form:
 *
 *	keyword types
 *
 * where 'types' is a comma-separated list of type names.
 *
 * Keywords:
 *
 *	fixed		each of 'types' is fixed-width; regions consisting
 *			of fixed-width types are not deformed by plowing.
 *			Contacts are automatically fixed size and so do not
 *			need to be included in this list.  Space can never
 *			be fixed-size and so is automatically omitted from
 *			the list.
 *
 *	covered		each of 'types' cannot be uncovered as a result of
 *			plowing.  This means that if material of type X
 *			covers a horizontal edge initially, it will continue
 *			to cover it after plowing.
 *
 *	drag		each of 'types' will drag along with it the LHS of
 *			any trailing minimum-width material when it moves.
 *			This is so transistors will drag their gates when
 *			the transistor moves, so we don't leave huge amounts
 *			of poly behind us.
 *
 * Results:
 *	Returns TRUE always.
 *
 * Side effects:
 *	Updates PlowFixedTypes, PlowCoveredTypes, and PlowDragTypes.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
PlowTechLine(sectionName, argc, argv)
    char *sectionName;	/* Unused */
    int argc;
    char *argv[];
{
    TileTypeBitMask types;

    if (argc != 2)
    {
	TechError("Malformed line\n");
	return (TRUE);
    }

    DBTechNoisyNameMask(argv[1], &types);

    TTMaskAndMask(&types, &DBAllButSpaceBits);

    if (strcmp(argv[0], "fixed") == 0)
    {
	TTMaskSetMask(&PlowFixedTypes, &types);
    }
    else if (strcmp(argv[0], "covered") == 0)
    {
	TTMaskSetMask(&PlowCoveredTypes, &types);
    }
    else if (strcmp(argv[0], "drag") == 0)
    {
	TTMaskSetMask(&PlowDragTypes, &types);
    }
    else
    {
	TechError("Illegal keyword \"%s\".\n", argv[0]);
    }
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlowTechFinal --
 *
 * Final processing of the lines from the plowing section of a technology
 * file.  Add all contacts to the list of fixed types.  Also sets the mask
 * PlowContactTypes to have bits set for each image of each contact.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates PlowFixedTypes and PlowContactTypes.
 *
 * ----------------------------------------------------------------------------
 */

PlowTechFinal()
{
    PlowContactTypes = DBImageBits;
    TTMaskSetMask(&PlowFixedTypes, &PlowContactTypes);
}

/*
 * ----------------------------------------------------------------------------
 *
 * plowTechShow --
 *
 * For debugging purposes.
 * Print the complete table of all plowing rules.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the file 'f'.
 *
 * ----------------------------------------------------------------------------
 */

plowTechShow(f)
    FILE *f;
{
    plowTechShowTable(plowWidthRulesTbl, "Width Rules", f);
    plowTechShowTable(plowSpacingRulesTbl, "Spacing Rules", f);
}

plowTechShowTable(table, header, f)
    PlowRule *table[TT_MAXTYPES][TT_MAXTYPES];
    char *header;
    FILE *f;
{
    PlowRule *pr;
    TileType i, j;

    (void) fprintf(f, "\n\n------------ %s ------------\n", header);
    for (i = 0; i < DBNumTypes; i++)
	for (j = 0; j < DBNumTypes; j++)
	    if (pr = table[i][j])
	    {
		(void) fprintf(f, "\n%s -- %s:\n",
		    DBTypeLongName(i), DBTypeLongName(j));
		for ( ; pr; pr = pr->pr_next)
		    plowTechPrintRule(pr, f);
	    }
}

plowTechPrintRule(pr, f)
    PlowRule *pr;
    FILE *f;
{
    (void) fprintf(f, "\tDISTANCE=%d, PLANE=%s, FLAGS=",
		pr->pr_dist, DBPlaneLongName(pr->pr_pNum));
    if (pr->pr_flags & PR_WIDTH) (void) fprintf(f, " Width");
    if (pr->pr_flags & PR_PENUMBRAONLY) (void) fprintf(f, " PenumbraOnly");
    if (pr->pr_flags & PR_EDGE) (void) fprintf(f, " Edge");
    if (pr->pr_flags & PR_EDGE4WAY) (void) fprintf(f, " Edge4way");
    if (pr->pr_flags & PR_EDGEBACK) (void) fprintf(f, " EdgeBack");
    (void) fprintf(f, "\n");
    (void) fprintf(f, "\tLTYPES = %s\n", maskToPrint(&pr->pr_ltypes));
    (void) fprintf(f, "\tOKTYPES = %s\n", maskToPrint(&pr->pr_oktypes));
    (void) fprintf(f, "\t-------------------------------\n");
}
