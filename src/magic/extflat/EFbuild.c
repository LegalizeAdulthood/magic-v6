/*
 * EFbuild.c -
 *
 * Procedures for building up the hierarchical representation
 * of a circuit.  These are all called from efReadDef() in EFread.c.
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
static char rcsid[] = "$Header: EFbuild.c,v 6.0 90/08/28 18:13:26 mayo Exp $";
#endif  not lint

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#endif
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "utils.h"
#include "malloc.h"
#include "extflat.h"
#include "EFint.h"

/*
 * To avoid allocating ridiculously large amounts of memory to hold
 * transistor types and the names of node types, we maintain the following
 * string tables.  Each string (transistor type or Magic layername) appears
 * exactly once in its respective table; each Fet structure's fet_type field
 * is an index into EFFetTypes[], and each node layer name an index into
 * EFLayerNames[].
 */

    /* The following are ridiculously high */
#define	MAXFETTYPES	100
#define	MAXTYPES	100

    /* Table of transistor types */
char *EFFetTypes[MAXFETTYPES];
int EFFetNumTypes = 0;

    /* Table of Magic layers */
char *EFLayerNames[MAXTYPES] = { "space" };
int EFLayerNumNames = 1;

    /* Forward declarations */
Connection *efAllocConn();
EFNode *efBuildFetNode();

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildNode --
 *
 * Process a "node" line from a .ext file.
 * Creates a new node with an initial name of 'nodeName'
 * and capacitance to substrate 'nodeCap'.  If there is
 * already a node by the name of 'nodeName', adds 'nodeCap'
 * to its existing capacitance.
 *
 * In addition, the arguments 'av' and 'ac' are an (argv, argc)
 * vector of pairs of perimeters and areas for each of the
 * resist classes; these are either stored in the newly created
 * node, or added to the values already stored in an existing one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the HashTable and node list of 'def'.
 *
 * EFNode tables:
 *	Each hash table of nodes is organized in the following way.
 *	This organization is true both for the node tables for each
 *	Def, and for the global table of flattened nodes maintained
 *	in EFflatten.c (although the flattened nodes use the HierName
 *	struct for representing hierarchical names efficiently).
 *
 *	Each HashEntry points to a EFNodeName struct.  The EFNodeName
 *	is a link back to the hash key (a HierName), as well as
 *	a link to the actual EFNode for that name.  The EFNode points
 *	to the first EFNodeName in the NULL-terminated list of all
 *	EFNodeNames pointing to that EFNode; the intent is that this
 *	first EFNodeName is the "official" or highest-precedence
 *	name for the node.
 *
 *	The nodes themselves are linked into a circular, doubly
 *	linked list, for ease in merging two nodes into a single
 *	one as a result of a "connect" statement.
 *
 *	HashEntries	EFNodeNames	       EFNodes
 *
 *			    +---------------+
 *			    |		    |
 *			    V		    | to    from
 *	+-------+	+-------+	    | prev  prev
 *	|	| ---->	|	|-+ 	    |	^   |
 *	+-------+	+-------+ | 	    |	|   |
 *			    |	  | 	    |	|   V
 *			    V	  | 	+---------------+
 *	+-------+	+-------+ +--->	|		|
 *	|	| ---->	|	| ---->	|		|
 *	+-------+	+-------+ +--->	|		|
 *			    |	  |	+---------------+
 *			    V	  |		^   |
 *	+-------+	+-------+ |		|   |
 *	|	| ---->	|	|-+		|   V
 *	+-------+	+-------+	      from  to
 *			    |		      next  next
 *			    V
 *			   NIL
 *
 * ----------------------------------------------------------------------------
 */

