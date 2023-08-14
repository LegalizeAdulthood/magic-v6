/*
 * drc.h --
 *
 * Definitions for the DRC module.
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
 * Needs to include: magic.h database.h
 *
 * rcsid $Header: drc.h,v 6.0 90/08/28 18:12:38 mayo Exp $
 */

#define	_DRC

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err1 = Need_to_include_database_header;
#endif	_DATABASE


/* ----------------- component of DRC table ---------------------------- */

typedef struct drccookie
{
    int    	       drcc_dist;	/* Extent of rule from edge. */
    TileTypeBitMask       drcc_mask;	/* Legal types on RHS */
    TileTypeBitMask       drcc_corner;	/* Types that trigger corner check */
    struct drccookie * drcc_next;
    char             * drcc_why;	/* Explanation of error found */
    int		       drcc_cdist;	/* Size of corner extension. */
    int		       drcc_flags;	/* Miscellaneous flags, see below. */
    int		       drcc_plane;	/* Index of plane on which to check
					 * legal types.
					 */
} DRCCookie;

/* DRCCookie flags:
 * DRC_FORWARD:		Rule applies from left to right (or bottom to top).
 * DRC_REVERSE:		Rule applies from right to left (or top to bottom).
 * DRC_BOTHCORNERS:	Must make corner extensions in both directions.
 * DRC_XPLANE:		Means check areas for rule are on different plane
 *			than edges from which rule triggers.
 */

#define		DRC_FORWARD		0
#define		DRC_REVERSE		1
#define		DRC_BOTHCORNERS		2
#define		DRC_XPLANE		4
#define		DRC_CHECKCONNECT	010
#define		DRC_ZEROSPACERULE	0x20
#define		DRC_LEFT		0x100
#define		DRC_RIGHT		0x200
#define		DRC_TOP			0x400
#define		DRC_BOTTOM		0x800

/*
 * Design rule table
 */

extern DRCCookie        *DRCRulesTbl[TT_MAXTYPES][TT_MAXTYPES];

				/*  macro to determine if two TileTypes
				 *  are in the same plane
				 */
#define SamePlane(i,j)	((i == TT_SPACE) || (j == TT_SPACE) || \
			 (DBPlane(i) == DBPlane(j)))

/* This is client data passed down through the various DRC checking
 * routines, and contains information about the area and rule being
 * checked.
 */
struct drcClientData
{
    CellDef	* dCD_celldef;		/* CellDef, plane and area to DRC. */
    Plane	* dCD_plane;
    Rect	* dCD_rect;
    Tile 	* dCD_initial;		/* Initial tile for search (left side
					 * for forward rules, right for reverse 
					 * rules).
					 */
    Rect	* dCD_clip;		/* Clip error tiles against this. */
    int		* dCD_errors;		/* Count of errors found. */
    int		  dCD_which;		/* tells which edge of initial we 
    					   started from */
    DRCCookie	* dCD_cptr;		/* Rule being checked. */
    Rect	* dCD_constraint;	/* Constraint area from rule. */
    void	(* dCD_function)(); 	/* Function to call for each
				    	 * error found. */
    ClientData	dCD_clientData;		/* Parameter for dCD_function */
};

/* Describes a cell whose contents require design-rule checking of
 * some sort.  These are linked together for processing by the
 * continuous checker.
 */

typedef struct drcpendingcookie
{
    CellDef                 * dpc_def;
    struct drcpendingcookie * dpc_next;
} DRCPendingCookie;

extern DRCPendingCookie * DRCPendingRoot;

#define DRCYANK	"__DRCYANK__"	/* predefined DRC yank buffer */

/* Things shared between DRC functions, but not used by the
 * outside world:
 */

extern int  dbDRCDebug;
extern int  TechHalo;	      	/* largest action distance of design rules */
extern int  DRCStepSize;	/* chunk size for decomposing large areas */
extern PaintResultType DRCPaintTable[NP][NT][NT];
extern TileTypeBitMask DRCExactOverlapTypes;

extern int  DRCstatEdges;	/* counters for statistics gathering */
extern int  DRCstatSlow;
extern int  DRCstatRules;
extern int  DRCstatTiles;
extern int  DRCstatInteractions;
extern int  DRCstatIntTiles;
extern int  DRCstatSquares;
extern int  DRCstatArrayTiles;

#ifdef	DRCRULESHISTO
#	define	DRC_MAXRULESHISTO 30	/* Max rules per edge for statistics */
extern int  DRCstatHRulesHisto[DRC_MAXRULESHISTO];
extern int  DRCstatVRulesHisto[DRC_MAXRULESHISTO];
#endif	DRCRULESHISTO

extern void drcPaintError();
extern void drcPrintError();
extern int drcIncludeArea();
extern int drcExactOverlapTile();

extern CellUse *DRCuse, *DRCDummyUse;
extern CellDef *DRCdef;

/*
 * Exported procedures and variables.
 */

extern Void DRCTechInit();
extern bool DRCTechAddRule();
extern Void DRCTechFinal();
extern void DRCTechRuleStats();

extern void DRCInit();
extern void DRCContinuous();
extern void DRCCheckThis();
extern void DRCPrintRulesTable();
extern void DRCWhy();
extern void DRCPrintStats();
extern void DRCCheck();
extern void DRCCount();
extern int DRCFind();
extern void DRCCatchUp();
extern bool DRCBackGround;	/* global flag to enable/disable
				 * continuous DRC
			     	 */
extern DRCPendingCookie *DRCPendingRoot;
extern CellDef *DRCdef;
extern bool DRCFindInteractions();

/* The following macro can be used by the outside world to see if
 * the background checker needs to be called.
 */

#define DRCHasWork ((DRCPendingRoot != NULL) && (DRCBackGround))
