/*
 * database.h --
 *
 * Definitions for the database module.
 * This file defines everything that is visible to clients
 * outside the database module.
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
 * Needs to include: magic.h, tiles.h
 *
 * rcsid "$Header: database.h,v 6.0 90/08/28 18:10:36 mayo Exp $"
 */

#define	_DATABASE

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_TILES
int err1 = Need_to_include_tiles_header;
#endif	_TILES
#ifndef	_HASH
int err2 = Need_to_include_hash_header;
#endif	_HASH

/* ----------------------- Tunable constants -------------------------- */

#define	MAXPLANES	15	/* Maximum number of planes per cell */

    /*
     * The "unnamed" cell.
     * This is visible only within Magic, and just determines the
     * name by which the initial cell-without-a-name appears.
     */
#define	UNNAMED	"(UNNAMED)"

/* --------------------- Tile types and masks ------------------------- */

/*
 * Tile types are small integers (currently from 0 to TT_MAXTYPES-1).
 * They are used in constructing TileTypeBitMask bitmasks with the
 * operators defined below.  A TileTypeBitMask contains one bit for
 * each possible TileType.
 *
 * The last TT_RESERVEDTYPES tile types are reserved for use by clients.
 * Because unsigned chars are used to hold tile types (PaintResultType),
 * this number should not be increased without changing the definition
 * of PaintResultType later in this file.
 *
 * If TT_MASKWORDS changes, the macros TTMaskAndMask(), etc. must also
 * be changed.
 */

typedef int TileType;

#define	TT_MAXTYPES		80			/* See above! */
#define	TT_RESERVEDTYPES	2			/* See above! */

#define	TT_BPW			(8 * sizeof (unsigned))
#define	TT_WORDMASK		(TT_BPW - 1)
#define	TT_WORDSHIFT		5			/* LOG2(TT_BPW) */
#define	TT_MASKWORDS		((TT_MAXTYPES + TT_BPW - 1) / TT_BPW)

typedef struct
{
    unsigned	tt_words[TT_MASKWORDS];
} TileTypeBitMask;

/*
 * Although some tile types are assigned on a technology-specific basis,
 * certain types are independent of the technology used, and are
 * defined below:
 */
#define	TT_SPACE	0	/* Space tile */
#define	TT_PAINTBASE	1	/* First non-space type */
#define TT_CHECKPAINT   1       /* DRC -- paint has changed */
#define TT_CHECKSUBCELL 2	/* DRC -- subcells have changed */
#define TT_ERROR_P      3       /* DRC -- paint error */
#define TT_ERROR_S      4       /* DRC -- subcell error */
#define TT_ERROR_PS     5       /* DRC --             */
#define TT_MAGNET	6	/* magnet for interactive router */
#define TT_FENCE	7	/* fence for interactive router */
#define TT_ROTATE	8	/* rotate for interactive router */
#define TT_SELECTBASE 	TT_MAGNET /* First selectable type */
#define	TT_TECHDEPBASE	9	/* First technology-dependent type */

/* Pseudo type signifying unexpanded subcells.  Never painted.  -  Only
   used in a few places, e.g.  TouchingTypes() and mzrouter spacing arrays.
 */
#define TT_SUBCELL	TT_MAXTYPES  

/* The following type is used in the paint result tables to save space*/
typedef unsigned char PaintResultType;

/* Which word in a mask contains the bit for type 't'? */
#define	ttWord(t)	((t) >> TT_WORDSHIFT)

/* Which bit in the above word is for type 't'? */
#define	ttBit(t)	((t) & TT_WORDMASK)

/* Mask for above word with only bit 't' set */
#define	ttMask(t)	(1 << ttBit(t))

/* Operations for manipulating TileTypeBitMasks */
#define TTMaskSetType(m, t)	((m)->tt_words[ttWord(t)] |= ttMask(t))
#define	TTMaskClearType(m, t)	((m)->tt_words[ttWord(t)] &= ~ttMask(t))
#define	TTMaskHasType(m, t)	(((m)->tt_words[ttWord(t)] & ttMask(t)) != 0)
#define	TTMaskSetOnlyType(m, t)	(TTMaskZero(m), TTMaskSetType(m, t))

