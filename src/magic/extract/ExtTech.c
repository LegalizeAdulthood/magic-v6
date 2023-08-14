/*
 * ExtTech.c --
 *
 * Circuit extraction.
 * Code to read and process the sections of a technology file
 * that are specific to circuit extraction.
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
static char sccsid[] = "@(#)ExtTech.c	4.8 MAGIC (Berkeley) 10/26/85";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "utils.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "malloc.h"
#include "textio.h"
#include "tech.h"
#include "debug.h"
#include "extract.h"
#include "extractInt.h"
#ifdef SYSV
#include <string.h>
#endif

/* Current extraction style */
ExtStyle *ExtCurStyle = NULL;

/* List of all styles */
ExtStyle *ExtAllStyles = NULL;

/*
 * Table used for parsing the extract section of a .tech file
 * Each line in the extract section is of a type determined by
 * its first keyword.  There is one entry in the following table
 * for each such keyword.
 */

typedef enum
{
    AREAC, CONTACT, CSCALE, FET, FETRESIST, LAMBDA, OVERC, PERIMC,
    RESIST, RSCALE, SIDEHALO, SIDEOVERLAP, SIDEWALL, STEP, STYLE
} Key;

static struct keydesc
{
    char	*k_name;
    Key		 k_key;
    int		 k_minargs;
    int		 k_maxargs;
    char	*k_usage;
} keyTable[] = {
    "areacap",		AREAC,		3,	3,
"types capacitance",

    "contact",		CONTACT,	4,	4,
"type size resistance",

    "cscale",		CSCALE,		2,	2,
"capacitance-scalefactor",

    "fet",		FET,		8,	9,
"types terminal-types min-#-terminals name [subs-types] subs-node gscap gate-chan-cap",

    "fetresist",	FETRESIST,	4,	4,
"type region ohms-per-square",

    "lambda",		LAMBDA,		2,	2,
"units-per-lambda",

    "overlap",		OVERC,		4,	5,
"toptypes bottomtypes capacitance [shieldtypes]",

    "perimc",		PERIMC,		4,	4,
"intypes outtypes capacitance",

    "resist",		RESIST,		3,	3,
"types resistance",

    "rscale",		RSCALE,		2,	2,
"resistance-scalefactor",

    "sidehalo",		SIDEHALO,	2,	2,
"halo(lambda)",

    "sideoverlap",	SIDEOVERLAP,	5,	5,
"intypes outtypes ovtypes capacitance",

    "sidewall",		SIDEWALL,	6,	6,
"intypes outtypes neartypes fartypes capacitance",

    "step",		STEP,		2,	2,
"size(lambda)",

    "style",		STYLE,		2,	2,
"stylename",

    0
};

/*
 * ----------------------------------------------------------------------------
 *
 * ExtSetStyle --
 *
 * Set the current extraction style to 'name', or print
 * the available and current styles if 'name' is NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Just told you.
 *
 * ----------------------------------------------------------------------------
 */

