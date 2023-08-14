/*
 * CIFint.h --
 *
 * Defines things shared internally by the cif module of Magic,
 * but not generally needed outside the cif module.
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
 * rcsid "$Header: CIFint.h,v 6.0 90/08/28 18:05:04 mayo Exp $"
 */

#define _CIFINT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err1 = Need_to_include_database_header;
#endif	_DATABASE

/* The main data structure used in the cif module is a description
 * of how to generate CIF layers from the Magic tiles.  There may
 * be several different styles for generating CIF from the same Magic
 * information, e.g. for fabricating at different geometries.  Each
 * of these CIF styles involves three kinds of data.  A "CIFStyle"
 * record gives overall information such as the number of layers.
 * One "CIFLayer" gives overall information for each layer, and
 * then a list of one or more "CIFOp" records describes a sequence
 * of geometrical operations to perform to generate the layer.  This
 * data structure is built up by reading the technology file.
 */

/* A CIFOp starts from a partially-completed CIF layer, does something
 * to it, which may possibly involve some existing layers or temporary
 * layers, and creates the next stage of the partially-completed
 * CIF layer.  Example operations are to AND with some existing paint,
 * or to grow by a certain amount.
 */

typedef struct cifop
{
    TileTypeBitMask co_paintMask;/* Zero or more paint layers to consider. */
    TileTypeBitMask co_cifMask;	 /* Zero or more other CIF layers. */
    int co_opcode;		/* Which geometric operation to use.  See
				 * below for the legal ones.
				 */
    int co_distance;		/* Grow or shrink distance (if needed). */
    int co_bloats[TT_MAXTYPES];	/* For CIFOP_BLOAT, CIFOP_BLOATMAX, and
				 * CIFOP_BLOATMIN: distance to grow each
				 * layer.  For CIFOP_SQUARES: [0] is the
				 * border around squares, [1] is the size
				 * of each square, and [2] is the distance
				 * between squares.  For other operations,
				 * not used.
				 */
    struct cifop *co_next;	/* Next in list of operations to perform. */
} CIFOp;

/* The opcodes defined so far are:
 *
 * CIFOP_AND -		AND current results with the layers indicated by
 *			the masks.
 * CIFOP_ANDNOT -	Wherever there is material indicated by the masks,
 *			erase those areas from the current results.
 * CIFOP_OR -		OR current results with the layers indicated by
 *			the masks.
 * CIFOP_GROW -		Grow the current results uniformly by co_distance.
 * CIFOP_SHRINK -	Shrink the current results uniformly by co_distance.
 * CIFOP_BLOAT -	Find layers in paintMask, then bloat selectively
 *			according to co_bloats, and OR the results into
 *			the current plane.
 * CIFOP_SQUARES -	Generates a pattern of squares (used for making
 *			contact vias.  Each square is co_distance large,
 *			the squares are separated from each other by
 *			co_distance, and they are inside the edge of
 *			the material by at least co_distance.
 * CIFOP_BLOATMAX -	Like CIFOP_BLOAT, except whole side of tile gets
 *			bloated by same amount, which is max bloat from
 *			anywhere along side.  Bloats can be negative.
 * CIFOP_BLOATMIN -	Same as CIFOP_BLOAT, except use min bloat from
 *			anywhere along side.
 */

#define CIFOP_AND	1
#define CIFOP_OR	2
#define CIFOP_GROW	3
#define CIFOP_SHRINK	4
#define CIFOP_BLOAT	5
#define CIFOP_SQUARES	6
#define CIFOP_BLOATMAX	7
#define CIFOP_BLOATMIN	8
#define CIFOP_ANDNOT	9

/* The following data structure contains all the information about
 * a particular CIF layer.
 */

