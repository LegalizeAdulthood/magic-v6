/*
 * extractInt.h --
 *
 * Defines things shared internally by the extract module of Magic,
 * but not generally needed outside the extract module.
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
 * rcsid "$Header: extractInt.h,v 6.0 90/08/28 18:15:43 mayo Exp $"
 */

#define _EXTINT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err1 = Need_to_include_database_header;
#endif	_DATABASE
#ifndef	_TILES
int err2 = Need_to_include_tile_header;
#endif	_TILES
#ifndef	_HASH
int err3 = Need_to_include_hash_header;
#endif	_HASH
#ifndef	_GEOMETRY
int err4 = Need_to_include_geometry_header;
#endif	_GEOMETRY

#undef	NT
#define	NT	TT_MAXTYPES
#undef	NP
#define	NP	PL_MAXTYPES

/* -------------------------- Label lists ----------------------------- */

/*
 * List of labels for a node.
 * We keep around pointers to the entire labels for
 * later figuring out which are attached to the gates,
 * sources, or drains of transistors.
 */
typedef struct ll
{
    Label	*ll_label;	/* Actual Label in the source CellDef */
    struct ll	*ll_next;	/* Next LabelList in this region */
    int		 ll_attr;	/* Which terminal of a transistor this is
				 * an attribute of.
				 */
} LabelList;

#define	LL_NOATTR	-1	/* Value for ll_attr above if the label is
				 * not a transistor attribute.
				 */
#define	LL_GATEATTR	-2	/* Value for ll_attr if the label is a gate
				 * attribute, rather than one of the diffusion
				 * terminals' attributes.
				 */

/*
 * Types of labels.
 * These can be or'd into a mask and passed to extLabType().
 */
#define	LABTYPE_NAME		0x01	/* Normal node name */
#define	LABTYPE_NODEATTR	0x02	/* Node attribute */
#define	LABTYPE_GATEATTR	0x04	/* Transistor gate attribute */
#define	LABTYPE_TERMATTR	0x08	/* Transistor terminal (source/drain)
					 * attribute.
					 */

/* ----------------------------- Regions ------------------------------ */

/*
 * The following are the structures built up by the various
 * clients of ExtFindRegions.  The general rule for these
 * structures is that their initial fields must be identical
 * to those in a Region, but subsequent fields are up to
 * the individual client.
 *
 * Regions marked as GENERIC are the types accepted by
 * procedures in ExtRegion.c.
 */

    /*
     * GENERIC Region struct.
     * All this provides is a pointer to the next Region.
     * This is the type passed to functions like ExtFreeRegions,
     * and is the type returned by ExtFindRegions.  Clients should
     * cast pointers of this type to their own, client type.
     */
typedef struct reg
{
    struct reg	*reg_next;	/* Next region in list */
} Region;

    /*
     * GENERIC region with labels.
     * Any other structure that wants to reference node names
     * must include the same fields as this one as its first part.
     */
typedef struct lreg
{
    struct lreg	*lreg_next;	/* Next region in list */
    int		 lreg_pnum;	/* Lowest numbered plane in this region */
    int		 lreg_type;	/* Type of tile that contains lreg_ll */
    Point	 lreg_ll;	/* Lower-leftmost point in this region on
				 * plane lreg_pnum.  We take the min first
				 * in X, then in Y.
				 */
    LabelList	*lreg_labels;	/* List of labels for this region.  These are
				 * any labels connected to the geometry making
				 * up this region.  If the list is empty, make
				 * up a name from lreg_pnum and lreg_ll.
				 */
} LabRegion;

    /*
     * Node region: labelled region with resistance and capacitance.
     * Used for each node in the flat extraction of a cell.
     */

typedef struct
{
    int		 pa_perim;
    int		 pa_area;
} PerimArea;

