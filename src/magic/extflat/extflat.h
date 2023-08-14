/*
 * extflat.h --
 *
 * Internal definitions for the procedures to flatten hierarchical
 * (.ext) circuit extraction files.
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
 * rcsid $Header: extflat.h,v 6.0 90/08/28 18:13:45 mayo Exp $
 */

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC

typedef unsigned char u_char;

/*
 * Arguments to EFFlatBuild().
 */
#define	EF_FLATNODES	0x01	/* Flatten nodes */
#define	EF_FLATCAPS	0x02	/* Flatten capacitors */
#define	EF_FLATRESISTS	0x04	/* Flatten resistors */
#define	EF_FLATDISTS	0x08	/* Flatten distances */

/* Flags to control output of node names.  Stored in EFTrimFlags */
#define	EF_TRIMGLOB	0x01	/* Delete trailing '!' from names */
#define	EF_TRIMLOCAL	0x02	/* Delete trailing '#' from names */

/* ------------------------ Hierarchical names ------------------------ */

/*
 * One of the biggest consumers of memory space when flattening a circuit
 * are the full hierarchical names of all nodes.  Most of this space is
 * wasted since it's redundant.  Also, a lot of time is spent comparing
 * long names whose initial components are identical.
 *
 * The following structure allows hierarchical names to be represented
 * with sharing.  Names are represented as a sequence of components,
 * from the lowest level of the hierarchy pointing back toward the root.
 * Hence, comparisons are likely to detect differences between names
 * early on.  Second, many children can share the same parent, so
 * storage space should be comparable to that needed for an unflattened
 * hierarchy (with arrays flattened, however).
 */
typedef struct hiername
{
    struct hiername	*hn_parent;	/* Back-pointer toward root */
    int			 hn_hash;	/* For speed in hashing */
    char		 hn_name[4];	/* String is allocated here */
} HierName;

/*
 * Size of a HierName big enough to hold a string containing
 * n bytes (not including the NULL byte).
 */
#define	HIERNAMESIZE(n)	((n) + sizeof (HierName) - 3)

/* Indicates where the HierName was allocated: passed to EFHNFree() */
#define	HN_ALLOC	0	/* Normal name (FromStr) */
#define	HN_CONCAT	1	/* Concatenation of two HierNames */
#define HN_GLOBAL	2	/* Global name */
#define HN_FROMUSE	3	/* From a cell use */

/* ----------------------- Node attribute lists ----------------------- */

typedef struct efattr
{
    struct efattr	*efa_next;	/* Next in list */
    Rect		 efa_loc;	/* Location of attr label */
    int			 efa_type;	/* Tile type attr attached to */
    char		 efa_text[4];	/* String is allocated here */
} EFAttr;

/*
 * Size of an EFAttr big enough to hold a string containing
 * n bytes (not including the NULL byte).
 */
#define	ATTRSIZE(n)	((n) + sizeof (EFAttr) - 3)

/* ------------------- Hierarchical and flat nodes -------------------- */

/*
 * Each entry in the a nodename hash table points to a EFNodeName.
 * Several EFNodeNames may point to the same EFNode.  Such EFNodeNames
 * are linked into a NULL-terminated list by the name_next pointers.
 * The first name in this list, pointed to by the efnode_name field of
 * the EFNode they all point to, is the canonical name for this node.
 *
 * The name_hier field points to the HierName for this node, which
 * will have only a single component for EFNodes within a Def, but
 * multiple components for hierarchical node names.
 */
typedef struct efnn
{
    struct efnode	*efnn_node;	/* Corresponding node */
    struct efnn		*efnn_next;	/* Next name for this node */
    HierName		*efnn_hier;	/* HierName for this node */
} EFNodeName;

/*
 * Both hierarchical and flat nodes use the same structure.  Hierarchical
 * nodes appear along with each cell def.  Flat nodes are pointed to by
 * the global hash table.
 *
 * Hierarchical nodes are linked in a doubly-linked list with all
 * other nodes in the same cell, and flat nodes are similarly linked
 * with all other flat nodes in the circuit.  The list is doubly
 * linked to allow nodes to be deleted easily when it is necessary
 * to merge two nodes into a single node.
 *
 * There is a third way in which a node can exist if only its name is
 * of interest, namely as an EFNodeHdr.  The first part of an EFNode
 * is an EFNodeHdr.
 */

    /* Represents perimeter and area for a resistance class */
typedef struct
{
    int		 pa_area;
    int		 pa_perim;
} PerimArea;

