/*
 * CIFread.h --
 *
 * This file contains definitions used by the CIF reader, but not
 * by the CIF writing code.  The definitions are only used internally
 * to this module.
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
 *
 *
 * rcsid "$Header: CIFread.h,v 6.1 90/09/12 17:14:22 mayo Exp $
 */

#ifndef	_DATABASE
int err1 = Need_to_include_database_header;
#endif	_DATABASE
#ifndef	_CIFINT
int err1 = Need_to_include_cifint_header;
#endif	_CIFINT

/* The structures below are built up by CIFreadtech.c to describe
 * various styles for reading CIF.
 */

/* The following structure describes a sequence of geometric
 * operations used to produce information for a single Magic
 * layer.  There may be several of these structures for the
 * same Magic layer;  in that case, the results end up being
 * OR'ed together.
 */

typedef struct
{
    TileType crl_magicType;	/* Magic layer to paint results. */
    CIFOp *crl_ops;		/* List of operations to generate
				 * info for Magic layer.
				 */
    int crl_flags;		/* Miscellaneous flags (see below). */
} CIFReadLayer;

/* The CIFReadLayer flags are:
 *
 * CIFR_SIMPLE: Means this layer is a simple one, coming from only
 *		a single OR operation, so it can be handled specially.
 */

#define CIFR_SIMPLE 1

/* The following structure defines a complete CIF read-in style.
 * The constant MAXCIFRLAYERS must be less than TT_MAXTYPES, and
 * is used both as the largest number of distinct CIF layer names
 * in all read styles, and as the larges number of distinct "layer"
 * commands in any one read style.
 */

#define MAXCIFRLAYERS 64

typedef struct cifrstyle
{
    char *crs_name;		/* Name for this style of CIF input. */
    TileTypeBitMask crs_cifLayers;
				/* Mask of CIF layers understood in
				 * this style.
				 */
    int crs_nLayers;		/* Number of CIFReadLayers involved. */
    int crs_scaleFactor;	/* Number of CIF units per Magic unit. */
    TileType crs_labelLayer[MAXCIFRLAYERS];
				/* Gives the Magic layer to use for labels
				 * on each possible CIF layer.
				 */
    CIFReadLayer *crs_layers[MAXCIFRLAYERS];
    struct cifrstyle *crs_next;	/* Next in list of styles (NULL for
				 * end of list).
				 */
} CIFReadStyle;

/* For parsing CIF, we need to keep track of paths (wire locations
 * or polygon boundaries.  These are just linked lists of points.
 */

typedef struct cifpath
{
    Point cifp_point;		/* A point in the path. */
    struct cifpath *cifp_next;	/* The next point in the path, or NULL. */
} CIFPath;

#define cifp_x cifp_point.p_x
#define cifp_y cifp_point.p_y

/* Procedures */

extern bool CIFParseBox(), CIFParseWire(), CIFParsePoly();
extern bool CIFParseFlash(), CIFParseLayer(), CIFParseStart();
extern bool CIFParseFinish(), CIFParseDelete(), CIFParseUser();
extern bool CIFParseCall(), CIFParseTransform(), CIFParseInteger();
extern bool CIFParsePath(), CIFParsePoint(), CIFParseSInteger();
extern void CIFSkipToSemi(), CIFSkipSep(), CIFSkipBlanks();
extern void CIFReadError(), CIFFreePath();
extern void CIFReadCellInit(), CIFReadCellCleanup();
extern LinkedRect *CIFPolyToRects();
extern Transform *CIFDirectionToTrans();
extern int CIFReadNameToType();

/* Variables shared by the CIF-reading modules, see CIFreadutils.c
 * for more details:
 */

extern int cifReadScale1, cifReadScale2;
extern int cifNReadLayers;
extern Plane *cifReadPlane;
extern Plane **cifCurReadPlanes;
extern TileType cifCurLabelType;
extern CIFReadStyle *cifCurReadStyle;
extern bool cifSubcellBeingRead;
extern CellDef *cifReadCellDef;
extern FILE *cifInputFile;
extern bool cifParseLaAvail;
extern int cifParseLaChar;

/* Macros to read characters, with one-character look-ahead. */

#define PEEK()	( cifParseLaAvail \
		? cifParseLaChar \
		: (cifParseLaAvail = TRUE, \
			cifParseLaChar = getc(cifInputFile)))

#define TAKE()	( cifParseLaAvail \
		? (cifParseLaAvail = FALSE, cifParseLaChar) \
		: (cifParseLaChar = getc(cifInputFile)))

/* constants */
