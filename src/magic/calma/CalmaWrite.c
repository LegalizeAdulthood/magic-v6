/*
 * CalmaWrite.c --
 *
 * Output of Calma GDS-II stream format.
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
static char rcsid[]="$Header: CalmaWrite.c,v 6.0 90/08/28 18:03:48 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef	SYSV
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "utils.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "tech.h"
#include "cif.h"
#include "CIFint.h"
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "textio.h"
#include "calmaInt.h"

    /* Exports */
bool CalmaDoLabels = TRUE;	/* If FALSE, don't output labels with GDS-II */
bool CalmaDoLower = TRUE;	/* If TRUE, allow lowercase labels. */
bool CalmaFlattenArrays = FALSE;/* If TRUE, output arrays as individual uses */

    /* Forward declarations */
extern int calmaWriteInitFunc();
extern int calmaWriteMarkFunc();
extern int calmaWritePaintFunc();
extern int calmaWriteUseFunc();

/* Number assigned to each cell */
int calmaCellNum;

/* Factor by which to scale Magic coordinates for cells and labels. */
int calmaWriteScale;

/* Scale factor for outputting paint: */
int calmaPaintScale;

/*
 * Current layer number and "type".
 * In GDS-II format, this is output with each rectangle.
 */
int calmaPaintLayerNumber;
int calmaPaintLayerType;

/* Imports */
extern time_t time();

/* -------------------------------------------------------------------- */

/*
 * Macros to output various pieces of Calma information.
 * These are macros for speed.
 */

/* -------------------------------------------------------------------- */

/*
 * calmaOutRH --
 *
 * Output a Calma record header.
 * This consists of a two-byte count of the number of bytes in the
 * record (including the two count bytes), a one-byte record type,
 * and a one-byte data type.
 */
#define	calmaOutRH(count, type, datatype, f) \
    { calmaOutI2(count, f); (void) putc(type, f); (void) putc(datatype, f); }

/*
 * calmaOutI2 --
 *
 * Output a two-byte integer.
 * Calma byte order is the same as the network byte order used
 * by the various network library procedures.
 */
#define	calmaOutI2(n, f) \
    { \
	union { short u_s; char u_c[2]; } u; \
	u.u_s = htons(n); \
	(void) putc(u.u_c[0], f); \
	(void) putc(u.u_c[1], f); \
    }
/*
 * calmaOutI4 --
 *
 * Output a four-byte integer.
 * Calma byte order is the same as the network byte order used
 * by the various network library procedures.
 */
#define calmaOutI4(n, f) \
    { \
	union { long u_i; char u_c[4]; } u; \
	u.u_i = htonl(n); \
	(void) putc(u.u_c[0], f); \
	(void) putc(u.u_c[1], f); \
	(void) putc(u.u_c[2], f); \
	(void) putc(u.u_c[3], f); \
    }

static char calmaMapTable[] =
{
      0,    0,    0,    0,    0,    0,    0,    0,	/* NUL - BEL */
      0,    0,    0,    0,    0,    0,    0,    0,	/* BS  - SI  */
      0,    0,    0,    0,    0,    0,    0,    0,	/* DLE - ETB */
      0,    0,    0,    0,    0,    0,    0,    0,	/* CAN - US  */
      0,    0,    0,    0,  '$',    0,    0,    0,	/* SP  - '   */
      0,    0,    0,    0,    0,    0,    0,    0,	/* (   - /   */
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',	/* 0   - 7   */
    '8',  '9',    0,    0,    0,    0,    0,    0,	/* 8   - ?   */
      0,  'A',  'B',  'C',  'D',  'E',  'F',  'G',	/* @   - G   */
    'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',	/* H   - O   */
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',	/* P   - W   */
    'X',  'Y',  'Z',    0,    0,    0,    0,  '_',	/* X   - _   */
      0,  'a',  'b',  'c',  'd',  'e',  'f',  'g',	/* `   - g   */
    'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',	/* h   - o   */
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',	/* p   - w   */
    'x',  'y',  'z',    0,    0,    0,    0,    0,	/* x   - DEL */
};