/* If TT_MASKWORDS (currently 3) changes, the following must also change */
#define	TTMaskZero(m)		((m)->tt_words[0] = 0, (m)->tt_words[1] = 0, \
				(m)->tt_words[2] = 0)
#define	TTMaskIsZero(m)		((m)->tt_words[0] == 0 && (m)->tt_words[1] == 0\
				&& (m)->tt_words[2] == 0)
#define	TTMaskEqual(m, n)	((m)->tt_words[0] == (n)->tt_words[0] \
			      && (m)->tt_words[1] == (n)->tt_words[1] \
			      && (m)->tt_words[2] == (n)->tt_words[2])
#define TTMaskIntersect(m, n)	(((m)->tt_words[0] & ((n)->tt_words[0])) \
			      || ((m)->tt_words[1] & ((n)->tt_words[1])) \
			      || ((m)->tt_words[2] & ((n)->tt_words[2])))

#define	TTMaskCom(m)		(((m)->tt_words[0] = ~(m)->tt_words[0]), \
				 ((m)->tt_words[1] = ~(m)->tt_words[1]), \
				 ((m)->tt_words[2] = ~(m)->tt_words[2]))
#define	TTMaskCom2(m, n)	(((m)->tt_words[0] = ~(n)->tt_words[0]), \
				 ((m)->tt_words[1] = ~(n)->tt_words[1]), \
				 ((m)->tt_words[2] = ~(n)->tt_words[2]))

#define	TTMaskSetMask(m, n)	(((m)->tt_words[0] |= (n)->tt_words[0]), \
				 ((m)->tt_words[1] |= (n)->tt_words[1]), \
				 ((m)->tt_words[2] |= (n)->tt_words[2]))
#define	TTMaskSetMask3(m, n, o) \
	(((m)->tt_words[0] = (n)->tt_words[0] | (o)->tt_words[0]), \
	 ((m)->tt_words[1] = (n)->tt_words[1] | (o)->tt_words[1]), \
	 ((m)->tt_words[2] = (n)->tt_words[2] | (o)->tt_words[2]))

#define	TTMaskAndMask(m, n)	(((m)->tt_words[0] &= (n)->tt_words[0]), \
				 ((m)->tt_words[1] &= (n)->tt_words[1]), \
				 ((m)->tt_words[2] &= (n)->tt_words[2]))
#define	TTMaskAndMask3(m, n, o) \
	(((m)->tt_words[0] = (n)->tt_words[0] & (o)->tt_words[0]), \
	 ((m)->tt_words[1] = (n)->tt_words[1] & (o)->tt_words[1]), \
	 ((m)->tt_words[2] = (n)->tt_words[2] & (o)->tt_words[2]))

#define	TTMaskClearMask(m, n)	(((m)->tt_words[0] &= ~(n)->tt_words[0]), \
				 ((m)->tt_words[1] &= ~(n)->tt_words[1]), \
				 ((m)->tt_words[2] &= ~(n)->tt_words[2]))
#define	TTMaskClearMask3(m, n, o) \
	(((m)->tt_words[0] = (n)->tt_words[0] & ~(o)->tt_words[0]), \
	 ((m)->tt_words[1] = (n)->tt_words[1] & ~(o)->tt_words[1]), \
	 ((m)->tt_words[2] = (n)->tt_words[2] & ~(o)->tt_words[2]))

/* ---------------------- Planes and masks ---------------------------- */
/*
 * Plane numbers are also small integers.  Certain planes are
 * technology-specific, but others are always present independent
 * of the technology used.
 *
 * Note that the CELL plane and the DRC_CHECK plane are invisible
 * as far as normal painting operations are concerned, but that the
 * DRC_CHECK plane does get written out to the .cad file.
 *
 * The following macros are used for converting between plane numbers
 * and masks of same.
 */
#define	PlaneNumToMaskBit(p)	(1 << (p))
#define	PlaneMaskHasPlane(m, p)	(((m) & PlaneNumToMaskBit(p)) != 0)


