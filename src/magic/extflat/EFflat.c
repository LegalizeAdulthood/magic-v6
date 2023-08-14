/*
 * EFflat.c -
 *
 * Procedures to flatten the hierarchical description built
 * by efReadDef().
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
static char rcsid[] = "$Header: EFflat.c,v 6.0 90/08/28 18:13:31 mayo Exp $";
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

/* Initial size of the hash table of all flattened node names */
#define	INITFLATSIZE	1024

/* Hash table containing all flattened capacitors */
HashTable efCapHashTable;

/* Hash table containing all flattened distances */
HashTable efDistHashTable;

/* Head of circular list of all flattened nodes */
EFNode efNodeList;

/* Root of the tree being flattened */
Def *efFlatRootDef;
Use efFlatRootUse;
HierContext efFlatContext;

/* Forward declarations */
int efFlatSingleCap();
int efFlatGlobHash(), efFlatGlobCmp();
char *efFlatGlobCopy();

/*
 * ----------------------------------------------------------------------------
 *
 * EFFlatBuild --
 *
 * First pass of flattening a circuit.
 * Builds up the flattened tables of nodes, capacitors, etc, depending
 * on the bits contained in flags: EF_FLATNODES causes the node table
 * to be built, EF_FLATCAPS the internodal capacitor table (implies
 * EF_FLATNODES), and EF_FLATDISTS the distance table.
 *
 * Callers who want various pieces of information should call
 * the relevant EFVisit procedures (e.g., EFVisitFets(), EFVisitCaps(),
 * EFVisitNodes(), etc).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates lots of memory.
 *	Be certain to call EFFlatDone() when this memory is
 *	no longer needed.
 *
 * ----------------------------------------------------------------------------
 */

