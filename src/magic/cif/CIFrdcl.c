/* CIFreadcell.c -
 *
 *	This file contains more routines to parse CIF files.  In
 *	particular, it contains the routines to handle cells,
 *	both definitions and calls, and user-defined features
 *	like labels.
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
static char rcsid[] = "$Header: CIFrdcl.c,v 6.1 90/09/03 14:33:27 stark Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "CIFint.h"
#include "CIFread.h"
#include "utils.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "drc.h"

/* The following variable is made available to the outside world,
 * and is the cell definition currently being modified.
 */

CellDef *cifReadCellDef;

/*
 * The following hash table is used internally to keep track of
 * of all the cells we've seen definitions for or calls on.
 * The hash table entries contain pointers to cellDefs, and
 * are indexed by CIF cell number.  If the CDAVAILABLE bit is
 * set it means we've read the cell's contents.  If not set, it
 * means that the cell has been called but not yet defined.
 */
HashTable CifCellTable;

/* The following variable is used to save and restore current
 * paint layer information so that we can resume the correct
 * layer after a subcell definition.
 */

Plane *cifOldReadPlane = NULL;

/* The following boolean is TRUE if a subcell definition is being
 * read.  FALSE means we're working on the EditCell.
 */

bool cifSubcellBeingRead;

/* The following two collections of planes are used to hold CIF
 * information while cells are being read in (one set for the
 * outermost, unnamed cell, and one for the current subcell).
 * When a cell is complete, then geometrical operations are
 * performed on the layers and stuff is painted into Magic.
 */

Plane *cifEditCellPlanes[MAXCIFRLAYERS];
Plane *cifSubcellPlanes[MAXCIFRLAYERS];
Plane **cifCurReadPlanes = cifEditCellPlanes;	/* Set of planes currently
						 * in force.
						 */
TileType cifCurLabelType = TT_SPACE;	/* Magic layer on which to put '94'
					 * labels that aren't identified by
					 * type.
					 */

/* The following variable is used to hold a subcell id between
 * the 91 statement and the (immediately-following?) call statement.
 * The string this points to is dynamically allocated, so it must
 * also be freed explicitly.
 */