#define	PL_MAXTYPES	MAXPLANES	/* Maximum number of planes per cell */
#define	PL_CELL		0		/* Cell plane */
#define PL_DRC_CHECK	1		/* DRC plane for CHECK tiles */
#define PL_DRC_ERROR	2		/* DRC plane for ERROR tiles */
#define PL_M_HINT	3		/* magnet hints for irouter */
#define PL_F_HINT       4		/* fence hints for irouter */
#define PL_R_HINT	5		/* rotate hints for irouter */
#define PL_SELECTBASE	PL_M_HINT	/* First plane with selectable types */
#define	PL_TECHDEPBASE	6		/* First technology-dependent plane */
#define PL_PAINTBASE	1		/* Base of paint planes */

/* --------------------------- Labels --------------------------------- */

/*
 * The body of a tile may have a list of labels associated with it.
 * Each label contains a location that indicates the point at which the
 * label is supposed to appear attached.  This location must be within
 * the tile on whose list the label appears.
 */

typedef struct label
{
    TileType             lab_type;      /* Type of material to which this label
                                         * is attached.  This material, or
                                         * other materials that connect it,
                                         * must be present everywhere under
                                         * the area of the label (Magic
                                         * enforces this).  If there isn't
                                         * any such material, lab_type is
                                         * TT_SPACE.
                                         */
    Rect                 lab_rect;      /* Area of label. */
    int                  lab_pos;       /* Position at which text is to be
                                         * displayed relative to the box.
                                         */
    struct label        *lab_next;      /* Next label in list */
    char                 lab_text[4];   /* Actual text of label.  This field
                                         * is just a place-holder: the actual
                                         * field will be large enough to
                                         * contain the label name.  This
                                         * MUST be the last field in the
                                         * structure.
                                         */
} Label;

/*
 * Macros for dealing with label rectangles.
 */

#define	LABELTOUCHPOINT(p, r) \
			((p)->p_x >= (r)->r_xbot && (p)->p_x <= (r)->r_xtop \
		      && (p)->p_y >= (r)->r_ybot && (p)->p_y <= (r)->r_ytop)

/* ------------------- Cell definitions and uses ---------------------- */

/*
 * There are two structures used for cells:
 *
 *	CellDef -- one for the definition of the cell
 *	CellUse -- one for each instantiation of the cell.
 *
 * The CellDef is shared by all of the CellUses of that cell.
 */

typedef struct celldef
{
    int			 cd_flags;	/* See definitions below */
    Rect		 cd_bbox;	/* Bounding box for cell */
    char		*cd_file;	/* File containing cell definition */
    char		*cd_name;	/* Name of cell */
    struct celluse	*cd_parents;	/* NULL-terminated list of all uses */
    Plane		*cd_planes[MAXPLANES];	/* Tiles */
    ClientData		 cd_client;	/* This space for rent */
    int			 cd_timestamp;	/* Unique integer identifying last
					 * time that paint, labels, or subcell
					 * placements changed in this def.
					 */
    Label		*cd_labels;	/* Label list for this cell */
    Label		*cd_lastLabel; 	/* Last label in list for this cell. */
    char		*cd_technology;	/* Name of technology for this cell 
					 * (Not yet used.)
					 */
    ClientData		 cd_props;	/* Private data used to maintain
					 * lists of properties.  Properties
					 * are name-value pairs and provide
					 * a flexible way of extending the
					 * data that is stored in a CellDef.
					 */
    ClientData		 cd_filler;	/* UNUSED */
    HashTable		 cd_idHash;	/* Maps cell use ids to cell uses.
					 * Indexed by cell use id; value
					 * is a pointer to the CellUse.
					 */
    TileTypeBitMask	 cd_types;	/* Types of tiles in the cell */
} CellDef;