Void
EFFlatBuild(name, flags)
    char *name;		/* Name of root def being flattened */
    int flags;		/* Say what to flatten; see above */
{
    efFlatRootDef = efDefLook(name);
    if (efHNStats) efHNPrintSizes("before building flattened table");

    /* Keyed by a full HierName */
    HashInitClient(&efNodeHashTable, INITFLATSIZE, HT_CLIENTKEYS,
	efHNCompare, (char *(*)()) NULL, efHNHash, (Void (*)()) NULL);

    /* Keyed by a pair of HierNames */
    HashInitClient(&efDistHashTable, INITFLATSIZE, HT_CLIENTKEYS,
	efHNDistCompare, efHNDistCopy, efHNDistHash, efHNDistKill);

    /* Keyed by pairs of EFNode pointers (i.e., EFCoupleKeys) */
    HashInit(&efCapHashTable, INITFLATSIZE, HashSize(sizeof (EFCoupleKey)));

    /* Keyed by a string and a HierName */
    HashInitClient(&efHNUseHashTable, INITFLATSIZE, HT_CLIENTKEYS,
	efHNUseCompare, (char *(*)()) NULL, efHNUseHash, (Void (*)()) NULL);

    /* Circular list of all nodes contains no elements initially */
    efNodeList.efnode_next = (EFNodeHdr *) &efNodeList;
    efNodeList.efnode_prev = (EFNodeHdr *) &efNodeList;

    efFlatContext.hc_hierName = (HierName *) NULL;
    efFlatContext.hc_use = &efFlatRootUse;
    efFlatContext.hc_trans = GeoIdentityTransform;
    efFlatContext.hc_x = efFlatContext.hc_y = 0;
    efFlatRootUse.use_def = efFlatRootDef;

    if (flags & EF_FLATNODES)
    {
	(void) efFlatNodes(&efFlatContext);
	(void) efFlatKills(&efFlatContext);
	efFlatGlob();
    }

    /* Must happen after kill processing */
    if (flags & EF_FLATCAPS)
	(void) efFlatCaps(&efFlatContext);

    /* Distances are independent of kill processing */
    if (flags & EF_FLATDISTS)
	(void) efFlatDists(&efFlatContext);

    if (efHNStats) efHNPrintSizes("after building flattened table");
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFFlatDone --
 *
 * Cleanup by removing all memory used by the flattened circuit
 * representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees lots of memory.
 *
 * ----------------------------------------------------------------------------
 */

Void
EFFlatDone()
{
#ifdef	MALLOCTRACE
    /* Hash table statistics */
    printf("\n\nStatistics for node hash table:\n");
    HashStats(&efNodeHashTable);
#endif	MALLOCTRACE

    /* Free temporary storage */
    efFreeNodeTable(&efNodeHashTable);
    efFreeNodeList(&efNodeList);
    HashKill(&efCapHashTable);
    HashKill(&efNodeHashTable);
    HashKill(&efHNUseHashTable);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatNodes --
 *
 * Recursive procedure to flatten the nodes in hc->hc_use->use_def,
 * using a depth-first post-order traversal of the hierarchy.
 *
 * Algorithm:
 *	We first recursivly call efFlatNodes for all of our children uses.
 *	This adds their node names to the global node table.  Next we add
 *	our own nodes to the table.  Some nodes will have to be merged
 *	by connections made in this def, or at least will require adjustments
 *	to their resistance or capacitance.  We walk down the connection
 *	list hc->hc_use->use_def->def_conns to do this merging.  Whenever
 *	two nodes merge, the EFNodeName list for the resulting node is
 *	rearranged to begin with the highest precedence name from the lists
 *	for the two nodes being combined.  See efNodeMerge for a discussion
 *	of precedence.
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	Adds node names to the table of flattened node names efNodeHashTable.
 *	May merge nodes from the list efNodeList as per the connection
 *	list hc->hc_use->use_def->def_conns.
 *
 * ----------------------------------------------------------------------------
 */

efFlatNodes(hc)
    HierContext *hc;
{
    /* Recursively flatten each use */
    (void) efHierSrUses(hc, efFlatNodes, (ClientData) NULL);

    /* Add all our own nodes to the table */
    efAddNodes(hc);

    /* Process our own connections and adjustments */
    (void) efAddConns(hc);

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efAddNodes --
 *
 * Add all the nodes defined by the def 'hc->hc_use->use_def' to the
 * global symbol table.  Each global name is prefixed by the hierarchical
 * name component hc->hc_hierName.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds node names to the table of flattened node names efNodeHashTable.
 *
 * ----------------------------------------------------------------------------
 */

efAddNodes(hc)
    HierContext *hc;
{
    Def *def = hc->hc_use->use_def;
    EFNodeName *nn, *newname, *oldname;
    EFNode *node, *newnode;
    EFAttr *ap, *newap;
    HierName *hierName;
    int scale, size, asize;
    HashEntry *he;
    Transform t;

    scale = def->def_scale;
    t = hc->hc_trans;
    t.t_a *= scale; t.t_b *= scale; t.t_c *= scale;
    t.t_d *= scale; t.t_e *= scale; t.t_f *= scale;
    size = sizeof (EFNode) + (efNumResistClasses-1) * sizeof (PerimArea);

    for (node = (EFNode *) def->def_firstn.efnode_next;
	    node != &def->def_firstn;
	    node = (EFNode *) node->efnode_next)
    {
	MALLOC(EFNode *, newnode, size);
	newnode->efnode_attrs = (EFAttr *) NULL;
	for (ap = node->efnode_attrs; ap; ap = ap->efa_next)
	{
	    asize = ATTRSIZE(strlen(ap->efa_text));
	    MALLOC(EFAttr *, newap, asize);
	    (void) strcpy(newap->efa_text, ap->efa_text);
	    GeoTransRect(&t, &ap->efa_loc, &newap->efa_loc);
	    newap->efa_type = ap->efa_type;
	    newap->efa_next = newnode->efnode_attrs;
	    newnode->efnode_attrs = newap;
	}
	newnode->efnode_cap = node->efnode_cap;
	newnode->efnode_client = (ClientData) NULL;
	newnode->efnode_flags = node->efnode_flags;
	newnode->efnode_type = node->efnode_type;
	bcopy((char *) node->efnode_pa, (char *) newnode->efnode_pa,
		efNumResistClasses * sizeof (PerimArea));
	GeoTransRect(&t, &node->efnode_loc, &newnode->efnode_loc);

	/* Prepend to global node list */
	newnode->efnode_next = efNodeList.efnode_next;
	newnode->efnode_prev = (EFNodeHdr *) &efNodeList;
	efNodeList.efnode_next->efnhdr_prev = (EFNodeHdr *) newnode;
	efNodeList.efnode_next = (EFNodeHdr *) newnode;

	/* Add each name for this node to the hash table */
	newnode->efnode_name = (EFNodeName *) NULL;
	for (nn = node->efnode_name; nn; nn = nn->efnn_next)
	{
	    /*
	     * Construct the full hierarchical name of this node.
	     * The path down to this point is given by hc->hc_hierName,
	     * to which nn->efnn_hier is "appended".  Exception: nodes
	     * marked with EF_FETTERM (fet substrate nodes used before
	     * declared, so intended to refer to default global names)
	     * are added as global nodes.
	     */
	    if (node->efnode_flags & EF_FETTERM) hierName = nn->efnn_hier;
	    else hierName = EFHNConcat(hc->hc_hierName, nn->efnn_hier);
	    he = HashFind(&efNodeHashTable, (char *) hierName);

	    /*
	     * The name should only have been in the hash table already
	     * if the node was marked with EF_FETTERM as described above.
	     */
	    if (oldname = (EFNodeName *) HashGetValue(he))
	    {
		if (hierName != nn->efnn_hier)
		    EFHNFree(hierName, hc->hc_hierName, HN_CONCAT);
		if (oldname->efnn_node != newnode)
		    efNodeMerge(oldname->efnn_node, newnode);
		newnode = oldname->efnn_node;
		continue;
	    }

	    /*
	     * We only guarantee that the first name for the node remains
	     * first (since the first name is the "canonical" name for the
	     * node).  The order of the remaining names will be reversed.
	     */
	    MALLOC(EFNodeName *, newname, sizeof (EFNodeName));
	    HashSetValue(he, (char *) newname);
	    newname->efnn_node = newnode;
	    newname->efnn_hier = hierName;
	    if (newnode->efnode_name)
	    {
		newname->efnn_next = newnode->efnode_name->efnn_next;
		newnode->efnode_name->efnn_next = newname;
	    }
	    else
	    {
		newname->efnn_next = (EFNodeName *) NULL;
		newnode->efnode_name = newname;
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efAddConns --
 *
 * Make all the connections for a given def.  This may cause previously
 * distinct nodes in the flat table to merge.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May merge nodes from the list efNodeList as per the connection
 *	list hc->hc_use->use_def->def_conns.
 *
 * ----------------------------------------------------------------------------
 */

efAddConns(hc)
    HierContext *hc;
{
    register Connection *conn;

    if (efWatchNodes)
	printf("Processing %s (%s)\n",
		EFHNToStr(hc->hc_hierName),
		hc->hc_use->use_def->def_name);

    for (conn = hc->hc_use->use_def->def_conns; conn; conn = conn->conn_next)
    {
	/* Special case for speed when no array info is present */
	if (conn->conn_1.cn_nsubs == 0)
	    efAddOneConn(hc, conn->conn_name1, conn->conn_name2, conn);
	else
	    efHierSrArray(hc, conn, efAddOneConn, (ClientData) NULL);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efAddOneConn --
 *
 * Do the work of adding a single connection.  The names of the nodes
 * to be connected are 'name1' and 'name2' (note that these are regular
 * strings, not HierNames).  The resistance of the merged node is to be
 * adjusted by 'deltaR' and its capacitance by 'deltaC'.  If 'name2' is
 * NULL, we just adjust the R and C of the node 'name1'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May merge nodes from the list efNodeList.
 *
 * ----------------------------------------------------------------------------
 */

efAddOneConn(hc, name1, name2, conn)
    HierContext *hc;
    char *name1, *name2;	/* These are strings, not HierNames */
    Connection *conn;
{
    register HashEntry *he1, *he2;
    register EFNode *node, *newnode;
    register int n;

    he1 = EFHNLook(hc->hc_hierName, name1, "connect(1)");
    if (he1 == NULL)
	return 0;

    /* Adjust the resistance and capacitance of its corresponding node */
    node = ((EFNodeName *) HashGetValue(he1))->efnn_node;
    node->efnode_cap += conn->conn_value;
    for (n = 0; n < efNumResistClasses; n++)
    {
	node->efnode_pa[n].pa_area += conn->conn_pa[n].pa_area;
	node->efnode_pa[n].pa_perim += conn->conn_pa[n].pa_perim;
    }

    /* Merge this node with conn_name2 if one was specified */
    if (name2)
    {
	he2 = EFHNLook(hc->hc_hierName, name2, "connect(2)");
	if (he2 == NULL)
	    return 0;
	newnode = ((EFNodeName *) HashGetValue(he2))->efnn_node;
	if (node != newnode)
	    efNodeMerge(node, newnode);
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatGlob --
 *
 * This procedure checks to ensure that all occurrences of the same global
 * name are connected.  It also adds the reduced form of the global name
 * (i.e., the global name with no pathname prefix) to the hash table
 * efNodeHashTable, making this the preferred name of the global node.
 *
 * Algorithm:
 *	Scan through the node table looking for globals.  Add each
 *	global to a global name table keyed by just the first component
 *	of the HierName.  The value of the entry in this table is set
 *	initially to the EFNodeName for the first occurrence of the
 *	global node.  If another occurrence of the global name is
 *	found whose EFNode differs from this one's, it's an error.
 *	However, we still merge all the pieces of a global node
 *	into a single one at the end.
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
efFlatGlob()
{
    EFNodeName *nameFlat, *nameGlob;
    EFNode *nodeFlat, *nodeGlob;
    HashEntry *heFlat, *heGlob;
    HierName *hnFlat, *hnGlob;
    HashTable globalTable;
    HashSearch hs;

    HashInitClient(&globalTable, INITFLATSIZE, HT_CLIENTKEYS,
	efFlatGlobCmp, efFlatGlobCopy, efFlatGlobHash, (Void (*)()) NULL);

    /*
     * The following loop examines each global name (the last component of
     * each flat HierName that ends in the global symbol '!'), using the
     * hash table globalTable to keep track of how many times each global
     * name has been seen.  Each global name should be seen exactly once.
     * The only exceptions are fet substrate nodes (nodes marked with the
     * flag EF_FETTERM), which automatically merge with other global nodes
     * with the same name, since they're only implicitly connected anyway.
     */
    for (nodeFlat = (EFNode *) efNodeList.efnode_next;
	    nodeFlat != &efNodeList;
	    nodeFlat = (EFNode *) nodeFlat->efnode_next)
    {
	/*
	 * Ignore nodes whose names aren't global.  NOTE: we rely on
	 * the fact that EFHNBest() prefers global names to all others,
	 * so if the first name in a node's list isn't global, none of
	 * the rest are either.
	 */
	nameFlat = nodeFlat->efnode_name;
	hnFlat = nameFlat->efnn_hier;
	if (!EFHNIsGlob(hnFlat))
	    continue;

	/*
	 * Look for an entry corresponding to the global part of hnFlat
	 * (only the leaf component) in the global name table.  If one
	 * isn't found, an entry gets created.
	 */
	heGlob = HashFind(&globalTable, (char *) hnFlat);
	nameGlob = (EFNodeName *) HashGetValue(heGlob);
	if (nameGlob == NULL)
	{
	    /*
	     * Create a new EFNodeName that points to nodeFlat, but
	     * don't link it in to nodeFlat->efnode_name yet.
	     */
	    MALLOC(EFNodeName *, nameGlob, sizeof (EFNodeName));
	    HashSetValue(heGlob, (ClientData) nameGlob);
	    nameGlob->efnn_node = nodeFlat;
	    nameGlob->efnn_hier = (HierName *) heGlob->h_key.h_ptr;
	}
	else if (nameGlob->efnn_node != nodeFlat)
	{
	    /*
	     * If either node is a fet substrate node (marked with EF_FETTERM)
	     * it's OK to merge them; otherwise, it's an error, but we still
	     * merge the nodes.  When merging, we blow away nodeGlob and
	     * absorb it into nodeFlat for simplicity in control of the main
	     * loop.  Note that since nameGlob isn't on the efnode_name list
	     * for nodeGlob, we have to update its node backpointer explicitly.
	     */
	    nodeGlob = nameGlob->efnn_node;
	    if ((nodeGlob->efnode_flags & EF_FETTERM) == 0
		    && (nodeFlat->efnode_flags & EF_FETTERM) == 0)
	    {
		efFlatGlobError(nameGlob, nameFlat);
	    }
	    efNodeMerge(nodeFlat, nodeGlob);
	    nameGlob->efnn_node = nodeFlat;
	}
    }

    /*
     * Now make another pass through the global name table,
     * prepending the global name (the HierName consisting of
     * the trailing component only that was allocated when the
     * name was added to globalTable above) to its node, and
     * also adding it to the global hash table efNodeHashTable.
     */
    HashStartSearch(&hs);
    while (heGlob = HashNext(&globalTable, &hs))
    {
	/*
	 * Add the name to the flat node name hash table, and
	 * prepend the EFNodeName to the node's list, but only
	 * if the node didn't already exist in efNodeHashTable.
	 * Otherwise, free nameGlob.
	 */
	nameGlob = (EFNodeName *) HashGetValue(heGlob);
	hnGlob = nameGlob->efnn_hier;
	heFlat = HashFind(&efNodeHashTable, (char *) hnGlob);
	if (HashGetValue(heFlat) == NULL)
	{
	    nodeFlat = nameGlob->efnn_node;
	    HashSetValue(heFlat, (ClientData) nameGlob);
	    nameGlob->efnn_next = nodeFlat->efnode_name;
	    nodeFlat->efnode_name = nameGlob;
	}
	else
	{
	    FREE((char *) nameGlob);
	    EFHNFree(hnGlob, (HierName *) NULL, HN_GLOBAL);
	}
    }

    HashKill(&globalTable);
}

Void
efFlatGlobError(nameGlob, nameFlat)
    EFNodeName *nameGlob, *nameFlat;
{
    EFNode *nodeGlob = nameGlob->efnn_node, *nodeFlat = nameFlat->efnn_node;
    register EFNodeName *nn;
    int count;

    printf("*** Global name %s not fully connected:\n",
			nameGlob->efnn_hier->hn_name);
    printf("One portion contains the names:\n");
    for (count = 0, nn = nodeGlob->efnode_name;
	    count < 10 && nn;
	    count++, nn = nn->efnn_next)
    {
	printf("    %s\n", EFHNToStr(nn->efnn_hier));
    }
    if (nn) printf("    .... (no more names will be printed)\n");
    printf("The other portion contains the names:\n");
    for (count = 0, nn = nodeFlat->efnode_name;
	    count < 10 && nn;
	    count++, nn = nn->efnn_next)
    {
	printf("    %s\n", EFHNToStr(nn->efnn_hier));
    }
    if (nn) printf("    .... (no more names will be printed)\n");
    printf("I'm merging the two pieces into a single node, but you\n");
    printf("should be sure eventually to connect them in the layout.\n\n");
}

int
efFlatGlobCmp(hierName1, hierName2)
    register HierName *hierName1, *hierName2;
{
    if (hierName1 == hierName2)
	return 0;

    return (hierName1 == NULL || hierName2 == NULL
		|| hierName1->hn_hash != hierName2->hn_hash
		|| strcmp(hierName1->hn_name, hierName2->hn_name) != 0);
}

char *
efFlatGlobCopy(hierName)
    register HierName *hierName;
{
    register HierName *hNew;
    int size;

    size = HIERNAMESIZE(strlen(hierName->hn_name));
    MALLOC(HierName *, hNew, size);
    (void) strcpy(hNew->hn_name, hierName->hn_name);
    hNew->hn_parent = (HierName *) NULL;
    hNew->hn_hash = hierName->hn_hash;
    if (efHNStats)
	efHNRecord(size, HN_GLOBAL);

    return (char *) hNew;
}

int
efFlatGlobHash(hierName)
    register HierName *hierName;
{
    return hierName->hn_hash;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatKills --
 *
 * Recursively mark all killed nodes, using a depth-first post-order
 * traversal of the hierarchy.  The algorithm is the same as for
 * efFlatNodes above.
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	May mark node entries in the global name table as killed
 *	by setting EF_KILLED in the efnode_flags field.
 *
 * ----------------------------------------------------------------------------
 */

efFlatKills(hc)
    HierContext *hc;
{
    Def *def = hc->hc_use->use_def;
    HashEntry *he;
    EFNodeName *nn;
    Kill *k;

    /* Recursively visit each use */
    (void) efHierSrUses(hc, efFlatKills, (ClientData) NULL);

    /* Process all of our kill information */
    for (k = def->def_kills; k; k = k->kill_next)
    {
	if (he = EFHNConcatLook(hc->hc_hierName, k->kill_name, "kill"))
	{
	    nn = (EFNodeName *) HashGetValue(he);
	    nn->efnn_node->efnode_flags |= EF_KILLED;
	}
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatCaps --
 *
 * Recursive procedure to flatten all capacitors in the circuit.
 * Produces a single, global hash table (efCapHashTable) indexed
 * by pairs of EFNode pointers, where the value of each entry is the
 * capacitance between the two nodes.
 *
 * Algorithm:
 *	Before this procedure is called, efFlatNodes() should have been
 *	called to create a global table of all node names.  We do a recursive
 *	traversal of the design rooted at 'hc->hc_use->use_def', and construct
 *	full hierarchical names from the terminals of each capacitor
 *	encountered.
 *
 *	These full names are used to find via a lookup in efNodeHashTable the
 *	canonical name of the node for which this full name is an alias.  The
 *	canonical name is output as the node to which this terminal connects.
 *
 *	Capacitance where one of the nodes is GND is treated specially;
 *	instead of adding an entry to the global hash table, we update
 *	the substrate capacitance of the other node appropriately (KLUDGE).
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

efFlatCaps(hc)
    HierContext *hc;
{
    register Connection *conn;

    /* Recursively flatten capacitors */
    (void) efHierSrUses(hc, efFlatCaps, (ClientData) 0);

    /* Output our own capacitors */
    for (conn = hc->hc_use->use_def->def_caps; conn; conn = conn->conn_next)
    {
	/* Special case for speed if no arraying info */
	if (conn->conn_1.cn_nsubs == 0)
	    efFlatSingleCap(hc, conn->conn_name1, conn->conn_name2, conn);
	else
	    efHierSrArray(hc, conn, efFlatSingleCap, (ClientData) NULL);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatSingleCap --
 *
 * Add a capacitor with value 'conn->conn_value' between the nodes
 * 'name1' and 'name2' (text names, not hierarchical names).  Don't
 * add the capacitor if either terminal is a killed node.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds an entry to efCapHashTable indexed by the nodes of 'name1'
 *	and 'name2' respectively.  If the two nodes are the same, though,
 *	nothing happens.  If either node is ground (GND!), the capacitance
 *	is added to the substrate capacitance of the other node instead of
 *	creating a hash table entry.
 *
 * ----------------------------------------------------------------------------
 */

efFlatSingleCap(hc, name1, name2, conn)
    HierContext *hc;		/* Contains hierarchical pathname to cell */
    char *name1, *name2;	/* Names of nodes connecting to capacitor */
    Connection *conn;		/* Contains capacitance to add */
{
    EFNode *n1, *n2;
    HashEntry *he;
    EFCoupleKey ck;

    if ((he = EFHNLook(hc->hc_hierName, name1, "cap(1)")) == NULL)
	return 0;
    n1 = ((EFNodeName *) HashGetValue(he))->efnn_node;
    if (n1->efnode_flags & EF_KILLED)
	return 0;

    if ((he = EFHNLook(hc->hc_hierName, name2, "cap(2)")) == NULL)
	return 0;
    n2 = ((EFNodeName *) HashGetValue(he))->efnn_node;
    if (n2->efnode_flags & EF_KILLED)
	return 0;

    /* Do nothing if the nodes aren't different */
    if (n1 == n2)
	return 0;

    if (EFHNIsGND((HierName *) n1->efnode_name->efnn_hier))
	n2->efnode_cap += conn->conn_value;	/* node 2 to substrate */
    else if (EFHNIsGND((HierName *) n2->efnode_name->efnn_hier))
	n1->efnode_cap += conn->conn_value;	/* node 1 to substrate */
    else
    {
	/* node1 to node2 */
	if (n1 < n2) ck.ck_1 = n1, ck.ck_2 = n2;
	else ck.ck_1 = n2, ck.ck_2 = n1;
	he = HashFind(&efCapHashTable, (char *) &ck);
	HashSetValue(he, conn->conn_value + (int) HashGetValue(he));
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFlatDists --
 *
 * Recursive procedure to flatten all distance information in the circuit.
 * Produces a single, global hash table (efDistHashTable) indexed
 * by Distance structures, where the value of each entry is the same
 * as the key and gives the min and maximum distances between the two
 * points.
 *
 * Results:
 *	Returns 0 to keep efHierSrUses going.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

efFlatDists(hc)
    HierContext *hc;
{
    Distance *dist, *distFlat, distKey;
    HashEntry *he, *heFlat;
    HashSearch hs;

    /* Recursively flatten distances */
    (void) efHierSrUses(hc, efFlatDists, (ClientData) 0);

    /* Process our own distances */
    HashStartSearch(&hs);
    while (he = HashNext(&hc->hc_use->use_def->def_dists, &hs))
    {
	dist = (Distance *) HashGetValue(he);
	efHNBuildDistKey(hc->hc_hierName, dist, &distKey);
	heFlat = HashFind(&efDistHashTable, (char *) &distKey);
	if (distFlat = (Distance *) HashGetValue(heFlat))
	{
	    /*
	     * This code differs from that in efBuildDist(), in that
	     * we replace the min/max information in distFlat from
	     * that in dist, rather than computing a new min/max.
	     * The reason is that the information in dist (in the
	     * parent) is assumed to override that already computed
	     * in the child.
	     */
	    distFlat->dist_min = dist->dist_min;
	    distFlat->dist_max = dist->dist_max;
	    EFHNFree(distKey.dist_1, hc->hc_hierName, HN_CONCAT);
	    EFHNFree(distKey.dist_2, hc->hc_hierName, HN_CONCAT);
	}
	else
	{
	    /*
	     * If there was no entry in the table already with this
	     * key, make the HashEntry point to its key (which is
	     * the newly malloc'd Distance structure).
	     */
	    HashSetValue(heFlat, (ClientData) he->h_key.h_ptr);
	}
    }

    return 0;
}
