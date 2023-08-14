/*
 * ExtHier.c --
 *
 * Circuit extraction.
 * Lower-level procedures common both to ordinary subtree extraction,
 * and to array extraction.
 * The procedures in this file are not re-entrant.
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
static char rcsid[] = "$Header: ExtHier.c,v 6.0 90/08/28 18:15:12 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <math.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "textio.h"
#include "styles.h"
#include "windows.h"
#include "dbwind.h"
#include "debug.h"
#include "extract.h"
#include "extractInt.h"

/* Local data */

    /* Passed to search functions by extHierConnections */
ExtTree *extHierCumFlat;	/* Cum buffer */
ExtTree *extHierOneFlat;	/* Subtree being compared with extHierCumFlat */
Tile *extHierOneTile;		/* Tile from extHierOneFlat */
int extHierPNum;		/* Plane on which extHierOneTile lives */

    /* List of free cells around for use in yanking subtrees */
ExtTree *extHierFreeOneList = (ExtTree *) NULL;

    /* Appended to the name of each new CellDef created by extHierNewOne() */
int extHierOneNameSuffix = 0;

/* Forward declarations */
int extHierConnectFunc1();
int extHierConnectFunc2();
Node *extHierNewNode();

/*
 * ----------------------------------------------------------------------------
 *
 * extHierConnections --
 *
 * Process connections between the two ExtTrees 'oneFlat' and 'cumFlat'.
 * This consists of detecting overlaps or abutments between connecting
 * tiles on the same plane, and recording the connection in the hash
 * table ha->ha_connHash.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds connections to ha->ha_connHash.
 *	Doesn't change resistance or capacitance of the connected
 *	nodes; that is the job of extHierAdjustments().
 *
 * ----------------------------------------------------------------------------
 */

Void
extHierConnections(ha, cumFlat, oneFlat)
    HierExtractArg *ha;
    ExtTree *cumFlat, *oneFlat;
{
    CellDef *sourceDef = oneFlat->et_use->cu_def;

    extHierCumFlat = cumFlat;
    extHierOneFlat = oneFlat;
    for (extHierPNum = PL_TECHDEPBASE; extHierPNum < DBNumPlanes; extHierPNum++)
    {
	(void) DBSrPaintArea((Tile *) NULL,
		sourceDef->cd_planes[extHierPNum], &ha->ha_subArea,
		&DBAllButSpaceBits, extHierConnectFunc1, (ClientData) ha);
    }
}

/*
 * extHierConnectFunc1 --
 *
 * Called for each tile 'oneTile' in the ExtTree 'oneFlat' above
 * that lies in the area ha->ha_subArea.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	None here, but see extHierConnectFunc2().
 */

extHierConnectFunc1(oneTile, ha)
    Tile *oneTile;	/* Comes from 'oneFlat' in extHierConnections */
    HierExtractArg *ha;	/* Extraction context */
{
    CellDef *cumDef = extHierCumFlat->et_use->cu_def;
    Rect r;

    /*
     * Find all tiles in the same plane as 'srcTile', but in the
     * yank buffer cumDef.  Adjust connectivity for each tile found.
     * Widen the rectangle to detect connectivity by abutment.
     */
    extHierOneTile = oneTile;
    TITORECT(oneTile, &r);
    GEOCLIP(&r, &ha->ha_subArea);
    r.r_xbot--, r.r_ybot--, r.r_xtop++, r.r_ytop++;
    (void) DBSrPaintArea((Tile *) NULL,
		cumDef->cd_planes[extHierPNum], &r,
		&DBAllButSpaceBits, extHierConnectFunc2, (ClientData) ha);

    return (0);
}

/*
 * extHierConnectFunc2 --
 *
 * Called once for each tile 'cum' in extHierCumFlat->et_use->cu_def
 * on the same plane as extHierOneTile that also overlaps or abuts
 * the intersection of extHierOneTile with ha->ha_subArea.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Makes a connection between the nodes of the two tiles
 *	if the types of extHierOneTile and 'cum' connect.
 *	Otherwise, if the tiles actually overlap (as opposed
 *	to merely abut), mark it with feedback as an error.
 */

