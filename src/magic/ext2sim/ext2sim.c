/*
 * ext2sim.c --
 *
 * Program to flatten hierarchical .ext files and produce
 * a .sim file, suitable for use as input to simulators
 * such as esim and crystal.
 *
 * Flattens the tree rooted at file.ext, reading in additional .ext
 * files as specified by "use" lines in file.ext.  The output is left
 * in file.sim, unless '-o esSimFile' is specified, in which case the
 * output is left in 'esSimFile'.
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
static char rcsid[] = "$Header: ext2sim.c,v 6.0 90/08/28 19:03:23 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include "magic.h"
#include "paths.h"
#include "geometry.h"
#include "hash.h"
#include "utils.h"
#include "pathvisit.h"
#include "extflat.h"
#include "runstats.h"

/* Forward declarations */
int mainArgs();
int capVisit(), fetVisit(), nodeVisit(), resistVisit();

/* Options specific to ext2sim */
bool esFetNodesOnly = FALSE;
bool esNoAlias = FALSE;
bool esNoAttrs = FALSE;
bool esNoLabel = FALSE;
char esDefaultOut[FNSIZE], esDefaultAlias[FNSIZE], esDefaultLabel[FNSIZE];
char *esOutName = esDefaultOut, *esAliasName = esDefaultAlias;
char *esLabelName = esDefaultLabel;
FILE *esSimF = NULL;
FILE *esAliasF = NULL;
FILE *esLabF = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * main --
 *
 * Top level of ext2sim.
 *
 * ----------------------------------------------------------------------------
 */

