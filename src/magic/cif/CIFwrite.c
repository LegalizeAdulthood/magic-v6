/*
 * CIFwrite.c --
 *
 * Output of CIF.
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
static char rcsid[] = "$Header: CIFwrite.c,v 6.0 90/08/28 18:05:26 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "utils.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "tech.h"
#include "stack.h"
#include "cif.h"
#include "CIFint.h"
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "textio.h"

    /* Forward declarations */
extern int cifWriteInitFunc();
extern int cifWriteMarkFunc();
extern int cifWritePaintFunc();
extern int cifWriteUseFunc();

/* Current cell number in CIF numbering scheme */

static int cifCellNum;

/* Stack of definitions left to process for generating CIF */

Stack *cifStack;

/* Factor by which to scale Magic coordinates for cells and labels. */

int cifWriteScale;

/* Scale factor for outputting paint: */

int cifPaintScale;

/* Current layer name.  If non-NULL, then cifWritePaintFunc outputs
 * this string to the file before outputting a rectangle, then nulls.
 * This way, the layer name only gets output when there's actually
 * stuff on that layer.
 */

char *cifPaintLayerName;

/* TRUE if area labels should be output */
bool CIFDoAreaLabels = FALSE;

/*
 * ----------------------------------------------------------------------------
 *
 * CIFWrite --
 *
 * Write out the entire tree rooted at the supplied CellDef in CIF format,
 * to the specified file.
 *
 * Results:
 *	TRUE if the cell could be written successfully, FALSE otherwise.
 *
 * Side effects:
 *	Writes a file to disk.
 *	In the event of an error while writing out the cell,
 *	the external integer errno is set to the UNIX error
 *	encountered.
 *
 * Algorithm:
 *	We make a depth-first traversal of the entire design tree,
 *	marking each cell with a CIF symbol number and then outputting
 *	it to the CIF file.  If a given cell is not read in when we
 *	visit it, we read it in.
 *
 *	No hierarchical design rule checking or bounding box computation
 *	occur during this traversal -- both are explicitly avoided.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFWrite(rootDef, f)
    CellDef *rootDef;	/* Pointer to CellDef to be written */
    FILE *f;		/* Open output file */
{
    bool good;
    int oldCount = DBWFeedbackCount;
    CellUse dummy;

    /*
     * Make sure that the entire hierarchy rooted at rootDef is
     * read into memory and that timestamp mismatches are resolved
     * (this is needed so that we know that bounding boxes are OK).
     */

    dummy.cu_def = rootDef;
    DBCellReadArea(&dummy, &rootDef->cd_bbox);
    DBFixMismatch();

    if (CIFCurStyle->cs_reducer == 0)
    {
	TxError("The current CIF output style can only be used for writing\n");
	TxError("Calma output.  Try picking another output style.\n");
	return (TRUE);
    }

    /*
     * Go through all cells currently having CellDefs in the
     * def symbol table and mark them with negative symbol numbers.
     */
    (void) DBCellSrDefs(0, cifWriteInitFunc, (ClientData) NULL);
    cifCellNum = (-2);
    rootDef->cd_client = (ClientData) -1;

    /*
     * Start by pushing the root def on the stack of cell defs
     * to be processed.
     */
    cifStack = StackNew(100);
    StackPush((ClientData) rootDef, cifStack);
    cifOut(f);
    StackFree(cifStack);
    if ((int) rootDef->cd_client < 0)
	rootDef->cd_client = (ClientData) (- (int) rootDef->cd_client);

    /* See if any problems occurred. */

    if (DBWFeedbackCount != oldCount)
    {
	TxPrintf("%d problems occurred.  See feedback entries.\n",
	    DBWFeedbackCount - oldCount);
    }

    /*
     * Now we are almost done.
     * Just output a call on the root cell
     */
    (void) fprintf(f, "C %d;\nEnd\n", (int) rootDef->cd_client);
    good = !ferror(f);
    (void) fclose(f);
    return (good);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifWriteInitFunc --
 *
 * Filter function called on behalf of CIFWrite() above.
 * Responsible for setting the cif number of each cell to zero.
 *
 * Results:
 *	Returns 0 to indicate that the search should continue.
 *
 * Side effects:
 *	Modify the cif numbers of the cells they are passed.
 *
 * ----------------------------------------------------------------------------
 */

int
cifWriteInitFunc(def)
    CellDef *def;
{
    def->cd_client = (ClientData) 0;
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifWriteMarkFunc --
 *
 * Called to add each cell def in the subcell plane of a parent
 * to the stack of cells to be processed.
 *
 * Results:
 *	Returns 0 to indicate that DBCellEnum() should continue.
 *
 * Side effects:
 *	Pushes the cell def on the stack.
 * ----------------------------------------------------------------------------
 */

int
cifWriteMarkFunc(use)
    CellUse *use;
{
    if (use->cu_def->cd_client != (ClientData) 0) return 0;
    use->cu_def->cd_client = (ClientData) cifCellNum;
    cifCellNum -= 1;
    StackPush((ClientData) use->cu_def, cifStack);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifOut --
 *
 * Main loop of CIF generation.  Pull a cell def from the stack
 * and process it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes CIF to be output.
 *	Returns when the stack is empty.
 * ----------------------------------------------------------------------------
 */

cifOut(outf)
    FILE *outf;
{
    CellDef *def;

    while (!StackEmpty(cifStack))
    {
	def = (CellDef *) StackPop(cifStack);
	if ((int) def->cd_client >= 0) continue;	/* Already output */
	if (SigInterruptPending) continue;

	def->cd_client = (ClientData) (- (int) def->cd_client);

	/* Read the cell in if it is not already available. */
	if ((def->cd_flags & CDAVAILABLE) == 0)
	{
	    if (!DBCellRead(def, (char *) NULL, TRUE)) continue;
	}

	/* Add any subcells to the stack.  This must be done before
	 * outputting CIF to make sure that the subcells all have
	 * CIF numbers.
	 */

	(void) DBCellEnum(def, cifWriteMarkFunc, (ClientData) 0);

	/* Output CIF for this cell */

	cifOutFunc(def, outf);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifOutFunc --
 *
 * Write out the definition for a single cell as CIF.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Appends to the open CIF output file.
 *
 * ----------------------------------------------------------------------------
 */

cifOutFunc(def, f)
    CellDef *def;	/* Pointer to cell def to be written */
    FILE *f;		/* Open output file */
{
    Rect bigArea;
    register Label *lab;
    int type;
    CIFLayer *layer;

    (void) fprintf(f, "DS %d %d 2;\n", (int) def->cd_client,
	CIFCurStyle->cs_reducer);
    cifWriteScale = CIFCurStyle->cs_scaleFactor/CIFCurStyle->cs_reducer;
    if (def->cd_name != (char *) NULL)
	if (def->cd_name[0] != '\0')
	{
	    if (strcmp(def->cd_name, "(UNNAMED)") == 0)
		(void) fprintf(f, "9 UNNAMED;\n");
	    else
		(void) fprintf(f, "9 %s;\n", def->cd_name);
	}

    /*
     * Output all the tiles associated with this cell.  Skip temporary
     * layers.
     */
    
    GEO_EXPAND(&def->cd_bbox, CIFCurStyle->cs_radius, &bigArea);
    CIFErrorDef = def;
    CIFGen(def, &bigArea, CIFPlanes, &DBAllTypeBits, TRUE, TRUE);
    CIFGenSubcells(def, &bigArea, CIFPlanes);
    CIFGenArrays(def, &bigArea, CIFPlanes);
    for (type = 0; type < CIFCurStyle->cs_nLayers; type++)
    {
	layer = CIFCurStyle->cs_layers[type];
	if (layer->cl_flags & CIF_TEMP) continue;
	cifPaintLayerName = layer->cl_name;
	cifPaintScale = 1;
	(void) DBSrPaintArea((Tile *) NULL, CIFPlanes[type],
	    &TiPlaneRect, &CIFSolidBits, cifWritePaintFunc,
	    (ClientData) f);
    }

    /* Output labels */
    for (lab = def->cd_labels; lab; lab = lab->lab_next)
    {
	int type = CIFCurStyle->cs_labelLayer[lab->lab_type];
	Point center, size;

	center.p_x = lab->lab_rect.r_xbot + lab->lab_rect.r_xtop;
	center.p_y = lab->lab_rect.r_ybot + lab->lab_rect.r_ytop;
	center.p_x *= cifWriteScale;
	center.p_y *= cifWriteScale;

	if (CIFDoAreaLabels)
	{
	    size.p_x = lab->lab_rect.r_xtop - lab->lab_rect.r_xbot;
	    size.p_y = lab->lab_rect.r_ytop - lab->lab_rect.r_ybot;
	    size.p_x *= cifWriteScale * 2;
	    size.p_y *= cifWriteScale * 2;
	    if (type >= 0)
	    {
		(void) fprintf(f, "95 %s %d %d %d %d %s;\n",
		    lab->lab_text, size.p_x, size.p_y, center.p_x, center.p_y,
		    CIFCurStyle->cs_layers[type]->cl_name);
	    }
	    else
	    {
		(void) fprintf(f, "95 %s %d %d %d %d;\n",
		    lab->lab_text, size.p_x, size.p_y, center.p_x, center.p_y);
	    }
	}
	else
	{
	    if (type >= 0)
	    {
		(void) fprintf(f, "94 %s %d %d %s;\n",
		    lab->lab_text, center.p_x, center.p_y,
		    CIFCurStyle->cs_layers[type]->cl_name);
	    }
	    else
	    {
		(void) fprintf(f, "94 %s %d %d;\n",
		    lab->lab_text, center.p_x, center.p_y);
	    }
	}
    }

    /*
     * Output the calls that the child makes to its children.  For
     * arrays it is necessary to output one call for each instance.
     */
    (void) DBCellEnum(def, cifWriteUseFunc, (ClientData) f);
    (void) fprintf(f, "DF;\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifWriteUseFunc --
 *
 * Filter function, called by DBCellEnum on behalf of cifOutFunc above,
 * to write out each CellUse called by the CellDef being output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Appends to the open CIF output file.
 *
 * ----------------------------------------------------------------------------
 */

int
cifWriteUseFunc(use, f)
    CellUse *use;
    FILE *f;
{
    int x, y, topx, topy;
    register Transform *t;
    int cifnum;

    cifnum = (int) use->cu_def->cd_client;
    if (cifnum < 0) cifnum = (-cifnum);
    topx = use->cu_xhi - use->cu_xlo;
    if (topx < 0) topx = -topx;
    topy = use->cu_yhi - use->cu_ylo;
    if (topy < 0) topy = -topy;
    for (x = 0; x <= topx; x++)
    {
	for (y = 0; y <= topy; y++)
	{
	    /*
	     * We eventually want to tag each use with its unique
	     * use identifier, which should include array subscripting
	     * information.
	     */
	    (void) fprintf(f, "C %d", cifnum);

	    /*
	     * The following translates from the abcdef transforms
	     * that we use internally to the rotate and mirror
	     * specs used in CIF.  It only works because
	     * orientations are orthogonal in magic.  Check all
	     * 8 possible positions if you don't believe this.
	     */

	    t = &use->cu_transform;
	    if ((t->t_a != t->t_e) || ((t->t_a == 0) && (t->t_b == t->t_d)))
		(void) fprintf(f, " MX R %d %d", -(t->t_a), -(t->t_d));
	    else
		(void) fprintf(f, " R %d %d", t->t_a, t->t_d);

	    (void) fprintf(f, " T %d %d;\n",
		(t->t_c + t->t_a*(use->cu_xsep)*x + t->t_b*(use->cu_ysep)*y)
		    *2*cifWriteScale,
		(t->t_f + t->t_d*(use->cu_xsep)*x + t->t_e*(use->cu_ysep)*y)
		    *2*cifWriteScale);
	}
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifWritePaintFunc --
 *
 * Filter function used to write out a single paint tile.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the disk file.
 *
 * ----------------------------------------------------------------------------
 */

int
cifWritePaintFunc(tile, f)
    register Tile *tile;		/* Tile to be written out. */
    FILE *f;				/* File in which to write. */
{
    Rect r;

    /* Output the layer name if it hasn't been done already. */

    if (cifPaintLayerName != NULL)
    {
	(void) fprintf(f, "L %s;\n", cifPaintLayerName);
	cifPaintLayerName = NULL;
    }

    TiToRect(tile, &r);

    /* The only tricky thing here is that we MUST scale the rectangle
     * up by a factor of two to avoid round-off errors in computing
     * its center point (what a bogosity in CIF!!).  This is compensated
     * by shrinking by a factor of two in the "DS" statement.
     */

    (void) fprintf(f, "    B %d %d %d %d;\n",
	(2*cifPaintScale*(r.r_xtop - r.r_xbot))/CIFCurStyle->cs_reducer,
	(2*cifPaintScale*(r.r_ytop - r.r_ybot))/CIFCurStyle->cs_reducer,
	(cifPaintScale*(r.r_xtop + r.r_xbot))/CIFCurStyle->cs_reducer,
	(cifPaintScale*(r.r_ytop + r.r_ybot))/CIFCurStyle->cs_reducer);
    CIFRects += 1;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFWriteFlat --
 *
 * Write out the entire tree rooted at the supplied CellDef in CIF format,
 * to the specified file, but write non-hierarchical CIF.  
 *   
 * Results:
 *	TRUE if the cell could be written successfully, FALSE otherwise.
 *
 * Side effects:
 *	Writes a file to disk.
 *	In the event of an error while writing out the cell,
 *	the external integer errno is set to the UNIX error
 *	encountered.
 *
 * Algorithm:
 * 	We operate on the cell in chunks chosen to keep the memory utilization
 * reasonable.  Foreach chunk, we use DBTreeSrTiles and cifHierCopyFunc to 
 * flatten the 
 * chunk into a yank buffer ("eliminating" the subcell problem), then use 
 * cifOut to generate the CIF.
 *	No hierarchical design rule checking or bounding box computation
 *	occur during this operation -- both are explicitly avoided.
 *
 * ----------------------------------------------------------------------------
 */


CIFWriteFlat(rootDef, f)
    CellDef *rootDef;	/* Pointer to CellDef to be written */
    FILE *f;		/* Open output file */
{
    bool good;
    int oldCount = DBWFeedbackCount;
    TileTypeBitMask cifMask;
    SearchContext scx;

    cifStack = StackNew(1);
    CIFInitCells();
    UndoDisable();
    CIFDummyUse->cu_def = rootDef;
    /*
     * Calculate the size of the chunks we are going to use to flatten this 
     * puppy.
     */
/* %% pokefpowefw */
    /*
     * Now process each chunk.  We cheat and use cifOut(), so we need to have 
     * a stack for the single flattened "component" to be on.
     */
/* %%    for(;;) */
    { 
	scx.scx_use = CIFDummyUse;
	scx.scx_trans = GeoIdentityTransform;
	GEO_EXPAND(&rootDef->cd_bbox, CIFCurStyle->cs_radius, &scx.scx_area);
	(void) DBTreeSrTiles(&scx, &DBAllButSpaceAndDRCBits, 0,
			     cifHierCopyFunc, (ClientData) CIFComponentDef);
	DBReComputeBbox(CIFComponentDef);
	cifCellNum = (-2);
	CIFComponentDef->cd_client = (ClientData) -1;
	StackPush((ClientData) CIFComponentDef, cifStack);
	cifOut(f);
	/* cifStack SHould be empty now */
	if(!StackEmpty(cifStack))
	{
	    TxPrintf("Stack error in CIFWriteInverted()!!  Your cif is probably corrupted.\n");
	    StackFree(cifStack);
	    return FALSE;
	}
	DBCellClearDef(CIFComponentDef);
    }
    StackFree(cifStack);
    /*
     * Now we are almost done.
     * Just output a call on the root cell
     */
    (void) fprintf(f, "C %d;\nEnd\n", (int) CIFComponentDef->cd_client);
    DBCellClearDef(CIFComponentDef);
    good = !ferror(f);
    (void) fclose(f); 
    
    /* Report any errors that occurred. */

    if (DBWFeedbackCount != oldCount)
    {
	TxPrintf("%d problems occurred.  See feedback entries.\n",
		 DBWFeedbackCount-oldCount);
    }
 
    return good;
}