typedef struct
{
    char *cl_name;		/* Name of layer. */
    CIFOp *cl_ops;		/* List of operations.  If NULL, layer is
				 * determined entirely by cl_initial.
				 */
    int cl_growDist;		/* Largest distance material may move in
				 * this layer from its original Magic
				 * position, due to grows.  Expressed
				 * in CIF units.  If this layer uses temp
				 * layers, this distance must include grows
				 * from the temp layers.
				 */
    int cl_shrinkDist;		/* Same as above, except for shrinks. */
    int cl_flags;		/* Bunches of flags:  see below. */
    int cl_calmanum;		/* Number (0-63) of this layer for output as
				 * Calma (GDS-II stream format), or -1 if
				 * this layer should not be output.
				 */
    int cl_calmatype;		/* Data type (0-63) for Calma output, or -1
				 * if this layer should not be output.
				 */
} CIFLayer;

/* The CIFLayer flags are:
 *
 * CIF_TEMP:	Means that this is a temporary layer used to build
 *		up CIF information.  It isn't output in the CIF file.
 */

#define CIF_TEMP 1

/* The following data structure describes a complete set of CIF
 * layers.  The number of CIF layers (MAXCIFLAYERS) must not be
 * greater than the number of tile types (TT_MAXTYPES)!!
 */

#define MAXCIFLAYERS 64

typedef struct cifstyle
{
    char *cs_name;		/* Name used for this kind of CIF. */
    int cs_nLayers;		/* Number of layers. */
    int cs_radius;		/* Radius of interaction for hierarchical
				 * processing (expressed in Magic units).
				 */
    int cs_stepSize;		/* If non-zero, user-specified step size
				 * for hierarchical processing (in Magic
				 * units).
				 */
    int cs_scaleFactor;		/* Number of centimicrons per Magic unit. */
    int cs_reducer;		/* Reduction factor (used only to reduce
				 * number of zeroes in CIF files and make
				 * file more readable).  A special value
				 * of 0 means that this style can't be
				 * used for generating CIF, but is only
				 * for generating Calma output.
				 */
    TileTypeBitMask cs_yankLayers;
				/* For hierarchical processing, only these
				 * Magic types need to be yanked.
				 */
    TileTypeBitMask cs_hierLayers;
				/* For hierarchical processing, only these
				 * CIF layers need to be generated.
				 */
    int cs_labelLayer[TT_MAXTYPES];
				/* Each entry corresponds to one Magic layer,
				 * and gives index of CIF real layer to use
				 * for labels attached to this Magic layer.
				 * -1 means no known CIF layer for this Magic
				 * layer.
				 */
    CIFLayer *cs_layers[MAXCIFLAYERS];
				/* Describes how to generate each layer.*/
    struct cifstyle *cs_next;
				/* Pointer to next in list of styles. */
} CIFStyle;


/* procedures */

extern bool CIFNameToMask();
extern void CIFGenSubcells();
extern void CIFGenArrays();
extern void CIFGen();
extern void CIFClearPlanes();
extern Plane *CIFGenLayer();
extern void CIFInitCells();
extern int cifHierCopyFunc();

/* Shared variables and structures: */

extern Plane *CIFPlanes[];		/* Normal place to store CIF. */
extern CIFStyle *CIFStyleList;		/* List of all CIF styles. */
extern CIFStyle *CIFCurStyle;		/* Current style being used. */
extern CellUse *CIFComponentUse;	/* Flatten stuff in here if needed. */
extern CellDef *CIFComponentDef;	/* Corresponds to CIFComponentUse. */
extern CellUse *CIFDummyUse;		/* Used to dummy up a CellUse for a
					 * def.
					 */

/* Statistics counters: */

extern int CIFTileOps;
extern int CIFHierTileOps;
extern int CIFRects;
extern int CIFHierRects;

/* Tables used for painting and erasing CIF. */

extern PaintResultType CIFPaintTable[], CIFEraseTable[];

/* Procedures and variables for reporting errors. */

extern int CIFErrorLayer;
extern CellDef *CIFErrorDef;
extern void CIFError();

/* The following determines the tile type used to hold the CIF
 * information on its paint plane.
 */

#define CIF_SOLIDTYPE 1
extern TileTypeBitMask CIFSolidBits;