main(argc, argv)
    char *argv[];
{
    int flatFlags;
    char *inName;
    FILE *f;

    /* Process command line arguments */
    EFInit();
    inName = EFArgs(argc, argv, mainArgs, (ClientData) NULL);
    if (inName == NULL)
	exit (1);

    /*
     * Initializations specific to this program.
     * Make output, alias, and label names be of the form
     * inName.suffix if they weren't explicitly specified,
     * where suffix is .sim, .al, or .nodes.
     */

    if (esOutName == esDefaultOut)
	(void) sprintf(esDefaultOut, "%s.sim", inName);
    if (esAliasName == esDefaultAlias)
	(void) sprintf(esDefaultAlias, "%s.al", inName);
    if (esLabelName == esDefaultLabel)
	(void) sprintf(esDefaultLabel, "%s.nodes", inName);

    if ((esSimF = fopen(esOutName, "w")) == NULL)
    {
	perror(esOutName);
	exit (1);
    }
    if (!esNoAlias && (esAliasF = fopen(esAliasName, "w")) == NULL)
    {
	perror(esAliasName);
	exit (1);
    }
    if (!esNoLabel && (esLabF = fopen(esLabelName, "w")) == NULL)
    {
	perror(esLabelName);
	exit (1);
    }

    /* Read the hierarchical description of the input circuit */
    EFReadFile(inName);
    (void) fprintf(esSimF, "| units: %d	tech: %s\n", EFScale, EFTech);

    /* Convert the hierarchical description to a flat one */
    flatFlags = EF_FLATNODES;
    if (EFCapThreshold < INFINITE_THRESHOLD) flatFlags |= EF_FLATCAPS;
    if (EFResistThreshold < INFINITE_THRESHOLD) flatFlags |= EF_FLATRESISTS;
    EFFlatBuild(inName, flatFlags);
    EFVisitFets(fetVisit, (ClientData) NULL);
    if (flatFlags & EF_FLATCAPS)
	EFVisitCaps(capVisit, (ClientData) NULL);
    EFVisitResists(resistVisit, (ClientData) NULL);
    EFVisitNodes(nodeVisit, (ClientData) NULL);

#ifdef	free_all_mem
    EFFlatDone();
    EFDone();
#endif	free_all_mem

    printf("Memory used: %s\n", RunStats(RS_MEM, NULL, NULL));
    exit (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * mainArgs --
 *
 * Process those arguments that are specific to ext2sim.
 * Assumes that *pargv[0][0] is '-', indicating a flag
 * argument.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	After processing an argument, updates *pargc and *pargv
 *	to point to after the argument.
 *
 *	May initialize various global variables based on the
 *	arguments given to us.
 *
 *	Exits in the event of an improper argument.
 *
 * ----------------------------------------------------------------------------
 */

Void
mainArgs(pargc, pargv)
    int *pargc;
    char ***pargv;
{
    char **argv = *pargv, *cp;
    int argc = *pargc;

    switch (argv[0][1])
    {
	case 'A':
	    esNoAlias = TRUE;
	    break;
	case 'B':
	    esNoAttrs = TRUE;
	    break;
	case 'F':
	    esFetNodesOnly = TRUE;
	    break;
	case 'L':
	    esNoLabel = TRUE;
	    break;
	case 'a':
	    if ((esAliasName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 'l':
	    if ((esLabelName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 'o':
	    if ((esOutName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	default:
	    (void) fprintf(stderr, "Unrecognized flag: %s\n", argv[0]);
	    goto usage;
    }

    *pargv = argv;
    *pargc = argc;
    return;

usage:
    (void) fprintf(stderr, "\
Usage: ext2sim [-a aliasfile] [-A] [-B] [-l labelfile] [-L]\n\
		[-o simfile] file\n\
       or else see options to extcheck(1)\n");
    exit (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * fetVisit --
 *
 * Procedure to output a single fet to the .sim file.
 * Called by EFVisitFets().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSimF.
 *
 * Format of a .sim fet line:
 *
 *	type gate source drain l w x y g= s= d=
 *
 * where
 *	type is a name identifying this type of transistor
 *	gate, source, and drain are the nodes to which these three
 *		terminals connect
 *	l, w are the length and width of the channel
 *	x, y are the x, y coordinates of a point within the channel.
 *	g=, s=, d= are the (optional) attributes; if present, each
 *		is followed by a comma-separated list of attributes.
 *
 * ----------------------------------------------------------------------------
 */

int
fetVisit(fet, hierName, trans, l, w)
    Fet *fet;		/* Fet being output */
    HierName *hierName;	/* Hierarchical path down to this fet */
    Transform *trans;	/* Coordinate transform for output */
    int l, w;
{
    FetTerm *gate, *source, *drain;
    int scale;
    Rect r;

    /* If no terminals, can't do much of anything */
    if (fet->fet_nterm < 2)
	return 0;

    /* If only two terminals, connect the source to the drain */
    gate = &fet->fet_terms[0];
    source = drain = &fet->fet_terms[1];
    if (fet->fet_nterm >= 3)
	drain = &fet->fet_terms[2];

    /* Kludge for .sim: transistor types can only be one character */
    (void) fprintf(esSimF, "%c", EFFetTypes[fet->fet_type][0]);

    /* Output gate, source, and drain node names */
    fetOutNode(hierName, gate->fterm_node->efnode_name->efnn_hier, esSimF);
    fetOutNode(hierName, source->fterm_node->efnode_name->efnn_hier, esSimF);
    fetOutNode(hierName, drain->fterm_node->efnode_name->efnn_hier, esSimF);

    /*
     * Scale L and W appropriately by the same amount as distance
     * values in the transform.  The transform will have a scale
     * different from 1 only in the case when the scale factors of
     * some of the .ext files differed, making it necessary to scale
     * all dimensions explicitly instead of having a single scale
     * factor at the beginning of the .sim file.
     */
    GeoTransRect(trans, &fet->fet_rect, &r);
    scale = GeoScale(trans);
    (void) fprintf(esSimF, " %d %d %d %d",
		l*scale, w*scale, r.r_xbot, r.r_ybot);

    /* Attributes, if present */
    if (!esNoAttrs)
    {
	if (gate->fterm_attrs)
	    (void) fprintf(esSimF, " g=%s", gate->fterm_attrs);
	if (source->fterm_attrs)
	    (void) fprintf(esSimF, " s=%s", source->fterm_attrs);
	if (drain->fterm_attrs)
	    (void) fprintf(esSimF, " d=%s", drain->fterm_attrs);
    }
    (void) fprintf(esSimF, "\n");
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * fetOutNode --
 *
 * Output the name of the node whose hierarchical prefix down to this
 * point is 'prefix' and whose name from the end of suffix down to the
 * leaves is 'suffix', just as in the arguments to EFHNConcat().
 *
 * Sets the efnode_client field of the node found in the global name
 * table to esFetNodesOnly.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the file 'outf'.
 *	Sets the efnode_client field as described above.
 *
 * ----------------------------------------------------------------------------
 */

Void
fetOutNode(prefix, suffix, outf)
    HierName *prefix;
    HierName *suffix;
    FILE *outf;
{
    HashEntry *he;
    EFNodeName *nn;

    he = EFHNConcatLook(prefix, suffix, "output");
    if (he == NULL)
    {
	(void) fprintf(outf, " GND");
	return;
    }

    /* Canonical name */
    nn = (EFNodeName *) HashGetValue(he);
    (void) putc(' ', outf);
    EFHNOut(nn->efnn_node->efnode_name->efnn_hier, outf);
    nn->efnn_node->efnode_client = (ClientData) esFetNodesOnly;
}

/*
 * ----------------------------------------------------------------------------
 *
 * capVisit --
 *
 * Procedure to output a single capacitor to the .sim file.
 * Called by EFVisitCaps().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSimF.
 *
 * Format of a .sim cap line:
 *
 *	C node1 node2 cap
 *
 * where
 *	node1, node2 are the terminals of the capacitor
 *	cap is the capacitance in femtofarads (NOT attofarads).
 *
 * ----------------------------------------------------------------------------
 */

int
capVisit(hierName1, hierName2, cap)
    HierName *hierName1;
    HierName *hierName2;
    int cap;
{
    cap = (cap + 500) / 1000;
    if (cap <= EFCapThreshold)
	return 0;

    (void) fprintf(esSimF, "C ");
    EFHNOut(hierName1, esSimF);
    (void) fprintf(esSimF, " ");
    EFHNOut(hierName2, esSimF);
    (void) fprintf(esSimF, " %d\n", cap);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * resistVisit --
 *
 * Procedure to output a single resistor to the .sim file.
 * Called by EFVisitResists().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSimF.
 *
 * Format of a .sim resistor line:
 *
 *	r node1 node2 res
 *
 * where
 *	node1, node2 are the terminals of the resistor
 *	res is the resistance in ohms (NOT milliohms)
 *
 *
 * ----------------------------------------------------------------------------
 */

int
resistVisit(hierName1, hierName2, res)
    HierName *hierName1;
    HierName *hierName2;
    int res;
{
    res = (res+ 500)/1000;

    (void) fprintf(esSimF, "r ");
    EFHNOut(hierName1, esSimF);
    (void) fprintf(esSimF, " ");
    EFHNOut(hierName2, esSimF);
    (void) fprintf(esSimF, " %d\n", res);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * nodeVisit --
 *
 * Procedure to output a single node to the .sim file, along with
 * its aliases to the .al file and its location to the .nodes file.
 * Called by EFVisitNodes().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the files esSimF, esAliasF, and esLabF.
 *
 * ----------------------------------------------------------------------------
 */

int
nodeVisit(node, res, cap)
    register EFNode *node;
    int res, cap;
{
    register EFNodeName *nn;
    HierName *hierName;
    bool isGlob;
    char *fmt;
    EFAttr *ap;

    if (esFetNodesOnly && node->efnode_client == (ClientData) 0)
	return 0;

    hierName = (HierName *) node->efnode_name->efnn_hier;
    cap = (cap + 500) / 1000;
    res = (res + 500) / 1000;
    if (cap > EFCapThreshold)
    {
	(void) fprintf(esSimF, "C ");
	EFHNOut(hierName, esSimF);
	(void) fprintf(esSimF, " GND %d\n", cap);
    }
    if (res > EFResistThreshold)
    {
	(void) fprintf(esSimF, "R ");
	EFHNOut(hierName, esSimF);
	(void) fprintf(esSimF, " %d\n", res);
    }
    if (node->efnode_attrs && !esNoAttrs)
    {
	(void) fprintf(esSimF, "A ");
	EFHNOut(hierName, esSimF);
	for (fmt = " %s", ap = node->efnode_attrs; ap; ap = ap->efa_next)
	{
	    (void) fprintf(esSimF, fmt, ap->efa_text);
	    fmt = ",%s";
	}
	putc('\n', esSimF);
    }

    if (esAliasF)
    {
	isGlob = EFHNIsGlob(hierName);
	for (nn = node->efnode_name->efnn_next; nn; nn = nn->efnn_next)
	{
	    if (isGlob && EFHNIsGlob(nn->efnn_hier))
		continue;
	    (void) fprintf(esAliasF, "= ");
	    EFHNOut(hierName, esAliasF);
	    (void) fprintf(esAliasF, " ");
	    EFHNOut(nn->efnn_hier, esAliasF);
	    (void) fprintf(esAliasF, "\n");
	}
    }

    if (esLabF)
    {
	(void) fprintf(esLabF, "94 ");
	EFHNOut(hierName, esLabF);
	(void) fprintf(esLabF, " %d %d %s;\n",
		    node->efnode_loc.r_xbot, node->efnode_loc.r_ybot,
		    EFLayerNames[node->efnode_type]);
    }

    return 0;
}