/*
 * Cell definition flags:
 *	CDAVAILABLE means cell has been loaded from disk into main store.
 *	CDMODIFIED means cell has been changed since last write to disk.
 *	    This bit applies only to the real contents of the cell (paint,
 *	    labels, and subcell placements), and not to hint information
 *	    like child bounding box guesses and child timestamps.
 *	CDNOTFOUND means we failed to find the cell on disk once, so
 *	    don't give any more error messages about it.
 *	CDINTERNAL means that this cell is used internally by Magic (e.g.
 *	    for DRC checking or channel decomposition or selection) and
 *	    has nothing to do with the user's layout.  Many things don't
 *	    apply to internal cells (such as design-rule checking).
 *	CDGETNEWSTAMP means the cell has been modified since the
 *	    last time a timestamp was assigned to it, so it needs
 *	    to have a new stamp assigned.
 *	CDSTAMPSCHANGED means that a timestamp has changed in one or
 *	    more children of this cell.
 *	CDBOXESCHANGED means that bounding boxes of one or more children
 *	    have changed.
 */

#define	CDAVAILABLE	001
#define	CDMODIFIED	002
#define	CDNOTFOUND	004
#define CDINTERNAL	010
#define CDGETNEWSTAMP	020
#define CDSTAMPSCHANGED	040
#define CDBOXESCHANGED	0100

/*
 * Description of an array.
 * The bounds xlo .. xhi and ylo .. yhi are transformed versions
 * of the bounds xlo' .. xhi' and ylo' .. yhi' supplied by the
 * user:
 *
 * User supplies:
 *	xlo'	index of leftmost array element in root coordinates
 *	xhi'	index of rightmost array element in root coordinates
 *	ylo'	index of bottommost array element in root coordinates
 *	yhi'	index of topmost array element in root coordinates
 *
 * There is no constraint on the order of any of these indices; xlo' may
 * be less than, equal to, or greater than xhi', and similarly for ylo'
 * and yhi'.
 *
 * In addition, the separations xsep and ysep are transformed versions
 * of the separations xsep' and ysep' supplied by the user:
 *
 * User supplies:
 *	xsep'	(positive) X spacing between array elements in root coords
 *	ysep'	(positive) Y spacing between array elements in root coords
 *
 * When the array is made via DBMakeArray, both the indices and the spacings
 * are transformed down to the coordinates of the CellDef that is the child
 * of the use containing the ArrayInfo.
 *
 * The significance of the various values is as follows:  the [xlo, ylo]	
 * element of the array is gotten by transforming the celldef by the
 * transformation in the celluse.  the [x, y] element is gotten by
 * transforming the celldef by xsep*abs(x-xlo) in x, ysep*abs(y-ylo) in
 * y, and then transforming by the transformation in the celluse.
 */

typedef struct
{
    int		ar_xlo, ar_xhi;		/* Inclusive low/high X bounds */
    int		ar_ylo, ar_yhi;		/* Inclusive low/high Y bounds */
    int		ar_xsep, ar_ysep;	/* X,Y sep between array elements */
} ArrayInfo;

/*
 * Since a cell may be used in an orientation different from that
 * in which it was defined, each cell use contains a transform
 * that will generate coordinates in the world of the parent from
 * the coordinates in the world of the child.  Cells may also be
 * arrayed.  Note:  arraying occurs before the transformation, then
 * the entire array is transformed.
 */

typedef struct celluse
{
    unsigned		 cu_expandMask;	/* Mask of windows in which this use
					 * is expanded.
					 */
    unsigned		 cu_flags;	/* CURRENTLY UNUSED */
    Transform		 cu_transform;	/* Transform to parent coordinates */
    char		*cu_id;		/* Unique identifier of this use */
    ArrayInfo		 cu_array;	/* Arraying information */
    CellDef		*cu_def;	/* The definition of the cell */
    struct celluse	*cu_nextuse;	/* Next in list of uses of our def,
					 * or NULL for end of list.
					 */
    CellDef		*cu_parent;	/* Cell def containing this use */
    Rect		 cu_bbox;	/* Bounding box of this use, with
					 * arraying taken into account, in
					 * coordinates of the parent def.
					 */
    ClientData		 cu_client;	/* This space for rent */
    int			 cu_delta;	/* Used by snowplow */
} CellUse;