char *cifSubcellId = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadCellInit --
 *
 * 	This procedure initializes the data structures in this
 *	module just prior to reading a CIF file.
 *
 *	If ptrkeys is 0, the keys used in this hash table will
 *	be strings; if it is 1, the keys will be CIF numbers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cell hash table is initialized, and things are set up
 *	to put information in the EditCell first.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFReadCellInit(ptrkeys)
    int ptrkeys;
{
    int i;

    HashInit(&CifCellTable, 32, ptrkeys);
    cifReadCellDef = EditCellUse->cu_def;
    cifSubcellBeingRead = FALSE;
    cifCurReadPlanes = cifEditCellPlanes;
    for (i = 0; i < MAXCIFRLAYERS; i += 1)
    {
	if (cifEditCellPlanes[i] == NULL)
	    cifEditCellPlanes[i] = DBNewPlane((ClientData) TT_SPACE);
	if (cifSubcellPlanes[i] == NULL)
	    cifSubcellPlanes[i] = DBNewPlane((ClientData) TT_SPACE);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifFindCell --
 *
 * 	This local procedure is used to find a cell in the subcell
 *	table, and create a new subcell if there isn't already
 *	one there.  If a new subcell is created, its CDAVAILABLE
 *	is left FALSE.
 *
 * Results:
 *	The return value is a pointer to the definition for the
 *	cell whose CIF number is cifNum.
 *
 * Side effects:
 *	A new CellDef may be created.
 *
 * ----------------------------------------------------------------------------
 */

CellDef *
cifFindCell(cifNum)
    int cifNum;			/* The CIF number of the desired cell. */
{
    HashEntry *h;
    CellDef *def;

    h = HashFind(&CifCellTable, (char *) cifNum);
    if (HashGetValue(h) == 0)
    {
	char name[15];
	(void) sprintf(name, "%d", cifNum);
	def = DBCellLookDef(name);
	if (def == NULL)
	{
	    def = DBCellNewDef(name, (char *) NULL);

	    /* Tricky point:  call DBReComputeBbox here to make SURE
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

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseStart --
 *
 * 	Parse the beginning of a symbol (cell) definition.
 *	ds ::= D { blank } S integer [ sep integer sep integer ]
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Set up information for the new cell, including the CIF
 *	planes and creating a Magic cell (if one doesn't exist
 *	already).
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseStart()
{
    int		number;
    
    if (cifSubcellBeingRead)
    {
	CIFReadError("definition start inside other definition; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    if (cifSubcellId != NULL)
    {
	CIFReadError("pending call identifier %s discarded.\n", cifSubcellId);
	(void) StrDup(&cifSubcellId, (char *) NULL);
    }

    /* Take the `S'. */

    TAKE();
    if (!CIFParseInteger(&number))
    {
	CIFReadError("definition start, but no symbol number; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }

    if (!CIFParseInteger(&cifReadScale1))
    {
	cifReadScale1 = 1;
	cifReadScale2 = 1;
    }
    else
    {
	if (!CIFParseInteger(&cifReadScale2))
	{
	    CIFReadError(
	        "only one of two scale factors given; ignored.\n");
	    cifReadScale1 = 1;
	    cifReadScale2 = 1;
	}
    }

    /*
     * Set up the cell definition.
     */

    cifReadCellDef = cifFindCell(number);
    DBCellClearDef(cifReadCellDef);
    DBCellSetAvail(cifReadCellDef);

    cifOldReadPlane = cifReadPlane;
    cifReadPlane = (Plane *) NULL;
    cifSubcellBeingRead = TRUE;
    cifCurReadPlanes = cifSubcellPlanes;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFPaintCurrent --
 *
 * 	This procedure does geometrical processing on the current
 *	set of CIF planes, and paints the results into the current
 *	CIF cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots of information gets added to the current Magic cell.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFPaintCurrent()
{
    Plane *plane;
    int i;

    for (i = 0; i < cifCurReadStyle->crs_nLayers; i += 1)
    {
	TileType type;
	extern int cifPaintCurrentFunc();	/* Forward declaration. */

	plane = CIFGenLayer(cifCurReadStyle->crs_layers[i]->crl_ops,
	    &TiPlaneRect, (CellDef *) NULL, cifCurReadPlanes);
	
	/* Generate a paint/erase table, then paint from the CIF
	 * plane into the current Magic cell.
	 */

	type = cifCurReadStyle->crs_layers[i]->crl_magicType;

	(void) DBSrPaintArea((Tile *) NULL, plane, &TiPlaneRect,
		&CIFSolidBits, cifPaintCurrentFunc, (ClientData) type);
	
	/* Recycle the plane, which was dynamically allocated. */

	DBFreePaintPlane(plane);
	TiFreePlane(plane);
    }

    /* Now go through all the current planes and zero them out. */

    for (i = 0; i < MAXCIFRLAYERS; i += 1)
    {
	DBClearPaintPlane(cifCurReadPlanes[i]);
    }
}

/* Below is the search function invoked for each CIF tile type
 * found for the current layer.
 */

int
cifPaintCurrentFunc(tile, type)
    Tile *tile;			/* Tile of CIF information. */
    TileType type;		/* Magic type to be painted. */
{
    Rect area;
    int pNum;

    /* Compute the area of the CIF tile, then scale it into
     * Magic coordinates.
     */
    
    TiToRect(tile, &area);
    area.r_xbot = CIFScaleCoord(area.r_xbot);
    area.r_xtop = CIFScaleCoord(area.r_xtop);
    area.r_ybot = CIFScaleCoord(area.r_ybot);
    area.r_ytop = CIFScaleCoord(area.r_ytop);

    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	if (DBPaintOnPlane(type, pNum))
	{
	    DBPaintPlane(cifReadCellDef->cd_planes[pNum], &area,
		    DBStdPaintTbl(type, pNum), (PaintUndoInfo *) NULL);
	}

    return  0;		/* To keep the search alive. */
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseFinish --
 *
 * 	This procedure is called at the end of a cell definition.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Process the CIF planes and paint the results into the Magic
 *	cell.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseFinish()
{
    if (!cifSubcellBeingRead)
    {
	CIFReadError("definition finish without definition start; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    if (cifSubcellId != NULL) 
    {
	CIFReadError("pending call identifier %s discarded.\n", cifSubcellId);
	(void) StrDup(&cifSubcellId, (char *) NULL);
    }
	
    /* Take the `F'. */

    TAKE();

    /* Do the geometrical processing and paint this material back into
     * the appropriate cell of the database.  Then restore the saved
     * layer info.
     */
    
    CIFPaintCurrent();

    DBAdjustLabels(cifReadCellDef, &TiPlaneRect);
    DBReComputeBbox(cifReadCellDef);
    cifReadCellDef = EditCellUse->cu_def;
    cifReadPlane = cifOldReadPlane;
    cifReadScale1 = 1;
    cifReadScale2 = 1;
    cifSubcellBeingRead = FALSE;
    cifCurReadPlanes = cifEditCellPlanes;
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseDelete --
 *
 * 	This procedure is called to handle delete-symbol statements.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	The mapping between numbers and cells is modified to eliminate
 *	some symbols.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseDelete()
{
    int		number;

    /* Take the `D'. */

    TAKE();
    if (!CIFParseInteger(&number))
    {
	CIFReadError("definition delete, but no symbol number; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    CIFReadError("definition delete not implemented; ignored.\n");
    CIFSkipToSemi();
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseName --
 *
 * 	Parse a name, which is a string of alphabetics, numerics,
 *	or underscores, possibly preceded by whitespace.
 *
 * Results:
 *	The return value is a pointer to the name read from the
 *	CIF file.  This is a statically-allocated area, so the
 *	caller should copy out of this area before invoking this
 *	procedure again.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

char *
cifParseName()
{
    char	ch;
    char	*bufferp;
    static char	buffer[128];

    /* Skip white space. */

    for (ch = PEEK() ; ch == ' ' || ch == '\t' ; ch = PEEK())
	TAKE();
    
    /* Read the string. */

    bufferp = &buffer[0];
    for (ch = PEEK() ; (! isspace(ch)) && ch != ';' ; ch = PEEK())
    {
	*bufferp++ = TAKE();
    }
    *bufferp = '\0';
    return buffer;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseUser9 --
 *
 * 	This procedure processes user extension 9: the name of the
 *	current symbol (cell) definition.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	The current CIF symbol is renamed from its default "cifxx" name
 *	to the given name.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseUser9()
{
    char *name;

    name = cifParseName();
    if (!DBCellRenameDef(cifReadCellDef, name))
    {
	CIFReadError("%s already exists, so cell from CIF is named %s.\n",
		name, cifReadCellDef->cd_name);
    }
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseCall --
 *
 * 	This procedure processes subcell uses.  The syntax of a call is
 *	call ::= C integer transform
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	A subcell is added to the current Magic cell we're generating.
 *
 * ----------------------------------------------------------------------------
 */

bool
CIFParseCall()
{
    int		called;
    Transform	transform;
    CellUse 	*use;
    
    /* Take the `C'. */

    TAKE();
    if (!CIFParseInteger(&called))
    {
	CIFReadError("call, but no symbol number; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }

    /* Get optional transformation. */

    (void) CIFParseTransform(&transform);

    /* Find the use and add it to the current cell.  Give it an
     * id also.
     */
	
    use = DBCellNewUse(cifFindCell(called), cifSubcellId);
    (void) DBLinkCell(use, cifReadCellDef);
    DBSetTrans(use, &transform);
    DBPlaceCell(use, cifReadCellDef);

    (void) StrDup(&cifSubcellId, (char *) NULL);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseUser91 --
 *
 * 	This procedure handles 91 user commands, which provide id's
 *	for following cell calls.  The syntax is:
 *	91 ::= 91 blanks name
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	The identifier is saved until the call is read.  Then it is
 *	used as the identifier for the cell.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseUser91()
{
    if (cifSubcellId != NULL)
    {
	CIFReadError("91 command with identifier %s pending; %s discarded.\n" ,
	    cifSubcellId , cifSubcellId);
    }
    (void) StrDup(&cifSubcellId, cifParseName());
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseUser94 --
 *
 * 	This procedure parses 94 user commands, which are labelled
 *	points.  The syntax is:
 *	94 ::= 94 blanks name point
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	A label is added to the current cell.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseUser94()
{
    Rect rectangle;
    char *name = NULL;
    TileType type;
    int layer;

    (void) StrDup(&name, cifParseName());
    if (! CIFParsePoint(&rectangle.r_ll))
    {
	CIFReadError("94 command, but no location; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }

    /* Scale the coordinates, then make the location into a
     * rectangle.
     */

    rectangle.r_xbot = CIFScaleCoord(rectangle.r_xbot);
    rectangle.r_ybot = CIFScaleCoord(rectangle.r_ybot);
    rectangle.r_ur = rectangle.r_ll;

    /* Get a layer, lookup the layer, then add the label to the
     * current cell.  Tricky business: in order for the default
     * label location to be computed
     */
    
    CIFSkipBlanks();
    if (PEEK() != ';')
    {
	char *name = cifParseName();
	layer = CIFReadNameToType(name, FALSE);
	if (layer < 0)
	{
	    CIFReadError("label attached to unknown layer %s.\n",
		    name);
	    type = TT_SPACE;
	}
	else type = cifCurReadStyle->crs_labelLayer[layer];
    } else {
	type = cifCurLabelType;
    }
    if (type >=0 )
    {
    	 (void) DBPutLabel(cifReadCellDef, &rectangle, -1, name, type);
    }
    freeMagic(name);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseUser95 --
 *
 * 	This procedure parses 95 user commands, which are labelled
 *	points.  The syntax is:
 *	95 ::= 95 blanks name length width point
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	An area label is added to the current cell.
 *
 * ----------------------------------------------------------------------------
 */

bool
cifParseUser95()
{
    Rect rectangle;
    Point size, center;
    char *name = NULL;
    TileType type;
    int layer;

    (void) StrDup(&name, cifParseName());
    if (! CIFParsePoint(&size))
    {
	CIFReadError("95 command, but no size; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }
    if (! CIFParsePoint(&center))
    {
	CIFReadError("95 command, but no location; ignored.\n");
	CIFSkipToSemi();
	return FALSE;
    }

    /* Scale the coordinates and create the rectangular area */
    center.p_x = CIFScaleCoord(center.p_x);
    center.p_y = CIFScaleCoord(center.p_y);
    size.p_x = CIFScaleCoord(size.p_x);
    size.p_y = CIFScaleCoord(size.p_y);
    rectangle.r_xbot = center.p_x - size.p_x/2;
    rectangle.r_ybot = center.p_y - size.p_y/2;
    rectangle.r_xtop = center.p_x + (size.p_x - size.p_x/2);
    rectangle.r_ytop = center.p_y + (size.p_y - size.p_y/2);

    /* Get a layer, lookup the layer, then add the label to the
     * current cell.  Tricky business: in order for the default
     * label location to be computed
     */
    CIFSkipBlanks();
    if (PEEK() != ';')
    {
	char *name = cifParseName();
	layer = CIFReadNameToType(name, FALSE);
	if (layer < 0)
	{
	    CIFReadError("label attached to unknown layer %s.\n",
		    name);
	    type = TT_SPACE;
	}
	else type = cifCurReadStyle->crs_labelLayer[layer];
    }
    else type = TT_SPACE;
    if (type >=0 )
    {
    	 (void) DBPutLabel(cifReadCellDef, &rectangle, -1, name, type);
    }

    freeMagic(name);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseUser --
 *
 * 	This procedure is called to process user-defined statements.
 *	The syntax is user ::= digit usertext.
 *
 * Results:
 *	TRUE is returned if the parse completed successfully, and
 *	FALSE is returned otherwise.
 *
 * Side effects:
 *	Depends on the user command.
 *
 * ----------------------------------------------------------------------------
 */
bool
CIFParseUser()
{
    char	ch;

    ch = TAKE();
    switch (ch)
    {
	case '9':
		ch = PEEK();
		switch (ch)
		{
		    case '1':
			(void) TAKE();
			return cifParseUser91();
		    case '4':
			(void) TAKE();
			return cifParseUser94();
		    case '5':
			(void) TAKE();
			return cifParseUser95();
		    default:
			if (isspace(ch)) return cifParseUser9();
		}
	default:
		CIFReadError("unimplemented user extension; ignored.\n");
		CIFSkipToSemi();
		return FALSE;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadCellCleanup --
 *
 * 	This procedure is called after processing the CIF file.
 *	It performs various cleanup functions on the cells that
 *	have been read in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The area of each cell is DRC'ed and redisplayed.  Error
 *	messages are output for any cells whose contents weren't
 *	in the CIF file.  An error message is also output if
 *	we're still in the middle of reading a subcell.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFReadCellCleanup()
{
    HashEntry *h;
    HashSearch hs;
    CellDef *def;

    if (cifSubcellBeingRead)
    {
	CIFReadError("CIF ended partway through a symbol definition.\n");
	(void) CIFParseFinish();
    }

    HashStartSearch(&hs);
    while (TRUE)
    {
	h = HashNext(&CifCellTable, &hs);
	if (h == NULL) break;

	def = (CellDef *) HashGetValue(h);
	if (def == NULL)
	{
	    CIFReadError("cell table has NULL entry (Magic error).\n");
	    continue;
	}
	if (!(def->cd_flags & CDAVAILABLE))
	{
	    CIFReadError("cell %s was used but not defined.\n", def->cd_name);
        }

	DRCCheckThis(def, TT_CHECKPAINT, &def->cd_bbox);
	DBWAreaChanged(def, &def->cd_bbox, DBW_ALLWINDOWS, &DBAllButSpaceBits);
	DBCellSetModified(def, TRUE);
    }

    HashKill(&CifCellTable);

    /* Finally, do geometrical processing on the top-level cell. */

    CIFPaintCurrent();
    DBAdjustLabels(EditCellUse->cu_def, &TiPlaneRect);
    DBReComputeBbox(EditCellUse->cu_def);
    DBWAreaChanged(EditCellUse->cu_def, &EditCellUse->cu_def->cd_bbox,
	    DBW_ALLWINDOWS, &DBAllButSpaceBits);
    DBCellSetModified(EditCellUse->cu_def, TRUE);
}
