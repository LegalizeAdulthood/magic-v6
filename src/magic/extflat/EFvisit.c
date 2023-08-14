/*
 * EFvisit.c -
 *
 * Procedures to traverse and output flattened nodes, capacitors,
 * transistors, resistors, and Distances.
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
static char rcsid[] = "$Header: EFvisit.c,v 6.0 90/08/28 18:13:43 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <math.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "hash.h"
#include "malloc.h"
#include "utils.h"
#include "extflat.h"
#include "EFint.h"

/* Root of the tree being flattened */
Def *efFlatRootDef;
Use efFlatRootUse;
HierContext efFlatContext;

/*
 * ----------------------------------------------------------------------------
 *
 * EFVisitFets --
 *
 * Visit all the fets in the circuit.
 * Must be called after EFFlatBuild().
 * For each fet in the circuit, call the user-supplied procedure
 * (*fetProc)(), which should be of the following form:
 *
 *	(*fetProc)(fet, hierName, trans, l, w, cdata)
 *	    Fet *fet;
 *	    HierName *hierName;
 *	    Transform *trans;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * The procedure should return 0 normally, or 1 to abort the
 * search.
 *
 * We ensure that no fets connected to killed nodes are passed
 * to this procedure.
 *
 * Results:
 *	Returns 0 if terminated normally, or 1 if the search
 *	was aborted.
 *
 * Side effects:
 *	Whatever (*fetProc)() does.
 *
 * ----------------------------------------------------------------------------
 */

EFVisitFets(fetProc, cdata)
    int (*fetProc)();
    ClientData cdata;
{
    CallArg ca;

    ca.ca_proc = fetProc;
    ca.ca_cdata = cdata;
    return efVisitFets(&efFlatContext, (ClientData) &ca);
}

/*
 * Procedure to visit recursively all fets in the design.
 * Does all the work of EFVisitFets() above.
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	Calls the client procedure (*ca->ca_proc)().
 */

