/*
 * ext2spice.c --
 *
 * Program to flatten hierarchical .ext files and produce
 * a .spice deck.
 *
 * Flattens the tree rooted at file.ext, reading in additional .ext
 * files as specified by "use" lines in file.ext.  The output is left
 * in file.spice, unless '-o esSpiceFile' is specified, in which case
 * the output is left in 'esSpiceFile'.
 *
 * Lumped node resistances are converted into star networks: each
 * fet terminal connecting to the node is replaced by a resistor
 * that connects to the node, where the value of the resistor is
 * half the lumped nodal resistance.
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
static char rcsid[] = "$Header: ext2spice.c,v 6.0 90/08/28 19:03:38 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include "magic.h"
#include "paths.h"
#include "geometry.h"
#include "malloc.h"
#include "hash.h"
#include "utils.h"
#include "pathvisit.h"
#include "extflat.h"
#include "runstats.h"

/* Stored in client field of each node */
typedef struct
{
    int		 nc_nodeNum;		/* Spice node number */
    int		 nc_nodeResist;		/* Node resistance in ohms */
} NodeClient;

/* Forward declarations */
int mainArgs();
int capVisit(), fetVisit(), nodeVisit(), resistVisit();

/* Options specific to ext2spice */
char esDefaultOut[FNSIZE];
char *esOutName = esDefaultOut;
FILE *esSpiceF = NULL;
bool esFetNodesOnly = FALSE;
int esCapNum;
int esFetNum;
int esResNum;
int esNodeNum = 100;
char *esICFile = NULL;