typedef struct efnhdr
{
    int		 efnhdr_flags;	/* See below */
    EFNodeName	*efnhdr_name;	/* Canonical name for this node, this is a ptr
				 * to the first element in a null-terminated
				 * list of all the EFNodeNames for this node.
				 */
    struct efnhdr *efnhdr_next;	/* Next node in list */
    struct efnhdr *efnhdr_prev;	/* Previous node in list */
} EFNodeHdr;

/* Node flags */
    /*
     * If set, this node was killed and neither it nor anything connected
     * to it should be output.  There should have been a new, identical
     * structure in the input that was connected to the new node.
     */
#define	EF_KILLED	0x01

    /*
     * If set, this node was allocated as a substrate terminal for a
     * fet, and so should be automatically merged with nodes of the
     * same name after all nodes have been flattened, rather than
     * complaining about it being unconnected.
     */
#define	EF_FETTERM	0x02

typedef struct efnode
{
    EFNodeHdr	 efnode_hdr;	/* See above */
#define	efnode_name	efnode_hdr.efnhdr_name
#define	efnode_next	efnode_hdr.efnhdr_next
#define	efnode_prev	efnode_hdr.efnhdr_prev
#define	efnode_flags	efnode_hdr.efnhdr_flags

    int		 efnode_cap;	/* Total capacitance to ground for this node */
    int		 efnode_type;	/* Index into type table for node */
    Rect	 efnode_loc;	/* Location of a 1x1 rect contained in this
				 * node.  This information is provided in the
				 * .ext file so it will be easy to map between
				 * node names and locations.
				 */
    EFAttr	*efnode_attrs;	/* Node attribute list */
    ClientData	 efnode_client;	/* For hire */
    PerimArea	 efnode_pa[1];	/* Dummy; each node actually has
				 * efNumResistClasses array elements
				 * allocated to it.
				 */
} EFNode;

/* -------------------------- Transistors ----------------------------- */

/*
 * Each transistor can contain several terminals.
 * Each terminal is described by the following structure.
 * We use a EFNode pointer for the terminal to which a fet connects;
 * this assumes that fets appear after all the nodes for a cell.
 */
typedef struct fetterm
{
    EFNode	*fterm_node;	/* Node to which we're connected */
    char	*fterm_attrs;	/* Attribute list */
    int		 fterm_perim;	/* Length (perimeter) of terminal */
} FetTerm;

/*
 * Transistor itself.
 * The fet_substrate and fet_type pointers are actually pointer into shared
 * tables of names, rather than being individually allocated for each
 * transistor.
 */
typedef struct fet
{
    struct fet	*fet_next;	/* Next fet in def */
    u_char	 fet_type;	/* Index into fet type table */
    u_char	 fet_nterm;	/* Number of terminals in fet */
    EFNode	*fet_subsnode;	/* Substrate node */
    int		 fet_area;	/* Area (in lambda) */
    int		 fet_perim;	/* Total perimeter */
    Rect	 fet_rect;	/* 1x1 rectangle inside fet */
    FetTerm	 fet_terms[1];	/* Terminals.  The actual number will depend
				 * on fet_nterm above, so the size of this
				 * structure will vary.
				 */
} Fet;

/* Size of a Fet structure for 'n' terminals (including the gate) */
#define	FetSize(n)	(sizeof (Fet) + ((n)-1)*sizeof (FetTerm))

/* -------------------------------------------------------------------- */

/*
 * A big number, used for thresholds for capacitance and resistance
 * when no processing is desired.
 */
#define	INFINITE_THRESHOLD	(((unsigned long) (~0)) >> 1)

/* Max filename length */
#define	FNSIZE		1024


extern int EFScale;		/* Scale factor to multiply all coords by */
extern char *EFTech;		/* Technology of extracted circuit */
extern char *EFSearchPath;	/* Path to search for .ext files */
extern char *EFLibPath;		/* Library search path */
extern char *EFVersion;		/* Version of extractor we work with */
extern char *EFArgTech;		/* Tech file given as command line argument */

    /*
     * Thresholds used by various extflat clients to filter out
     * unwanted resistors and capacitors.  Resistance is in milliohms,
     * capacitance in attofarads.
     */
extern int EFResistThreshold;
extern int EFCapThreshold;

    /* Table of transistor types */
extern char *EFFetTypes[];
extern int EFFetNumTypes;

    /* Table of Magic layers */
extern char *EFLayerNames[];
extern int EFLayerNumNames;

    /* Output control flags */
extern int EFTrimFlags;

/* -------------------------- Exported procedures --------------------- */

extern char *EFArgs();

    /* HierName manipulation */
extern HashEntry *EFHNLook();
extern HashEntry *EFHNConcatLook();
extern HierName *EFHNConcat();
extern HierName *EFStrToHN();
extern char *EFHNToStr();
