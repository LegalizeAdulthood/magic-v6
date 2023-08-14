/*
 * extcheck.c --
 *
 * Program to check .ext files for consistency without producing
 * any output.  Checks for disconnected global nodes as well as
 * for version consistency.  Counts the number of interesting
 * things in the circuit (fets, capacitors, resistors, nodes).
 *
 * Flattens the tree rooted at file.ext, reading in additional .ext
 * files as specified by "use" lines in file.ext.
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
static char rcsid[] = "$Header: extcheck.c,v 6.0 90/08/28 19:08:15 mayo Exp $";
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

int ecNumFets;
int ecNumCaps;
int ecNumResists;
int ecNumThreshCaps;
int ecNumThreshResists;
int ecNumNodes;
int ecNumGlobalNodes;
int ecNumNodeCaps;
int ecNumNodeResists;

/* Forward declarations */
int nodeVisit(), fetVisit(), capVisit(), resistVisit();

/*
 * ----------------------------------------------------------------------------
 *
 * main --
 *
 * Top level of extcheck.
 *
 * ----------------------------------------------------------------------------
 */

main(argc, argv)
    char *argv[];
{
    char *inName;

    /* Process command line arguments */
    EFInit();
    inName = EFArgs(argc, argv, (int (*)()) NULL, (ClientData) NULL);
    if (inName == NULL)
	exit (1);

    /* Read the hierarchical description of the input circuit */
    EFReadFile(inName);
    if (EFArgTech) EFTech = StrDup((char **) NULL, EFArgTech);
    if (EFScale == 0) EFScale = 1;

    /* Convert the hierarchical description to a flat one */
    EFFlatBuild(inName, EF_FLATNODES|EF_FLATCAPS|EF_FLATRESISTS);

    EFVisitFets(fetVisit, (ClientData) NULL);
    if (EFCapThreshold < INFINITE_THRESHOLD)
	EFVisitCaps(capVisit, (ClientData) NULL);
    if (EFResistThreshold < INFINITE_THRESHOLD)
	EFVisitResists(resistVisit, (ClientData) NULL);
    EFVisitNodes(nodeVisit, (ClientData) NULL);

#ifdef	free_all_mem
    EFFlatDone();
    EFDone();
#endif	free_all_mem

    printf("Memory used: %s\n", RunStats(RS_MEM, NULL, NULL));
    printf("%d fets\n", ecNumFets);
    printf("%d nodes (%d global, %d local)\n",
	    ecNumNodes, ecNumGlobalNodes, ecNumNodes - ecNumGlobalNodes);
    printf("%d nodes above capacitance threshold\n", ecNumNodeCaps);
    printf("%d nodes above resistance threshold\n", ecNumNodeResists);
    printf("%d internodal capacitors (%d above threshold)\n",
	    ecNumCaps, ecNumThreshCaps);
    printf("%d explicit resistors (%d above threshold)\n",
	    ecNumResists, ecNumThreshResists);
    exit (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * nodeVisit --
 * fetVisit --
 * capVisit --
 * resistVisit --
 *
 * Called once for each of the appropriate type of object.
 * Each updates various counts.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

int
nodeVisit(node, res, cap)
    register EFNode *node;
    int res, cap;
{
    cap = (cap + 500) / 1000;
    res = (res + 500) / 1000;

    ecNumNodes++;
    if (EFHNIsGlob(node->efnode_name->efnn_hier))
	ecNumGlobalNodes++;
    if (res > EFResistThreshold) ecNumNodeResists++;
    if (cap > EFCapThreshold) ecNumNodeCaps++;
    return 0;
}

int
fetVisit()
{
    ecNumFets++;
    return 0;
}

    /*ARGSUSED*/
int
capVisit(hn1, hn2, cap)
    HierName *hn1, *hn2;	/* UNUSED */
    int cap;
{
    ecNumCaps++;
    cap = (cap + 500) / 1000;
    if (cap > EFCapThreshold) ecNumThreshCaps++;
    return 0;
}

    /*ARGSUSED*/
int
resistVisit(hn1, hn2, res)
    HierName *hn1, *hn2;	/* UNUSED */
    int res;
{
    ecNumResists++;
    res = (res + 500) / 1000;
    if (res > EFResistThreshold) ecNumThreshResists++;
    return 0;
}