/*
 * ----------------------------------------------------------------------------
 *
 * CalmaWrite --
 *
 * Write out the entire tree rooted at the supplied CellDef in Calma
 * GDS-II stream format, to the specified file.
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
 *
 *	Calma names can be strings of up to CALMANAMELENGTH characters.
 *	Because general names won't map into Calma names, we use the
 *	original cell name only if it is legal Calma, and otherwise
 *	generate a unique numeric name for the cell.
 *
 *	We make a depth-first traversal of the entire design tree, outputting
 *	each cell to the Calma file.  If a given cell has not been read in
 *	when we visit it, we read it in ourselves.
 *
 *	No hierarchical design rule checking or bounding box computation
 *	occur during this traversal -- both are explicitly avoided.
 *
 * ----------------------------------------------------------------------------
 */

bool
CalmaWrite(rootDef, f)
    CellDef *rootDef;	/* Pointer to CellDef to be written */
    FILE *f;		/* Open output file */
{
    int oldCount = DBWFeedbackCount, problems;
    bool good;
    CellUse dummy;

    /*
     * Make sure that the entire hierarchy rooted at rootDef is
     * read into memory and that timestamp mismatches are resolved
     * (this is needed so that we know that bounding boxes are OK).
     */

    dummy.cu_def = rootDef;
    DBCellReadArea(&dummy, &rootDef->cd_bbox);
    DBFixMismatch();

    /*
     * Go through all cells currently having CellDefs in the
     * def symbol table and mark them with negative numbers
     * to show that they should be output, but haven't yet
     * been.
     */
    (void) DBCellSrDefs(0, calmaWriteInitFunc, (ClientData) NULL);
    rootDef->cd_client = (ClientData) -1;
    calmaCellNum = -2;

    /* Output the header, identifying this file */
    calmaOutHeader(rootDef, f);

    /*
     * We perform a post-order traversal of the tree rooted at 'rootDef',
     * to insure that each child cell is output before it is used.  The
     * root cell is output last.
     */
    (void) calmaProcessDef(rootDef, f);

    /* Finish up by outputting the end-of-library marker */
    calmaOutRH(4, CALMA_ENDLIB, CALMA_NODATA, f);
    good = !ferror(f);
    (void) fclose(f);

    /* See if any problems occurred */
    if (problems = (DBWFeedbackCount - oldCount))
	TxPrintf("%d problems occurred.  See feedback entries.\n", problems);

    return (good);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaWriteInitFunc --
 *
 * Filter function called on behalf of CalmaWrite() above.
 * Responsible for setting the cif number of each cell to zero.
 *
 * Results:
 *	Returns 0 to indicate that the search should continue.
 *
 * Side effects:
 *	Modify the calma numbers of the cells they are passed.
 *
 * ----------------------------------------------------------------------------
 */

int
calmaWriteInitFunc(def)
    CellDef *def;
{
    def->cd_client = (ClientData) 0;
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaProcessUse --
 * calmaProcessDef --
 *
 * Main loop of Calma generation.  Performs a post-order, depth-first
 * traversal of the tree rooted at 'def'.  Only cells that have not
 * already been output are processed.
 *
 * The procedure calmaProcessDef() is called initially; calmaProcessUse()
 * is called internally by DBCellEnum().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes Calma GDS-II stream-format to be output.
 *	Returns when the stack is empty.
 *
 * ----------------------------------------------------------------------------
 */

calmaProcessUse(use, outf)
    CellUse *use;	/* Process use->cu_def */
    FILE *outf;		/* Stream file */
{
    return (calmaProcessDef(use->cu_def, outf));
}

calmaProcessDef(def, outf)
    CellDef *def;	/* Output this def's children, then the def itself */
    FILE *outf;		/* Stream file */
{
    /* Skip if already output */
    if ((int) def->cd_client > 0)
	return (0);

    /* Assign it a (negative) number if it doesn't have one yet */
    if ((int) def->cd_client == 0)
	def->cd_client = (ClientData) calmaCellNum--;

    /* Mark this cell */
    def->cd_client = (ClientData) (- (int) def->cd_client);

    /* Read the cell in if it is not already available. */
    if ((def->cd_flags & CDAVAILABLE) == 0)
	if (!DBCellRead(def, (char *) NULL, TRUE))
	    return (0);

    /*
     * Output the definitions for any of our descendants that have
     * not already been output.  Numbers are assigned to the subcells
     * as they are output.
     */
    (void) DBCellEnum(def, calmaProcessUse, (ClientData) outf);

    /* Output this cell */
    calmaOutFunc(def, outf);

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOutFunc --
 *
 * Write out the definition for a single cell as a GDS-II stream format
 * structure.  We try to preserve the original cell's name if it is legal
 * in GDS-II; otherwise, we generate a unique name.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Appends to the open Calma output file.
 *
 * ----------------------------------------------------------------------------
 */

calmaOutFunc(def, f)
    CellDef *def;	/* Pointer to cell def to be written */
    FILE *f;		/* Open output file */
{
    register Label *lab;
    CIFLayer *layer;
    Rect bigArea;
    int type;

    /* Output structure begin */
    calmaOutRH(28, CALMA_BGNSTR, CALMA_I2, f);
    calmaOutDate(def->cd_timestamp, f);
    calmaOutDate(time((time_t *) 0), f);

    /* Output structure name */
    calmaOutStructName(CALMA_STRNAME, def, f);

    /* Since DB units are millimicrons, multiply all units by 10 */
    calmaWriteScale = CIFCurStyle->cs_scaleFactor * 10;

    /*
     * Output the calls that the child makes to its children.  For
     * arrays we output a single call, unlike CIF, since Calma
     * supports the notion of arrays.
     */
    (void) DBCellEnum(def, calmaWriteUseFunc, (ClientData) f);

    /* Output all the tiles associated with this cell; skip temporary layers */
    GEO_EXPAND(&def->cd_bbox, CIFCurStyle->cs_radius, &bigArea);
    CIFErrorDef = def;
    CIFGen(def, &bigArea, CIFPlanes, &DBAllTypeBits, TRUE, TRUE);
    CIFGenSubcells(def, &bigArea, CIFPlanes);
    CIFGenArrays(def, &bigArea, CIFPlanes);

    for (type = 0; type < CIFCurStyle->cs_nLayers; type++)
    {
	layer = CIFCurStyle->cs_layers[type];
	if (layer->cl_flags & CIF_TEMP) continue;
	if (!CalmaIsValidLayer(layer->cl_calmanum)) continue;
	calmaPaintLayerNumber = layer->cl_calmanum;
	calmaPaintLayerType = layer->cl_calmatype;

	/* CIF units are centimicrons, Calma units are millimicrons */
	calmaPaintScale = 10;
	(void) DBSrPaintArea((Tile *) NULL, CIFPlanes[type],
	    &TiPlaneRect, &CIFSolidBits, calmaWritePaintFunc,
	    (ClientData) f);
    }

    /* Output labels */
    if (CalmaDoLabels)
	for (lab = def->cd_labels; lab; lab = lab->lab_next)
	    calmaWriteLabelFunc(lab,
			    CIFCurStyle->cs_labelLayer[lab->lab_type], f);

    /* End of structure */
    calmaOutRH(4, CALMA_ENDSTR, CALMA_NODATA, f);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaWriteUseFunc --
 *
 * Filter function, called by DBCellEnum on behalf of calmaOutFunc above,
 * to write out each CellUse called by the CellDef being output.  If the
 * CellUse is an array, we output it as a single array instead of as
 * individual uses like CIF.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Appends to the open Calma output file.
 *
 * ----------------------------------------------------------------------------
 */

int
calmaWriteUseFunc(use, f)
    CellUse *use;
    FILE *f;
{
    /*
     * r90, r180, and r270 are Calma 8-byte real representations
     * of the angles 90, 180, and 270 degrees.
     */
    static char r90[] = { 0x42, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static char r180[] = { 0x42, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static char r270[] = { 0x43, 0x10, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char *whichangle;
    int x, y, topx, topy, rows, cols, xxlate, yxlate, hdrsize;
    int rectype, stransflags;
    register Transform *t;
    bool isArray = FALSE;
    Point p, p2;

    topx = use->cu_xhi - use->cu_xlo;
    if (topx < 0) topx = -topx;
    topy = use->cu_yhi - use->cu_ylo;
    if (topy < 0) topy = -topy;

    /*
     * The following translates from the abcdef transforms that
     * we use internally to the rotation and mirroring specification
     * used in Calma stream files.  It only works because orientations
     * are orthogonal in magic, and no scaling is allowed in cell use
     * transforms.  Thus the elements a, b, d, and e always have one
     * of the following forms:
     *
     *		a  d
     *		b  e
     *
     * (counterclockwise rotations of 0, 90, 180, 270 degrees)
     *
     *	1  0	0  1	-1  0	 0 -1
     *	0  1   -1  0	 0 -1	 1  0
     *
     * (mirrored across the x-axis before counterclockwise rotation
     * by 0, 90, 180, 270 degrees):
     *
     *	1  0    0  1    -1  0    0 -1
     *	0 -1    1  0     0  1   -1  0
     *
     * Note that mirroring must be done if either a != e, or
     * a == 0 and b == d.
     *
     */
    t = &use->cu_transform;
    stransflags = 0;
    whichangle = (t->t_a == -1) ? r180 : (char *) NULL;
    if (t->t_a != t->t_e || (t->t_a == 0 && t->t_b == t->t_d))
    {
	stransflags |= CALMA_STRANS_UPSIDEDOWN;
	if (t->t_a == 0)
	{
	    if (t->t_b == 1) whichangle = r90;
	    else whichangle = r270;
	}
    }
    else if (t->t_a == 0)
    {
	if (t->t_b == -1) whichangle = r90;
	else whichangle = r270;
    }

    if (CalmaFlattenArrays)
    {
	for (x = 0; x <= topx; x++)
	{
	    for (y = 0; y <= topy; y++)
	    {
		/* Structure reference */
		calmaOutRH(4, CALMA_SREF, CALMA_NODATA, f);
		calmaOutStructName(CALMA_SNAME, use->cu_def, f);

		/* Transformation flags */
		calmaOutRH(6, CALMA_STRANS, CALMA_BITARRAY, f);
		calmaOutI2(stransflags, f);

		/* Rotation if there is one */
		if (whichangle)
		{
		    calmaOutRH(12, CALMA_ANGLE, CALMA_R8, f);
		    calmaOut8(whichangle, f);
		}

		/* Translation */
		xxlate = t->t_c + t->t_a*(use->cu_xsep)*x
				+ t->t_b*(use->cu_ysep)*y;
		yxlate = t->t_f + t->t_d*(use->cu_xsep)*x
				+ t->t_e*(use->cu_ysep)*y;
		xxlate *= calmaWriteScale;
		yxlate *= calmaWriteScale;
		calmaOutRH(12, CALMA_XY, CALMA_I4, f);
		calmaOutI4(xxlate, f);
		calmaOutI4(yxlate, f);

		/* End of element */
		calmaOutRH(4, CALMA_ENDEL, CALMA_NODATA, f);
	    }
	}
    }
    else
    {
	/* Is it an array? */
	isArray = (topx > 0 || topy > 0);
	rectype = isArray ? CALMA_AREF : CALMA_SREF;

	/* Structure reference */
	calmaOutRH(4, rectype, CALMA_NODATA, f);
	calmaOutStructName(CALMA_SNAME, use->cu_def, f);

	/* Transformation flags */
	calmaOutRH(6, CALMA_STRANS, CALMA_BITARRAY, f);
	calmaOutI2(stransflags, f);

	/* Rotation if there is one */
	if (whichangle)
	{
	    calmaOutRH(12, CALMA_ANGLE, CALMA_R8, f);
	    calmaOut8(whichangle, f);
	}

	/* If array, number of columns and rows in the array */
	if (isArray)
	{
	    calmaOutRH(8, CALMA_COLROW, CALMA_I2, f);
	    cols = topx + 1;
	    rows = topy + 1;
	    calmaOutI2(cols, f);
	    calmaOutI2(rows, f);
	}

	/* Translation */
	xxlate = t->t_c * calmaWriteScale;
	yxlate = t->t_f * calmaWriteScale;
	hdrsize = isArray ? 28 : 12;
	calmaOutRH(hdrsize, CALMA_XY, CALMA_I4, f);
	calmaOutI4(xxlate, f);
	calmaOutI4(yxlate, f);

	/* Array sizes if an array */
	if (isArray)
	{
	    /* Column reference point */
	    p.p_x = use->cu_xsep * cols;
	    p.p_y = 0;
	    GeoTransPoint(t, &p, &p2);
	    p2.p_x *= calmaWriteScale;
	    p2.p_y *= calmaWriteScale;
	    calmaOutI4(p2.p_x, f);
	    calmaOutI4(p2.p_y, f);

	    /* Row reference point */
	    p.p_x = 0;
	    p.p_y = use->cu_ysep * rows;
	    GeoTransPoint(t, &p, &p2);
	    p2.p_x *= calmaWriteScale;
	    p2.p_y *= calmaWriteScale;
	    calmaOutI4(p2.p_x, f);
	    calmaOutI4(p2.p_y, f);
	}

	/* End of element */
	calmaOutRH(4, CALMA_ENDEL, CALMA_NODATA, f);
    }

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOutStructName --
 *
 * Output the name of a cell def.
 * If the name is legal GDS-II, use it; otherwise, generate one
 * that is legal and unique.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the disk file.
 *
 * ----------------------------------------------------------------------------
 */

calmaOutStructName(type, def, f)
    int type;
    CellDef *def;
    FILE *f;
{
    char defname[CALMANAMELENGTH+1];
    register unsigned char c;
    register char *cp;
    int calmanum;

    /* Is the def name a legal Calma name? */
    for (cp = def->cd_name; c = (unsigned char) *cp; cp++)
	if (c > 127 || calmaMapTable[c] == 0)
	    goto bad;
    if (cp <= def->cd_name + CALMANAMELENGTH)
    {
	/* Yes, it's legal: use it */
	(void) strcpy(defname, def->cd_name);
    }
    else
    {
	/* Bad name: use XXXXXcalmaNum */
bad:
	calmanum = (int) def->cd_client;
	if (calmanum < 0) calmanum = -calmanum;
	(void) sprintf(defname, "XXXXX%d", calmanum);
    }

    calmaOutStringRecord(type, defname, f);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaWritePaintFunc --
 *
 * Filter function used to write out a single paint tile.
 *
 *			**** NOTE ****
 * There are loads of Calma systems out in the world that
 * don't understand CALMA_BOX, so we output CALMA_BOUNDARY
 * even though CALMA_BOX is more appropriate.  Bletch.
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
calmaWritePaintFunc(tile, f)
    register Tile *tile;		/* Tile to be written out. */
    FILE *f;				/* File in which to write. */
{
    Rect r;

    TiToRect(tile, &r);
    r.r_xbot *= calmaPaintScale;
    r.r_ybot *= calmaPaintScale;
    r.r_xtop *= calmaPaintScale;
    r.r_ytop *= calmaPaintScale;

    /* Boundary */
    calmaOutRH(4, CALMA_BOUNDARY, CALMA_NODATA, f);

    /* Layer */
    calmaOutRH(6, CALMA_LAYER, CALMA_I2, f);
    calmaOutI2(calmaPaintLayerNumber, f);

    /* Data type */
    calmaOutRH(6, CALMA_DATATYPE, CALMA_I2, f);
    calmaOutI2(calmaPaintLayerType, f);

    /* Coordinates */
    calmaOutRH(44, CALMA_XY, CALMA_I4, f);
    calmaOutI4(r.r_xbot, f); calmaOutI4(r.r_ybot, f);
    calmaOutI4(r.r_xtop, f); calmaOutI4(r.r_ybot, f);
    calmaOutI4(r.r_xtop, f); calmaOutI4(r.r_ytop, f);
    calmaOutI4(r.r_xbot, f); calmaOutI4(r.r_ytop, f);
    calmaOutI4(r.r_xbot, f); calmaOutI4(r.r_ybot, f);

    /* End of element */
    calmaOutRH(4, CALMA_ENDEL, CALMA_NODATA, f);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaWriteLabelFunc --
 *
 * Output a single label to the stream file 'f'.
 *
 * The CIF type to which this label is attached is 'type'; if this
 * is < 0 then the label is not output.
 *
 * Non-point labels are collapsed to point labels located at the center
 * of the original label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the FILE 'f'.
 *
 * ----------------------------------------------------------------------------
 */

calmaWriteLabelFunc(lab, type, f)
    Label *lab;	/* Label to output */
    int type;	/* CIF layer number, or -1 if not attached to a layer */
    FILE *f;	/* Stream file */
{
    Point p;
    int calmanum;

    if (type < 0)
	return;

    calmanum = CIFCurStyle->cs_layers[type]->cl_calmanum;
    if (!CalmaIsValidLayer(calmanum))
	return;

    calmaOutRH(4, CALMA_TEXT, CALMA_NODATA, f);

    calmaOutRH(6, CALMA_LAYER, CALMA_I2, f);
    calmaOutI2(calmanum, f);

    calmaOutRH(6, CALMA_TEXTTYPE, CALMA_I2, f);
    calmaOutI2(CIFCurStyle->cs_layers[type]->cl_calmatype, f);

    p.p_x = (lab->lab_rect.r_xbot + lab->lab_rect.r_xtop) * calmaWriteScale / 2;
    p.p_y = (lab->lab_rect.r_ybot + lab->lab_rect.r_ytop) * calmaWriteScale / 2;
    calmaOutRH(12, CALMA_XY, CALMA_I4, f);
    calmaOutI4(p.p_x, f);
    calmaOutI4(p.p_y, f);

    /* Text of label */
    calmaOutStringRecord(CALMA_STRING, lab->lab_text, f);

    /* End of element */
    calmaOutRH(4, CALMA_ENDEL, CALMA_NODATA, f);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOutHeader --
 *
 * Output the header description for a Calma file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the FILE 'f'.
 *
 * ----------------------------------------------------------------------------
 */

Void
calmaOutHeader(rootDef, f)
    CellDef *rootDef;
    FILE *f;
{
    /* useru = 0.001, mum = 1.0e-9 in Calma 8-byte real format */
    static char useru[] = { 0x3e, 0x41, 0x89, 0x37, 0x4b, 0xc6, 0xa7, 0xef };
    static char mum[] = { 0x39, 0x44, 0xb8, 0x2f, 0xa0, 0x9b, 0x5a, 0x53 };

    /* GDS II version 3.0 */
    calmaOutRH(6, CALMA_HEADER, CALMA_I2, f);
    calmaOutI2(3, f);

    /* Beginning of library */
    calmaOutRH(28, CALMA_BGNLIB, CALMA_I2, f);
    calmaOutDate(rootDef->cd_timestamp, f);
    calmaOutDate(time((time_t *) 0), f);

    /* Library name (name of root cell) */
    calmaOutStructName(CALMA_LIBNAME, rootDef, f);

    /*
     * Units.
     * User units are microns; this is really unimportant.
     * Database units are millimicrons, since there are lots
     * of programs that don't understand anything else.
     */
    calmaOutRH(20, CALMA_UNITS, CALMA_R8, f);
    calmaOut8(useru, f);	/* User units per database unit */
    calmaOut8(mum, f);		/* Meters per database unit */
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOutDate --
 *
 * Output a date/time specification to the FILE 'f'.
 * This consists of outputting 6 2-byte quantities,
 * or a total of 12 bytes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the FILE 'f'.
 *
 * ----------------------------------------------------------------------------
 */

calmaOutDate(t, f)
    time_t t;	/* Time (UNIX format) to be output */
    FILE *f;	/* Stream file */
{
    struct tm *datep = localtime(&t);

    calmaOutI2(datep->tm_year, f);
    calmaOutI2(datep->tm_mon+1, f);
    calmaOutI2(datep->tm_mday, f);
    calmaOutI2(datep->tm_hour, f);
    calmaOutI2(datep->tm_min, f);
    calmaOutI2(datep->tm_sec, f);
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOutStringRecord --
 *
 * Output a complete string-type record.  The actual record
 * type is given by 'type'.  Up to the first CALMANAMELENGTH characters
 * of the string 'str' are output.  Any characters in 'str'
 * not in the legal Calma stream character set are output as
 * 'X' instead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the FILE 'f'.
 *
 * ----------------------------------------------------------------------------
 */

calmaOutStringRecord(type, str, f)
    int type;		/* Type of this record (data type is ASCII string) */
    register char *str;	/* String to be output (<= CALMANAMELENGTH chars) */
    register FILE *f;	/* Stream file */
{
    int len;
    register unsigned char c;

    len = strlen(str);

    /*
     * Make sure length is even.
     * Output at most CALMANAMELENGTH characters.
     */
    if (len & 01) len++;
    if (len > CALMANAMELENGTH) len = CALMANAMELENGTH;
    calmaOutI2(len+4, f);	/* Record length */
    (void) putc(type, f);		/* Record type */
    (void) putc(CALMA_ASCII, f);	/* Data type */

    /* Output the string itself */
    while (len--)
    {
	c = (unsigned char) *str++;
	if (c == 0) putc('\0', f);
	else
	{
	    if (c > 127 || calmaMapTable[c] == 0) c = 'X';
	    else c = calmaMapTable[c];
	    if (!CalmaDoLower && islower(c))
		(void) putc(toupper(c), f);
	    else
		(void) putc(c, f);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * calmaOut8 --
 *
 * Output 8 bytes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the FILE 'f'.
 *
 * ----------------------------------------------------------------------------
 */

calmaOut8(str, f)
    register char *str;	/* 8-byte string to be output */
    register FILE *f;	/* Stream file */
{
    register i;

    for (i = 0; i < 8; i++)
	(void) putc(*str++, f);
}
