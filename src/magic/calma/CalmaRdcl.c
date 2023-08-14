/*
 * CalmaReadcell.c --
 *
 * Input of Calma GDS-II stream format.
 * Processing for cells.
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
static char rcsid[] = "$Header: CalmaRdcl.c,v 6.0 90/08/28 18:03:39 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "utils.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "malloc.h"
#include "tech.h"
#include "cif.h"
#include "CIFint.h"
#include "CIFread.h"
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "textio.h"
#include "calmaInt.h"
#include <sys/types.h>

int calmaNonManhattan;

extern void CIFPaintCurrent();

extern HashTable calmaDefInitHash;

/*
 * ----------------------------------------------------------------------------
 *
 * calmaParseStructure --
 *
 * Process a complete GDS-II structure (cell) including its closing
 * CALMA_ENDSTR record.  In the event of a syntax error, we skip
 * ahead to the closing CALMA_ENDSTR, output a warning, and keep going.
 *
 * Results:
 *	TRUE if successful, FALSE if the next item in the input is
 *	not a structure.
 *
 * Side effects:
 *	Reads a new cell.
 *	Consumes input.
 *
 * ----------------------------------------------------------------------------
 */

bool
calmaParseStructure()
{
    static int structs[] = { CALMA_STRCLASS, CALMA_STRTYPE, -1 };
    int nbytes, rtype, nsrefs, osrefs;
    char strname[CALMANAMELENGTH + 2], newname[CALMANAMELENGTH*2];
    HashEntry *he;
    int suffix;

    /* Make sure this is a structure; if not, let the caller know we're done */
    PEEKRH(nbytes, rtype);
    if (nbytes <= 0 || rtype != CALMA_BGNSTR)
	return (FALSE);

    /* Read the structure name */
    if (!calmaSkipExact(CALMA_BGNSTR)) goto syntaxerror;;
    if (!calmaReadStringRecord(CALMA_STRNAME, strname)) goto syntaxerror;
    TxPrintf("Reading \"%s\".\n", strname);

    /* Set up the cell definition */
    he = HashFind(&calmaDefInitHash, strname);
    if (HashGetValue(he))
    {
	for (suffix = 1; HashGetValue(he) != NULL; suffix++)
	{
	    (void) sprintf(newname, "%s_%d", strname, suffix);
	    he = HashFind(&calmaDefInitHash, newname);
	}
	TxError("Cell \"%s\" was already defined in this file.\n", strname);
	TxError("Giving this cell a new name: %s\n", newname);
	(void) strcpy(strname, newname);
    }
    cifReadCellDef = calmaFindCell(strname);
    DBCellClearDef(cifReadCellDef);
    DBCellSetAvail(cifReadCellDef);
    HashSetValue(he, cifReadCellDef);
    cifCurReadPlanes = cifSubcellPlanes;

    /* Skip CALMA_STRCLASS or CALMA_STRTYPE */
    calmaSkipSet(structs);

    /* Initialize the hash table for layer errors */
    HashInit(&calmaLayerHash, 32, sizeof (CalmaLayerType) / sizeof (unsigned));

    /* Body of structure: a sequence of elements */
    osrefs = nsrefs = 0;
    calmaNonManhattan = 0;
    while (calmaParseElement(&nsrefs))
    {
	if (SigInterruptPending)
	    goto done;
	if (nsrefs > osrefs && (nsrefs % 100) == 0)
	    TxPrintf("    %d uses\n", nsrefs);
	osrefs = nsrefs;
    }
    if (calmaNonManhattan)
	TxPrintf("    %d non-Manhattan paths converted to stair-steps.\n",
		calmaNonManhattan);

    /* Make sure it ends with an ENDSTR record */
    if (!calmaSkipExact(CALMA_ENDSTR)) goto syntaxerror;

    /*
     * Do the geometrical processing and paint this material back into
     * the appropriate cell of the database.
     */
    CIFPaintCurrent();
    DBAdjustLabels(cifReadCellDef, &TiPlaneRect);
    DBReComputeBbox(cifReadCellDef);
    DRCCheckThis(cifReadCellDef, TT_CHECKPAINT, &cifReadCellDef->cd_bbox);
    DBWAreaChanged(cifReadCellDef, &cifReadCellDef->cd_bbox,
	DBW_ALLWINDOWS, &DBAllButSpaceBits);
    DBCellSetModified(cifReadCellDef, TRUE);

    /*
     * Assign use-identifiers to all the cell uses.
     * These identifiers are generated so as to be
     * unique.
     */
    DBGenerateUniqueIds(cifReadCellDef, FALSE);

done:
    HashKill(&calmaLayerHash);
    return (TRUE);

    /* Syntax error: skip to CALMA_ENDSTR */
syntaxerror:
    HashKill(&calmaLayerHash);
    return (calmaSkipTo(CALMA_ENDSTR));
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaParseElement --
 *
 * Process one element from a GDS-II structure, including its
 * trailing CALMA_ENDEL record.  In the event of a syntax error, we skip
 * ahead to the closing CALMA_ENDEL, output a warning, and keep going.
 *
 * Results:
 *	TRUE if we processed an element, FALSE when we reach something that
 *	is not an element.  In the latter case, we leave the non-element
 *	record unconsumed.
 *
 * Side effects:
 *	Consumes input.
 *	Depends on the kind of element encountered.
 *	If we process a SREF or AREF, increment *pnsrefs.
 *
 * ----------------------------------------------------------------------------
 */

bool
calmaParseElement(pnsrefs)
    int *pnsrefs;
{
    static int node[] = { CALMA_ELFLAGS, CALMA_PLEX, CALMA_LAYER,
			  CALMA_NODETYPE, CALMA_XY, -1 };
    int nbytes, rtype;

    READRH(nbytes, rtype);
    if (nbytes < 0)
    {
	calmaReadError("Unexpected EOF.\n");
	return (FALSE);
    }

    switch (rtype)
    {
	case CALMA_AREF:
	case CALMA_SREF:
	    calmaElementSref();
	    (*pnsrefs)++;
	    break;
	case CALMA_BOUNDARY:	
	    calmaElementBoundary();
	    break;
	case CALMA_BOX:
	    calmaElementBox();
	    break;
	case CALMA_PATH:	
	    calmaElementPath();
	    break;
	case CALMA_TEXT:	
	    calmaElementText();
	    break;
	case CALMA_NODE:
	    calmaReadError("NODE elements not supported: skipping.\n");
	    calmaSkipSet(node);
	    break;
	default:
	    UNREADRH(nbytes, rtype);
	    return (FALSE);
    }

    return (calmaSkipTo(CALMA_ENDEL));
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaElementSref --
 *
 * Process a structure reference (either CALMA_SREF or CALMA_AREF).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Consumes input.
 *	Adds a new cell use to the current def.
 *
 * ----------------------------------------------------------------------------
 */

bool
calmaElementSref()
{
    int nbytes, rtype, cols, rows, nref, n;
    int xlo, ylo, xhi, yhi, xsep, ysep;
    char sname[CALMANAMELENGTH + 2];
    bool isArray = FALSE;
    Transform trans, tinv;
    Point refarray[3], p;
    CellUse *use;
    CellDef *def;

    /* Skip CALMA_ELFLAGS, CALMA_PLEX */
    calmaSkipSet(calmaElementIgnore);

    /* Read subcell name */
    if (!calmaReadStringRecord(CALMA_SNAME, sname)) return;

    /* Read subcell transform */
    if (!calmaReadTransform(&trans, sname))
    {
	printf("Couldn't read transform.\n");
	return;
    }

    /* Get number of columns and rows if array */
    READRH(nbytes, rtype);
    if (nbytes < 0) return;
    if (rtype == CALMA_COLROW)
    {
	isArray = TRUE;
	READI2(cols);
	READI2(rows);
	xlo = 0; xhi = cols - 1;
	ylo = 0; yhi = rows - 1;
	if (feof(calmaInputFile)) return;
	(void) calmaSkipBytes(nbytes - CALMAHEADERLENGTH - 4);
    }
    else
    {
	UNREADRH(nbytes, rtype);
    }

    /*
     * Read reference points.
     * For subcells, there will be a single reference point.
     * For arrays, there will be three; for their meanings, see below.
     */
    READRH(nbytes, rtype)
    if (nbytes < 0) return;
    if (rtype != CALMA_XY)
    {
	calmaUnexpected(CALMA_XY, rtype);
	return;
    }

    /* Length of remainder of record */
    nbytes -= CALMAHEADERLENGTH;

    /*
     * Read the reference points for the SREF/AREF.
     * Scale down by cifCurReadStyle->crs_scaleFactor, but complain
     * if they don't scale exactly.
     * Make sure we only read three points.
     */
    nref = nbytes / 8;
    if (nref > 3)
    {
	calmaReadError("Too many points (%d) in SREF/AREF\n", nref);
	nref = 3;
    }
    else if (nref < 1)
    {
	calmaReadError("Missing reference points in SREF/AREF (using 0,0)\n");
	refarray[0].p_x = refarray[0].p_y = 0;
	refarray[1].p_x = refarray[1].p_y = 0;
	refarray[2].p_x = refarray[2].p_y = 0;
    }

    for (n = 0; n < nref; n++)
    {
	READPOINT(&refarray[n]);
	calmaScalePoint(&refarray[n], cifCurReadStyle->crs_scaleFactor);
	if (feof(calmaInputFile))
	    return;
    }

    /* Skip remainder */
    nbytes -= nref*8;
    if (nbytes)
	(void) calmaSkipBytes(nbytes);

    /*
     * Figure out the inter-element spacing of array elements,
     * and also the translation part of the transform.
     * The first reference point for both SREFs and AREFs is the
     * translation of the use's or array's lower-left.
     */
    trans.t_c = refarray[0].p_x;
    trans.t_f = refarray[0].p_y;
    GeoInvertTrans(&trans, &tinv);
    if (isArray)
    {
	/*
	 * The remaining two points for an array are displaced from
	 * the first reference point by:
	 *    - the inter-column spacing times the number of columns,
	 *    - the inter-row spacing times the number of rows.
	 */
	xsep = ysep = 0;
	if (cols)
	{
	    GeoTransPoint(&tinv, &refarray[1], &p);
	    if (p.p_x % cols)
	    {
		n = (p.p_x + (cols+1)/2) / cols;
		calmaReadError("# cols doesn't divide displacement ref pt\n");
		TxError("    %d / %d -> %d\n", p.p_x, cols, n);
		xsep = n;
	    }
	    else xsep = p.p_x / cols;
	}
	if (rows)
	{
	    GeoTransPoint(&tinv, &refarray[2], &p);
	    if (p.p_y % rows)
	    {
		n = (p.p_y + (rows+1)/2) / rows;
		calmaReadError("# rows doesn't divide displacement ref pt\n");
		TxError("    %d / %d -> %d\n", p.p_y, rows, n);
		ysep = n;
	    }
	    ysep = p.p_y / rows;
	}
    }

    /*
     * Create a new cell use with this transform.  If the
     * cell being referenced doesn't exist, create it.
     * Don't give it a use-id; we will do that only after
     * we've placed all cells.
     */
    def = calmaFindCell(sname);
    if (DBIsAncestor(def, cifReadCellDef))
    {
	TxError("Cell %s is an ancestor of %s",
			def->cd_name, cifReadCellDef->cd_name);
	TxError(" and can't be used as a subcell.\n");
	TxError("(Use skipped)\n");
	return;
    }
    use = DBCellNewUse(calmaFindCell(sname), (char *) NULL);
    if (isArray)
	DBMakeArray(use, &GeoIdentityTransform, xlo, ylo, xhi, yhi, xsep, ysep);
    DBSetTrans(use, &trans);
    DBPlaceCell(use, cifReadCellDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaScalePoint --
 *
 * Scale both of the coordinates of a reference points for an SREF/AREF
 * down by a factor of 'scale'.  If 'scale' doesn't evenly divide both
 * coordinates of the point, complain and round each coordinate to the
 * nearest scaled value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies *p as described above.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaScalePoint(p, scale)
    register Point *p;
    register int scale;
{
    if (p->p_x % scale || p->p_y % scale)
    {
	register int round;
	Point p2;

	calmaReadError("SREF/AREF ref point doesn't scale exactly\n");
	round = (scale + 1) >> 1;
	p2.p_x = (p->p_x + round) / scale;
	p2.p_y = (p->p_y + round) / scale;
	TxError("    (%d,%d) / %d = (%d,%d)\n",
		p->p_x, p->p_y, scale, p2.p_x, p2.p_y);
	*p = p2;
    }
    else
    {
	p->p_x /= scale;
	p->p_y /= scale;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaFindCell --
 *
 * This local procedure is used to find a cell in the subcell table,
 * and create a new subcell if there isn't already one there.
 * If a new subcell is created, its CDAVAILABLE is left FALSE.
 *
 * Results:
 *	The return value is a pointer to the definition for the
 *	cell whose name is 'name'.
 *
 * Side effects:
 *	A new CellDef may be created.
 *
 * ----------------------------------------------------------------------------
 */

CellDef *
calmaFindCell(name)
    char *name;		/* Name of desired cell */
{
    HashEntry *h;
    CellDef *def;

    h = HashFind(&CifCellTable, name);
    if (HashGetValue(h) == 0)
    {
	def = DBCellLookDef(name);
	if (def == NULL)
	{
	    def = DBCellNewDef(name, (char *) NULL);

	    /*
	     * Tricky point:  call DBReComputeBbox here to make SURE
	     * that the cell has a valid bounding box.  Otherwise,
	     * if the cell is used in a parent before being defined
	     * then it will cause a core dump.
	     */
	     DBReComputeBbox(def);
	}
	HashSetValue(h, def);
    }
    return (CellDef *) HashGetValue(h);
}