typedef struct nreg
{
    struct nreg	*nreg_next;	/* Next region in list */
    int		 nreg_pnum;	/* Lowest numbered plane in this region */
    int		 nreg_type;	/* Type of tile that contains nreg_ll */
    Point	 nreg_ll;	/* Lower-leftmost point in this region on
				 * plane nreg_pnum.  We take the min first
				 * in X, then in Y.
				 */
    LabelList	*nreg_labels;	/* See LabRegion for description */
    int		 nreg_cap;	/* Capacitance to ground */
    int		 nreg_resist;	/* Resistance estimate */
    PerimArea	 nreg_pa[1];	/* Dummy; each node actually has
				 * ExtCurStyle->exts_numResistClasses
				 * array elements allocated to it.
				 */
} NodeRegion;

    /*
     * Transistor region: labelled region with perimeter and area.
     * Used for each transistor in the flat extraction of a cell.
     */
typedef struct treg
{
    struct treg	*treg_next;	/* Next region in list */
    int		 treg_pnum;	/* UNUSED */
    int		 treg_type;	/* Type of tile that contains treg_ll */
    Point	 treg_ll;	/* UNUSED */
    LabelList	*treg_labels;	/* Attribute list */
    Tile	*treg_tile;	/* Some tile in the channel */
    int		 treg_area;	/* Area of channel */
} TransRegion;

/*
 * The following is used to map the full coordinate space into
 * the positive integers, for constructing internally generated
 * node names.
 */
#define	extCoord(x)	(((x) < 0) ? (1 - ((x) << 1)) : ((x) << 1))

/*
 * The following updates reg->lreg_ll and reg->lreg_pnum so that
 * they are always the lowest leftmost coordinate in a cell, on
 * the plane with the lowest number.
 */
#define	extSetNodeNum(reg, pn, tp) \
    if (1) { \
	if ((pn) < (reg)->lreg_pnum) \
	{ \
	    (reg)->lreg_type = TiGetType(tp); \
	    (reg)->lreg_pnum = (pn); \
	    (reg)->lreg_ll = (tp)->ti_ll; \
	} \
	else if ((pn) == (reg)->lreg_pnum) \
	{ \
	    if (LEFT(tp) < (reg)->lreg_ll.p_x) \
	    { \
		(reg)->lreg_ll = (tp)->ti_ll; \
		(reg)->lreg_type = TiGetType(tp); \
	    } \
	    else if (LEFT(tp) == (reg)->lreg_ll.p_x \
		    && BOTTOM(tp) < (reg)->lreg_ll.p_y) \
	    { \
		(reg)->lreg_ll.p_y = BOTTOM(tp); \
		(reg)->lreg_type = TiGetType(tp); \
	    } \
	} \
    } else

/*
 * The following constructs a node name from the plane number 'n'
 * and lower left Point l, and places it in the string 's' (which must
 * be large enough).
 */
#define	extMakeNodeNumPrint(s, n, l) \
    (void) sprintf((s), "%d_%d_%d#", (n), extCoord((l).p_x), extCoord((l).p_y))

/*
 * Argument passed to filter functions for finding regions.
 */
typedef struct
{
    TileTypeBitMask	*fra_connectsTo; /* Array of TileTypeBitMasks.  The
					  * element fra_connectsTo[t] has a
					  * bit set for each type that
					  * connects to 't'.
					  */
    CellDef		*fra_def;	 /* Def being searched */
    int			 fra_pNum;	 /* Plane currently searching */
    ClientData		 fra_uninit;	 /* This value appears in the ti_client
					  * field of a tile if it's not yet
					  * been visited.
					  */
    Region	      *(*fra_first)();	 /* Function to init new region */
    int		       (*fra_each)();	 /* Function for each tile in region */
    Region		*fra_region;	 /* Ptr to Region struct for current
					  * region.  May be set by fra_first
					  * and used by fra_each.
					  */
} FindRegion;

#define	TILEAREA(tp)	((TOP(tp) - BOTTOM(tp)) * (RIGHT(tp) - LEFT(tp)))