#define	cu_xlo	cu_array.ar_xlo
#define	cu_ylo	cu_array.ar_ylo
#define	cu_xhi	cu_array.ar_xhi
#define	cu_yhi	cu_array.ar_yhi
#define	cu_xsep	cu_array.ar_xsep
#define	cu_ysep	cu_array.ar_ysep

/*
 * The field cu_expandMask contains an expansion mask with one bit set
 * for each window in which the cellUse is to be displayed as expanded.
 */

#define	DBIsExpand(use, mask)	(((use)->cu_expandMask & (mask)) == (mask))

/*
 * All subcells used by a cell are part of another tile plane,
 * called the `cell plane'.  To handle overlap, each tile in
 * this plane points to a list of cell uses that appear in the
 * area covered by the tile.  Where there is overlap, this list
 * will contain more than one tile.
 */

typedef struct celltilebody
{
    CellUse		*ctb_use;	/* Cell used */
    struct celltilebody	*ctb_next;	/* Next tile body on list */
} CellTileBody;

/* -------------------- Search context information -------------------- */

/* Search contexts are used in hierarchical searches */
typedef struct
{
    CellUse	*scx_use;	/* Pointer to cell use currently searched */
    int		 scx_x, scx_y;	/* X and Y array elementS if scx_use is array */
    Rect	 scx_area;	/* Area searched in scx_use->cu_def coords */
    Transform	 scx_trans;	/* Composite transform from coordinates
				 * of the cell use (scx_use) all the way
				 * back to those of the "root" of the
				 * search.
				 */
} SearchContext;

/* ------------------- Pathname of a terminal (label) ----------------- */

/* The following structure is used to build hierarchical label names */
typedef struct
{
    char	*tp_first;	/* Pointer to first character in pathname */
    char	*tp_next;	/* Pointer to next character to be filled in */
    char	*tp_last;	/* Pointer to last available character slot
				 * in pathname.
				 */
} TerminalPath;

/* --------------- Contexts for hierarchical tile searches ------------ */

/*
 * The TreeContext is the glue which holds together the SearchContext
 * of a given search (which varies depending on where in the search we
 * happen to be) and the TreeFilter (see below) which does not.
 */
typedef struct treeContext
{
    SearchContext *tc_scx;		/* Search context (varies) */
    struct treeFilter *tc_filter;	/* Constant search criteria */
} TreeContext;

/*
 * The TreeFilter is that portion of the argument to the
 * filter procedures associated with tree searches that does
 * not change during the entire search, but serves mainly to
 * pass the search criteria down to the filter functions.
 */
typedef struct treeFilter
{
    int (*tf_func)();		/* Client's filter function */
    ClientData tf_arg;		/* Client's argument to pass to filter */
    TileTypeBitMask *tf_mask;	/* Only process tiles with these types */
    int tf_xmask;		/* Expand mask */
    int tf_planes;		/* Mask of planes which will be visited */
    TerminalPath *tf_tpath;	/* Buffer to hold hierarchical label names */
} TreeFilter;

/* -------------- Undo information passed to DBPaintPlane ------------- */

typedef struct
{
    CellDef *pu_def;	/* Cell definition being modified */
    int pu_pNum;	/* Index of plane within cell def */
} PaintUndoInfo;

/* ---------------------- Codes for paint/erase ----------------------- */

    /* The following are obsolete and will go away */
#define	ERASE	0	/* Erase type from existing tiles */
#define	PAINT	1	/* Paint type over existing tiles */
#define	WRITE	2	/* Write type unconditionally */

/* -------------------- Exported procedure headers -------------------- */

    /* Painting/erasing */
extern void DBPaint();
extern void DBErase();
extern Void DBPaintPlane();
extern Void DBPaintPlaneByProc();
extern Void DBPaintPlaneMergeOnce();
extern void DBPaintMask();
extern void DBEraseMask();
extern void DBClearPaintPlane();

    /* I/O */
extern bool DBCellRead();
extern char *DBGetTech();
extern bool DBCellWrite();
extern void DBCellReadArea();

    /* Labels */
