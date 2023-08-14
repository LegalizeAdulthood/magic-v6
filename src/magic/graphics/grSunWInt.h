/*
 * grSunWInt.h --
 *
 * 	INTERNAL definitions for the SUN Windows graphics routines.
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
 * rcsid "$Header: grSunWInt.h,v 6.0 90/08/28 18:41:19 mayo Exp $"
 */

#define _GRSUNWINT
#ifndef _WINDOWS
    err1 = Need_To_Include_Windows_Header
#endif

/* ------------------------- types --------------------------------- */

typedef struct {	/* Data attached to each window */
    int gr_fd;
    struct pixwin *gr_pw;
    Window *gr_w;
} grSunWRec;

/* ------------------------- constants --------------------------------- */
#ifdef 	sun

#include <pixrect/pixrect_hs.h>

#endif	sun

#define BUTTON_PROG	"grSunProg"	   /* Program to watch the buttons on
					    * a Sun2/120 with separate color
					    * display.
					    */
 
#define BUTTON_PROG2	"grSunProg2"	   /* Program to watch the buttons on
					    * a Sun2/160 with a single color
					    * display.
					    */

#define	STIPMASKSIZE	64		   /* Size of mask through which we
					    * spray stipples.  WARNING:
					    * this must be a multiple of 8
					    * and a power of 2; code in
					    * sunSetSPattern depends on the
					    * multiple of 8 and code in
					    * sunWFillRect depends on the
					    * power of 2.
					    */
 

/* ------------------------- variables --------------------------------- */
extern int sunWMask;		/* Write mask for SunW driver. */
extern int sunWColor;
extern int sunWIntType;		/* Kind of Sun; one of the codes below */
#define	SUNTYPE_BW	0	/* B&W display only */
#define	SUNTYPE_110	1	/* 3/110C type; slow stencil ops */
#define	SUNTYPE_160	2	/* 3/160C type; integral framebuffer */

extern struct pixrect *sunWStipple;
extern struct pixrect *grSunWCpr;/* A pixrect onto the color disp. */

extern int grSunWColorWindowFD;	/* A file descriptor for the color window */
extern FILE *grSunWFileButtons;	/* Button pushes will come in on this pipe */
extern FILE *grSunWFileReqPoint;	/* Send a char over this pipe to get a  */
extern FILE *grSunWFilePoint;	/* point back on this pipe */

extern int grSunWCurCursor;
extern bool grSunWCursorOn;
extern grSunWRec *grCurSunWData;
extern int sunWCMapDepth;	/* Max loc in color map (must be power of 2) */
extern int sunWCMapPlanes;	/* Number of bit planes for color map */
extern char sunWCMapName[];
extern FILE *grSunWRootWindow;
extern Window *grLockedWindow;	/* for windSun.c */


/* ------------------------- procedures --------------------------------- */
extern void grSunWTextInit();

/* Invert a coordinate relative to the sun screen. */
#define INVERTY(y)	(GrScreenRect.r_ytop - (y))

/* Invert a coordinate relative to the current window. */
#define INVERTWY(y)	(grLockedWindow->w_allArea.r_ytop - (y))

/* Coordinates of a Sun rectangle, before inversion with above INVERT macros */
#define	XBOT(sr)	((sr)->r_left)
#define YBOT(sr)	((sr)->r_top + (sr)->r_height - 1)
#define	XTOP(sr)	((sr)->r_left + (sr)->r_width - 1)
#define YTOP(sr)	((sr)->r_top)


/* Macro to keep from holding a display lock for too long (10 seconds). 
 *
 * We keep track of how many items (rectangles or text) that we have
 * drawn, and if it reaches a certain limit we release and then
 * reaquire the lock.
 */

extern int grSunWNumDraws;		/* Set to 0 by grSunWGetLock() */
extern void grSunWGetLock();
extern void grSunWReleaseLock();
#define	NUM_DRAWS_BEFORE_DIDDLE	500
#define	DIDDLE_LOCK()	{if (grSunWNumDraws++ > NUM_DRAWS_BEFORE_DIDDLE) \
	{grSunWReleaseLock(); grSunWGetLock();}}
