/*
 * graphicsInt.h --
 *
 * Internal definitions for the graphics module.
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
 * rcsid "$Header: graphicsInt.h,v 6.0 90/08/28 18:41:49 mayo Exp $"
 */

#define _GRAPHINT
#ifndef	_MAGIC
    int err = Need_to_include_magic_h
#endif
#ifndef	_WINDOWS
    int err = Need_to_include_windows_h
#endif


/* data structures */
typedef struct {
    int mask, color, outline, fill, stipple;
} GR_STYLE_LINE;

extern GR_STYLE_LINE grStyleTable[];
extern int grStippleTable[][8];
extern int grNumBitPlanes;
extern int grBitPlaneMask;


/* procedures */
extern Void (*grPutTextPtr)();
extern Void (*grSetSPatternPtr)();
extern Void (*grDefineCursorPtr)();
extern bool (*grDrawGridPtr)();
extern Void (*grDrawLinePtr)();
extern Void (*grSetWMandCPtr)();
extern Void (*grFillRectPtr)();
extern Void (*grSetStipplePtr)();
extern Void (*grSetLineStylePtr)();
extern Void (*grSetCharSizePtr)();

extern char *grFgets();
extern Void grSimpleLock(), grSimpleUnlock();
extern void grNoLock();
#define	GR_CHECK_LOCK()	{if (grLockedWindow == NULL) grNoLock();} 


/* constants */

/* Rectangle filling styles.  
 * (note: this must match the array in grStyle.c)
 */

#define GR_STSOLID 	0
#define GR_STCROSS 	1
#define GR_STOUTLINE	2
#define GR_STSTIPPLE 	3
#define GR_STGRID 	4


#define GR_NUM_STIPPLES	32
#define	GR_NUM_STYLES	256	/* Max number of display styles */

/* The size of the crosses drawn for degenerate box outlines: */
#define GR_CROSSSIZE 3

/* This becomes TRUE if we should quit drawing things and return */
extern bool SigInterruptPending;

/* clipping stuff from GrLock() */
extern Window* grLockedWindow;
extern Rect grCurClip;
extern LinkedRect *grCurObscure;

/* Strings used to generate file names for glyphs, colormaps, and
 * display styles.
 */

extern char *grDStyleType;
extern char *grCMapType;
extern char *grCursorType;

/*
 * Used to pass display-style information to lower levels
 * of a graphics driver.
 */
extern int grCurDStyle;

/* Device-dependent limit on # of stipples (must be <= GR_NUM_STIPPLES): */

extern int grMaxStipples;

/* Used to setup current color, etc. */
extern bool grDriverInformed;
extern void grInformDriver();

/* Macro to check for a bogusly small grid. 
 * Turn off grid if there are more than GR_NUMGRIDS gridlines on the
 * screen in either direction, or if they are less than 4 pixels apart.
 */
#define GR_NUM_GRIDS 160
#define GRID_TOO_SMALL(x,y,max) ( \
     ((((GrScreenRect.r_xtop - GrScreenRect.r_xbot) << 12) / (x)) >= (max)) || \
     ((((GrScreenRect.r_ytop - GrScreenRect.r_ybot) << 12) / (y)) >= (max)) || \
     (((x) >> 12) < 4) || (((y) >> 12) < 4) )