/*
 * ----------------------------------------------------------------------------
 *
 * main --
 *
 * Top level of ext2spice.
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
     * Make output name to be of the form inName.spice
     * if it wasn't explicitly specified.
     */
    if (esOutName == esDefaultOut)
	(void) sprintf(esDefaultOut, "%s.spice", inName);
    if ((esSpiceF = fopen(esOutName, "w")) == NULL)
    {
	perror(esOutName);
	exit (1);
    }

    /* Read the hierarchical description of the input circuit */
    EFReadFile(inName);
    (void) fprintf(esSpiceF, "** SPICE file created for circuit %s\n", inName);
    (void) fprintf(esSpiceF, "** Technology: %s\n", EFTech);
    (void) fprintf(esSpiceF, "**\n\n");
    (void) fprintf(esSpiceF, "** NODE: 0 = GND\n");
    (void) fprintf(esSpiceF, "** NODE: 1 = Vdd\n");
    (void) fprintf(esSpiceF, "** NODE: 2 = Error\n");

    /* Convert the hierarchical description to a flat one */
    flatFlags = EF_FLATNODES;
    if (EFCapThreshold < INFINITE_THRESHOLD) flatFlags |= EF_FLATCAPS;
    if (EFResistThreshold < INFINITE_THRESHOLD) flatFlags |= EF_FLATRESISTS;
    EFFlatBuild(inName, flatFlags);

    /* Assign node numbers to Vdd and GND */
    assignNodeNum("GND!", 0);
    assignNodeNum("Vdd!", 1);

    EFVisitFets(fetVisit, (ClientData) NULL);
    if (flatFlags & EF_FLATCAPS)
	EFVisitCaps(capVisit, (ClientData) NULL);
    if (flatFlags & EF_FLATRESISTS)
	EFVisitResists(resistVisit, (ClientData) NULL);
    EFVisitNodes(nodeVisit, (ClientData) NULL);
    if (esICFile)
	esDoIC(esICFile);
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
 * assignNodeNum --
 *
 * Assign node numbers for power and ground.  Allocates the NodeClient
 * for node 'name' and initializes its nc_nodeNum to spiceNodeNum.  The
 * node's lumped resistance is forced to zero, since this procedure is
 * only supposed to be called for power nodes.
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
assignNodeNum(name, spiceNodeNum)
    char *name;
    int spiceNodeNum;
{
    HashEntry *he;
    EFNodeName *nn;
    EFNode *node;
    NodeClient *nc;

    he = EFHNLook((HierName *) NULL, name, "power&ground");
    if (he == NULL)
    {
	printf("Can't find %s node\n", name);
	exit (1);
    }

    nn = (EFNodeName *) HashGetValue(he);
    node = nn->efnn_node;
    if (nc = (NodeClient *) node->efnode_client)
    {
	printf("Node %s was already assigned a node number %d\n",
			name, nc->nc_nodeNum);
	printf("when attempting to assign a new node number %d\n",
			spiceNodeNum);
	printf("This probably means that power and ground are shorted!\n");
	exit (1);
    }

    MALLOC(NodeClient *, nc, sizeof (NodeClient));
    node->efnode_client = (ClientData) nc;
    nc->nc_nodeNum = spiceNodeNum;
    nc->nc_nodeResist = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * mainArgs --
 *
 * Process those arguments that are specific to ext2spice.
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
	case 'F':
	    esFetNodesOnly = TRUE;
	    break;
	case 'i':
	    if ((cp = ArgStr(&argc, &argv, ".ic file")) == NULL)
		goto usage;
	    esICFile = StrDup((char **) NULL, cp);
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
Usage: ext2spice [-i icfile] [-o spicefile] [-F] file\n\
       or else see options to extcheck(1)\n");
    exit (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * fetVisit --
 *
 * Procedure to output a single fet to the .spice file.
 * If any of the fet terminals are connected to a node with
 * a lumped resistance above the threshold, we output a
 * resistor whose value is half the node resistance between
 * the fet terminal and the node.
 *
 * Called by EFVisitFets().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSpiceF.
 *
 * Format of a .spice fet line:
 *
 *	Mfetname drain gate source substrate model L=len W=wid
 *
 * where
 *	fetname uniquely identifies this fet (a number),
 *	drain, gate, source, and substrate are the node numbers
 *	    of the respective nodes,
 *	model is the transistor type,
 *	len and wid are the transistor channel length and width
 *	    in microns.
 *
 * Format of a .spice resistor line:
 *
 *	Rname node1 node2 res
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
    int gNode, sNode, dNode, subsNode;
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

    /*
     * Find the node numbers for the four fet terminals.
     * If any of the nodes have a lumped resistance greater
     * than the threshold, output a resistor between the fet
     * terminal and the node whose value is half of the lumped
     * resistor.
     */
    dNode = esFetHier(hierName, drain->fterm_node),
    gNode = esFetHier(hierName, gate->fterm_node),
    sNode = esFetHier(hierName, source->fterm_node),
    subsNode = esFetHier(hierName, fet->fet_subsnode),

    (void) fprintf(esSpiceF, "M%d %d %d %d %d %s",
		esFetNum++, dNode, gNode, sNode, subsNode,
		EFFetTypes[fet->fet_type]);

    /*
     * Scale L and W appropriately by the same amount as distance
     * values in the transform.  The transform will have a scale
     * different from 1 only in the case when the scale factors of
     * some of the .ext files differed, making it necessary to scale
     * all dimensions explicitly instead of having a single scale
     * factor at the beginning of the .sim file.
     *
     * Multiply by overall scalefactor to get centimicrons.
     */
    GeoTransRect(trans, &fet->fet_rect, &r);
    scale = GeoScale(trans) * EFScale;
    l *= scale;
    w *= scale;
    (void) fprintf(esSpiceF, " L=%.1fU W=%.1fU\n",
		((double) l) / 100.0, ((double) w) / 100.0);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * esFetHier --
 *
 * Called to convert a hierarchical prefix and a suffix node into
 * a node number for a fet terminal.  If the node whose name is the
 * concatenation of prefix and suffixNode->efnode_name->efnn_hier
 * has a lumped resistance greater than EFResistThreshold, we output
 * a resistor whose value is half the lumped resistance and that
 * connects the node with a newly allocated node; we return the
 * newly allocated node number.  Otherwise, we just return the
 * original node's number.
 *
 * Results:
 *	Returns a node number.
 *
 * Side effects:
 *	May allocate NodeClients for nodes which don't already
 *	have their efnode_client field set.  Also may output
 *	to esSpiceF.
 *
 * ----------------------------------------------------------------------------
 */

int
esFetHier(prefix, suffixNode)
    HierName *prefix;
    EFNode *suffixNode;
{
    HierName *suffix;
    EFNodeName *nn;
    NodeClient *nc;
    HashEntry *he;
    EFNode *node;
    int nodenum;

    suffix = suffixNode->efnode_name->efnn_hier;
    he = EFHNConcatLook(prefix, suffix, "fet");
    if (he == NULL)
	return 2;		/* Errors are node #2 */

    nn = (EFNodeName *) HashGetValue(he);
    node = nn->efnn_node;
    if (node->efnode_client == NULL)
    {
	MALLOC(NodeClient *, nc, sizeof (NodeClient));
	node->efnode_client = (ClientData) nc;
	nc->nc_nodeNum = esNodeNum++;
	nc->nc_nodeResist = (EFNodeResist(node) + 500) / 1000;
    }

    nc = (NodeClient *) node->efnode_client;
    nodenum = nc->nc_nodeNum;
    if (nc->nc_nodeResist > EFResistThreshold)
    {
	/* Output a connecting resistor */
	nodenum = esNodeNum++;
	(void) fprintf(esSpiceF, "RLUMP%d %d %d %.1f\n",
		esResNum++, nc->nc_nodeNum, nodenum,
		((float) nc->nc_nodeResist) / 2.0);
    }

    return nodenum;
}

/*
 * ----------------------------------------------------------------------------
 *
 * capVisit --
 *
 * Procedure to output a single capacitor to the .spice file.
 * Called by EFVisitCaps().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSpiceF.
 *
 * Format of a .spice cap line:
 *
 *	Cname node1 node2 cap
 *
 * where
 *	name is a unique identifier for this capacitor
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
	return;

    (void) fprintf(esSpiceF, "C%d %d %d %dF\n",
	    esCapNum++, nodeNum(hierName1), nodeNum(hierName2), cap);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * resistVisit --
 *
 * Procedure to output a single resistor to the .spice file.
 * Called by EFVisitResists().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSpiceF.
 *
 * Format of a .spice resistor line:
 *
 *	Rname node1 node2 res
 *
 * where
 *	name is a unique identifier for this resistor
 *	node1, node2 are the terminals of the resistor
 *	res is the resistance in ohms (NOT milliohms)
 *
 * ----------------------------------------------------------------------------
 */

int
resistVisit(hierName1, hierName2, res)
    HierName *hierName1;
    HierName *hierName2;
    int res;
{
    res = (res + 500) / 1000;
    if (res <= EFResistThreshold)
	return;

    (void) fprintf(esSpiceF, "R%d %d %d %d\n",
	    esResNum++, nodeNum(hierName1), nodeNum(hierName2), res);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * nodeVisit --
 *
 * Procedure to output a single node to the .spice file.  EFNodes
 * are actually output as capacitors if the capacitance is sufficient
 * large.  Also, we output comments giving a mapping between node names
 * and numbers.
 *
 * Called by EFVisitNodes().
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Writes to the file esSpiceF.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
int
nodeVisit(node, res, cap)
    register EFNode *node;
    int res;	/*UNUSED*/
    int cap;
{
    register EFNodeName *nn;
    HierName *hierName;
    NodeClient *nc;
    int nodenum;

    if (esFetNodesOnly && node->efnode_client == NULL)
	return 0;

    hierName = (HierName *) node->efnode_name->efnn_hier;
    if (node->efnode_client)
	nodenum = ((NodeClient *) node->efnode_client)->nc_nodeNum;
    else nodenum = esNodeNum++;

    /* Capacitance is to ground */
    cap = (cap + 500) / 1000;
    if (cap > EFCapThreshold)
	(void) fprintf(esSpiceF, "C%d %d 0 %dF\n", esCapNum++, nodenum, cap);

    /* Output the comment equating the node number and its name */
    (void) fprintf(esSpiceF, "** NODE: %d = ", nodenum);
    EFHNOut((HierName *) hierName, esSpiceF);
    if (node->efnode_client == NULL)
	(void) fprintf(esSpiceF, " ===FLOATING===");
    putc('\n', esSpiceF);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * esNodeNum --
 *
 * Convert a hierarchical node name into a node number.
 *
 * Results:
 *	Returns the node number.  Returns 2 if a node by
 *	the given name couldn't be found.
 *
 * Side effects:
 *	May allocate a NodeClient field for the node we find
 *	if one doesn't already exist.
 *
 * ----------------------------------------------------------------------------
 */

int
nodeNum(hierName)
    HierName *hierName;
{
    EFNodeName *nn;
    NodeClient *nc;
    HashEntry *he;
    EFNode *node;

    he = EFHNLook(hierName, (char *) NULL, "fet");
    if (he == NULL)
	return 2;

    nn = (EFNodeName *) HashGetValue(he);
    node = nn->efnn_node;
    if (node->efnode_client == NULL)
    {
	MALLOC(NodeClient *, nc, sizeof (NodeClient));
	node->efnode_client = (ClientData) nc;
	nc->nc_nodeNum = esNodeNum++;
	nc->nc_nodeResist = (EFNodeResist(node) + 500) / 1000;
    }

    nc = (NodeClient *) node->efnode_client;
    return nc->nc_nodeNum;
}

/*
 * ----------------------------------------------------------------------------
 *
 * esDoIC --
 *
 * Read a file containing initial conditions for nodes, and output
 * to the Spice deck.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the file esSpiceF.
 *
 * ----------------------------------------------------------------------------
 */

Void
esDoIC(name)
    char *name;
{
    char line[1024], nodename[1024];
    int ic, lineNum, nodeNum;
    HashEntry *he;
    EFNodeName *nn;
    FILE *f;

    f = fopen(name, "r");
    if (f == NULL)
    {
	perror(name);
	printf("Can't open .ic file\n");
	return;
    }

    for (lineNum = 1; fgets(line, sizeof line, f); lineNum++)
    {
	if (sscanf(line, "%s %d", nodename, &ic) != 2)
	{
	    printf("Malformed .ic file line (#%d): %s", lineNum, line);
	    continue;
	}

	he = EFHNLook((HierName *) NULL, nodename, ".ic");
	if (he == NULL)
	    continue;

	nn = (EFNodeName *) HashGetValue(he);
	nodeNum = (int) nn->efnn_node->efnode_client;
	if (nodeNum == -1)
	    nodeNum = 0;

	(void) fprintf(esSpiceF, ".ic v(%d)=%d\n", nodeNum, ic);
    }

    (void) fclose(f);
}