extern int DBPutLabel();
extern bool DBEraseLabel();
extern void DBEraseLabelAll();
extern void DBEraseLabelsByContent();
extern void DBEraseLabelsByFunction();
extern void DBReOrientLabel();
extern void DBAdjustLabels();
extern TileType DBPickLabelLayer();

    /* Technology initialization */
extern int DBTechInit();
extern bool DBTechSetTech();
extern bool DBTechAddPlane();
extern bool DBTechAddType();
extern Void DBTechFinalType();
extern bool DBTechAddConnect();
extern bool DBTechAddContact();
extern bool DBTechAddCompose();
extern TileType DBTechNameType(), DBTechNoisyNameType();
extern int DBTechNamePlane(), DBTechNoisyNamePlane();
extern void DBTechNoisyNameMask();
extern int DBTechMinSetPlanes();
extern bool DBTechSubsetLayers();
extern Void DBTechInitPlane();
extern Void DBTechInitType();
extern Void DBTechInitCompose();
extern Void DBTechFinalCompose();
extern Void DBTechInitContact();
extern Void DBTechFinalContact();
extern Void DBTechFinalConnect();
extern Void DBTechInitConnect();

    /* Cell symbol table */
extern void DBCellInit();
extern CellDef *DBCellLookDef();
extern CellDef *DBCellNewDef();
extern CellDef *DBCellDefAlloc();
extern bool DBCellRenameDef();
extern bool DBCellDeleteDef();
extern void DBCellDefFree();
extern int DBCellSrDefs();
extern CellUse *DBCellNewUse();
extern bool DBCellDeleteUse();
extern CellUse *DBCellFindDup();

    /* Cell selection */
extern CellUse *DBSelectCell();
extern CellUse *DBFindUse();

    /* Insertion/deletion of cell uses into the cell tile plane of a parent */
extern void DBPlaceCell();
extern void DBDeleteCell();
extern void DBClearCellPlane();

    /* Insertion/deletion of cell uses into the name space of a parent */
extern bool DBLinkCell();
extern void DBUnlinkCell();

    /* Bounding boxes and arrays */
extern bool DBBoundPlane();
extern void DBReComputeBbox();
extern void DBReComputeBboxVert();
extern void DBMakeArray();
extern void DBSetArray();
extern void DBSetTrans();
extern void DBArrayOverlap();
extern void DBComputeArrayArea();
extern Transform *DBGetArrayTransform();
extern char *DBPrintUseId();

    /* Massive copying */

extern void DBCellCopyPaint();
extern void DBCellCopyAllPaint();
extern void DBCellCopyLabels();
extern void DBCellCopyAllLabels();
extern void DBCellCopyCells();
extern void DBCellCopyAllCells();

    /* Miscellaneous */
extern void DBCellClearDef();
extern void DBCellCopyDefBody();
extern void DBExpandAll(), DBExpand();
extern bool DBIsAncestor();
extern void DBCellSetAvail();
extern void DBCellClearAvail();
extern bool DBCellGetModified();
extern void DBCellSetModified();
extern void DBFixMismatch();
extern void DBTreeCopyConnect();
extern Plane *DBNewPlane();
extern TileType DBImageOnPlane();
extern PaintResultType (*DBNewPaintTable())[TT_MAXTYPES][TT_MAXTYPES];
typedef Void (*VoidProc)();
VoidProc DBNewPaintPlane();
extern void DBTechPrintTypes();

    /* Deallocation */
extern void DBClearCellPlane();
extern void DBClearPaintPlane();
extern void DBFreeCellPlane();
extern void DBFreePaintPlane();

    /* Cell properties */
extern void DBPropPut();
extern ClientData DBPropGet();
extern int DBPropEnum();
extern void DBPropClearAll();

    /* Searching */
extern int DBTreeSrTiles();
extern int DBNoTreeSrTiles();
extern int DBTreeSrLabels();
extern int DBTreeSrCells();
extern int DBSrRoots();
extern int DBCellEnum();
extern int DBArraySr();
extern bool DBNearestLabel();
extern int DBSrLabelLoc();