/* -------------------- Perimeter of a region ------------------------- */

/*
 * Segment of the boundary of a region whose perimeter
 * is being traced by ExtTracePerimeter() and extEnumTilePerim().
 */
typedef struct
{
    Tile	*b_inside;	/* Pointer to tile just inside segment */
    Tile	*b_outside;	/* Pointer to tile just outside segment */
    Rect	 b_segment;	/* Actual coordinates of segment */
    int		 b_direction;	/* Direction following segment (see below) */
    int		 b_plane;	/* extract argument for extSideOverlap   */
} Boundary;

#define	BoundaryLength(bp) \
	((bp)->b_segment.r_xtop - (bp)->b_segment.r_xbot \
    +    (bp)->b_segment.r_ytop - (bp)->b_segment.r_ybot)

/* Directions in which we can be following the boundary of a perimeter */

#define	BD_UP		0	/* Inside is to right */
#define	BD_LEFT		1	/* Inside is below */
#define	BD_DOWN		2	/* Inside is to left */
#define	BD_RIGHT	3	/* Inside is above */

/* -------- Yank buffers for hierarchical and array extraction -------- */

extern CellUse *extYuseCum;
extern CellDef *extYdefCum;

/* --------------- Argument passed to extHierYankFunc ----------------- */

typedef struct
{
    Rect	*hy_area;	/* Area (in parent coordinates) to be yanked */
    CellUse	*hy_target;	/* Yank into this use */
    bool	 hy_prefix;	/* If TRUE, prefix labels with use id */
} HierYank;

/* ----- Arguments to filter functions in hierarchical extraction ---- */

    /*
     * The following defines an extracted subtree.
     * The CellUse et_use will be either a cell we are extracting,
     * or a flattened subtree.  If et_lookNames is non-NULL, it
     * points to a CellDef that we should look in for node names.
     */
typedef struct extTree
{
    CellUse		*et_use;	/* Extracted cell, usually flattened */
    CellUse		*et_realuse;	/* If et_use is flattened, et_realuse
					 * points to the unflattened subtree's
					 * root use; otherwise it is NULL.
					 */
    CellDef		*et_lookNames;	/* See above */
    NodeRegion		*et_nodes;	/* List of nodes */
    HashTable		 et_coupleHash;	/* Table for coupling capacitance */
    struct extTree	*et_next;	/* Next one in list */
} ExtTree;

    /*
     * The following structure contains information passed down
     * through several levels of filter functions during hierarchical
     * extraction.
     *
     * The procedure ha_nodename is used to map from a tile into the
     * name of the node to which that tile belongs.  It should be of
     * the following format:
     *
     *	char *
     *	proc(tp, et, ha)
     *	    Tile *tp;
     *	    ExtTree *et;
     *	    HierExtractArg *ha;
     *	{
     *	}
     *
     * It should always return a non-NULL string; if the name of a
     * node can't be determined, the string can be "(none)".
     */
typedef struct
{
    FILE	*ha_outf;	/* The .ext file being written */
    CellUse	*ha_parentUse;	/* Use pointing to the def being extracted */
    char      *(*ha_nodename)();/* Map (tp, et, ha) into nodename; see above */
    ExtTree	 ha_cumFlat;	/* Cumulative yank buffer */
    HashTable	 ha_connHash;	/* Connections made during hier processing */

/* All areas are in parent coordinates */

    Rect	 ha_interArea;	/* Area of whole interaction being considered */
    Rect	 ha_clipArea;	/* Only consider capacitance, perimeter, and
				 * area that come from inside this area.  This
				 * rectangle is contained within ha_interArea.
				 */
    CellUse	*ha_subUse;	/* Root of the subtree being processed now */
    Rect	 ha_subArea;	/* Area of ha_subUse inside the interaction
				 * area, i.e, contained within ha_interArea.
				 */
} HierExtractArg;

