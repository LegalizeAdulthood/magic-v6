/*
 * cmwind.h --
 *
 *	Interface definitions for the 'glue' between the window
 *	manager and the color map editor.
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
 * rcsid $Header: cmwind.h,v 6.0 90/08/28 18:06:02 mayo Exp $
 */

#define _CMWIND

#ifndef _MAGIC
    err0 = Need_To_Include_Magic_Header;
#endif	_MAGIC
#ifndef	_GEOMETRY
    err1 = Need_To_Include_Geometry_Header;
#endif	_GEOMETRY
#ifndef _WINDOWS
    err2 = Need_To_Include_Windows_Header;
#endif	_WINDOWS


/* data structures */

typedef struct
{
    char		*cmw_cname;	/* Name of color */
    int			 cmw_color;	/* Index in color map if >= 0 */
} CMWclientRec;

#define	CB_RED		0	/* Red */
#define	CB_GREEN	1	/* Green */
#define	CB_BLUE		2	/* Blue */
#define	CB_HUE		3	/* Hue */
#define	CB_SAT		4	/* Saturation */
#define	CB_VALUE	5	/* Value */

/* The following data structure defines a color bar, used to display
 * the value of a particular color parameter as a horizontal bar.
 */

typedef struct
{
    char	*cb_name;	/* Name of color */
    int		 cb_code;	/* Which color bar */
    int		 cb_style;	/* Style for redisplay */
    Rect	 cb_rect;	/* Bounding rectangle of bar */
    Rect	 cb_textRect;	/* Bounding rectangle of box for name */
} ColorBar;

/* The following data structure defins a color pump, a little rectangle
 * that, when buttoned, increments or decrements a particular color value.
 */

typedef struct
{
    int		cp_code;	/* Which color bar */
    double	cp_amount;	/* Amount to increment or decrement. */
    Rect	cp_rect;	/* Bounding rectangle of pump */
} ColorPump;

/* Exported procedures */

extern void CMWloadWindow();
extern int CMWcommand();

extern Rect colorWindowRect;
extern WindClient CMWclientID;
extern char *CMWCommandNames[];
extern int (*cmwCommandProcs[])();
extern ColorBar colorBars[];
extern ColorPump colorPumps[];
extern Rect cmwCurrentColorArea;
extern void cmwUndoColor();
extern bool CMWCheckWritten();