Void
efBuildNode(def, nodeName, nodeCap, x, y, layerName, av, ac)
    Def *def;		/* Def to which this connection is to be added */
    char *nodeName;	/* One of the names for this node */
    int nodeCap;	/* Capacitance of this node to ground */
    int x, y;		/* Location of a point inside this node */
    char *layerName;	/* Name of tile type */
    char **av;		/* Pairs of area, perimeter strings */
    int ac;		/* Number of strings in av */
{
    register EFNodeName *newname;
    register EFNode *newnode;
    HashEntry *he;
    unsigned size;
    register int n;

    he = HashFind(&def->def_nodes, nodeName);
    if (newname = (EFNodeName *) HashGetValue(he))
    {
	if (efWarn)
	    efReadError("Warning: duplicate node name %s\n", nodeName);

	/* Just add to C, perim, area of existing node */
	newnode = newname->efnn_node;
	newnode->efnode_cap += nodeCap;
	for (n = 0; n < efNumResistClasses && ac > 1; n++, ac -= 2)
	{
	    newnode->efnode_pa[n].pa_area += atoi(*av++);
	    newnode->efnode_pa[n].pa_perim += atoi(*av++);
	}
	return;
    }

    /* Allocate a new node with 'nodeName' as its single name */
    MALLOC(EFNodeName *, newname, sizeof (EFNodeName));
    newname->efnn_hier = EFStrToHN((HierName *) NULL, nodeName);
    newname->efnn_next = NULL;
    HashSetValue(he, (char *) newname);

    /* New node itself */
    size = sizeof (EFNode) + (efNumResistClasses - 1) * sizeof (PerimArea);
    MALLOC(EFNode *, newnode, size);
    newnode->efnode_flags = 0;
    newnode->efnode_cap = nodeCap;
    newnode->efnode_attrs = (EFAttr *) NULL;
    newnode->efnode_loc.r_xbot = x;
    newnode->efnode_loc.r_ybot = y;
    newnode->efnode_loc.r_xtop = x + 1;
    newnode->efnode_loc.r_ytop = y + 1;
    newnode->efnode_client = (ClientData) NULL;
    if (layerName) newnode->efnode_type =
	    efBuildAddStr(EFLayerNames, &EFLayerNumNames, MAXTYPES, layerName);
    else newnode->efnode_type = 0;

    for (n = 0; n < efNumResistClasses && ac > 1; n++, ac -= 2)
    {
	newnode->efnode_pa[n].pa_area = atoi(*av++);
	newnode->efnode_pa[n].pa_perim = atoi(*av++);
    }
    for ( ; n < efNumResistClasses; n++)
	newnode->efnode_pa[n].pa_area = newnode->efnode_pa[n].pa_perim = 0;

    /* Update back pointers */
    newnode->efnode_name = newname;
    newname->efnn_node = newnode;

    /* Link the node into the list for this def */
    newnode->efnode_next = def->def_firstn.efnode_next;
    newnode->efnode_prev = (EFNodeHdr *) &def->def_firstn;
    def->def_firstn.efnode_next->efnhdr_prev = (EFNodeHdr *) newnode;
    def->def_firstn.efnode_next = (EFNodeHdr *) newnode;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildAttr --
 *
 * Prepend another node attribute to the list for node 'nodeName'.
 * The attribute is located at the coordinates given by 'r' and
 * is on the layer 'layerName'.  The text of the attribute is 'text'.
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
efBuildAttr(def, nodeName, r, layerName, text)
    Def *def;
    char *nodeName;
    Rect *r;
    char *layerName;
    char *text;
{
    HashEntry *he;
    EFNodeName *nn;
    EFAttr *ap;
    int size;

    he = HashFind(&def->def_nodes, nodeName);
    if (HashGetValue(he) == NULL)
    {
	efReadError("Attribute for nonexistent node %s ignored\n", nodeName);
	return;
    }
    nn = (EFNodeName *) HashGetValue(he);

    size = ATTRSIZE(strlen(text));
    MALLOC(EFAttr *, ap, size);
    (void) strcpy(ap->efa_text, text);
    ap->efa_type =
	efBuildAddStr(EFLayerNames, &EFLayerNumNames, MAXTYPES, layerName);
    ap->efa_loc = *r;
    ap->efa_next = nn->efnn_node->efnode_attrs;
    nn->efnn_node->efnode_attrs = ap;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildDist --
 *
 * Process a "dist" line from a .ext file.
 * Both of the names driver and receiver are pathnames with slashes.
 * Add a new Distance record to the hash table for Def, or update
 * an existing Distance record.
 *
 * This strategy allows the .ext file to contain several distance
 * lines for the same pair of points; we do the compression here
 * rather than requiring it be done during extraction.  It's necessary
 * to do compression at some point before flattening; see the description
 * in efFlatDists().
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
efBuildDist(def, driver, receiver, min, max)
    Def *def;		/* Def for which we're adding a new Distance */
    char *driver;	/* Source terminal */
    char *receiver;	/* Destination terminal */
    int min, max;	/* Minimum and maximum acyclic distance from source
			 * to destination.
			 */
{
    Distance *dist, distKey;
    HierName *hn1, *hn2;
    HashEntry *he;

    hn1 = EFStrToHN((HierName *) NULL, driver);
    hn2 = EFStrToHN((HierName *) NULL, receiver);
    distKey.dist_min = min;
    distKey.dist_max = max;
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
#ifdef	notdef
    (void) fprintf(stderr, "ADD %s ", EFHNToStr(distKey.dist_1));
    (void) fprintf(stderr, "%s ", EFHNToStr(distKey.dist_2));
    (void) fprintf(stderr, "%d %d\n", min, max);
#endif	notdef

    he = HashFind(&def->def_dists, (char *) &distKey);
    if (dist = (Distance *) HashGetValue(he))
    {
	/*
	 * There was already an entry in the table; update it
	 * to reflect new minimum and maximum distances.  We
	 * can free the keys since they were already in the
	 * table.
	 */
	dist->dist_min = MIN(dist->dist_min, min);
	dist->dist_max = MAX(dist->dist_max, max);
	EFHNFree(hn1, (HierName *) NULL, HN_ALLOC);
	EFHNFree(hn2, (HierName *) NULL, HN_ALLOC);
    }
    else
    {
	/*
	 * When the key was installed in the hash table, it was
	 * a copy of the Distance 'distKey'.  Leave this as the
	 * value of the HashEntry.
	 */
	HashSetValue(he, (ClientData) he->h_key.h_ptr);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildKill --
 *
 * Process a "killnode" line from a .ext file.
 * Prepends a Kill to the list for def.
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
efBuildKill(def, name)
    Def *def;		/* Def for which we're adding a new Kill */
    char *name;		/* Name of node to die */
{
    Kill *kill;

    MALLOC(Kill *, kill, sizeof (Kill));
    kill->kill_name = EFStrToHN((HierName *) NULL, name);
    kill->kill_next = def->def_kills;
    def->def_kills = kill;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildEquiv --
 *
 * Process an "equiv" line from a .ext file.
 * One of the names 'nodeName1' or 'nodeName2' should be a name for
 * an existing node in the def 'def'.  We simply prepend this name to
 * the list of names for that node.
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
efBuildEquiv(def, nodeName1, nodeName2)
    Def *def;		/* Def for which we're adding a new node name */
    char *nodeName1;	/* One of node names to be made equivalent */
    char *nodeName2;	/* Other name to be made equivalent.  One of nodeName1
			 * or nodeName2 must already be known.
			 */
{
    EFNodeName *nn1, *nn2;
    HashEntry *he1, *he2;

    /* Look up both names in the hash table for this def */
    he1 = HashFind(&def->def_nodes, nodeName1);
    he2 = HashFind(&def->def_nodes, nodeName2);

    nn1 = (EFNodeName *) HashGetValue(he1);
    nn2 = (EFNodeName *) HashGetValue(he2);

    if (nn2 == (EFNodeName *) NULL)
    {
	/* Create nodeName1 if it doesn't exist */
	if (nn1 == (EFNodeName *) NULL)
	{
	    if (efWarn)
		efReadError("Creating new node %s\n", nodeName1);
	    efBuildNode(def, nodeName1, 0, 0, 0,
		    (char *) NULL, (char **) NULL, 0, (char *) NULL);
	    nn1 = (EFNodeName *) HashGetValue(he1);
	}

	/* Make nodeName2 be another alias for node1 */
	efNodeAddName(nn1->efnn_node, he2,
			EFStrToHN((HierName *) NULL, nodeName2));
	return;
    }

    /* If both names exist and are for different nodes, merge them */
    if (nn1)
    {
	if (nn1->efnn_node != nn2->efnn_node)
	{
	    if (efWarn)
		efReadError("Merged nodes %s and %s\n", nodeName1, nodeName2);
	    efNodeMerge(nn1->efnn_node, nn2->efnn_node);
	}
	return;
    }

    /* Make nodeName1 be another alias for node2 */
    efNodeAddName(nn2->efnn_node, he1,
			EFStrToHN((HierName *) NULL, nodeName1));
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildFet --
 *
 * Process a "fet" line from a .ext file.
 * The number of terminals in the fet is argc/3 (which must be integral).
 * Each block of 3 strings in argv describes a single terminal; see the
 * comments below for their interpretation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prepends this fet to the list for the def 'def'.
 *
 * ----------------------------------------------------------------------------
 */

Void
efBuildFet(def, type, r, area, perim, substrate, argc, argv)
    Def *def;		/* Def to which this connection is to be added */
    char *type;		/* Type of this transistor */
    Rect *r;		/* Coordinates of 1x1 rectangle entirely inside fet */
    int area, perim;	/* Total area, perimeter of channel */
    char *substrate;	/* Name of node to which substrate is connected */
    int argc;		/* Size of argv; should be a multiple of 3 */
    char *argv[];	/* Tokens for the rest of the fet line.  These are
			 * taken in groups of 3, one for each terminal.
			 * Each group of 3 consists of the node name to which
			 * the terminal connects, the length of the terminal,
			 * and an attribute list (or the token 0).
			 */
{
    register int n, nterminals = argc / 3;
    register FetTerm *term;
    register Fet *newfet;
    register char **av;

    MALLOC(Fet *, newfet, (unsigned) FetSize(nterminals));
    newfet->fet_nterm = nterminals;
    newfet->fet_area = area;
    newfet->fet_perim = perim;
    newfet->fet_rect = *r;
    newfet->fet_type = efBuildAddStr(EFFetTypes, &EFFetNumTypes, MAXFETTYPES, type);

#define	TERM_NAME	0
#define	TERM_PERIM	1
#define	TERM_ATTRS	2

    newfet->fet_subsnode = efBuildFetNode(def, substrate, TRUE);
    for (av = argv, n = 0; n < nterminals; n++, av += 3)
    {
	term = &newfet->fet_terms[n];
	term->fterm_node = efBuildFetNode(def, av[TERM_NAME], FALSE);
	term->fterm_perim = atoi(av[TERM_PERIM]);

	/* If the attr list is '0', this signifies no attributes */
	if (av[TERM_ATTRS][0] == '0' && av[TERM_ATTRS][1] == '\0')
	    term->fterm_attrs = (char *) NULL;
	else
	    term->fterm_attrs = StrDup((char **) NULL, av[TERM_ATTRS]);
    }

#undef	TERM_NAME
#undef	TERM_PERIM
#undef	TERM_ATTRS

    /* Add this fet to the list for def */
    newfet->fet_next = def->def_fets;
    def->def_fets = newfet;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildFetNode --
 *
 * Look for the node named 'name' in the local table for 'def', or
 * in the global node name table.  If it doesn't already exist,
 * create a EFNode for it.  If 'isSubsNode' is TRUE, this is node
 * is a substrate node and may not exist yet; otherwise, the node
 * must already exist.
 *
 * Results:
 *	Returns a pointer to the EFNode for 'name'.
 *
 * Side effects:
 *	May create a new node, as per above.
 *
 * ----------------------------------------------------------------------------
 */

EFNode *
efBuildFetNode(def, name, isSubsNode)
    Def *def;
    char *name;
    bool isSubsNode;
{
    register HashEntry *he;
    register EFNodeName *nn;

    he = HashFind(&def->def_nodes, name);
    nn = (EFNodeName *) HashGetValue(he);
    if (nn == (EFNodeName *) NULL)
    {
	/* Create node if it doesn't already exist */
	if (efWarn && !isSubsNode)
	    efReadError("Node %s doesn't exist so creating it\n", name);
	efBuildNode(def, name, 0, 0, 0,
		(char *) NULL, (char **) NULL, 0, (char *) NULL);

	nn = (EFNodeName *) HashGetValue(he);
	if (isSubsNode)
	{
	    if (!EFHNIsGlob(nn->efnn_hier))
		efReadError("Default fet substrate node %s is not a global\n",
		    name);
	    nn->efnn_node->efnode_flags |= EF_FETTERM;
	}
    }

    return nn->efnn_node;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildAddStr --
 *
 * Return the index of 'str' in 'table'.
 * Add the string 'str' to the table 'table' if it's not already there.
 *
 * Results:
 *	See above.
 *
 * Side effects:
 *	Increments *pMax if we add an entry to the table.
 *
 * ----------------------------------------------------------------------------
 */

int
efBuildAddStr(table, pMax, size, str)
    register char *table[];	/* Table to search */
    int *pMax;			/* Increment this if we add an entry */
    int size;			/* Maximum size of table */
    register char *str;		/* String to add */
{
    register int n, max;

    max = *pMax;
    for (n = 0; n < max; n++)
	if (strcmp(table[n], str) == 0)
	    return n;

    if (max >= size)
    {
	printf("Too many entries in table (max is %d) to add %s\n", size, str);
	printf("Recompile libextflat.a with a bigger table size\n");
	exit (1);
    }

    table[n++] = StrDup((char **) NULL, str);
    *pMax = n;

    return max;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildUse --
 *
 * Process a "use" line from a .ext file.
 * Creates a new use by the name 'subUseId' of the def named 'subDefName'.
 * If 'subDefName' doesn't exist, it is created, but left marked as
 * unavailable so that readfile() will read it in after it is done
 * with this file.  If 'subUseId' ends in an array subscript, e.g,
 *	useid[xlo:xhi:xsep][ylo:yhi:ysep]
 * its ArrayInfo is filled in from this information; otherwise, its
 * ArrayInfo is marked as not being needed (xlo == xhi, ylo == yhi).
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
efBuildUse(def, subDefName, subUseId, ta, tb, tc, td, te, tf)
    Def *def;		/* Def to which this connection is to be added */
    char *subDefName;	/* Def of which this a use */
    char *subUseId;	/* Use identifier for the def 'subDefName' */
    int ta, tb, tc,
	td, te, tf;	/* Elements of a transform from coordinates of
			 * subDefName up to def.
			 */
{
    Use *newuse;
    Def *newdef;
    register char *cp;

    newdef = efDefLook(subDefName);
    if (newdef == NULL)
	newdef = efDefNew(subDefName);

    MALLOC(Use *, newuse, sizeof (Use));
    newuse->use_def = newdef;
    newuse->use_trans.t_a = ta;
    newuse->use_trans.t_b = tb;
    newuse->use_trans.t_c = tc;
    newuse->use_trans.t_d = td;
    newuse->use_trans.t_e = te;
    newuse->use_trans.t_f = tf;
    newuse->use_next = def->def_uses;
    def->def_uses = newuse;

    /* Set the use identifier and array information */
    if ((cp = index(subUseId, '[')) == NULL)
    {
	newuse->use_id = StrDup((char **) NULL, subUseId);
	newuse->use_xlo = newuse->use_xhi = 0;
	newuse->use_ylo = newuse->use_yhi = 0;
	newuse->use_xsep = newuse->use_ysep = 0;
	return;
    }

    *cp = '\0';
    newuse->use_id = StrDup((char **) NULL, subUseId);
    *cp = '[';
    (void) sscanf(cp, "[%d:%d:%d][%d:%d:%d]",
		    &newuse->use_xlo, &newuse->use_xhi, &newuse->use_xsep,
		    &newuse->use_ylo, &newuse->use_yhi, &newuse->use_ysep);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildConnect --
 *
 * Process a "connect" line from a .ext file.
 * Creates a connection record for the names 'nodeName1' and
 * 'nodeName2'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a new connection record, and prepends it to the
 *	list for def.
 *
 * ----------------------------------------------------------------------------
 */

Void
efBuildConnect(def, nodeName1, nodeName2, deltaC, av, ac)
    Def *def;		/* Def to which this connection is to be added */
    char *nodeName1;	/* Name of first node in connection */
    char *nodeName2;	/* Name of other node in connection */
    int deltaC;		/* Adjustment in capacitance */
    char **av;		/* Strings for area, perimeter adjustment */
    int ac;		/* Number of strings in av */
{
    int n;
    Connection *conn;
    unsigned size = sizeof (Connection)
		    + (efNumResistClasses - 1) * sizeof (PerimArea);

    MALLOC(Connection *, conn, size);
    if (efConnInitSubs(conn, nodeName1, nodeName2))
    {
	conn->conn_value = deltaC;
	conn->conn_next = def->def_conns;
	for (n = 0; n < efNumResistClasses && ac > 1; n++, ac -= 2)
	{
	    conn->conn_pa[n].pa_area = atoi(*av++);
	    conn->conn_pa[n].pa_perim = atoi(*av++);
	}
	for ( ; n < efNumResistClasses; n++)
	    conn->conn_pa[n].pa_area = conn->conn_pa[n].pa_perim = 0;
	def->def_conns = conn;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildResistor --
 *
 * Process a "resistor" line from a .ext file.
 * Creates a resistor record for the names 'nodeName1' and
 * 'nodeName2'.  Both 'nodeName1' and 'nodeName2' must be non-NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a new connection record, and prepends it to the
 *	def_resistors list for def.
 *
 * ----------------------------------------------------------------------------
 */

Void
efBuildResistor(def, nodeName1, nodeName2, resistance)
    Def *def;		/* Def to which this connection is to be added */
    char *nodeName1;	/* Name of first node in resistor */
    char *nodeName2;	/* Name of second node in resistor */
    int resistance;	/* Resistor value */
{
    Connection *conn;

    MALLOC(Connection *, conn, sizeof (Connection));
    if (efConnInitSubs(conn, nodeName1, nodeName2))
    {
	conn->conn_value = resistance;
	conn->conn_next = def->def_resistors;
	def->def_resistors = conn;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efBuildCap --
 *
 * Process a "cap" line from a .ext file.
 * Creates a capacitor record for the names 'nodeName1' and
 * 'nodeName2'.  Both 'nodeName1' and 'nodeName2' must be non-NULL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a new connection record, and prepends it to the
 *	def_caps list for def.
 *
 * ----------------------------------------------------------------------------
 */

Void
efBuildCap(def, nodeName1, nodeName2, cap)
    Def *def;		/* Def to which this connection is to be added */
    char *nodeName1;	/* Name of first node in capacitor */
    char *nodeName2;	/* Name of second node in capacitor */
    int cap;		/* Capacitor value */
{
    Connection *conn;

    MALLOC(Connection *, conn, sizeof (Connection));
    if (efConnInitSubs(conn, nodeName1, nodeName2))
    {
	conn->conn_value = cap;
	conn->conn_next = def->def_caps;
	def->def_caps = conn;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efConnInitSubs --
 *
 * Fill in and check the subscript information for the newly allocated
 * Connection 'conn'.
 *
 * Results:
 *	Returns TRUE if successful, FALSE on error.
 *
 * Side effects:
 *	Fills in the two ConnNames conn->conn_1 and conn->conn_2.
 *	Frees 'conn' in the event of an error.
 *
 * ----------------------------------------------------------------------------
 */

bool
efConnInitSubs(conn, nodeName1, nodeName2)
    Connection *conn;
    char *nodeName1, *nodeName2;
{
    ConnName *c1, *c2;
    int n;

    c1 = &conn->conn_1;
    c2 = &conn->conn_2;
    if (!efConnBuildName(c1, nodeName1) || !efConnBuildName(c2, nodeName2))
	goto bad;

    if (c1->cn_nsubs != c2->cn_nsubs)
    {
	efReadError("Number of subscripts don't match\n");
	goto bad;
    }

    for (n = 0; n < c1->cn_nsubs; n++)
    {
	if (c1->cn_subs[n].r_hi - c1->cn_subs[n].r_lo
		!= c2->cn_subs[n].r_hi - c2->cn_subs[n].r_lo)
	{
	    efReadError("Subscript %d range mismatch\n", n);
	    goto bad;
	}
    }
    return TRUE;

bad:
    if (c1->cn_name) FREE((char *) c1->cn_name);
    if (c2->cn_name) FREE((char *) c2->cn_name);
    FREE((char *) conn);
    return FALSE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efConnBuildName --
 *
 * Fill in the fields of 'cnp' from the string 'name'.
 * If 'name' contains no trailing subscript ranges (which are
 * of the form [lo1:hi1] or [lo1:hi1,lo2:hi2], or [lo1:hi1][lo2:hi2] for
 * compatibility with older versions of Magic), we set cnp->cn_nsubs
 * to zero and cnp->cn_name to a copy of 'name'.  Otherwise, we decode
 * the subscripts and fill in cnp->cn_subs and cnp->cn_nsubs appropriately.
 *
 * Results:
 *	Returns TRUE if successful, FALSE on error.
 *
 * Side effects:
 *	Fills in the fields of the ConnName 'cnp'.
 *
 * ----------------------------------------------------------------------------
 */

bool
efConnBuildName(cnp, name)
    register ConnName *cnp;
    char *name;
{
    register char *srcp, *dstp, *cp, *dp;
    register int nsubs;
    register Range *rp;
    char newname[1024];
    char c;

    cnp->cn_nsubs = 0;
    if (name == NULL)
    {
	cnp->cn_name = NULL;
	return TRUE;
    }

    cp = name;
    /* Make sure it's an array subscript range before treating it specially */
again:
    if ((cp = index(cp, '[')) == NULL)
    {
	cnp->cn_name = StrDup((char **) NULL, name);
	return TRUE;
    }
    for (dp = cp + 1; *dp && *dp != ':'; dp++)
    {
	if (*dp == ']')
	{
	    cp = dp+1;
	    goto again;
	}
    }

    /* Copy the initial part of the name */
    for (srcp = name, dstp = newname; srcp < cp; *dstp++ = *srcp++)
	/* Nothing */;

    /* Replace each subscript range with %d */
    for (nsubs = 0, rp = cnp->cn_subs; (c = *cp) == '[' || c == ','; nsubs++)
    {
	if (nsubs >= MAXSUBS)
	{
	    efReadError("Too many array subscripts (maximum=2)\n");
	    return FALSE;
	}
	if (sscanf(++cp, "%d:%d", &rp[nsubs].r_lo, &rp[nsubs].r_hi) != 2)
	{
	    efReadError("Subscript syntax error\n");
	    return FALSE;
	}
	if (rp[nsubs].r_lo > rp[nsubs].r_hi)
	{
	    efReadError("Backwards subscript range [%d:%d]\n",
			rp[nsubs].r_lo, rp[nsubs].r_hi);
	    return FALSE;
	}

	while (*cp && *cp != ']' && *cp != ',')
	    cp++;
	if (*cp == ']') cp++;
    }

    /* Generate format for sprintf */
    *dstp++ = '[';
    *dstp++ = '%';
    *dstp++ = 'd';
    if (nsubs == 2)
    {
	*dstp++ = ',';
	*dstp++ = '%';
	*dstp++ = 'd';
    }
    *dstp++ = ']';

    /* Copy remainder of path */
    while (*dstp++ = *cp++)
	/* Nothing */;

    cnp->cn_name = StrDup((char **) NULL, newname);
    cnp->cn_nsubs = nsubs;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efNodeAddName --
 *
 * Add a name to the list for 'node'.
 * We already have a HashEntry for the new name.
 * The new name is added to the front of the list
 * for 'node' only if it is higher in precedence
 * than the name already at the front of the list.
 * Sets the value of 'he' to be the newly allocated
 * EFNodeName.
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
efNodeAddName(node, he, hn)
    EFNode *node;
    HashEntry *he;
    HierName *hn;
{
    EFNodeName *newnn;
    EFNodeName *oldnn;

    MALLOC(EFNodeName *, newnn, sizeof (EFNodeName));
    newnn->efnn_node = node;
    newnn->efnn_hier = hn;
    HashSetValue(he, (char *) newnn);

    /* Link in the new name */
    oldnn = node->efnode_name;
    if (oldnn == NULL || EFHNBest(newnn->efnn_hier, oldnn->efnn_hier))
    {
	/* New head of list */
	newnn->efnn_next = oldnn;
	node->efnode_name = newnn;
    }
    else
    {
	/* Link it in behind the head of the list */
	newnn->efnn_next = oldnn->efnn_next;
	oldnn->efnn_next = newnn;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efNodeMerge --
 *
 * Combine two nodes.  The resistances and capacitances are summed.
 * The attribute lists are appended.  The location chosen is the
 * lower-leftmost, with lowest being considered before leftmost.
 * The canonical name of the new node is taken to be the highest
 * precedence among the names for all nodes.
 *
 * One of the nodes will no longer be referenced, so we arbitrarily
 * make this node2 and free its memory.
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
efNodeMerge(node1, node2)
    EFNode *node1, *node2;	/* Hierarchical nodes */
{
    EFNodeName *nn, *nnlast;
    EFAttr *ap;
    register int n;

    /* Sanity check: ignore if same node */
    if (node1 == node2)
	return;

    if (efWatchNodes)
    {
	if (HashLookOnly(&efWatchTable, (char *) node1->efnode_name->efnn_hier)
	    || (node2->efnode_name
		&& HashLookOnly(&efWatchTable,
				(char *) node2->efnode_name->efnn_hier)))
	{
	    printf("\ncombine: %s\n",
		EFHNToStr(node1->efnode_name->efnn_hier));
	    printf("  with   %s\n\n", 
		node2->efnode_name
		    ? EFHNToStr(node2->efnode_name->efnn_hier)
		    : "(unnamed)");
	}
    }

    /* Sum capacitances, perimeters, areas */
    node1->efnode_cap += node2->efnode_cap;
    for (n = 0; n < efNumResistClasses; n++)
    {
	node1->efnode_pa[n].pa_area += node2->efnode_pa[n].pa_area;
	node1->efnode_pa[n].pa_perim += node2->efnode_pa[n].pa_perim;
    }

    /* Make all EFNodeNames point to node1 */
    if (node2->efnode_name)
    {
	for (nn = node2->efnode_name; nn; nn = nn->efnn_next)
	{
	    nnlast = nn;
	    nn->efnn_node = node1;
	}

	/* Concatenate list of EFNodeNames, taking into account precedence */
	if (EFHNBest(node2->efnode_name->efnn_hier,
		     node1->efnode_name->efnn_hier))
	{
	    /*
	     * New official name is that of node2.
	     * The new list of names is:
	     *	node2-names, node1-names
	     */
	    nnlast->efnn_next = node1->efnode_name;
	    node1->efnode_name = node2->efnode_name;
	}
	else
	{
	    /*
	     * Keep old official name.
	     * The new list of names is:
	     *	node1-names[0], node2-names, node1-names[1-]
	     */
	    nnlast->efnn_next = node1->efnode_name->efnn_next;
	    node1->efnode_name->efnn_next = node2->efnode_name;
	}
    }

    /* Merge attribute lists */
    if (ap = node2->efnode_attrs)
    {
	while (ap->efa_next)
	    ap = ap->efa_next;
	ap->efa_next = node1->efnode_attrs;
	node1->efnode_attrs = ap;
	node2->efnode_attrs = (EFAttr *) NULL;	/* Sanity */
    }

    /*
     * Choose the new location only if node2's location is a valid one,
     * i.e, node2 wasn't created before it was mentioned.  This is mainly
     * to deal with new fets, resistors, and capacitors created by resistance
     * extraction, which appear with their full hierarchical names in the
     * .ext file for the root cell.
     */
    if (node2->efnode_type > 0)
    {
	if (node2->efnode_loc.r_ybot < node1->efnode_loc.r_ybot
	    || (node2->efnode_loc.r_ybot == node1->efnode_loc.r_ybot
		    && node2->efnode_loc.r_xbot < node1->efnode_loc.r_xbot))
	{
	    node1->efnode_loc = node2->efnode_loc;
	    node1->efnode_type = node2->efnode_type;
	}
    }

    /* Unlink node2 from list for def */
    node2->efnode_prev->efnhdr_next = node2->efnode_next;
    node2->efnode_next->efnhdr_prev = node2->efnode_prev;

    /*
     * Only if both nodes were EF_FETTERM do we keep EF_FETTERM set
     * in the resultant node.
     */
    if ((node2->efnode_flags & EF_FETTERM) == 0)
	node1->efnode_flags &= ~EF_FETTERM;

    /* Get rid of node2 */
    FREE((char *) node2);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFreeNodeTable --
 *
 * Free the EFNodeNames (and the HierNames they point to) pointed to by
 * the entries in the HashTable 'table'.  Each EFNodeName is assumed to
 * be pointed to by exactly one HashEntry, but each HierName can be
 * pointed to by many entries (some of which may be in other HashTables).
 * As a result, the HierNames aren't freed here; instead, an entry is
 * added to efFreeHashTable for each HierName encountered.  Everything
 * is then freed at the end by EFDone().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *	Adds an entry to hnTable for each HierName.
 *
 * ----------------------------------------------------------------------------
 */

Void
efFreeNodeTable(table)
    HashTable *table;
{
    HashSearch hs;
    register HashEntry *he;
    register HierName *hn;
    register EFNodeName *nn;

    HashStartSearch(&hs);
    while (he = HashNext(table, &hs))
	if (nn = (EFNodeName *) HashGetValue(he))
	{
	    for (hn = nn->efnn_hier; hn; hn = hn->hn_parent)
		(void) HashFind(&efFreeHashTable, (char *) hn);
	    FREE((char *) nn);
	}
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFreeNodeList --
 *
 * Free the circular list of nodes of which 'head' is the head.
 * Don't free 'head' itself, since it's statically allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 * ----------------------------------------------------------------------------
 */

Void
efFreeNodeList(head)
    EFNode *head;
{
    register EFNode *node;
    register EFAttr *ap;

    for (node = (EFNode *) head->efnode_next;
	    node != head;
	    node = (EFNode *) node->efnode_next)
    {
	for (ap = node->efnode_attrs; ap; ap = ap->efa_next)
	    FREE((char *) ap);
	FREE((char *) node);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * efFreeConn --
 *
 * Free the Connection *conn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 * ----------------------------------------------------------------------------
 */

Void
efFreeConn(conn)
    register Connection *conn;
{
    if (conn->conn_name1) FREE(conn->conn_name1);
    if (conn->conn_name2) FREE(conn->conn_name2);
    FREE((char *) conn);
}
