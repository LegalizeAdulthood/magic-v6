/*
 * windows.h --
 *
 *	Interface definitions for Magic's window manager.  This
 *	package manages a set of overlapping windows.
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
 * rcsid $Header: windows.h,v 6.0 90/08/28 19:02:34 mayo Exp $
 *
 */

#define _WINDOWS

#ifndef _MAGIC
    err = Need_To_Include_Magic_Header
#endif
#ifndef _GEOMETRY
    err = Need_To_Include_Geometry_Header
#endif



typedef ClientData WindClient;	/* A unique ID of a client of the
				 * window package.  The value 'NULL' is
				 * indicates an invalid value.
				 */

/*
 * The following structure describes a window.  Windows are overlapping
 * rectangles.  Each window contains a 'surface', whose contents are maintained
 * by a client of the window package.  The client is responsible for redrawing
 * the surface when requested.  The window package maintains the transform
 * from surface coordinates to screen coordinates and also controls which
 * portion of the surface is currently visable in the window.  See the 
 * comments in windMain.c and above the proc 'WindAddClient' for more info.
 *
 * To see an example of a simple window client, look at the colormap window 
 * located in '~cad/src/magic/cmwind/CMWmain.c'.  A more complex window is
 * the layout window, located in '~cad/src/magic/dbwind/DBWprocs.c'.
 *
 * The following key letters in parens after a field indicate who should 
 * read & write it:
 *
 *	P - Private (window package use only).
 *	R - Read-only (clients should not modify this, but may read it).
 *	W - Writable by client at any time.
 *	C - Writable by the client only during window creation time (in the
 *	    create proc passed to WindAddClient) and readable at all times.
 *	L - Writable by the client at creation time and when the window is
 *	    loaded with a new surface.
 *	G - Private to the graphics module.
 *
 * The comments below often mention a procedure that sets a given field.  This
 * is the normal procedure used to set the field, but there may be other seldom
 * used procs in the window package that set the field.  
 */
typedef struct WIND_S1 {
    struct WIND_S1 *w_nextWindow; /* A doubly-linked list (P) */
    struct WIND_S1 *w_prevWindow; /* A doubly-linked list (P) */
    ClientData w_clientData;	/* Used by the client an any manner (W) */
    WindClient w_client;	/* The client of this window (R) */
    char *w_caption;		/* The caption at the top of the window. (R)
				 * Set via WindCaption().
				 */
    ClientData w_surfaceID;	/* A unique ID for this surface, other than
				 * NULL. (R)
				 * Set via WindLoad().
				 */
    Rect w_allArea;		/* The entire area of the window in the screen
				 * coordinates for the window, including the 
				 * border and obscured areas. (R)
				 *
				 * If we are using Magic's window package,
				 * then this field is equal to w_frameArea
				 * since all windows share the same coordinate
				 * system.  In SunWindows, the lower-left
				 * corner of this area is at (0, 0) because
				 * each Sun window has it's own coordinate
				 * system with the origin in the lower left
				 * corner.
				 *
				 * This field is recomputed by WindReframe().
				 */
    Rect w_frameArea;		/* The location of the window on the screen (C)
				 * If a window's create proc modifies this, it 
				 * needs to set w_allArea and w_screenArea by 
				 * calling WindSetWindowAreas().
				 *
				 * Also, see comments for w_allArea.
				 *
				 * This field is initialized to a reasonable
				 * value at window creation time, but the
				 * 'reposition' proc passed to WindAddClient
				 * has a way of overriding it.
				 */
    Rect w_screenArea;		/* The area of the window in the screen
				 * coordinates of the window -- not including
				 * the border but including obscured areas. (R)
				 *
				 * This field is recomputed by WindReframe().
				 */
    Rect w_surfaceArea;		/* An area in surface coordinates that
				 * is large enough to contain everything
				 * displayed in w_screenArea plus at least
				 * one pixel of border on all sides. (R)
				 * This field, and w_origin and w_scale below,
				 * are modified by procedures that move and
				 * scroll windows, e.g. WindMove and WindSroll.
				 */
    Point w_origin;		/* This screen point, in 1/4096 pixels,
				 * corresponds to w_surfaceArea.r_ll.
				 * Used to transform between surface and screen
				 * coordinates. (R)
				 */
    int w_scale;		/* Defines how many 1/4096ths of a pixel
				 * correspond to 1 world unit.  Used to
				 * transform between surface and screen
				 * coordinates. (R)
				 */
    LinkedRect *w_clipAgainst;  /* A linked list of areas which obscure 
				 * portions of this window. (R)
				 * Normally clients just pass this down to the 
				 * graphics package.
				 * Changed via WindOver(), WindUnder(), and
				 * WindFullScreen().
				 */
    Point w_stippleOrigin;	/* A point that serves as the origin (in screen
				 * coordinates) of stipple patterns within
				 * this window. (R) 
				 * This field is maintained for the benifit of
				 * device drivers, but is often unused.
				 */
    int w_flags;		/* A collection of flag bits:
				 *
				 *	WIND_SCROLLABLE		(C)
				 *	WIND_SCROLLBARS		(C)
				 *	WIND_FULLSCREEN		(P)
				 *	WIND_ISICONIC		(P)
				 *	WIND_REDRAWICON		(P)
				 *
				 * Note: It is an error for a client to set
				 * WIND_SCROLLBARS but not WIND_SCROLLABLE.
				 * If WIND_SCROLLABLE is set, then the client
				 * must fill in the pointer w_bbox;
				 */
    Rect w_oldArea;		/* If the window has been blown up to full-
				 * screen size, this records its old area so
				 * it can be shrunk back later. (P)
				 */
    int w_oldDepth;		/* If the window is full-screen, this records
				 * its old depth on the list of windows, so
				 * it can be put back where it came from. (P)
				 */
    Rect *w_bbox;		/* A pointer to the bounding box of the stuff 
				 * in this window.  Used for WindView() and for 
				 * scroll bars. (L)
				 * This MUST be set if WIND_SCROLLABLE is set.
				 */
    int w_wid;			/* The window ID for this window.  Windows are
				 * assigned small non-negative integers for
				 * easy reference. (R)
				 * Set at window creation time.
				 */
    ClientData w_grdata;	/* Data private to the graphics package. (G)
				 * Often used to contain variables needed to
				 * interface to SUNs window package.
				 */
    char *w_iconname;		/* Short name for the icon. */
    ClientData w_redrawAreas;	/* List of areas that need to be redrawn. (P)
				 * Set by WindAreaChanged(), cleared by
				 * WindUpdate().  Initialized by 
				 * WindSeparateRedisplay().
				 */
} Window;