Void
ExtSetStyle(name)
    char *name;
{
    ExtStyle *style, *match;
    int length;

    if (name == NULL)
	goto badStyle;

    match = NULL;
    length = strlen(name);
    for (style = ExtAllStyles; style; style = style->exts_next)
    {
	if (strncmp(name, style->exts_name, length) == 0)
	{
	    if (match != NULL)
	    {
		TxError("Extraction style \"%s\" is ambiguous.\n", name);
		goto badStyle;
	    }
	    match = style;
	}
    }

    if (match != NULL)
    {
	ExtCurStyle = match;
	TxPrintf("Extraction style is now \"%s\"\n", name);
	return;
    }

    TxError("\"%s\" is not one of the extraction styles Magic knows.\n", name);

badStyle:
    TxPrintf("The extraction styles are: ");
    for (style = ExtAllStyles; style; style = style->exts_next)
    {
	if (style == ExtAllStyles)
	    TxPrintf("%s", style->exts_name);
	else TxPrintf(", %s", style->exts_name);
    }
    TxPrintf(".\n");
    if (ExtCurStyle != NULL)
	TxPrintf("The current style is \"%s\".\n", ExtCurStyle->exts_name);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTechStyleNew --
 *
 * Allocate a new style with zeroed technology variables.
 *
 * Results:
 *	Allocates a new ExtStyle and returns it.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

ExtStyle *
extTechStyleNew(name)
    char *name;
{
    ExtStyle *style;
    register TileType r, s;

    style = (ExtStyle *) mallocMagic(sizeof (ExtStyle));
    style->exts_next = NULL;
    style->exts_name = StrDup((char **) NULL, name);

    style->exts_sidePlanes = style->exts_overlapPlanes = 0;
    TTMaskZero(&style->exts_transMask);

    for (r = 0; r < NP; r++)
    {
	TTMaskZero(&style->exts_sideTypes[r]);
	TTMaskZero(&style->exts_overlapTypes[r]);
    }

    for (r = 0; r < NT; r++)
    {
	TTMaskZero(&style->exts_nodeConn[r]);
	TTMaskZero(&style->exts_resistConn[r]);
	TTMaskZero(&style->exts_transConn[r]);
	style->exts_allConn[r] = DBAllTypeBits;

	style->exts_sheetResist[r] = 0;
	style->exts_viaSize[r] = 0;
	style->exts_viaResist[r] = 0;
	style->exts_areaCap[r] = 0;
	style->exts_overlapOtherPlanes[r] = 0;
	TTMaskZero(&style->exts_overlapOtherTypes[r]);
	TTMaskZero(&style->exts_sideEdges[r]);
	for (s = 0; s < NT; s++)
	{
	    TTMaskZero(&style->exts_sideCoupleOtherEdges[r][s]);
	    TTMaskZero(&style->exts_sideOverlapOtherTypes[r][s]);
	    style->exts_sideOverlapOtherPlanes[r][s] = 0;
	    style->exts_sideCoupleCap[r][s] = (EdgeCap *) NULL;
	    style->exts_sideOverlapCap[r][s] = (EdgeCap *) NULL;
	    style->exts_perimCap[r][s] = 0;
	    style->exts_overlapCap[r][s] = 0;

	    TTMaskZero(&style->exts_overlapShieldTypes[r][s]);
	    style->exts_overlapShieldPlanes[r][s] = 0;
	}

	TTMaskZero(&style->exts_perimCapMask[r]);
	TTMaskZero(&style->exts_transSDTypes[r]);
	style->exts_transSDCount[r] = 0;
	style->exts_transGateCap[r] = 0;
	style->exts_transSDCap[r] = 0;
	style->exts_transSubstrateName[r] = (char *) NULL;
	HashInit(&style->exts_transResist[r], 8, HT_STRINGKEYS);
	style->exts_linearResist[r] = 0;
    }

    style->exts_sideCoupleHalo = 0;

    style->exts_stepSize = 100;
    style->exts_unitsPerLambda = 100;
    style->exts_resistScale = 1000;
    style->exts_capScale = 1000;
    style->exts_numResistClasses = 0;

    for (r = 0; r < DBNumTypes; r++)
    {
	style->exts_resistByResistClass[r] = 0;
	TTMaskZero(&style->exts_typesByResistClass[r]);
	style->exts_typesResistChanged[r] = DBAllButSpaceAndDRCBits;
	TTMaskSetType(&style->exts_typesResistChanged[r], TT_SPACE);
	style->exts_typeToResistClass[r] = -1;
    }

    return (style);
}

/*
 * ----------------------------------------------------------------------------
 *
 * ExtTechLine --
 *
 * Process a line from the "extract" section of a technology file.
 *
 * Each line in the extract section of a technology begins
 * with a keyword that identifies the format of the rest of
 * the line.
 *
 * The following three kinds of lines are used to define the resistance
 * and parasitic capacitance to substrate of each tile type:
 *
 *	resist	 types resistance
 *	areacap	 types capacitance
 *	perimcap inside outside capacitance
 *
 * where 'types', 'inside', and 'outside' are comma-separated lists
 * of tile types, 'resistance' is an integer giving the resistance
 * per square in milli-ohms, and 'capacitance' is an integer giving
 * capacitance (per square lambda for areacap, or per lambda perimeter
 * for perimcap) in attofarads.
 *
 * The perimeter (sidewall) capacitance depends both on the types
 * inside and outside the perimeter.  For a given 'perimcap' line,
 * any segment of perimeter with a type in 'inside' inside the
 * perimeter and a type in 'outside' ontside the perimeter will
 * have the indicated capacitance.
 *
 * Both area and perimeter capacitance computed from the information
 * above apply between a given node and the substrate beneath it, as
 * determined by extSubstrate[].
 *
 * Contact resistances are specified by:
 *
 *	contact	type	minsize	resistance
 *
 * where type is the type of contact tile, minsize is chosen so that contacts
 * that are integer multiples of minsize get an additional contact cut for each
 * increment of minsize, and resistance is in milliohms.
 *
 * +++ FOR NOW, CONSIDER ALL SUBSTRATE TO BE AT GROUND +++
 *
 * Overlap coupling capacitance is specified by:
 *
 *	overlap	 toptypes bottomtypes capacitance [shieldtypes]
 *
 * where 'toptypes' and 'bottomtypes' are comma-separated lists of tile types,
 * and 'capacitance' is an integer giving capacitance in attofarads per
 * square lambda of overlap.  The sets 'toptypes' and 'bottomtypes' should
 * be disjoint.  Also, the union of the planes of 'toptypes' should be disjoint
 * from the union of the planes of 'bottomtypes'.  If 'shieldtypes' are
 * present, they should also be a comma-separated list of types, on
 * planes disjoint from those of either 'toptypes' or 'bottomtypes'.
 *
 * Whenever a tile of a type in 'toptypes' overlaps one of a type in
 * 'bottomtypes', we deduct the capacitance to substrate of the 'toptypes'
 * tile for the area of the overlap, and create an overlap capacitance
 * between the two nodes based on 'capacitance'.  When material in
 * 'shieldtypes' appears over any of this overlap area, however, we
 * only deduct the substrate capacitance; we don't create an overlap
 * capacitor.
 *
 * Sidewall coupling capacitance is specified by:
 *
 *	sidewall  intypes outtypes neartypes fartypes capacitance
 *
 * where 'intypes', 'outtypes', 'neartypes', and 'fartypes' are all comma-
 * separated lists of types, and 'capacitance' is an integer giving capacitance
 * in attofarads.  All of the tiles in all four lists should be on the same
 * plane.
 *
 * Whenever an edge of the form i|j is seen, where 'i' is in intypes and
 * 'j' is in outtypes, we search on the 'j' side for a distance of
 * ExtCurStyle->exts_sideCoupleHalo for edges with 'neartypes' on the
 * close side and 'fartypes' on the far side.  We create a capacitance
 * equal to the length of overlap, times capacitance, divided by the
 * separation between the edges (poor approximation, but better than
 * none).
 *
 * Sidewall overlap coupling capacitance is specified by:
 *
 *	sideoverlap  intypes outtypes ovtypes capacitance
 *
 * where 'intypes', 'outtypes', and 'ovtypes' are comma-separated lists
 * of types, and 'capacitance' is an integer giving capacitance in attofarads
 * per lambda.  Both intypes and outtypes should be in the same plane, and
 * ovtypes should be in a different plane from intypes and outtypes.
 *
 * The next kind of line describes transistors:
 *
 *	fet	 types terminals min-#terminals names substrate gscap gccap
 *
 * where 'types' and 'terminals' are comma-separated lists of tile types.
 * The meaning is that each type listed in 'types' is a transistor, whose
 * source and drain connect to any of the types listed in 'terminals'.
 * These transistors must have exactly min-#terminals terminals, in addition
 * to the gate (whose connectivity is specified in the system-wide connectivity
 * table in the "connect" section of the .tech file).  Currently gscap and
 * gccap are unused, but refer to the gate-source (or gate-drain) capacitance
 * and the gate-channel capacitance in units of attofarads per lambda and
 * attofarads per square lambda respectively.
 *
 * The resistances of transistors is specified by:
 *
 *	fetresist type region ohms
 *
 * where type is a type of tile that is a fet, region is a string ("linear"
 * is treated specially), and ohms is the resistance per square of the fet
 * type while operating in "region".  The values of fets in the "linear"
 * region are stored in a separate table.
 *
 * Results:
 *	Returns TRUE normally, or FALSE if the line from the
 *	technology file is so malformed that Magic should abort.
 *	Currently, we always return TRUE.
 *
 * Side effects:
 *	Initializes the per-technology variables that appear at the
 *	beginning of this file.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
ExtTechLine(sectionName, argc, argv)
    char *sectionName;
    int argc;
    char *argv[];
{
    int n, size, val, p1, p2, p3, gscap, gccap, nterm, pov, pshield, class;
    TileTypeBitMask types1, types2, near, far, ov, shield, subsTypes;
    char *subsName, *transName, *cp;
    register TileType s, t, r;
    struct keydesc *kp;
    bool isLinear;
    HashEntry *he;
    EdgeCap *cnew;
    ExtStyle *es;
    bool bad;

    if (argc < 1)
    {
	TechError("Each line must begin with a keyword\n");
	return (TRUE);
    }

    n = LookupStruct(argv[0], (LookupTable *) keyTable, sizeof keyTable[0]);
    if (n < 0)
    {
	TechError("Illegal keyword.  Legal keywords are:\n\t");
	for (n = 0; keyTable[n].k_name; n++)
	    TxError(" %s", keyTable[n].k_name);
	TxError("\n");
	return (TRUE);
    }

    kp = &keyTable[n];
    if (argc < kp->k_minargs || argc > kp->k_maxargs)
	goto usage;

    if (kp->k_key == STYLE)
    {
	ExtCurStyle = extTechStyleNew(argv[1]);
	if (ExtAllStyles == NULL) ExtAllStyles = ExtCurStyle;
	else
	{
	    /* Append to end of style list */
	    for (es = ExtAllStyles; es->exts_next; es = es->exts_next)
		/* Nothing */;
	    es->exts_next = ExtCurStyle;
	}
	return (TRUE);
    }

    if (ExtCurStyle == NULL)
    {
	TechError(
	    "No extraction style declared: starting new style \"default\".\n");
	ExtCurStyle = extTechStyleNew("default");
	ExtAllStyles = ExtCurStyle;
    }

    switch (kp->k_key)
    {
	case AREAC:
	case CONTACT:
	case FET:
	case FETRESIST:
	case OVERC:
	case PERIMC:
	case RESIST:
	case SIDEWALL:
	case SIDEOVERLAP:
	    DBTechNoisyNameMask(argv[1], &types1);
	    break;
	default:
	    break;
    }

    switch (kp->k_key)
    {
	case AREAC:
	    val = atoi(argv[2]);
	    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		if (TTMaskHasType(&types1, t))
		    ExtCurStyle->exts_areaCap[t] = val;
	    break;
	case CONTACT:
	    if (!StrIsInt(argv[2]))
	    {
		TechError("Contact minimum size %s must be numeric\n", argv[2]);
		break;
	    }
	    if (!StrIsInt(argv[3]))
	    {
		TechError("Contact resistivity %s must be numeric\n", argv[3]);
		break;
	    }
	    size = atoi(argv[2]);
	    val = atoi(argv[3]);
	    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		if (TTMaskHasType(&types1, t))
		{
		    ExtCurStyle->exts_viaSize[t] = size;
		    ExtCurStyle->exts_viaResist[t] = val;
		}
	    break;
	case CSCALE:
	    ExtCurStyle->exts_capScale = atoi(argv[1]);
	    break;
	case FET:
	    DBTechNoisyNameMask(argv[2], &types2);
	    nterm = atoi(argv[3]);
	    transName = argv[4];
	    subsName = argv[5];
	    cp = index(subsName, '!');
	    if (cp == NULL || cp[1] != '\0')
	    {
		TechError("Fet substrate node %s is not a global name\n",
		    subsName);
	    }
	    subsTypes = DBZeroTypeBits;
	    if (!StrIsInt(argv[6]))
	    {
		DBTechNoisyNameMask(argv[6], &subsTypes);
		gscap = atoi(argv[7]);
		gccap = (argc > 8) ? atoi(argv[8]) : 0;
	    }
	    else
	    {
		gscap = atoi(argv[6]);
		gccap = (argc > 7) ? atoi(argv[7]) : 0;
	    }
	    TTMaskSetMask(&ExtCurStyle->exts_transMask, &types1);
	    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		if (TTMaskHasType(&types1, t))
		{
		    TTMaskSetMask(ExtCurStyle->exts_transConn+t,&types1);
		    ExtCurStyle->exts_transSDTypes[t] = types2;
		    ExtCurStyle->exts_transSDCount[t] = nterm;
		    ExtCurStyle->exts_transSDCap[t] = gscap;
		    ExtCurStyle->exts_transGateCap[t] = gccap;
		    ExtCurStyle->exts_transName[t] =
			    StrDup((char **) NULL, transName);
		    ExtCurStyle->exts_transSubstrateName[t] =
			    StrDup((char **) NULL, subsName);
		    ExtCurStyle->exts_transSubstrateTypes[t] = subsTypes;
		}
	    break;
	case FETRESIST:
	    if (!StrIsInt(argv[3]))
	    {
		TechError("Fet resistivity %s must be numeric\n", argv[3]);
		break;
	    }
	    val = atoi(argv[3]);
	    isLinear = (strcmp(argv[2], "linear") == 0);
	    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		if (TTMaskHasType(&types1, t))
		{
		    he = HashFind(&ExtCurStyle->exts_transResist[t], argv[2]);
		    HashSetValue(he, val);
		    if (isLinear)
			ExtCurStyle->exts_linearResist[t] = val;
		}
	    break;
	case LAMBDA:
	    ExtCurStyle->exts_unitsPerLambda = atoi(argv[1]);
	    break;
	case OVERC:
	    DBTechNoisyNameMask(argv[2], &types2);
	    val = atoi(argv[3]);
	    bad = FALSE;
	    shield = DBZeroTypeBits;
	    if (argc > 4)
		DBTechNoisyNameMask(argv[4], &shield);
	    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    {
		if (!TTMaskHasType(&types1, s)) continue;
		for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		{
		    if (!TTMaskHasType(&types2, t)) continue;
		    if (s == t)
		    {
			TechError(
"Can't have overlap capacitance between tiles of the same type (%s)\n",
			    DBTypeLongName(s));
			bad = TRUE;
			continue;
		    }
		    p1 = DBPlane(s), p2 = DBPlane(t);
		    if (p1 == p2)
		    {
			TechError(
"Can't have overlap capacitance between tiles on the same plane (%s, %s)\n",
			    DBTypeLongName(s), DBTypeLongName(t));
			bad = TRUE;
			continue;
		    }
		    if (ExtCurStyle->exts_overlapCap[s][t])
		    {
			TechError(
"Only one of \"overlap %s %s\" or \"overlap %s %s\" allowed\n",
			    DBTypeLongName(s), DBTypeLongName(t),
			    DBTypeLongName(t), DBTypeLongName(s));
			bad = TRUE;
			continue;
		    }
		    ExtCurStyle->exts_overlapCap[s][t] = val;
		    ExtCurStyle->exts_overlapPlanes |= PlaneNumToMaskBit(p1);
		    ExtCurStyle->exts_overlapOtherPlanes[s]
			    |= PlaneNumToMaskBit(p2);
		    TTMaskSetType(&ExtCurStyle->exts_overlapTypes[p1], s);
		    TTMaskSetType(&ExtCurStyle->exts_overlapOtherTypes[s], t);
		    if (argc == 4) continue;

		    /* Shielding */
		    pshield = 0;
		    for (r = TT_TECHDEPBASE; r < DBNumTypes; r++)
		    {
			if (TTMaskHasType(&shield, r))
			{
			    p3 = DBPlane(r);
			    if (p3 == p1 || p3 == p2)
			    {
				TechError(
"Shielding type (%s) must be on a different plane from shielded types.\n",
					DBTypeLongName(r));
				bad = TRUE;
				continue;
			    }
			    pshield |= PlaneNumToMaskBit(p3);
			}
		    }
		    ExtCurStyle->exts_overlapShieldPlanes[s][t] = pshield;
		    ExtCurStyle->exts_overlapShieldTypes[s][t] = shield;
		}
	    }
	    if (bad)
		return (TRUE);
	    break;
	case SIDEOVERLAP:
	    DBTechNoisyNameMask(argv[2], &types2);
	    DBTechNoisyNameMask(argv[3], &ov);
	    pov = extTechTypesToPlanes(&ov);
	    val = atoi(argv[4]);
	    p3 = -1;
	    if (TTMaskHasType(&types1, TT_SPACE))
		TechError("Can't have space on inside of edge [ignored]\n");
	    /* It's ok to have the overlap be to space as long as a plane is */
	    /* specified.						     */
	    if (TTMaskHasType(&ov, TT_SPACE))
	    {
	    	 if ((cp = index(argv[3],'/')) == NULL)
		 {
		      TechError("Must specify plane for sideoverlap to space\n");
		 }
		 cp++;
		 p3 =  (int) dbTechNameLookup(cp, &dbPlaneNameLists);
		 if (p3 < 0)
		 {
		      TechError("Unknown overlap plane %s\n",argv[3]);
		 }
	    }
	    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    {
		if (!TTMaskHasType(&types1, s))
		    continue;
		p1 = DBPlane(s);
		if (PlaneMaskHasPlane(pov, p1))
		    goto diffplane;
		ExtCurStyle->exts_sidePlanes |= PlaneNumToMaskBit(p1);
		TTMaskSetType(&ExtCurStyle->exts_sideTypes[p1], s);
		TTMaskSetMask(&ExtCurStyle->exts_sideEdges[s], &types2);
		for (t = 0; t < DBNumTypes; t++)
		{
		    if (!TTMaskHasType(&types2, t))
			continue;
		    p2 = DBPlane(t);
		    if (t != TT_SPACE && PlaneMaskHasPlane(pov, p2))
			goto diffplane;
		    TTMaskSetMask(&ExtCurStyle->exts_sideOverlapOtherTypes[s][t], &ov);
		    ExtCurStyle->exts_sideOverlapOtherPlanes[s][t] |= pov;
		    MALLOC(EdgeCap *, cnew, sizeof (EdgeCap));
		    /* cnew->ec_far is unused */
		    cnew->ec_cap = val;
		    cnew->ec_near = ov;
		    cnew->ec_plane = p3;
		    cnew->ec_next = ExtCurStyle->exts_sideOverlapCap[s][t];
		    ExtCurStyle->exts_sideOverlapCap[s][t] = cnew;
		}
	    }
	    break;
	case SIDEWALL:
	    DBTechNoisyNameMask(argv[2], &types2);
	    DBTechNoisyNameMask(argv[3], &near);
	    DBTechNoisyNameMask(argv[4], &far);
	    if (TTMaskHasType(&types1, TT_SPACE))
		TechError("Can't have space on inside of edge [ignored]\n");
	    val = atoi(argv[5]);
	    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	    {
		if (!TTMaskHasType(&types1, s))
		    continue;
		ExtCurStyle->exts_sidePlanes |= PlaneNumToMaskBit(DBPlane(s));
		TTMaskSetType(&ExtCurStyle->exts_sideTypes[DBPlane(s)], s);
		TTMaskSetMask(&ExtCurStyle->exts_sideEdges[s], &types2);
		for (t = 0; t < DBNumTypes; t++)
		{
		    if (!TTMaskHasType(&types2, t))
			continue;
		    TTMaskSetMask(&ExtCurStyle->exts_sideCoupleOtherEdges[s][t], &far);
		    MALLOC(EdgeCap *, cnew, sizeof (EdgeCap));
		    cnew->ec_cap = val;
		    cnew->ec_near = near;
		    cnew->ec_far = far;
		    cnew->ec_next = ExtCurStyle->exts_sideCoupleCap[s][t];
		    cnew->ec_plane = -1;
		    ExtCurStyle->exts_sideCoupleCap[s][t] = cnew;
		}
	    }
	    break;
	case SIDEHALO:
	    ExtCurStyle->exts_sideCoupleHalo = atoi(argv[1]);
	    break;
	case PERIMC:
	    DBTechNoisyNameMask(argv[2], &types2);
	    val = atoi(argv[3]);
	    if (val == 0)
		break;
	    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
		for (t = 0; t < DBNumTypes; t++)
		    if (TTMaskHasType(&types1, s) && TTMaskHasType(&types2, t))
		    {
			ExtCurStyle->exts_perimCap[s][t] = val;
			TTMaskSetType(&ExtCurStyle->exts_perimCapMask[s], t);
		    }
	    break;
	case RESIST:
	    val = atoi(argv[2]);
	    class = ExtCurStyle->exts_numResistClasses++;
	    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
		if (TTMaskHasType(&types1, t))
		{
		    ExtCurStyle->exts_sheetResist[t] = val;
		    ExtCurStyle->exts_typeToResistClass[t] = class;
		}
	    ExtCurStyle->exts_resistByResistClass[class] = val;
	    ExtCurStyle->exts_typesByResistClass[class] = types1;
	    break;
	case RSCALE:
	    ExtCurStyle->exts_resistScale = atoi(argv[1]);
	    break;
	case STEP:
	    val = atoi(argv[1]);
	    if (val <= 0)
	    {
		TechError("Hierarchical interaction step size must be > 0\n");
		return (FALSE);
	    }
	    ExtCurStyle->exts_stepSize = val;
	    break;
    }
    return (TRUE);