/*
 * Normally, nodes in overlapping subcells are expected to have labels
 * in the area of overlap.  When this is not the case, we have to use
 * a much more expensive algorithm for finding the labels attached to
 * the subcells' geometry in the overlap area.  The following structure
 * is used to hold information about the search in progress for such
 * labels.
 */
typedef struct
{
    HierExtractArg	*hw_ha;		/* Describes context of search */
    Label		*hw_label;	/* We update hw_label with a ptr to a
					 * newly allocated label if successful.
					 */
    Rect		 hw_area;	/* Area in parent coordinates of the
					 * area where we're searching.
					 */
    bool		 hw_autogen;	/* If TRUE, we trace out all geometry
					 * in the first node in the first cell
					 * found to overlap the search area,
					 * and use the internal name for that
					 * node.
					 */
    TerminalPath	 hw_tpath;	/* Hierarchical path down to label
					 * we are searching for, rooted at
					 * the parent being extracted.
					 */
    TileTypeBitMask	 hw_mask;	/* Mask of tile types that connect to
					 * the tile whose node is to be found,
					 * and which are on the same plane.
					 * Used when calling ExtFindRegions.
					 */
    bool		 hw_prefix;	/* If FALSE, we skip the initial
					 * use identifier when building
					 * hierarchical labels (as when
					 * extracting arrays; see hy_prefix
					 * in the HierYank struct).
					 */
    int		       (*hw_proc)();
} HardWay;

/* --------------------- Coupling capacitance ------------------------- */

/*
 * The following structure is the hash key used for computing
 * internodal coupling capacitance.  Each word is a pointer to
 * one of the nodes being coupled.  By convention, the first
 * word is the lesser of the two NodeRegion pointers.
 */
typedef struct
{
    NodeRegion	*ck_1, *ck_2;
} CoupleKey;

/* ------------------ Interface to debugging module ------------------- */

extern ClientData extDebugID;	/* Identifier returned by the debug module */

/* ----------------- Technology-specific information ------------------ */

/*
 * Structure used to define sidewall coupling capacitances.
 */
typedef struct edgecap
{
    struct edgecap	*ec_next;	/* Next edge capacitance rule in list */
    int			 ec_cap;	/* Capacitance (attofarads) */
    TileTypeBitMask	 ec_near;	/* Types closest to causing edge */
    TileTypeBitMask	 ec_far;	/* Types farthest from causing edge */
    int			 ec_plane;	/* if ec_near is TT_SPACE, ec_plane */
    					/* specifies which plane is to be   */
					/* used.  -1 if ec_near isn't space */
} EdgeCap;

/*
 * Parameters for the process being extracted.
 * We use integers here, rather than floats, to be nice to
 * machines like Sun workstations that don't have hardware
 * floating point.
 */

