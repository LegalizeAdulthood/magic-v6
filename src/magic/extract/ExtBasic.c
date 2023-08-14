/*
 * ExtBasic.c --
 *
 * Circuit extraction.
 * Flat extraction of a single CellDef.
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
static char sccsid[] = "@(#)ExtBasic.c	4.13 MAGIC (Berkeley) 12/5/85";
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
#include "debug.h"
#include "extract.h"
#include "extractInt.h"
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "stack.h"

/* --------------------- Data local to this file ---------------------- */

    /*
     * The following are used to accumulate perimeter and area
     * on each layer when building up the node list.  They are
     * used to compute the resistance of each node.  Each is
     * indexed by sheet resistivity class.
     */
int extResistPerim[NT], extResistArea[NT];

    /*
     * The following structure is used in extracting transistors.
     *
     * A "terminal" below refers to any port on the transistor that
     * is not the gate.  In most cases, these are the "diffusion"
     * ports of the transistor.
     */
#define	MAXSD	10	/* Maximum # of terminals per transistor */


typedef struct		/* Position of each terminal (below) tile position */
{
    int		pnum;
    Point	pt;
} TermTilePos;


struct transRec
{
    int		 tr_nterm;		/* Number of terminals */
    int		 tr_gatelen;		/* Perimeter of connection to gate */
    NodeRegion	*tr_gatenode;		/* Node region for gate terminal */
    NodeRegion	*tr_termnode[MAXSD];	/* Node region for each diff terminal */
    int		 tr_termlen[MAXSD];	/* Length of each diff terminal edge,
					 * used for computing L/W for the fet.
					 */
    int		 tr_perim;		/* Total perimeter */
    TermTilePos  tr_termpos[MAXSD];	/* lowest tile connecting to term */
} extTransRec;

#define	EDGENULL(r)	((r)->r_xbot > (r)->r_xtop || (r)->r_ybot > (r)->r_ytop)

/* Forward declarations */
int extTransTileFunc();
int extTransPerimFunc();
NodeRegion *extTransFindSubsNode();
int extTransFindSubsFunc();

/*
 * ----------------------------------------------------------------------------
 *
 * extBasic --
 *
 * Extract a single CellDef, and output the result to the
 * file 'outFile'.
 *
 * Results:
 *	Returns a list of Region structs that comprise all
 *	the nodes in 'def'.  It is the caller's responsibility
 *	to call ExtResetTile() and ExtFreeLabRegions() to restore
 *	the CellDef to its original state and to free the list
 *	of regions we build up.
 *
 * Side effects:
 *	Writes the result of extracting just the paint of
 *	the CellDef 'def' to the output file 'outFile'.
 *	The following kinds of records are output:
 *
 *		node
 *		equiv
 *		fet
 *
 * Interruptible in a limited sense.  We will still return a
 * Region list, but labels may not have been assigned, and
 * nodes and fets may not have been output.
 *
 * ----------------------------------------------------------------------------
 */