usage:
    TechError("Malformed line for keyword %s.  Correct usage:\n\t%s %s\n",
		    kp->k_name, kp->k_name, kp->k_usage);
    return (TRUE);

diffplane:
    TechError("Overlapped types in \"sideoverlap\" rule must be on\n\
\tdifferent plane from intypes and outtypes.\n");
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTechTypesToPlanes --
 *
 * Like DBTechTypesToPlanes, but makes sure that TT_SPACE is turned off in
 * its argument mask.
 *
 * Results:
 *	Returns a mask of the planes occupied by the types in the
 *	mask pointed to by 'm'.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

extTechTypesToPlanes(m)
    TileTypeBitMask *m;
{
    TileTypeBitMask mnew;

    mnew = *m;
    TTMaskClearType(&mnew, TT_SPACE);
    return (DBTechTypesToPlanes(&mnew));
}

/*
 * ----------------------------------------------------------------------------
 *
 * ExtTechFinal --
 *
 * Postprocess the technology specific information for extraction.
 * Builds the connectivity tables exts_nodeConn[], exts_resistConn[],
 * and exts_transConn[].
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the tables mentioned above.
 *	Leaves ExtCurStyle pointing to the first style in the list
 *	ExtAllStyles.
 *
 * ----------------------------------------------------------------------------
 */