/* Window flags, for w->w_flags */
#define WIND_FULLSCREEN 1	/* Set if the window has been blown up to
				 * full-screen size.
				 */
#define WIND_SCROLLABLE 2	/* Set if the window can scroll & zoom. */
#define WIND_SCROLLBARS 4	/* Set if the window has scroll bars. */
#define WIND_ISICONIC 	8	/* Set if the window is closed down to an
				 * icon (on Suns only)
				 */
#define WIND_REDRAWICON 16	/* The icon needs to be redrawn. */


/* Special values for Window IDs.  Some procedures that expect a Window ID will
 * also accept one of these flags instead.
 */
#define WIND_UNKNOWN_WINDOW	-2	/* We should pick one by looking at
					 * the location of the point. 
					 */
#define WIND_NO_WINDOW		-3	/* Use NULL for the window */

/* utility procs & special stuff */
extern Window *WindCreate();
extern WindClient WindGetClient();
extern WindClient WindAddClient();
extern void WindInit();
extern void WindUpdate();
extern void WindOutToIn();
extern void WindInToOut();
extern void WindSetWindowAreas();
extern void windFixSurfaceArea();


/* searching procs */
extern int WindSearch();
extern Window *WindSearchWid();


/* procs for moving the surface inside of a window (changing the view) */
extern void WindZoom();
extern void WindMove();
extern void WindView();
extern void WindScroll();


/* procs for moving the window itself */
extern void WindOver();
extern void WindUnder();
extern void WindReframe();
extern void WindFullScreen();


/* procs to transform into and out of screen coordinates */

extern void WindScreenToSurface();
extern void WindSurfaceToScreen();
extern void WindPointToSurface();
extern void WindPointToScreen();


/* procs to change things or inform the window manager about changes */
extern void WindCaption();
extern void WindAreaChanged();
extern void WindIconChanged();
extern bool WindLoad();
extern void WindSeparateRedisplay();


/* interface variables */
extern int WindNewButtons;	/* A mask of the buttons that are down now. */
extern int WindOldButtons;	/* The buttons that were down on the previous
				 * command.
				 */
extern Point WindCommandPoint;	/* The point for the current command. */
extern bool WindAnotherUpdatePlease; /* Set by a client during redisplay if it
				 * discovers that more stuff needs to be
				 * displayed, and wants the window package
				 * to do another WindUpdate().  Used by
				 * dbwind in dbwhlRedrawFunc() when it suddenly
				 * discovers that drawing feedback areas
				 * requires that all the mask geometry be
				 * redrawn.
				 */

/* Macro to set the maximum number of windows */
#define WIND_MAX_WINDOWS(x)	(windMaxWindows = MIN(windMaxWindows, (x)))
extern int windMaxWindows;  	/* Use above macro, never increase this! */

/* The type of windows that we will implement.  This variable must be set by 
 * the graphics package before WindInit() is called.
 */
extern int WindPackageType;
#define WIND_MAGIC_WINDOWS	0
#define WIND_SUN_WINDOWS	1
#define	WIND_X_WINDOWS		2

/* Scroll bar width.
 * May be set by the graphics package before WindInit() is called.
 * If the width is XX, the window package tries to load glyphs from
 * the file 'windowsXX.glyphs' located in ~cad/lib/magic/sys.  This file
 * contains the arrow for the scroll bars, and those icons should be of
 * this width.
 */
extern int WindScrollBarWidth;