typedef struct extstyle
{
    struct extstyle	*exts_next;	/* Next style in list */
    char		*exts_name;	/* Name of this style */

    /*
     * Connectivity tables.
     * Each table is an array of TileTypeBitMasks indexed by TileType.
     * The i-th element of each array is a mask of those TileTypes
     * to which type 'i' connects.
     */

	/* Everything is connected to everything else in this table */
    TileTypeBitMask	 exts_allConn[NT];

	/*
	 * Connectivity for determining electrical nodes.
	 * This should be essentially the same as DBConnectTbl[].
	 */
    TileTypeBitMask	 exts_nodeConn[NT];

	/*
	 * Connectivity for determining resistive regions.
	 * Two types should be marked as connected here if
	 * they are both connected in exts_nodeConnect[], and
	 * if they both have the same resistance per square.
	 */
    TileTypeBitMask	 exts_resistConn[NT];

	/*
	 * Connectivity for determining transistors.
	 * Each transistor type should connect only to itself.
	 * Nothing else should connect to anything else.
	 */
    TileTypeBitMask	 exts_transConn[NT];

    /*
     * Sheet resistivity for each tile type, in milli-ohms per square.
     * For types that are transistors or capacitors, this corresponds
     * to the sheet resistivity of the gate.
     */

	/* Maps from a tile type to the index of its sheet resistance entry */
    int			 exts_typeToResistClass[NT];

	/* Gives a mask of neighbors of a type with different resistivity */
    TileTypeBitMask	 exts_typesResistChanged[NT];

	/*
	 * Resistance information is also provided by the following tables:
	 * exts_typesByResistClass[] is an array of masks of those types
	 * having the same sheet resistivity, for each different value
	 * of sheet resistivity; exts_resistByResistClass[] is a parallel array
	 * giving the actual value of sheet resistivity.  Both are indexed
	 * from 0 up to (but not including) exts_numResistClasses.
	 */
    TileTypeBitMask	 exts_typesByResistClass[NT];
    int			 exts_resistByResistClass[NT];
    int			 exts_numResistClasses;

	/* Resistance per type */
    int			 exts_sheetResist[NT];

	/*
	 * Resistances for via holes.
	 * These have two parts: the size of a minimum-size contact
	 * (it's assumed to be square), and the resistance of its
	 * via hole.  The resistance is given in milliohms.
	 */
    int			 exts_viaSize[NT];
    int			 exts_viaResist[NT];

    /*
     * Capacitance to substrate for each tile type, in units of
     * attofarads per square lambda.
     */

	/*
	 * Capacitance per unit area.  This is zero for explicit capacitor
	 * types, which handle gate-channel capacitance specially.  For
	 * transistor types, this is at best an approximation that is
	 * truly valid only when the transistor is switched off.
	 */
    int			 exts_areaCap[NT];

	/*
	 * Capacitance per unit perimeter.  Sidewall capacitance depends both
	 * on the type inside the perimeter as well as the type outside it,
	 * so the table is doubly indexed by TileType.
	 *
	 * The mask exts_perimCapMask[t] contains bits for all those TileTypes
	 * 's' such that exts_perimCap[t][s] is nonzero.
	 */
    int			 exts_perimCap[NT][NT];
    TileTypeBitMask	 exts_perimCapMask[NT];

    /*
     * Overlap coupling capacitance for each pair of tile types, in units
     * of attofarads per square lambda of overlap.
     * Internodal capacitance due to overlap only occurs between tile
     * types on different tile planes that are not shielded by intervening
     * tiles.
     */

	/*
	 * The mask exts_overlapPlanes is a mask of those planes that must
	 * be searched for tiles having overlap capacitance, and the mask
	 * exts_overlapTypes[p] is those types having overlap capacitance
	 * on each plane p.  The intent is that exts_overlapTypes[p] lists
	 * only those types t for which some entry of exts_overlapCap[t][s]
	 * is non-zero.
	 */
    int			 exts_overlapPlanes;
    TileTypeBitMask	 exts_overlapTypes[NP];

	/*
	 * The mask exts_overlapOtherPlanes[t] is a mask of the planes that
	 * must be searched for tiles having overlap capacitance with tiles
	 * of type 't', and exts_overlapOtherTypes[t] is a mask of the types
	 * with which our overlap capacitance is non-zero.
	 */
    TileTypeBitMask	 exts_overlapOtherTypes[NT];
    int			 exts_overlapOtherPlanes[NT];
    
	/*
	 * Both exts_overlapShieldTypes[][] and exts_overlapShieldPlanes[][]
	 * are indexed by the same pair of types used to index the table
	 * exts_overlapCap[][]; they identify the types and planes that
	 * shield capacitance between their index types.
	 */
    TileTypeBitMask	 exts_overlapShieldTypes[NT][NT];
    int			 exts_overlapShieldPlanes[NT][NT];

	/*
	 * The table extOverlapCap[][] is indexed by two types to give the
	 * overlap coupling capacitance between them, per unit area.  Only
	 * one of extOverlapCap[i][j] and extOverlapCap[j][i] should be
	 * nonzero.  The capacitance to substrate of the tile of type 'i'
	 * is deducted when an overlap between i and j is detected, if
	 * extOverlapCap[i][j] is nonzero.
	 */
    int			 exts_overlapCap[NT][NT];


    /*
     * Sidewall coupling capacitance.  This capacitance is between edges
     * on the same plane, and is in units of attofarads.  It is multiplied
     * by the value interpolated from a fringing-field table indexed by the
     * common length of the pair of edges divided by their separation:
     *
     *		   |				|
     *		E1 +----------------------------+
     *				^
     *				+--- distance between edges
     *				v
     *			+-----------------------------------+ E2
     *			|				    |
     *
     *			<-----------------------> length in common
     */

	/*
	 * The entry exts_sideCoupleCap[i][j] is a list of the coupling
	 * capacitance info between edges with type 'i' on the inside
	 * and 'j' on the outside, and other kinds of edges.
	 */
    EdgeCap		*exts_sideCoupleCap[NT][NT];

	/*
	 * exts_sideCoupleOtherEdges[i][j] is a mask of those types on the
	 * far sides of edges to which an edge with 'i' on the inside and
	 * 'j' on the outside has coupling capacitance.
	 */
    TileTypeBitMask	 exts_sideCoupleOtherEdges[NT][NT];

	/*
	 * We search out a distance exts_sideCoupleHalo from each edge
	 * for other types with which we have coupling capacitance.
	 * This value determines how much extra gets yanked when
	 * computing hierarchical adjustments, so should be kept
	 * small to insure reasonable performance.
	 */
    int			 exts_sideCoupleHalo;

    /*
     * Sidewall-overlap coupling capacitance.
     * This is between an edge on one plane and a type on another plane
     * that overlaps the edge (from the outside of the edge), and is in
     * units of attofarads per lambda.
     *
     * When an edge with sidewall capacitance to substrate is found to
     * overlap a type to which it has sidewall overlap capacitance, the
     * original capacitance to substrate is replaced with the overlap
     * capacitance to the tile overlapped.
     */

	/*
	 * The entry exts_sideOverlapCap[i][j] is a list of the coupling
	 * capacitance info between edges with type 'i' on the inside
	 * and 'j' on the outside, and other kinds of tiles on other
	 * planes.  Only the ec_near mask in the EdgeCap record is
	 * valid; it identifies the types to which we have sidewall
	 * overlap capacitance.
	 */
    EdgeCap		*exts_sideOverlapCap[NT][NT];

	/*
	 * extSideOverlapOtherTypes[i][j] is a mask of those types to which
	 * an edge with 'i' on the inside and 'j' on the outside has coupling
	 * capacitance.  extSideOverlapOtherPlanes[i][j] is a mask of those
	 * planes to which edge [i][j] has overlap coupling capacitance.
	 */
    int			 exts_sideOverlapOtherPlanes[NT][NT];
    TileTypeBitMask	 exts_sideOverlapOtherTypes[NT][NT];

    /* Common to both sidewall coupling and sidewall overlap */

	/*
	 * exts_sideTypes[p] is a mask of those types 't' having sidewall
	 * coupling or sidewall overlap capacitance on plane p (i.e, for
	 * which a bin in exts_sideCoupleCap[t][] or exts_sideOverlapCap[t][]
	 * is non-empty), and exts_sidePlanes a mask of those planes containing
	 * tiles in exts_sideTypes[].
	 */
    int			 exts_sidePlanes;
    TileTypeBitMask	 exts_sideTypes[NP];

	/*
	 * The mask exts_sideEdges[i] is just a mask of those types j for
	 * which either exts_sideCoupleCap[i][j] or exts_sideOverlapCap[i][j]
	 * is non-empty.
	 */
    TileTypeBitMask	 exts_sideEdges[NT];

    /* Transistors */

	/* Name of each transistor type as output in .ext file */
    char		*exts_transName[NT];

	/* Contains one for each type of fet, zero for all other types */
    TileTypeBitMask	 exts_transMask;

	/*
	 * Per-square resistances for each possible transistor type,
	 * in the various regions that such a type might operate.
	 * The only operating region currently used is "linear",
	 * which the resistance extractor uses in its thresholding
	 * operation.  NOTE: resistances in this table are in OHMS
	 * per square, not MILLIOHMS!
	 */
    HashTable		 exts_transResist[NT];
    int			 exts_linearResist[NT];

	/*
	 * Mask of the types of tiles that connect to the channel terminals
	 * of a transistor type.  The intent is that these will be the
	 * diffusion terminals of a transistor, ie, its source and drain.
	 */
    TileTypeBitMask	 exts_transSDTypes[NT];

	/*
	 * Maximum number of terminals (source/drains) per transistor type.
	 * This table exists to allow the possibility of transistors with
	 * more than two diffusion terminals at some point in the future.
	 */
    int			 exts_transSDCount[NT];

	/* Currently unused: gate-source capacitance per unit perimeter */
    int			 exts_transSDCap[NT];

	/* Currently unused: gate-channel capacitance per unit area */
    int			 exts_transGateCap[NT];

	/*
	 * Each type of transistor has a substrate node.  By default,
	 * it is the one given by exts_transSubstrateName[t].  However,
	 * if the mask exts_transSubstrateTypes[t] is non-zero, and if
	 * the transistor overlaps material of one of the types in the
	 * mask, then the transistor substrate node is the node of the
	 * material it overlaps.
	 */
    char		*exts_transSubstrateName[NT];
    TileTypeBitMask	 exts_transSubstrateTypes[NT];

    /* Scaling */
	/*
	 * Step size used when breaking up a large cell for interaction
	 * checks during hierarchical extraction.  We check exts_stepSize
	 * by exts_stepSize chunks for interactions one at a time.
	 */
    int			 exts_stepSize;

	/*
	 * Number of linear units per lambda.  All perimeter dimensions
	 * that we output to the .ext file should be multiplied by
	 * exts_unitsPerLambda; we produce a "scale" line in the .ext file
	 * indicating this.  All area dimensions should be multiplied
	 * by exts_unitsPerLambda**2.
	 */
    int			 exts_unitsPerLambda;

	/*
	 * Scaling for resistance and capacitance.
	 * All resistances in the .ext file should be multiplied by
	 * exts_resistScale to get milliohms, and all capacitances by
	 * exts_capScale to get attofarads.  These numbers appear in
	 * the "scale" line in the .ext file.
	 */
    int			 exts_capScale;
    int			 exts_resistScale;
} ExtStyle;