Void
ExtTechFinal()
{
    ExtStyle *es;

    /* Create a "default" style if there isn't one */
    if (ExtAllStyles == NULL)
    {
	ExtCurStyle = extTechStyleNew("default");
	ExtAllStyles = ExtCurStyle;
    }

    /* Final cleanup for all styles */
    for (es = ExtAllStyles; es; es = es->exts_next)
	extTechFinalStyle(es);

    /* The current style is initially the first style in the list */
    ExtCurStyle = ExtAllStyles;
}

Void
extTechFinalStyle(style)
    ExtStyle *style;
{
    TileTypeBitMask maskBits;
    register TileType r, s, t;
    int p1;

    for (r = TT_TECHDEPBASE; r < DBNumTypes; r++)
    {
	maskBits = style->exts_nodeConn[r] = DBConnectTbl[r];
	if (!TTMaskHasType(&style->exts_transMask, r))
	{
	     TTMaskZero(&style->exts_transConn[r]);
	}
	for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	{
	    if (TTMaskHasType(&maskBits, s))
		if (style->exts_typeToResistClass[s]
			!= style->exts_typeToResistClass[r])
		    TTMaskClearType(&maskBits, s);
	}
	style->exts_resistConn[r] = maskBits;
    }

    /* r ranges over types, s over resistance entries */
    for (r = TT_TECHDEPBASE; r < DBNumTypes; r++)
    {
	s = style->exts_typeToResistClass[r];
	TTMaskClearMask(&style->exts_typesResistChanged[r],
			&style->exts_typesByResistClass[s]);
    }

    /*
     * Consistency check:
     * If a type R shields S from T, make sure that R is listed as
     * being in the list of overlapped types for S, even if there
     * was no overlap capacitance explicitly specified for this
     * pair of types in an "overlap" line.  This guarantees that
     * R will shield S from substrate even if there is no capacitance
     * associated with the overlap.
     */
    for (s = TT_TECHDEPBASE; s < DBNumTypes; s++)
	for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
	{
	    if (style->exts_overlapShieldPlanes[s][t] == 0)
		continue;
	    for (r = TT_TECHDEPBASE; r < DBNumTypes; r++)
	    {
		if (!TTMaskHasType(&style->exts_overlapShieldTypes[s][t], r))
		    continue;
		p1 = DBPlane(s);
		style->exts_overlapPlanes |= PlaneNumToMaskBit(p1);
		style->exts_overlapOtherPlanes[s]
			|= PlaneNumToMaskBit(DBPlane(r));
		TTMaskSetType(&style->exts_overlapTypes[p1], s);
		TTMaskSetType(&style->exts_overlapOtherTypes[s], r);
	    }
	}
}