extHierConnectFunc2(cum, ha)
    Tile *cum;		/* Comes from extHierCumFlat->et_use->cu_def */
    HierExtractArg *ha;	/* Extraction context */
{
    HashTable *table = &ha->ha_connHash;
    register Node *node1, *node2;
    register HashEntry *he;
    register NodeName *nn;
    char *name;
    Rect r;

    /* Compute the overlap area */
    r.r_xbot = MAX(LEFT(extHierOneTile), LEFT(cum));
    r.r_xtop = MIN(RIGHT(extHierOneTile), RIGHT(cum));
    r.r_ybot = MAX(BOTTOM(extHierOneTile), BOTTOM(cum));
    r.r_ytop = MIN(TOP(extHierOneTile), TOP(cum));

    /* If the tiles don't even touch, they don't connect */
    if (r.r_xtop < r.r_xbot || r.r_ytop < r.r_ybot
	    || (r.r_xtop == r.r_xbot && r.r_ytop == r.r_ybot))
	return (0);

    /*
     * Only make a connection if the types of 'extHierOneTile' and 'cum'
     * connect.  If they overlap and don't connect, it is an error.
     * If they do connect, mark their nodes as connected.
     */
    if (extConnectsTo(TiGetType(extHierOneTile), TiGetType(cum),
		ExtCurStyle->exts_nodeConn))
    {
	name = (*ha->ha_nodename)(cum, extHierCumFlat, ha, TRUE);
	he = HashFind(table, name);
	nn = (NodeName *) HashGetValue(he);
	node1 = nn ? nn->nn_node : extHierNewNode(he);

	name = (*ha->ha_nodename)(extHierOneTile, extHierOneFlat, ha, TRUE);
	he = HashFind(table, name);
	nn = (NodeName *) HashGetValue(he);
	node2 = nn ? nn->nn_node : extHierNewNode(he);

	if (node1 != node2)
	{
	    /*
	     * Both sets of names will now point to node1.
	     * We don't need to update node_cap since it
	     * hasn't been computed yet.
	     */
	    for (nn = node2->node_names; nn->nn_next; nn = nn->nn_next)
		nn->nn_node = node1;
	    nn->nn_node = node1;
	    nn->nn_next = node1->node_names;
	    node1->node_names = node2->node_names;
	    FREE((char *) node2);
	}
    }
    else if (r.r_xtop > r.r_xbot && r.r_ytop > r.r_ybot)
    {
	extNumFatal++;
	if (!DebugIsSet(extDebugID, extDebNoFeedback))
	    DBWFeedbackAdd(&r, "Illegal overlap (types do not connect)",
		ha->ha_parentUse->cu_def, 1, STYLE_MEDIUMHIGHLIGHTS);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierAdjustments --
 *
 * Process adjustments to substrate capacitance, coupling capacitance,
 * node perimeter, and node area between the subtree 'oneFlat' and the
 * cumulative yank buffer 'cumFlat'.  The subtree 'lookFlat' is used
 * for looking up node names when handling capacitance/perimeter/area
 * adjustment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates capacitance in the table cumFlat->et_coupleHash.
 *	Updates capacitance, perimeter, and area recorded in the
 *	nodes of 'cumFlat'.
 *
 * Algorithm:
 *	For each capacitor recorded in oneFlat->et_coupleHash, find
 *	the corresponding nodes in 'cumFlat' and subtract the
 *	capacitance from the entry indexed by these nodes in the
 *	table cumFlat->et_coupleHash.
 *
 *	For each node in oneFlat->et_nodes, find the corresponding
 *	node in 'lookFlat'.  Look for the Node with this name in
 *	the table ha->ha_connHash, and subtract the oneFlat node's
 *	capacitance, perimeter, and area from it.  If no Node is
 *	found in this table, don't do anything since the oneFlat
 *	node must not participate in any connections.
 *
 *	The node in 'cumFlat' corresponding to one in 'oneFlat'
 *	is the one containing some point in 'oneFlat', since 'oneFlat'
 *	is a strict subset of 'cumFlat'.
 *
 * ----------------------------------------------------------------------------
 */

Void
extHierAdjustments(ha, cumFlat, oneFlat, lookFlat)
    register HierExtractArg *ha;
    ExtTree *cumFlat, *oneFlat, *lookFlat;
{
    register HashEntry *he, *heCum;
    register int n;
    CoupleKey *ckpOne, ckCum;
    NodeRegion *np;
    HashSearch hs;
    NodeName *nn;
    Tile *tp;
    char *name;

    /* Update all coupling capacitors */
    if (ExtOptions & EXT_DOCOUPLING)
    {
	HashStartSearch(&hs);
	while (he = HashNext(&oneFlat->et_coupleHash, &hs))
	{
	    ckpOne = ((CoupleKey *) he->h_key.h_words);

	    /* Find nodes in cumFlat->et_coupleHash */
	    NODETONODE(ckpOne->ck_1, cumFlat, ckCum.ck_1);
	    NODETONODE(ckpOne->ck_2, cumFlat, ckCum.ck_2);
	    if (ckCum.ck_1 == NULL || ckCum.ck_2 == NULL) continue;

	    /* Skip if the same; reverse to make smaller node pointer first */
	    if (ckCum.ck_1 == ckCum.ck_2) continue;
	    if (ckCum.ck_2 < ckCum.ck_1)
		np = ckCum.ck_1, ckCum.ck_1 = ckCum.ck_2, ckCum.ck_2 = np;

	    /* Update the capacitor record in cumFlat->et_coupleHash */
	    heCum = HashFind(&cumFlat->et_coupleHash, (char *) &ckCum);
	    HashSetValue(heCum, HashGetValue(heCum) - HashGetValue(he));
	}
    }

    /*
     * Update all node values.
     * Find the corresponding tile in the ExtTree lookFlat, then look
     * for its name.  If this name appear in the connection hash table,
     * update the capacitance, perimeter, and area stored there; otherwise
     * ignore it.
     *
     * The FALSE argument to (*ha->ha_nodename)() means that we don't bother
     * looking for node names the hard way; if we didn't already have a valid
     * node name then it couldn't appear in the table ha->ha_connHash in the
     * first place.
     */
    for (np = oneFlat->et_nodes; np; np = np->nreg_next)
    {
	NODETOTILE(np, lookFlat, tp);  /* tp = extHierLookTile(np, lookFlat); */
	if (tp	&& (name = (*ha->ha_nodename)(tp, lookFlat, ha, FALSE))
		&& (he = HashLookOnly(&ha->ha_connHash, name))
		&& (nn = (NodeName *) HashGetValue(he)))
	{
	    /* Adjust the capacitance and resistance */
	    nn->nn_node->node_cap -= np->nreg_cap;
	    for (n = 0; n < ExtCurStyle->exts_numResistClasses; n++)
	    {
		nn->nn_node->node_pa[n].pa_perim -= np->nreg_pa[n].pa_perim;
		nn->nn_node->node_pa[n].pa_area -= np->nreg_pa[n].pa_area;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extOutputConns --
 *
 * Dump the contents of the hash table 'table' of connectivity and
 * node R, C adjustments to the output file outf.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Outputs a number of "merge" records to the file 'outf'.
 *
 * ----------------------------------------------------------------------------
 */

extOutputConns(table, outf)
    HashTable *table;
    FILE *outf;
{
    int c, cround = ExtCurStyle->exts_capScale / 2;
    register NodeName *nn, *nnext;
    register Node *node;
    register int n;
    NodeName *nfirst;
    HashSearch hs;
    HashEntry *he;

    HashStartSearch(&hs);
    while (he = HashNext(table, &hs))
    {
	nfirst = (NodeName *) HashGetValue(he);

	/*
	 * If nfirst->nn_node == NULL, the name for this hash entry
	 * had been output previously as a member of the merge list
	 * for a node appearing earlier in the table.  If so, we need
	 * only free the NodeName without any further processing.
	 */
	if (node = nfirst->nn_node)
	{
	    /*
	     * If there are N names for this node, output N-1 merge lines.
	     * Only the first merge line will contain the C, perimeter,
	     * and area updates.
	     */
	    c = (node->node_cap + cround) / ExtCurStyle->exts_capScale;
	    nn = node->node_names;
	    if (nnext = nn->nn_next)
	    {
		/* First merge */
		(void) fprintf(outf, "merge \"%s\" \"%s\" %d",
				nn->nn_name, nnext->nn_name, c);
		for (n = 0; n < ExtCurStyle->exts_numResistClasses; n++)
		    (void) fprintf(outf, " %d %d",
				node->node_pa[n].pa_area,
				node->node_pa[n].pa_perim);
		(void) fprintf(outf, "\n");
		nn->nn_node = (Node *) NULL;		/* Processed */

		/* Subsequent merges */
		for (nn = nnext; nnext = nn->nn_next; nn = nnext)
		{
		    (void) fprintf(outf, "merge \"%s\" \"%s\"\n",
				    nn->nn_name, nnext->nn_name, c);
		    nn->nn_node = (Node *) NULL;	/* Processed */
		}
	    }
	    nn->nn_node = (Node *) NULL;
	    FREE((char *) node);
	}
	FREE((char *) nfirst);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierNewNode --
 *
 * Create a new NodeName and Node to go with the HashEntry supplied.
 * The NodeName will point to the new Node, which will point back to the
 * NodeName.
 *
 * Results:
 *	Returns a pointer to the newly created Node.
 *
 * Side effects:
 *	Allocates memory.
 *	Sets (via HashSetValue) the value of HashEntry 'he' to the
 *	newly created NodeName.
 *
 * ----------------------------------------------------------------------------
 */

Node *
extHierNewNode(he)
    register HashEntry *he;
{
    register int n, nclasses;
    register NodeName *nn;
    register Node *node;

    nclasses = ExtCurStyle->exts_numResistClasses;
    n = (nclasses - 1) * sizeof (PerimArea) + sizeof (Node);
    MALLOC(NodeName *, nn, sizeof (NodeName));
    MALLOC(Node *, node, (unsigned) n);

    nn->nn_node = node;
    nn->nn_next = (NodeName *) NULL;
    nn->nn_name = he->h_key.h_name;
    node->node_names = nn;
    node->node_cap = 0;
    for (n = 0; n < nclasses; n++)
	node->node_pa[n].pa_perim = node->node_pa[n].pa_area = 0;
    HashSetValue(he, (char *) nn);

    return (node);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierLabFirst --
 * extHierLabEach --
 *
 * Filter functions passed to ExtFindRegions when tracing out labelled
 * regions as part of a hierarchical circuit extraction.
 *
 * Results:
 *	extHierLabFirst returns a pointer to a new LabRegion.
 *	extHierLabEach returns 0 always.
 *
 * Side effects:
 *	Memory is allocated by extHierLabFirst(); it conses the newly
 *	allocated region onto the front of the existing region list.
 *	The node-naming info (reg_ll, reg_pnum) is updated by
 *	extHierLabEach().
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
Region *
extHierLabFirst(tile, arg)
    Tile *tile;
    FindRegion *arg;
{
    register LabRegion *new;

    MALLOC(LabRegion *, new, sizeof (LabRegion));
    new->lreg_next = (LabRegion *) NULL;
    new->lreg_labels = (LabelList *) NULL;
    new->lreg_pnum = DBNumPlanes;

    /* Prepend it to the region list */
    new->lreg_next = (LabRegion *) arg->fra_region;
    arg->fra_region = (Region *) new;

    return ((Region *) new);
}

    /*ARGSUSED*/
int
extHierLabEach(tile, pNum, arg)
    register Tile *tile;
    register int pNum;
    FindRegion *arg;
{
    register LabRegion *reg;

    reg = (LabRegion *) arg->fra_region;
    extSetNodeNum(reg, pNum, tile);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierNewOne --
 *
 * Allocate a new ExtTree for use in hierarchical extraction.
 * This ExtTree will be used to hold an entire flattened subtree.
 * We try to return one from our free list if one exists; if none
 * are left, we create a new CellDef and CellUse and allocate a
 * new ExtTree.  The new CellDef has a name of the form __EXTTREEn__,
 * where 'n' is a small integer.
 *
 * The HashTable et_coupleHash will be initialized but empty.
 * The node list et_nodes, the next pointer et_next, and the CellDef
 * pointer et_lookNames will all be set to NULL.
 *
 * Results:
 *	Returns a pointer to a new ExtTree.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

ExtTree *
extHierNewOne()
{
    char defname[128];
    CellDef *dummy;
    ExtTree *et;

    if (extHierFreeOneList)
    {
	et = extHierFreeOneList;
	extHierFreeOneList = et->et_next;
    }
    else
    {
	MALLOC(ExtTree *, et, sizeof (ExtTree));
	(void) sprintf(defname, "__EXTTREE%d__", extHierOneNameSuffix++);
	DBNewYank(defname, &et->et_use, &dummy);
    }

    et->et_next = (ExtTree *) NULL;
    et->et_lookNames = (CellDef *) NULL;
    et->et_nodes = (NodeRegion *) NULL;
    if (ExtOptions & EXT_DOCOUPLING)
	HashInit(&et->et_coupleHash, 32, HashSize(sizeof (CoupleKey)));
    return (et);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extHierFreeOne --
 *
 * Return an ExtTree allocated via extHierNewOne() above to the
 * free list.  Frees the HashTable et->et_coupleHash, any NodeRegions
 * on the list et->et_nodes, any labels on the label list and any
 * paint in the cell et->et_use->cu_def.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *	The caller should NOT use et->et_next after this procedure
 *	has returned.
 *
 * ----------------------------------------------------------------------------
 */

Void
extHierFreeOne(et)
    ExtTree *et;
{
    if (ExtOptions & EXT_DOCOUPLING)
	HashKill(&et->et_coupleHash);
    if (et->et_nodes) ExtFreeLabRegions((LabRegion *) et->et_nodes);
    extHierFreeLabels(et->et_use->cu_def);
    DBCellClearDef(et->et_use->cu_def);

    et->et_next = extHierFreeOneList;
    extHierFreeOneList = et;
}