NodeRegion *
extBasic(def, outFile)
    CellDef *def;	/* Cell being extracted */
    FILE *outFile;	/* Output file */
{
    NodeRegion *nodeList, *extFindNodes();
    bool coupleInitialized = FALSE;
    TransRegion *transList;
    HashTable extCoupleHash;

    /*
     * Build up a list of the transistor regions for extOutputTrans()
     * below.  We're only interested in pointers from each region to
     * a tile in that region, not the back pointers from the tiles to
     * the regions.
     */
    transList = (TransRegion *) ExtFindRegions(def, &TiPlaneRect,
				    &ExtCurStyle->exts_transMask,
				    ExtCurStyle->exts_transConn,
				    extUnInit, extTransFirst, extTransEach);
    ExtResetTiles(def, extUnInit);

    /*
     * Build up a list of the electrical nodes (equipotentials)
     * for extOutputNodes() below.  For this, we definitely want
     * to leave each tile pointing to its associated Region struct.
     * Compute resistance and capacitance on the fly.
     * Use a special-purpose version of ExtFindRegions for speed.
     */
    if (!SigInterruptPending)
	nodeList = extFindNodes(def, (Rect *) NULL);

    /* Assign the labels to their associated regions */
    if (!SigInterruptPending)
	ExtLabelRegions(def, ExtCurStyle->exts_nodeConn);

    /*
     * Make sure all geometry with the same label is part of the
     * same electrical node.
     */
    if (!SigInterruptPending && (ExtDoWarn & EXTWARN_DUP))
	extFindDuplicateLabels(def, nodeList);

    /*
     * Build up table of coupling capacitances (overlap, sidewall).
     * This comes before extOutputNodes because we may have to adjust
     * node capacitances in this step.
     */
    if (!SigInterruptPending && (ExtOptions&EXT_DOCOUPLING))
    {
	coupleInitialized = TRUE;
	HashInit(&extCoupleHash, 256, HashSize(sizeof (CoupleKey)));
	extFindCoupling(def, &extCoupleHash, (Rect *) NULL);
    }

    /* Output each node, along with its resistance and capacitance to GND */
    if (!SigInterruptPending)
	extOutputNodes(nodeList, outFile);

    /* Output coupling capacitances */
    if (!SigInterruptPending && (ExtOptions&EXT_DOCOUPLING))
	extOutputCoupling(&extCoupleHash, outFile);

    /* Output transistors and connectivity between nodes */
    if (!SigInterruptPending)
	extOutputTrans(def, transList, outFile);

    /* Clean up */
    if (coupleInitialized) HashKill(&extCoupleHash);
    ExtFreeLabRegions((LabRegion *) transList);
    return (nodeList);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extSetResist --
 *
 * The input to this procedure is a pointer to a NodeRegion.
 * Its resistance is computed from the area and perimeter stored
 * in the arrays extResistPerim[] and extResistArea[].  These arrays
 * are then reset to zero.
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
 *	None.
 *
 * Side effects:
 *	See the comments above.
 *
 * ----------------------------------------------------------------------------
 */

extSetResist(reg)
    register NodeRegion *reg;
{
    register int n, perim, area;
    float s, fperim, v;

    for (n = 0; n < ExtCurStyle->exts_numResistClasses; n++)
    {
	reg->nreg_pa[n].pa_area = area = extResistArea[n];
	reg->nreg_pa[n].pa_perim = perim = extResistPerim[n];
	if (area > 0 && perim > 0)
	{
	    v = (double) (perim*perim - 16*area);

	    /* Approximate by one square if v < 0 */
	    if (v < 0) s = 0; else s = sqrt(v);

	    fperim = (float) perim;
	    reg->nreg_resist += (fperim+s)/(fperim-s)
				    * ExtCurStyle->exts_resistByResistClass[n];
	}

	/* Reset for the next pass */
	extResistArea[n] = extResistPerim[n] = 0;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extOutputNodes --
 *
 * The resistance and capacitance of each node have already been
 * computed, so all we need do is output them.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes a number of 'node' and 'equiv' records to the file 'outFile'.
 *
 * Interruptible.  If SigInterruptPending is detected, we stop outputting
 * nodes and return.
 *
 * ----------------------------------------------------------------------------
 */

Void
extOutputNodes(nodeList, outFile)
    NodeRegion *nodeList;	/* Nodes */
    FILE *outFile;		/* Output file */
{
    int rround = ExtCurStyle->exts_resistScale/2;
    int cround = ExtCurStyle->exts_capScale/2;
    register NodeRegion *reg;
    register LabelList *ll;
    register char *cp;
    register int n;
    Label *lab;
    char *text;

    for (reg = nodeList; reg && !SigInterruptPending; reg = reg->nreg_next)
    {
	/* Output the node */
	text = extNodeName((LabRegion *) reg);
	(void) fprintf(outFile, "node \"%s\" %d %d", text,
		(reg->nreg_resist+rround)/ExtCurStyle->exts_resistScale,
		(reg->nreg_cap+cround)/ExtCurStyle->exts_capScale);

	/* Output its location (lower-leftmost point and type name) */
	(void) fprintf(outFile, " %d %d %s",
		reg->nreg_ll.p_x, reg->nreg_ll.p_y,
		DBTypeShortName(reg->nreg_type));

	/* Output its area and perimeter for each resistivity class */
	for (n = 0; n < ExtCurStyle->exts_numResistClasses; n++)
	    (void) fprintf(outFile, " %d %d", reg->nreg_pa[n].pa_area,
				reg->nreg_pa[n].pa_perim);
	(void) putc('\n', outFile);

	/* Output its attribute list */
	for (ll = reg->nreg_labels; ll; ll = ll->ll_next)
	    if (extLabType(ll->ll_label->lab_text, LABTYPE_NODEATTR))
	    {
		/* Don't output the trailing character for node attributes */
		lab = ll->ll_label;
		(void) fprintf(outFile, "attr %s %d %d %d %d %s \"",
			text, lab->lab_rect.r_xbot, lab->lab_rect.r_ybot,
			      lab->lab_rect.r_xtop, lab->lab_rect.r_ytop,
			      DBTypeShortName(lab->lab_type));
		cp = lab->lab_text;
		n = strlen(cp) - 1;
		while (n-- > 0)
		    putc(*cp++, outFile);
		(void) fprintf(outFile, "\"\n");
	    }

	/* Output the alternate names for the node */
	for (ll = reg->nreg_labels; ll; ll = ll->ll_next)
	    if (ll->ll_label->lab_text == text)
	    {
		for (ll = ll->ll_next; ll; ll = ll->ll_next)
		    if (extLabType(ll->ll_label->lab_text, LABTYPE_NAME))
			(void) fprintf(outFile, "equiv \"%s\" \"%s\"\n",
					    text, ll->ll_label->lab_text);
		break;
	    }
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * extFindDuplicateLabels --
 *
 * Verify that no node in the list 'nreg' has a label that appears in
 * any other node in the list.  Leave a warning turd if one is.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Leaves feedback attached to each node that contains a label
 *	duplicated in another node.
 *
 * ----------------------------------------------------------------------------
 */

extFindDuplicateLabels(def, nreg)
    CellDef *def;
    NodeRegion *nreg;
{
    static char *badmesg =
	"Label \"%s\" attached to more than one unconnected node: %s";
    bool hashInitialized = FALSE;
    char message[512], name[512], *text;
    register NodeRegion *np, *np2;
    register LabelList *ll, *ll2;
    register HashEntry *he;
    NodeRegion *lastreg;
    NodeRegion badLabel;
    HashTable labelHash;
    Rect r;

    for (np = nreg; np; np = np->nreg_next)
    {
	for (ll = np->nreg_labels; ll; ll = ll->ll_next)
	{
	    text = ll->ll_label->lab_text;
	    if (!extLabType(text, LABTYPE_NAME))
		continue;

	    if (!hashInitialized)
		HashInit(&labelHash, 32, 0), hashInitialized = TRUE;
	    he = HashFind(&labelHash, text);
	    lastreg = (NodeRegion *) HashGetValue(he);
	    if (lastreg == (NodeRegion *) NULL)
		HashSetValue(he, (ClientData) np);
	    else if (lastreg != np && lastreg != &badLabel)
	    {
		/*
		 * Make a pass through all labels for all nodes.
		 * Leave a feedback turd over each instance of the
		 * offending label.
		 */
		for (np2 = nreg; np2; np2 = np2->nreg_next)
		{
		    for (ll2 = np2->nreg_labels; ll2; ll2 = ll2->ll_next)
		    {
			if (strcmp(ll2->ll_label->lab_text, text) == 0)
			{
			    extNumWarnings++;
			    if (!DebugIsSet(extDebugID, extDebNoFeedback))
			    {
				r.r_ll = r.r_ur = ll2->ll_label->lab_rect.r_ll;
				r.r_xbot--, r.r_ybot--, r.r_xtop++, r.r_ytop++;
				extMakeNodeNumPrint(name,
					    np2->nreg_pnum, np2->nreg_ll);
				(void) sprintf(message, badmesg, text, name);
				DBWFeedbackAdd(&r, message, def,
					    1, STYLE_PALEHIGHLIGHTS);
			    }
			}
		    }
		}

		/* Mark this label as already having generated an error */
		HashSetValue(he, (ClientData) &badLabel);
	    }
	}
    }

    if (hashInitialized)
	HashKill(&labelHash);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extNodeName --
 *
 * Given a pointer to a LabRegion, return a pointer to a string
 * that can be printed as the name of the node.  If the LabRegion
 * has a list of attached labels, use one of the labels; otherwise,
 * use its node number.
 *
 * Results:
 *	Returns a pointer to a string.  If the node had a label, this
 *	is a pointer to the lab_text field of the first label on the
 *	label list for the node; otherwise, it is a pointer to a static
 *	buffer into which we have printed the node number.
 *
 * Side effects:
 *	May overwrite the static buffer used to hold the printable
 *	version of a node number.
 *
 * ----------------------------------------------------------------------------
 */

char *
extNodeName(node)
    LabRegion *node;
{
    static char namebuf[100];	/* Big enough to hold a generated nodename */
    register LabelList *ll;

    if (node == (LabRegion *) NULL || SigInterruptPending)
	return ("(none)");

    for (ll = node->lreg_labels; ll; ll = ll->ll_next)
	if (extLabType(ll->ll_label->lab_text, LABTYPE_NAME))
	    return (ll->ll_label->lab_text);

    extMakeNodeNumPrint(namebuf, node->lreg_pnum, node->lreg_ll);
    return (namebuf);
}

/*
 * ---------------------------------------------------------------------
 *
 * ExtSortTerminals --
 *
 * Sort the terminals of a transistor so that the terminal with the
 * lowest leftmost coordinate on the plane with the lowest number is
 * output first.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The tr_termnode, tr_termlen, and tr_termpos entries may change.
 *
 * ---------------------------------------------------------------------
 */
Void ExtSortTerminals(tran)
    struct transRec  *tran;
{
    int		nsd, changed;
    TermTilePos	*p1, *p2;
    NodeRegion	*tmp_node;
    TermTilePos	tmp_pos;
    int		tmp_len;

    do
    {
	changed = 0;
	for( nsd = 0; nsd < tran->tr_nterm-1; nsd++ )
	{
	    p1 = &(tran->tr_termpos[nsd]);
	    p2 = &(tran->tr_termpos[nsd+1]);
	    if( p2->pnum > p1->pnum )
		continue;
	    else if( p2->pnum == p1->pnum )
	    {
		if( p2->pt.p_x > p1->pt.p_x )
		    continue;
		else if( p2->pt.p_x == p1->pt.p_x && p2->pt.p_y > p1->pt.p_y )
		    continue;
	    }
	    changed = 1;
	    tmp_node = tran->tr_termnode[nsd];
	    tmp_pos = tran->tr_termpos[nsd];
	    tmp_len = tran->tr_termlen[nsd];

	    tran->tr_termnode[nsd] = tran->tr_termnode[nsd+1];
	    tran->tr_termpos[nsd] = tran->tr_termpos[nsd+1];
	    tran->tr_termlen[nsd] = tran->tr_termlen[nsd+1];
	    
	    tran->tr_termnode[nsd+1] = tmp_node;
	    tran->tr_termpos[nsd+1] = tmp_pos;
	    tran->tr_termlen[nsd+1] = tmp_len;
	}
     }
     while( changed );
}

/*
 * ----------------------------------------------------------------------------
 *
 * extOutputTrans --
 *
 * For each TransRegion in the supplied list, corresponding to a single
 * transistor in the layout, compute and output:
 *	- Its type
 *	- Its area and perimeter
 *	- Its substrate node
 *	- For each of the gate, and the various diff terminals (eg,
 *	  source, drain):
 *		Node to which the terminal connects
 *		Length of the terminal
 *		Attributes (comma-separated), or 0 if none.
 *
 * The tiles in 'def' don't point back to the TransRegions in this list,
 * but rather to the NodeRegions corresponding to their electrical nodes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes a number of 'fet' records to the file 'outFile'.
 *
 * Interruptible.  If SigInterruptPending is detected, we stop traversing
 * the transistor list and return.
 *
 * ----------------------------------------------------------------------------
 */

Void
extOutputTrans(def, transList, outFile)
    CellDef *def;		/* Cell being extracted */
    TransRegion *transList;	/* Transistor regions built up in first pass */
    FILE *outFile;		/* Output file */
{
    NodeRegion *node, *subsNode;
    register TransRegion *reg;
    char *subsName;
    FindRegion arg;
    LabelList *ll;
    TileType t;
    int nsd;

    for (reg = transList; reg && !SigInterruptPending; reg = reg->treg_next)
    {
	/*
	 * Visit all of the tiles in the transistor region, updating
	 * extTransRec.tr_termnode[] and extTransRec.tr_termlen[],
	 * and the attribute lists for this transistor.
	 *
	 * Algorithm: first visit all tiles in the transistor, marking
	 * them with 'reg', then visit them again re-marking them with
	 * the gate node (extGetRegion(reg->treg_tile)).
	 */
	extTransRec.tr_nterm = 0;
	extTransRec.tr_gatelen = 0;
	extTransRec.tr_perim = 0;
	extTransRec.tr_gatenode = (NodeRegion *) extGetRegion(reg->treg_tile);

	arg.fra_def = def;
	arg.fra_connectsTo = ExtCurStyle->exts_transConn;
	arg.fra_pNum = DBPlane(TiGetType(reg->treg_tile));

	    /* Mark with reg and process each perimeter segment */
	arg.fra_uninit = (ClientData) extTransRec.tr_gatenode;
	arg.fra_region = (Region *) reg;
	arg.fra_each = extTransTileFunc;
	(void) ExtFindNeighbors(reg->treg_tile, arg.fra_pNum, &arg);

	    /* Re-mark with extTransRec.tr_gatenode */
	arg.fra_uninit = (ClientData) reg;
	arg.fra_region = (Region *) extTransRec.tr_gatenode;
	arg.fra_each = (int (*)()) NULL;
	(void) ExtFindNeighbors(reg->treg_tile, arg.fra_pNum, &arg);

	/*
	 * For types that require a minimum number of terminals,
	 * check to make sure that they all exist.  If they don't,
	 * issue a warning message and make believe the missing
	 * terminals are the same as the last terminal we do have.
	 */
	t = TiGetType(reg->treg_tile);
	nsd = ExtCurStyle->exts_transSDCount[t];
	if (extTransRec.tr_nterm < nsd)
	{
	    char mesg[256];
	    int missing = nsd - extTransRec.tr_nterm;

	    (void) sprintf(mesg, "fet missing %d terminal%s", missing,
					missing == 1 ? "" : "s");
	    if (extTransRec.tr_nterm > 0)
	    {
		node = extTransRec.tr_termnode[extTransRec.tr_nterm-1];
		(void) strcat(mesg, ";\n connecting remainder to node ");
		(void) strcat(mesg, extNodeName((LabRegion *) node));
		while (extTransRec.tr_nterm < nsd) 
		{
		    extTransRec.tr_termlen[extTransRec.tr_nterm] = 0;
		    extTransRec.tr_termnode[extTransRec.tr_nterm++] = node;
		}
	    }
	    if (ExtDoWarn & EXTWARN_FETS)
		extTransBad(def, reg->treg_tile, mesg);
	}
	else if (extTransRec.tr_nterm > nsd)
	{
	     
	}

	/*
	 * Output the transistor record.
	 * The type is ExtCurStyle->exts_transName[t], which should have
	 * some meaning to the simulator we are producing this file for.
	 * Use the default substrate node unless the transistor overlaps
	 * material whose type is in exts_transSubstrateTypes, in which
	 * case we use the node of the overlapped material.
	 */
	subsName = ExtCurStyle->exts_transSubstrateName[t];
	if (!TTMaskIsZero(&ExtCurStyle->exts_transSubstrateTypes[t])
		&& (subsNode = extTransFindSubsNode(def, reg)))
	{
	    subsName = extNodeName(subsNode);
	}
	(void) fprintf(outFile, "fet %s %d %d %d %d %d %d \"%s\"",
		    ExtCurStyle->exts_transName[t],
		    reg->treg_ll.p_x, reg->treg_ll.p_y, 
		    reg->treg_ll.p_x + 1, reg->treg_ll.p_y + 1,
		    reg->treg_area, extTransRec.tr_perim, subsName);

	/* gate */
	node = (NodeRegion *) extGetRegion(reg->treg_tile);
	ll = node->nreg_labels;
	extTransOutTerminal((LabRegion *) node, ll, LL_GATEATTR,
			extTransRec.tr_gatelen, outFile);

	ExtSortTerminals(&extTransRec);

	/* each non-gate terminal */
	for (nsd = 0; nsd < extTransRec.tr_nterm; nsd++)
	    extTransOutTerminal((LabRegion *) extTransRec.tr_termnode[nsd], ll,
			nsd, extTransRec.tr_termlen[nsd], outFile);

	(void) fputs("\n", outFile);
    }
}

NodeRegion *
extTransFindSubsNode(def, treg)
    CellDef *def;
    register TransRegion *treg;
{
    TileType t = TiGetType(treg->treg_tile);
    TileTypeBitMask *mask;
    NodeRegion *nreg;
    Rect tileArea;
    int pNum;

    TiToRect(treg->treg_tile, &tileArea);
    mask = &ExtCurStyle->exts_transSubstrateTypes[t];
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
	if (TTMaskIntersect(&DBPlaneTypes[pNum], mask))
	{
	    if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum], &tileArea,
		    mask, extTransFindSubsFunc, (ClientData) &nreg))
		return nreg;
	}
    }

    return (NodeRegion *) NULL;
}

int
extTransFindSubsFunc(tile, pnreg)
    register Tile *tile;
    NodeRegion **pnreg;
{
    if (tile->ti_client != (ClientData) extUnInit)
    {
	*pnreg = (NodeRegion *) tile->ti_client;
	return 1;
    }

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTransTileFunc --
 *
 * Filter function called by ExtFindNeighbors for each tile in a
 * transistor.  Responsible for collecting the nodes, lengths,
 * and attributes of all the terminals on this transistor.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	Fills in the transRec structure extTransRec.
 *
 * ----------------------------------------------------------------------------
 */

int
extTransTileFunc(tile)
    Tile *tile;
{
    TileTypeBitMask mask;

    register LabelList *ll;
    register Label *lab;
    Rect r;

    for (ll = extTransRec.tr_gatenode->nreg_labels; ll; ll = ll->ll_next)
    {
	/* Skip if already marked */
	if (ll->ll_attr != LL_NOATTR) continue;
	lab = ll->ll_label;
	TITORECT(tile, &r);
	if (GEO_TOUCH(&r, &lab->lab_rect) && 
		extLabType(lab->lab_text, LABTYPE_GATEATTR))  
	{
	     ll->ll_attr = LL_GATEATTR;
	}
    }
    /*
     * Visit each segment of the perimeter of this tile that
     * that borders on something of a different type.
     */
    mask = ExtCurStyle->exts_transConn[TiGetType(tile)];
    TTMaskCom(&mask);
    (void) extEnumTilePerim(tile, mask, extTransPerimFunc, (ClientData) NULL);
    return (0);
}

int
extTransPerimFunc(bp)
    register Boundary *bp;
{
    TileType tinside = TiGetType(bp->b_inside);
    TileType toutside = TiGetType(bp->b_outside);
    NodeRegion *diffNode = (NodeRegion *) extGetRegion(bp->b_outside);
    int len = BoundaryLength(bp);
    register int thisterm;
    register LabelList *ll;
    register Label *lab;
    Rect r;

    if (TTMaskHasType(&ExtCurStyle->exts_transSDTypes[tinside], toutside))
    {
	/*
	 * It's a diffusion terminal (source or drain).
	 * See if the node is already in our table; add it
	 * if it wasn't already there.
	 */
	for (thisterm = 0; thisterm < extTransRec.tr_nterm; thisterm++)
	    if (extTransRec.tr_termnode[thisterm] == diffNode)
		break;
	if (thisterm >= extTransRec.tr_nterm)
	{
	    extTransRec.tr_nterm++;
	    extTransRec.tr_termnode[thisterm] = diffNode;
	    extTransRec.tr_termlen[thisterm] = 0;
	    extTransRec.tr_termpos[thisterm].pnum = DBPlane(TiGetType(bp->b_outside));
	    extTransRec.tr_termpos[thisterm].pt = bp->b_outside->ti_ll;
	}
	else			/* update the region tile position */
	{
	    TermTilePos  *pos = &(extTransRec.tr_termpos[thisterm]);
	    Tile         *otile = bp->b_outside;

	    if( DBPlane(TiGetType(otile)) < pos->pnum )
	    {
		pos->pnum = DBPlane(TiGetType(otile));
		pos->pt = otile->ti_ll;
	    }
	    else if( DBPlane(TiGetType(otile)) == pos->pnum )
	    {
		if( LEFT(otile) < pos->pt.p_x )
		    pos->pt = otile->ti_ll;
		else if( LEFT(otile) == pos->pt.p_x && 
		  BOTTOM(otile) < pos->pt.p_y )
		    pos->pt.p_y = BOTTOM(otile);
	    }
	}

	/* Add the length to this terminal's perimeter */
	extTransRec.tr_termlen[thisterm] += len;

	/*
	 * Mark this attribute as belonging to this transistor
	 * if it is either:
	 *	(1) a terminal attribute whose LL corner touches bp->b_segment,
	 *   or	(2) a gate attribute that lies inside bp->b_inside.
	 */
	for (ll = extTransRec.tr_gatenode->nreg_labels; ll; ll = ll->ll_next)
	{
	    /* Skip if already marked */
	    if (ll->ll_attr != LL_NOATTR)
		continue;
	    lab = ll->ll_label;
	    if (GEO_ENCLOSE(&lab->lab_rect.r_ll, &bp->b_segment)
		    && extLabType(lab->lab_text, LABTYPE_TERMATTR))
	    {
		ll->ll_attr = thisterm;
	    }
	}
    }
    else if (extConnectsTo(tinside, toutside, ExtCurStyle->exts_nodeConn))
    {
	/* Not in a terminal, but are in something that connects to gate */
	extTransRec.tr_gatelen += len;
    }

    /*
     * Total perimeter (separate from terminals, for dcaps
     * that might not be surrounded by terminals on all sides).
     */
    extTransRec.tr_perim += len;

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTransOutTerminal --
 *
 * Output the information associated with one terminal of a
 * transistor.  This consists of three things:
 *	- the name of the node to which the terminal is connected
 *	- the length of the terminal along the perimeter of the transistor
 *	- a list of attributes pertinent to this terminal.
 *
 * If 'whichTerm' is LL_GATEATTR, this is the gate; otherwise, it is one
 * of the diffusion terminals.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes the above information to 'outFile'.
 *	Resets ll_attr for each attribute we output to LL_NOATTR.
 *
 * ----------------------------------------------------------------------------
 */

extTransOutTerminal(lreg, ll, whichTerm, len, outFile)
    LabRegion *lreg;		/* Node connected to terminal */
    register LabelList *ll;	/* Gate's label list */
    int whichTerm;		/* Which terminal we are processing.  The gate
				 * is indicated by LL_GATEATTR.
				 */
    int len;			/* Length of perimeter along terminal */
    FILE *outFile;		/* Output file */
{
    register char *cp;
    register int n;
    char fmt;

    (void) fprintf(outFile, " \"%s\" %d", extNodeName(lreg), len);
    for (fmt = ' '; ll; ll = ll->ll_next)
	if (ll->ll_attr == whichTerm)
	{
	    (void) fprintf(outFile, "%c\"", fmt);
	    cp = ll->ll_label->lab_text;
	    n = strlen(cp) - 1;
	    while (n-- > 0)
		putc(*cp++, outFile);
	    ll->ll_attr = LL_NOATTR;
	    (void) fprintf(outFile, "\"");
	    fmt = ',';
	}

    if (fmt == ' ')
	(void) fprintf(outFile, " 0");
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTransBad --
 *
 * For a transistor where an error was encountered, give feedback
 * as to the location of the error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Complains to the user.
 *
 * ----------------------------------------------------------------------------
 */

extTransBad(def, tp, mesg)
    CellDef *def;
    Tile *tp;
    char *mesg;
{
    Rect r;

    if (!DebugIsSet(extDebugID, extDebNoFeedback))
    {
	TiToRect(tp, &r);
	DBWFeedbackAdd(&r, mesg, def, 1, STYLE_PALEHIGHLIGHTS);
    }
    extNumWarnings++;
}

/*
 * ----------------------------------------------------------------------------
 *
 * extLabType --
 *
 * Check to see whether the text passed as an argument satisfies
 * any of the label types in 'typeMask'.
 *
 * Results:
 *	TRUE if the text is of one of the label types in 'typeMask',
 *	FALSE if not.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
extLabType(text, typeMask)
    register char *text;
    int typeMask;
{
    if (*text == '\0')
	return (FALSE);

    while (*text) text++;
    switch (*--text)
    {
	case '@':	/* Node attribute */
	    return (typeMask & LABTYPE_NODEATTR);
	case '$':	/* Terminal (source/drain) attribute */
	    return (typeMask & LABTYPE_TERMATTR);
	case '^':	/* Gate attribute */
	    return (typeMask & LABTYPE_GATEATTR);
	default:
	    return (typeMask & LABTYPE_NAME);
    }
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * extTransFirst --
 * extTransEach --
 *
 * Filter functions passed to ExtFindRegions when tracing out transistor
 * regions as part of flat circuit extraction.
 *
 * Results:
 *	extTransFirst returns a pointer to a new TransRegion.
 *	extTransEach returns NULL.
 *
 * Side effects:
 *	Memory is allocated by extTransFirst.
 *	We cons the newly allocated region onto the front of the existing
 *	region list.
 *
 *	The area of each transistor is updated by extTransEach.
 *
 * ----------------------------------------------------------------------------
 */

Region *
extTransFirst(tile, arg)
    Tile *tile;
    FindRegion *arg;
{
    register TransRegion *reg;

    MALLOC(TransRegion *, reg, sizeof (TransRegion));
    reg->treg_next = (TransRegion *) NULL;
    reg->treg_labels = (LabelList *) NULL;
    reg->treg_pnum = DBNumPlanes;
    reg->treg_area = 0;
    reg->treg_tile = tile;

    /* Prepend it to the region list */
    reg->treg_next = (TransRegion *) arg->fra_region;
    arg->fra_region = (Region *) reg;
    return ((Region *) reg);
}

    /*ARGSUSED*/
int
extTransEach(tile, pNum, arg)
    Tile *tile;
    int pNum;
    FindRegion *arg;
{
    register TransRegion *reg;

    reg = (TransRegion *) arg->fra_region;
    reg->treg_area += TILEAREA(tile);
    extSetNodeNum((LabRegion *) reg, pNum, tile);

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extFindNodes --
 *
 * Build up, in the manner of ExtFindRegions, a list of all the
 * node regions in the CellDef 'def'.  This procedure is heavily
 * optimized for speed.
 *
 * Results:
 *	Returns a pointer to a NULL-terminated list of NodeRegions
 *	that correspond to the nodes in the circuit.  The label lists
 *	for each node region have not yet been filled in.
 *
 * Side effects:
 *	Memory is allocated.
 *
 * ----------------------------------------------------------------------------
 */

Stack *extNodeStack = NULL;
Rect *extNodeClipArea = NULL;

NodeRegion *
extFindNodes(def, clipArea)
    CellDef *def;	/* Def whose nodes are being found */
    Rect *clipArea;	/* If non-NULL, ignore perimeter and area that extend
			 * outside this rectangle.
			 */
{
    int extNodeAreaFunc();
    FindRegion arg;
    int pNum, n;

    /* Reset perimeter and area prior to node extraction */
    for (n = 0; n < ExtCurStyle->exts_numResistClasses; n++)
	extResistArea[n] = extResistPerim[n] = 0;

    extNodeClipArea = clipArea;
    if (extNodeStack == (Stack *) NULL)
	extNodeStack = StackNew(64);

    arg.fra_def = def;
    arg.fra_region = (Region *) NULL;

    SigDisableInterrupts();
    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	(void) DBSrPaintClient((Tile *) NULL, def->cd_planes[pNum],
		    &TiPlaneRect, &DBAllButSpaceBits, extUnInit,
		    extNodeAreaFunc, (ClientData) &arg);
    SigEnableInterrupts();

    /* Compute resistance for last node */
    if (arg.fra_region && (ExtOptions & EXT_DORESISTANCE))
	extSetResist((NodeRegion *) arg.fra_region);

    return ((NodeRegion *) arg.fra_region);
}

extNodeAreaFunc(tile, arg)
    register Tile *tile;
    FindRegion *arg;
{
    int tilePlaneNum, pNum, pMask, len, cap, area, resistClass, n, nclasses;
    register TileTypeBitMask *mask, *resMask;
    register NodeRegion *reg;
    register Tile *tp;
    TileType type, t;
    NodeRegion *old;
    Rect r;

    /* Compute the resistance for the previous region */
    if (old = (NodeRegion *) arg->fra_region)
	if (ExtOptions & EXT_DORESISTANCE)
	    extSetResist(old);

    /* Allocate a new node */
    nclasses = ExtCurStyle->exts_numResistClasses;
    n = sizeof (NodeRegion) + (sizeof (PerimArea) * (nclasses - 1));
    MALLOC(NodeRegion *, reg, (unsigned) n);
    reg->nreg_labels = (LabelList *) NULL;
    reg->nreg_cap = 0;
    reg->nreg_resist = 0;
    reg->nreg_pnum = DBNumPlanes;
    reg->nreg_next = (NodeRegion *) NULL;
    for (n = 0; n < nclasses; n++)
	reg->nreg_pa[n].pa_perim = reg->nreg_pa[n].pa_area = 0;

    /* Prepend the new node to the region list */
    reg->nreg_next = (NodeRegion *) arg->fra_region;
    arg->fra_region = (Region *) reg;

    /* Mark this tile as pending and push it */
    PUSHTILE(tile);

    /* Continue processing tiles until there are none left */
    while (!StackEmpty(extNodeStack))
    {
	tile = (Tile *) STACKPOP(extNodeStack);

	/*
	 * Since tile was pushed on the stack, we know that it
	 * belongs to this region.  Check to see that it hasn't
	 * been visited in the meantime.  If it's still unvisited,
	 * visit it and process its neighbors.
	 */
	if (tile->ti_client == (ClientData) reg)
	    continue;
	tile->ti_client = (ClientData) reg;
	if (DebugIsSet(extDebugID, extDebNeighbor))
	    extShowTile(tile, "neighbor", 1);

	type = TiGetType(tile);
	mask = &ExtCurStyle->exts_nodeConn[type];
	resMask = &ExtCurStyle->exts_typesResistChanged[type];
	resistClass = ExtCurStyle->exts_typeToResistClass[type];
	tilePlaneNum = DBPlane(type);

	/*
	 * Make sure the lower-leftmost point in the node is
	 * kept up to date, so we can generate an internal
	 * node name that does not depend on any other nodes
	 * in this cell.
	 */
	extSetNodeNum((LabRegion *) reg, tilePlaneNum, tile);

	/*
	 * Keep track of the total area of this node, and the
	 * contribution to parasitic ground capacitance resulting
	 * from area.
	 */
	if (extNodeClipArea)
	{
	    TITORECT(tile, &r);
	    GEOCLIP(&r, extNodeClipArea);
	    area = (r.r_xtop - r.r_xbot) * (r.r_ytop - r.r_ybot);
	}
	else area = TILEAREA(tile);
	if (resistClass != -1)
	    extResistArea[resistClass] += area;
	reg->nreg_cap += area * ExtCurStyle->exts_areaCap[type];

	/*
	 * Walk along all four sides of tile.
	 * Sum perimeter capacitance as we go.
	 * Keep track of the contribution to the total perimeter
	 * of this node, for computing resistance.
	 */

	/* Top */
	for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp))
	{
	    if (extNodeClipArea)
	    {
		r.r_ybot = r.r_ytop = TOP(tile);
		r.r_xtop = MIN(RIGHT(tile), RIGHT(tp));
		r.r_xbot = MAX(LEFT(tile), LEFT(tp));
		GEOCLIP(&r, extNodeClipArea);
		len = EDGENULL(&r) ? 0 : r.r_xtop - r.r_xbot;
	    }
	    else len = MIN(RIGHT(tile), RIGHT(tp)) - MAX(LEFT(tile), LEFT(tp));
	    t = TiGetType(tp);
	    if (TTMaskHasType(mask, t) && tp->ti_client == extUnInit)
		PUSHTILE(tp);
	    if (cap = ExtCurStyle->exts_perimCap[type][t])
		reg->nreg_cap += cap * len;
	    if (TTMaskHasType(resMask, t) && resistClass != -1)
		extResistPerim[resistClass] += len;
	}

	/* Left */
	for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp = RT(tp))
	{
	    if (extNodeClipArea)
	    {
		r.r_xbot = r.r_xtop = LEFT(tile);
		r.r_ytop = MIN(TOP(tile), TOP(tp));
		r.r_ybot = MAX(BOTTOM(tile), BOTTOM(tp));
		GEOCLIP(&r, extNodeClipArea);
		len = EDGENULL(&r) ? 0 : r.r_ytop - r.r_ybot;
	    }
	    else len = MIN(TOP(tile), TOP(tp)) - MAX(BOTTOM(tile), BOTTOM(tp));
	    t = TiGetType(tp);
	    if (TTMaskHasType(mask, t) && tp->ti_client == extUnInit)
		PUSHTILE(tp);
	    if (cap = ExtCurStyle->exts_perimCap[type][t])
		reg->nreg_cap += cap * len;
	    if (TTMaskHasType(resMask, t) && resistClass != -1)
		extResistPerim[resistClass] += len;
	}

	/* Bottom */
	for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp = TR(tp))
	{
	    if (extNodeClipArea)
	    {
		r.r_ybot = r.r_ytop = BOTTOM(tile);
		r.r_xtop = MIN(RIGHT(tile), RIGHT(tp));
		r.r_xbot = MAX(LEFT(tile), LEFT(tp));
		GEOCLIP(&r, extNodeClipArea);
		len = EDGENULL(&r) ? 0 : r.r_xtop - r.r_xbot;
	    }
	    else len = MIN(RIGHT(tile), RIGHT(tp)) - MAX(LEFT(tile), LEFT(tp));
	    t = TiGetType(tp);
	    if (TTMaskHasType(mask, t) && tp->ti_client == extUnInit)
		PUSHTILE(tp);
	    if (cap = ExtCurStyle->exts_perimCap[type][t])
		reg->nreg_cap += cap * len;
	    if (TTMaskHasType(resMask, t) && resistClass != -1)
		extResistPerim[resistClass] += len;
	}

	/* Right */
	for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp = LB(tp))
	{
	    if (extNodeClipArea)
	    {
		r.r_xbot = r.r_xtop = RIGHT(tile);
		r.r_ytop = MIN(TOP(tile), TOP(tp));
		r.r_ybot = MAX(BOTTOM(tile), BOTTOM(tp));
		GEOCLIP(&r, extNodeClipArea);
		len = EDGENULL(&r) ? 0 : r.r_ytop - r.r_ybot;
	    }
	    else len = MIN(TOP(tile), TOP(tp)) - MAX(BOTTOM(tile), BOTTOM(tp));
	    t = TiGetType(tp);
	    if (TTMaskHasType(mask, t) && tp->ti_client == extUnInit)
		PUSHTILE(tp);
	    if (cap = ExtCurStyle->exts_perimCap[type][t])
		reg->nreg_cap += cap * len;
	    if (TTMaskHasType(resMask, t) && resistClass != -1)
		extResistPerim[resistClass] += len;
	}

	/* No capacitance */
	if ((ExtOptions & EXT_DOCAPACITANCE) == 0)
	    reg->nreg_cap = 0;

	/* If this is a contact, visit all the other planes */
	if (TTMaskHasType(&DBImageBits, type))
	{
	    pMask = DBConnPlanes[type];
	    for (pNum = tilePlaneNum - 1; pNum <= tilePlaneNum + 1; pNum += 2)
		if (PlaneMaskHasPlane(pMask, pNum))
		{
		    Plane *plane = arg->fra_def->cd_planes[pNum];

		    tp = plane->pl_hint;
		    GOTOPOINT(tp, &tile->ti_ll);
		    plane->pl_hint = tp;
		    if (tp->ti_client == extUnInit
			    && TTMaskHasType(mask, TiGetType(tp)))
			PUSHTILE(tp);
		}
	}

	/*
	 * The hairiest case is when this type connects to stuff on
	 * other planes, but isn't itself connected as a contact.
	 * For example, a CMOS pwell connects to diffusion of the
	 * same doping (p substrate diff).  In a case like this,
	 * we need to search the entire AREA of the tile plus a
	 * 1-lambda halo to find everything it overlaps or touches
	 * on the other plane.
	 */
	if (pMask = DBAllConnPlanes[type])
	{
	    Rect tileArea, biggerArea;

	    extNbrUn = extUnInit;
	    TITORECT(tile, &tileArea);
	    GEO_EXPAND(&tileArea, 1, &biggerArea);
	    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
		if (pNum != tilePlaneNum && PlaneMaskHasPlane(pMask, pNum))
		    (void) DBSrPaintArea((Tile *) NULL,
			    arg->fra_def->cd_planes[pNum], &biggerArea,
			    mask, extNbrPushFunc, (ClientData) &tileArea);
	}
    }

    return (0);
}