/* -------------------- Exported magic file suffix -------------------- */

extern char *DBSuffix;		/* Suffix appended to all Magic cell names */

/* -------------------- User Interface Stuff -------------------------- */

extern bool DBVerbose;		/* If FALSE, don't print warning messages */

/* ------------------ Exported technology variables ------------------- */

/***
 *** The following variables should be considered
 *** read-only to all clients of the database module.
 ***/

    /* Name of the current technology */
extern char *DBTechName;

    /*
     * Predefined masks of tile types.
     * The number of built-in types is just TT_TECHDEPBASE.
     */
extern TileTypeBitMask DBZeroTypeBits;		/* All zeroes */
extern TileTypeBitMask DBAllTypeBits;		/* All ones */
extern TileTypeBitMask DBBuiltinLayerBits;	/* All built-in types */
extern TileTypeBitMask DBAllButSpaceBits;	/* All but space */
extern TileTypeBitMask DBAllButSpaceAndDRCBits;	/* All but space and drc */
extern TileTypeBitMask DBSpaceBits;		/* Space only */
extern TileTypeBitMask DBContactBits;		/* Paintable contacts */
extern TileTypeBitMask DBImageBits;		/* All contact images */

    /*
     * Number of tile types, including those specied by the technology
     * file and those built-in to Magic, but not including those automatically
     * generated to represent contact images.  Also, a mask of those
     * types contained in the technology file.
     */
extern int DBNumUserLayers;
extern TileTypeBitMask DBUserLayerBits;		/* Including space */
extern TileTypeBitMask DBNonSpaceUserLayerBits;	/* Excluding space */

    /* Total number of Magic tile types in this technology */
extern int DBNumTypes;

    /* Total number of tile planes */
extern int DBNumPlanes;

/* Abbreviations */
#define	NT	TT_MAXTYPES
#define	NP	PL_MAXTYPES

    /* Gives the official long name of each plane: */
extern char		*DBPlaneLongNameTbl[NP];

    /* Gives a short name for each plane: */
extern char		*DBPlaneShortName();

    /* Gives for each plane a mask of all tile types stored in that plane: */
extern TileTypeBitMask	DBPlaneTypes[NP];

    /* Gives a TileTypeBitMask for everything that connects to a type.
     * Bit x is set in mask DBConnectTbl[y] if any image of x connects
     * to any image of y.
     */
extern TileTypeBitMask	DBConnectTbl[NT];

    /* Complement of above: everything not connected to a type */
extern TileTypeBitMask	DBNotConnectTbl[NT];

    /* Mask of all types that are components of a given type.  If
     * TTMaskHasType(&DBComponentTbl[r], s), then painting s over
     * r gives r.  For example, each of the residues of a contact
     * is a component of the contact.
     */
extern TileTypeBitMask DBComponentTbl[NT];

    /*
     * Gives a TileTypeBitMask of all types that correspond to a given
     * layer.  Used for contacts that have images in more than one
     * plane.
     */
extern TileTypeBitMask	DBLayerTypeMaskTbl[NT];

    /*
     * Gives a plane mask of all planes that have tiles that connect to
     * tiles of a given type.  This is only used for contacts; it tells
     * which other planes contain images of the contact.  A tile's own
     * plane is never included in its plane mask.  Non-contact types
     * always have 0 DBConnPlanes entries.
     */
extern unsigned int	DBConnPlanes[NT];

    /*
     * Similar to DBConnPlanes[], but this time it is non-zero
     * for those planes to which each type connects, exclusive
     * of its home plane and those planes to which it connects as a
     * contact.  These planes needn't be adjacent.  For example, a
     * p-well is connected to p-substrate diffusion that overlaps it;
     * also, p well contacts connect to a p-well.  This information
     * is currently used only in the circuit extractor.
     */