extern ExtStyle *ExtCurStyle;

/* ------------------- Hierarchical node merging ---------------------- */

/*
 * Table used to hold all merged nodes during hierarchical extraction.
 * Used for duplicate suppression.
 */
extern HashTable extHierMergeTable;

/*
 * Each hash entry in the above table points to a NodeName struct.
 * Each NodeName points to the Node corresponding to that name.
 * Each Node points back to a list of NodeNames that point to that
 * Node, and which are linked together along their nn_next fields.
 */
typedef struct nn
{
    struct node	*nn_node;	/* Node for which this is a name */
    char	*nn_name;	/* Text of name */
    struct nn	*nn_next;	/* Other names of nn_node */
} NodeName;

typedef struct node
{
    NodeName	*node_names;	/* List of names for this node.  The first name
				 * in the list is the "official" node name.
				 */
    int		 node_cap;	/* Capacitance to substrate */
    PerimArea	 node_pa[1];	/* Dummy; each node actually has
				 * ExtCurStyle->exts_numResistClasses
				 * array elements allocated to it.
				 */
} Node;

/* -------------------------------------------------------------------- */

/*
 * Value normally resident in the ti_client field of a tile,
 * indicating that the tile has not yet been visited in a
 * region search.
 */
extern ClientData extUnInit;