int
efVisitFets(hc, ca)
    register HierContext *hc;
    register CallArg *ca;
{
    Def *def = hc->hc_use->use_def;
    FetTerm *gate, *source, *drain;
    register Fet *fet;
    int scale, l, w;
    Transform t;

    /* Recursively visit fets in our children first */
    if (efHierSrUses(hc, efVisitFets, (ClientData) ca))
	return 1;

    /*
     * Compute the proper transform to convert distance values to
     * scaled distances.  This transform will have a scale different
     * from 1 only in the case when the scale factors of some of the
     * .ext files differed, making it necessary to scale all dimensions
     * explicitly instead of having a single scale factor applying to
     * the entire circuit.
     */
    scale = def->def_scale;
    t = hc->hc_trans;
    if (efScaleChanged && scale != 1)
    {
	t.t_a *= scale; t.t_b *= scale; t.t_c *= scale;
	t.t_d *= scale; t.t_e *= scale; t.t_f *= scale;
    }

    /* Visit our own fets */
    for (fet = def->def_fets; fet; fet = fet->fet_next)
    {
	if (efFetKilled(fet, hc->hc_hierName))
	    continue;

	gate = &fet->fet_terms[0];
	source = drain = &fet->fet_terms[1];
	if (fet->fet_nterm >= 3)
	    drain = &fet->fet_terms[2];

	/*
	 * L, W, and flat coordinates of a point inside the channel.
	 * Handle FETs with two terminals (capacitors) separately.
	 */
	if (fet->fet_nterm == 2)
	{
	    l = (fet->fet_perim - (int) sqrt((float)
		(fet->fet_perim * fet->fet_perim - 16 * fet->fet_area))) / 4;
	    w = fet->fet_area / l;
	}
	else
	{
	    l = gate->fterm_perim / 2;
	    w = (source->fterm_perim + drain->fterm_perim)/2;
	}
	if (gate->fterm_attrs) efFetFixLW(gate->fterm_attrs, &l, &w);
	if ((*ca->ca_proc)(fet, hc->hc_hierName, &t, l, w, ca->ca_cdata))
	    return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFetKilled --
 *
 * Check all of the nodes to which the fet 'fet' is connected (its
 * hierarchical prefix is hc->hc_hierName).  If any of these nodes
 * have been killed, then the fet is also killed.
 *
 * Results:
 *	TRUE if the fet is connected to a killed node, FALSE if it's ok.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
efFetKilled(fet, prefix)
    Fet *fet;
    HierName *prefix;
{
    HierName *suffix;
    HashEntry *he;
    EFNodeName *nn;
    int n;

    for (n = 0; n < fet->fet_nterm; n++)
    {
	suffix = fet->fet_terms[n].fterm_node->efnode_name->efnn_hier;
	he = EFHNConcatLook(prefix, suffix, "kill");
	if (he  && (nn = (EFNodeName *) HashGetValue(he))
		&& (nn->efnn_node->efnode_flags & EF_KILLED))
	    return TRUE;
    }

    return FALSE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFetFixLW --
 *
 * Called for any fets that have gate attributes; these attributes may
 * specify the L and W of the fet explicitly.  The attributes will be
 * of the form ext:l=value or ext:w=value, where value is either numerical
 * or symbolic; if symbolic the symbol must have been defined via efSymAdd().
 * If the value is symbolic but wasn't defined by efSymAdd(), it's ignored.
 * The variables *pL and *pW are changed to reflect the new L and W as
 * appropriate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

Void
efFetFixLW(attrs, pL, pW)
    char *attrs;
    int *pL, *pW;
{
    register char *cp, *ep;
    char attrName, savec;
    int value;

    cp = attrs;
    while (cp && *cp)
    {
	if (*cp != 'e' || strncmp(cp, "ext:", 4) != 0)
	    goto skip;

	cp += 4;
	if (*cp && cp[1] == '=')
	{
	    switch (*cp)
	    {
		case 'w': case 'W':
		    attrName = 'w';
		    goto both;
		case 'l': case 'L':
		    attrName = 'l';
		both:
		    cp += 2;
		    for (ep = cp; *ep && *ep != ','; ep++)
			/* Nothing */;
		    savec = *ep;
		    *ep = '\0';
		    if (StrIsInt(cp)) value = atoi(cp);
		    else if (!efSymLook(cp, &value)) goto done;

		    if (attrName == 'w')
			*pW = value;
		    else if (attrName == 'l')
			*pL = value;

		done:
		    *ep = savec;
	    }
	}

skip:
	/* Skip to next attribute */
	while (*cp && *cp++ != ',')
	    /* Nothing */;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFVisitResists --
 *
 * Visit all the resistors in the circuit.
 * Must be called after EFFlatBuild().
 * For each resistor in the circuit, call the user-supplied procedure
 * (*resProc)(), which should be of the following form, where hn1 and
 * hn2 are the HierNames of the two nodes connected by the resistor.
 *
 *	(*resProc)(hn1, hn2, resistance, cdata)
 *	    HierName *hn1, *hn2;
 *	    int resistance;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * The procedure should return 0 normally, or 1 to abort the
 * search.
 *
 * We ensure that no resistors connected to killed nodes are passed
 * to this procedure.
 *
 * Results:
 *	Returns 0 if terminated normally, or 1 if the search
 *	was aborted.
 *
 * Side effects:
 *	Whatever (*resProc)() does.
 *
 * ----------------------------------------------------------------------------
 */

EFVisitResists(resProc, cdata)
    int (*resProc)();
    ClientData cdata;
{
    CallArg ca;

    ca.ca_proc = resProc;
    ca.ca_cdata = cdata;
    return efVisitResists(&efFlatContext, (ClientData) &ca);
}

/*
 * Procedure to visit recursively all resistors in the design.
 * Does all the work of EFVisitResists() above.
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	Calls the client procedure (*ca->ca_proc)().
 */

extern int efVisitSingleResist();

int
efVisitResists(hc, ca)
    register HierContext *hc;
    register CallArg *ca;
{
    Def *def = hc->hc_use->use_def;
    register Connection *res;
    Transform t;
    int scale;

    /* Recursively visit resistors in our children first */
    if (efHierSrUses(hc, efVisitResists, (ClientData) ca))
	return 1;

    /* Visit our own resistors */
    for (res = def->def_resistors; res; res = res->conn_next)
    {
	/* Special case for speed if no arraying info */
	if (res->conn_1.cn_nsubs == 0)
	{
	    if (efVisitSingleResist(hc, res->conn_name1, res->conn_name2,
			res, ca))
		return 1;
	}
	else if (efHierSrArray(hc, res, efVisitSingleResist, (ClientData) ca))
	    return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efVisitSingleResist --
 *
 * Visit a resistor of res->conn_value milliohms between the nodes
 * 'name1' and 'name2' (text names, not hierarchical names).  Don't
 * process the resistor if either terminal is a killed node.
 *
 * Results:
 *	Whatever the user-supplied procedure (*ca->ca_proc)() returns.
 *
 * Side effects:
 *	Calls the user-supplied procedure.
 *
 * ----------------------------------------------------------------------------
 */

efVisitSingleResist(hc, name1, name2, res, ca)
    HierContext *hc;		/* Contains hierarchical pathname to cell */
    char *name1, *name2;	/* Names of nodes connecting to resistor */
    Connection *res;		/* Contains resistance to add */
    register CallArg *ca;
{
    EFNode *n1, *n2;
    HashEntry *he;
    EFCoupleKey ck;

    if ((he = EFHNLook(hc->hc_hierName, name1, "resist(1)")) == NULL)
	return 0;
    n1 = ((EFNodeName *) HashGetValue(he))->efnn_node;
    if (n1->efnode_flags & EF_KILLED)
	return 0;

    if ((he = EFHNLook(hc->hc_hierName, name2, "resist(2)")) == NULL)
	return;
    n2 = ((EFNodeName *) HashGetValue(he))->efnn_node;
    if (n2->efnode_flags & EF_KILLED)
	return 0;

    /* Do nothing if the nodes aren't different */
    if (n1 == n2)
	return 0;

    return (*ca->ca_proc)(n1->efnode_name->efnn_hier,
		n2->efnode_name->efnn_hier,
		res->conn_value, ca->ca_cdata);
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFVisitCaps --
 *
 * Visit all the capacitors built up by efFlatCaps.
 * Calls the user-provided procedure (*capProc)()
 * which should be of the following format:
 *
 *	(*capProc)(hierName1, hierName2, cap, cdata)
 *	    HierName *hierName1, *hierName2;
 *	    int cap;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * Here cap is the capacitance in attofarads.
 *
 * Results:
 *	Returns 1 if the client procedure returned 1;
 *	otherwise returns 0.
 *
 * Side effects:
 *	Calls the user-provided procedure (*capProc)().
 *
 * ----------------------------------------------------------------------------
 */

int
EFVisitCaps(capProc, cdata)
    int (*capProc)();
    ClientData cdata;
{
    HashSearch hs;
    HashEntry *he;
    EFCoupleKey *ck;
    int cap;

    HashStartSearch(&hs);
    while (he = HashNext(&efCapHashTable, &hs))
    {
	cap = (int) HashGetValue(he);
	ck = (EFCoupleKey *) he->h_key.h_words;
	if ((*capProc)(ck->ck_1->efnode_name->efnn_hier,
		       ck->ck_2->efnode_name->efnn_hier,
		       cap, cdata))
	    return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFVisitNodes --
 *
 * Procedure to visit all flat nodes in the circuit.
 * For each node, calls the procedure (*nodeProc)(),
 * which should be of the following form:
 *
 *	(*nodeProc)(node, r, c, cdata)
 *	    EFNode *node;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * Where 'r' and 'c' are the lumped resistance estimate
 * and capacitance to ground, in milliohms and attofarads
 * respectively.  When either falls below the threshold
 * for output, they are passed as 0.
 *
 * Results:
 *	Returns 1 if (*nodeProc)() returned 1 to abort the
 *	search; otherwise, returns 0.
 *
 * Side effects:
 *	Calls (*nodeProc)().
 *
 * ----------------------------------------------------------------------------
 */

int
EFVisitNodes(nodeProc, cdata)
    int (*nodeProc)();
    ClientData cdata;
{
    register EFNode *node;
    register EFNodeName *nn;
    HierName *hierName;
    int cap, res;

    for (node = (EFNode *) efNodeList.efnode_next;
	    node != &efNodeList;
	    node = (EFNode *) node->efnode_next)
    {
	res = EFNodeResist(node);
	cap = node->efnode_cap;
	hierName = (HierName *) node->efnode_name->efnn_hier;
	if (EFHNIsGND(hierName))
	    cap = 0;
	if (efWatchNodes)
	{
	    for (nn = node->efnode_name; nn; nn = nn->efnn_next)
		if (HashLookOnly(&efWatchTable, (char *) nn->efnn_hier))
		{
		    printf("Equivalent nodes:\n");
		    for (nn = node->efnode_name; nn; nn = nn->efnn_next)
			printf("\t%s\n", EFHNToStr(nn->efnn_hier));
		    break;
		}
	}

	if (node->efnode_flags & EF_KILLED)
	    continue;

	if ((*nodeProc)(node, res, cap, cdata))
	    return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFNodeResist --
 *
 * The input to this procedure is a pointer to a EFNode.
 * Its resistance is computed from the area and perimeter stored
 * in the array efnode_pa.
 *
 * We approximate the resistive region as a collection of rectangles
 * of width W and length L, one for each set of layers having a different
 * sheet resistivity.  We do so by noting that for a rectangle,
 *
 *		Area = L * W
 *		Perimeter = 2 * (L + W)
 *
 * Solving the two simultaneous equations for L yields the following
 * quadratic:
 *
 *		2 * (L**2) - Perimeter * L + 2 * Area = 0
 *
 * Solving this quadratic for L, the longer dimension, we get
 *
 *		L = (Perimeter + S) / 4
 *
 * where
 *
 *		S = sqrt( (Perimeter**2) - 16 * Area )
 *
 * The smaller dimension is W, ie,
 *
 *		W = (Perimeter - S) / 4
 *
 * The resistance is L / W squares:
 *
 *			Perimeter + S
 *		R =	-------------
 *			Perimeter - S
 *
 * Results:
 *	Returns the resistance.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
EFNodeResist(node)
    register EFNode *node;
{
    register int n, perim, area;
    float s, fperim;
    double v, dresist;
    int resist;

    resist = 0;
    for (n = 0; n < efNumResistClasses; n++)
    {
	area = node->efnode_pa[n].pa_area;
	perim = node->efnode_pa[n].pa_perim;
	if (area > 0 && perim > 0)
	{
	    v = (double) perim * (double) perim - 16.0 * area;

	    /* Approximate by one square if v < 0; shouldn't happen! */
	    if (v < 0.0) s = 0.0; else s = sqrt(v);

	    fperim = (float) perim;
	    dresist = (fperim + s)/(fperim - s) * efResists[n];
	    if (dresist + (double) MAXINT > (double) MAXINT)
		resist = MAXINT;
	    else
		resist += dresist;
	}
    }

    return (resist);
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFLookDist --
 *
 * Look for the Distance between two points given by their HierNames.
 *
 * Results:
 *	TRUE if a distance was found, FALSE if not.
 *
 * Side effects:
 *	Sets *pMinDist and *pMaxDist to the min and max distances
 *	if found.
 *
 * ----------------------------------------------------------------------------
 */

bool
EFLookDist(hn1, hn2, pMinDist, pMaxDist)
    HierName *hn1, *hn2;
    int *pMinDist, *pMaxDist;
{
    Distance distKey, *dist;
    HashEntry *he;

    if (EFHNBest(hn1, hn2))
    {
	distKey.dist_1 = hn1;
	distKey.dist_2 = hn2;
    }
    else
    {
	distKey.dist_1 = hn2;
	distKey.dist_2 = hn1;
    }
    he = HashLookOnly(&efDistHashTable, (char *) &distKey);
    if (he == NULL)
	return FALSE;

    dist = (Distance *) HashGetValue(he);
    *pMinDist = dist->dist_min;
    *pMaxDist = dist->dist_max;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNOut --
 *
 * Output a hierarchical node name.
 * The flags in EFTrimFlags control whether global (!) or local (#)
 * suffixes are to be trimmed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the files 'outf'.
 *
 * ----------------------------------------------------------------------------
 */

Void
EFHNOut(hierName, outf)
    HierName *hierName;
    register FILE *outf;
{
    bool trimGlob, trimLocal;
    register char *cp, c;

    if (hierName->hn_parent) efHNOutPrefix(hierName->hn_parent, outf);
    if (EFTrimFlags)
    {
	cp = hierName->hn_name; 
	trimGlob = (EFTrimFlags & EF_TRIMGLOB);
	trimLocal = (EFTrimFlags & EF_TRIMLOCAL);
	while (c = *cp++)
	{
	    if (*cp) (void) putc(c, outf);
	    else switch (c)
	    {
		case '!':	if (!trimGlob) (void) putc(c, outf); break;
		case '#':	if (trimLocal) break;
		default:	(void) putc(c, outf); break;
	    }
	}
    }
    else (void) fputs(hierName->hn_name, outf);
}

Void
efHNOutPrefix(hierName, outf)
    register HierName *hierName;
    register FILE *outf;
{
    register char *cp, c;

    if (hierName->hn_parent)
	efHNOutPrefix(hierName->hn_parent, outf);

    cp = hierName->hn_name;
    while (c = *cp++)
	putc(c, outf);
    putc('/', outf);
}