extern unsigned int	DBAllConnPlanes[NT];

    /*
     * Each TileType has a home plane.  The TileType only appears on
     * its home plane.  The only exception is TT_SPACE, which can appear
     * on any plane.  (For debugging, there may be other cases).
     *
     * DBTypePlaneTbl gives the home plane for a given TileType,
     * and DBTypePlaneMaskTbl gives a mask of the planes on which
     * it appears (all planes for TT_SPACE, one plane for others).
     */
extern int		DBTypePlaneTbl[TT_MAXTYPES];
extern int		DBTypePlaneMaskTbl[TT_MAXTYPES];

    /* Gives the long name for each tile type: */
extern char		*DBTypeLongNameTbl[TT_MAXTYPES];

    /* Gives a short name for a tile type: */
extern char		*DBTypeShortName();

    /*
     * The following give masks of all planes that may be affected
     * when material of a given type is painted/erased:
     */
extern unsigned int	DBTypePaintPlanesTbl[TT_MAXTYPES];
extern unsigned int	DBTypeErasePlanesTbl[TT_MAXTYPES];

    /*
     * Gives the resulting tile type when one tile type is painted over
     * another in a given plane:
     *
     *	newType = DBPaintResult[pNum][paintType][oldType]
     */
extern PaintResultType	DBPaintResultTbl[NP][NT][NT];

    /*
     * Gives the resulting tile type when one tile type is erased over
     * another in a given plane:
     *
     *	newType = DBEraseResult[pNum][paintType][oldType]
     */
extern PaintResultType	DBEraseResultTbl[NP][NT][NT];

    /*
     * Gives the resulting tile type when one tile type is 'written'
     * over a given plane.  This corresponds to the case where the
     * written type replaces the old tile without regard to the type
     * of the old tile.
     *
     *	paintType = DBWriteResultTbl[paintType][oldType]
     */
extern PaintResultType	DBWriteResultTbl[NT][NT];

/* --------------------- Exported macros ------------------------------ */

    /*
     * Macros for reading the paint/erase tables:
     *	resultType = DBStdPaintEntry(oldType, paintType, planeNum)
     *	resultType = DBStdEraseEntry(oldType, paintType, planeNum)
     */
#define	DBStdPaintEntry(h,t,p) 	(DBPaintResultTbl[p][t][h])
#define	DBStdEraseEntry(h,t,p)	(DBEraseResultTbl[p][t][h])

    /*
     * Macros for constructing the pointer to pass to DBPaintPlane
     * as the result table.
     */
#define	DBStdPaintTbl(t,p)	(&DBPaintResultTbl[p][t][0])
#define	DBStdEraseTbl(t,p)	(&DBEraseResultTbl[p][t][0])
#define DBStdWriteTbl(t)	(&DBWriteResultTbl[t][0])

    /*
     * int DBPlane(type) TileType type;
     * Returns the home plane of 'type'.
     */
#define	DBPlane(type)		(DBTypePlaneTbl[type])

    /*
     * char *DBTypeLongName(type) TileType type;
     * Returns the long name of 'type'.
     */
#define	DBTypeLongName(type)	(DBTypeLongNameTbl[type])

    /*
     * char *DBPlaneLongName(p) int p;
     * Returns the long name of plane 'plane'.
     */
#define	DBPlaneLongName(p)	(DBPlaneLongNameTbl[p])

    /*
     * bool DBConnectsTo(t1, t2) TileType t1, t2;
     * Returns TRUE if types 't1' and 't2' are electrically connected.
     */
#define	DBConnectsTo(t1, t2)	(TTMaskHasType(&DBConnectTbl[t1], t2))

    /*
     * bool DBPaintOnPlane(t, p) TileType t; int p;
     * bool DBEraseOnPlane(t, p) TileType t; int p;
     *
     * Return TRUE if tile type 't' has any effect on plane 'p'
     * when being painted/erased.  Always FALSE if 't' is the image
     * of a contact on a plane other than its base.
     */
#define	DBPaintOnPlane(t, p)	(PlaneMaskHasPlane(DBTypePaintPlanesTbl[t], p))
#define	DBEraseOnPlane(t, p)	(PlaneMaskHasPlane(DBTypeErasePlanesTbl[t], p))