#define extSetRegion(tp,r)	( (tp)->ti_client = (ClientData) (r) )
#define extGetRegion(tp)	( (tp)->ti_client )
#define extHasRegion(tp,und)	( (tp)->ti_client != (und) )


/* For non-recursive flooding algorithm */
#define	VISITPENDING	((ClientData) NULL)	/* Marks tiles on stack */
#define	PUSHTILE(tp) \
    if (1) { \
	(tp)->ti_client = VISITPENDING; \
	STACKPUSH((ClientData) (tp), extNodeStack); \
    } else

/* ------------------------- Region finding --------------------------- */

extern Region *ExtFindRegions();

/* Filter functions for ExtFindRegions() */
extern Region *extTransFirst();		extern int extTransEach();
extern Region *extResFirst();		extern int extResEach();
extern Region *extNodeFirst();		extern int extNodeEach();
extern Region *extHierLabFirst();	extern int extHierLabEach();

/* -------- Search for matching tile/node in another ExtTree ---------- */

/*
 * NODETOTILE(np, et, tp)
 *	NodeRegion *np;
 *	ExtTree *et;
 *	Tile *tp;
 *
 * Sets tp to be the tile containing the lower-leftmost point of the
 * NodeRegion *np, but in the tile planes of the ExtTree *et instead
 * of the tile planes originally containing *np.
 */
