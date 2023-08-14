/*
 * grSunInt.h --
 *
 * 	INTERNAL definitions for the SUN graphics routines.
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
 * rcsid "$Header: grSunInt.h,v 6.0 90/08/28 18:41:05 mayo Exp $"
 */

#define _GRSUNINT
#ifndef _WINDOWS
    err1 = Need_To_Include_Windows_Header
#endif

/* ------------------------- types --------------------------------- */

typedef struct {	/* Data attached to each window */
    int gr_fd;
    struct pixwin *gr_pw;
    Window *gr_w;
} grSunRec;

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
					    * sunFillRect depends on the
					    * power of 2.
					    */
 

/* ------------------------- variables --------------------------------- */
extern int sunWMask;		/* Write mask for Sun driver. */
extern int sunColor;
extern int sunIntType;		/* Kind of Sun; one of the codes below */
#define	SUNTYPE_BW	0	/* B&W display only */
#define	SUNTYPE_110	1	/* 3/110C type; slow stencil ops */
#define	SUNTYPE_160	2	/* 3/160C type; integral framebuffer */

extern struct pixrect *sunStipple;
extern struct pixrect *grSunCpr;/* A pixrect onto the color disp. */

extern int grSunColorWindowFD;	/* A file descriptor for the color window */
extern FILE *grSunFileButtons;	/* Button pushes will come in on this pipe */
extern FILE *grSunFileReqPoint;	/* Send a char over this pipe to get a  */
extern FILE *grSunFilePoint;	/* point back on this pipe */

extern int grSunCurCursor;
extern bool grSunCursorOn;
extern grSunRec *grCurSunData;
extern int sunCMapDepth;	/* Max loc in color map (must be power of 2) */
extern int sunCMapPlanes;	/* Number of bit planes for color map */
extern char sunCMapName[];
extern FILE *grSunRootWindow;
extern Window *grLockedWindow;	/* for windSun.c */


/* ------------------------- procedures --------------------------------- */
extern void grSunTextInit();

/* Invert a coordinate relative to the sun screen. */
#define INVERTY(y)	(GrScreenRect.r_ytop - (y))

/* Invert a coordinate relative to the current window. */
#define INVERTWY(y)	(grLockedWindow->w_allArea.r_ytop - (y))

/* Coordinates of a Sun rectangle, before inversion with above INVERT macros */
#define	XBOT(sr)	((sr)->r_left)
#define YBOT(sr)	((sr)->r_top + (sr)->r_height - 1)
#define	XTOP(sr)	((sr)->r_left + (sr)->r_width - 1)
#define YTOP(sr)	((sr)->r_top)

