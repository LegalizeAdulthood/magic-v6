/*
 * windInt.h --
 *
 *	Internal definitions for the window package.
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
 * rcsid $Header: windInt.h,v 6.0 90/08/28 19:02:20 mayo Exp $
 *
 */

#define _WINDINT
#ifndef _GEOMETRY
    err = Need_To_Include_Geometry_Header
#endif
#ifndef _WINDOWS
    err = Need_To_Include_Window_Header
#endif
#ifndef _GLYPHS
    err = Need_To_Include_Glyphs_Header
#endif
#ifndef _MAGIC
    err = Need_To_Include_Magic_Header
#endif

/* ----------------- data structures ----------------- */
typedef struct WIND_S3 {
    char *w_clientName;
    Void (*w_create)();
    bool (*w_delete)();
    Void (*w_redisplay)();
    Void (*w_command)();
    Void (*w_update)();
    Void (*w_exit)();
    Void (*w_reposition)();	/* called when a window moves or changes size */
    GrGlyph *w_icon;
    char **w_commandTable;
    struct WIND_S3 *w_nextClient;
} clientRec;

/* ----------------- variables ----------------- */
extern Window *windTopWindow;
extern Window *windBottomWindow;
extern clientRec *windFirstClientRec;
extern char *butTable[];
extern char *actTable[];
extern bool windPrintCommands;

/* ----------------- procedures ----------------- */
extern void windDisplayArea();
extern void windPrintCommand();
extern void windSetPoint();
extern void windDump();
extern void windClientInit();
extern Window *windSearchPoint();

/* ----------------- constants ----------------- */

/* the width of window borders */
extern int windCaptionPixels;
#define TOP_BORDER(w)	windCaptionPixels
#define THIN_LINE	2	
#define BOT_BORDER(w)	(((w)->w_flags & WIND_SCROLLBARS) \
	? 2*THIN_LINE + WindScrollBarWidth : 2*THIN_LINE)
#define LEFT_BORDER(w)	(((w)->w_flags & WIND_SCROLLBARS) \
	? 2*THIN_LINE + WindScrollBarWidth : 2*THIN_LINE)
#define RIGHT_BORDER(w)	2*THIN_LINE

/* Always leave room for the borders plus 25 pixels */
#define WIND_MIN_WIDTH	(6*THIN_LINE + 3*WindScrollBarWidth + 25)
#define WIND_MIN_HEIGHT	(windCaptionPixels + 4*THIN_LINE + \
	3*WindScrollBarWidth + 25)

#define DEFAULT_CLIENT	"layout"
#define WINDOW_CLIENT	"*window"

/* Default size for new windows. */

#define CREATE_HEIGHT	300
#define CREATE_WIDTH	300