#define	NODETOTILE(np, et, tp) \
	if (1) { \
	    Plane *myplane = (et)->et_use->cu_def->cd_planes[(np)->nreg_pnum]; \
\
	    (tp) = myplane->pl_hint; \
	    GOTOPOINT(tp, &(np)->nreg_ll); \
	    myplane->pl_hint = (tp); \
	}

/*
 * NODETONODE(nold, et, nnew)
 *	NodeRegion *nold;
 *	ExtTree *et;
 *	NodeRegion *nnew;
 *
 * Like NODETOTILE above, but leaves nnew pointing to the node associated
 * with the tile we find.
 */
#define	NODETONODE(nold, et, nnew) \
	if (1) { \
	    register Tile *tp; \
 \
	    (nnew) = (NodeRegion *) NULL; \
	    NODETOTILE((nold), (et), tp); \
	    if (tp && extHasRegion(tp, extUnInit)) \
		(nnew) = (NodeRegion *) extGetRegion(tp); \
	}

/* -------------------- Miscellaneous procedures ---------------------- */

extern char *extNodeName();
extern NodeRegion *extBasic();
extern NodeRegion *extFindNodes();
extern ExtTree *extHierNewOne();
extern int extNbrPushFunc();

/* --------------------- Miscellaneous globals ------------------------ */

extern int extNumFatal;		/* Number fatal errors encountered so far */
extern int extNumWarnings;	/* Number warning messages so far */
extern CellUse *extParentUse;	/* Dummy use for def being extracted */
extern ClientData extNbrUn;	/* Ditto */

    /*
     * This is really a (Stack *), but we use the struct tag to avoid
     * having to include stack.h in every .c file.  Used in the non-recursive
     * flooding algorithm.
     */
extern struct stack *extNodeStack;

/* ------------------ Connectivity table management ------------------- */

/*
 * The following is true if tile types 'r' and 's' are connected
 * according to the connectivity table 'tbl'
 */
#define extConnectsTo(r, s, tbl)	( TTMaskHasType(&(tbl)[(r)], (s)) )
#define	extTileConn(tp1, tp2, tbl) \
	extConnectsTo(TiGetType(tp1), TiGetType(tp2), (tbl))

/* -------------------------------------------------------------------- */

#include "extDebugInt.h"
