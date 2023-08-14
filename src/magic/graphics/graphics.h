/* graphics.h -
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
 * This file contains a bunch of macros that look like
 * normal procedure calls but really indirect through procedure
 * pointers in order to achieve graphics display independence.
 */

/* rcsid "$Header: graphics.h,v 6.0 90/08/28 18:41:46 mayo Exp $" */

#define	_GRAPHICS
#ifndef	_MAGIC
    int err = Need_to_include_magic_h
#endif
#ifndef	_GEOMETRY
    int err = Need_to_include_geometry_h
#endif


/* Housekeeping and initialization routines */
extern Void (*GrInitPtr)();
extern Void (*GrClosePtr)();
extern Void (*GrTextSizePtr)();

/* 
 * Display painting and text routines 
 *
 *    You must call GrLock() before using these, and 
 *    call GrUnlock() afterwards.
 */
extern Void (*GrLockPtr)();
extern Void (*GrUnlockPtr)();
extern bool GrHaveLock();
extern void GrClipTo();
extern void GrClipBox();
extern void GrClipLine();
extern bool GrPutText();
extern Void (*GrDrawGlyphPtr)();
extern Void (*GrBitBltPtr)();
extern int  (*GrReadPixelPtr)();
extern Void (*GrFlushPtr)();

/* Tablet routines */
extern Void (*GrEnableTabletPtr)();
extern Void (*GrDisableTabletPtr)();
extern Void (*GrSetCursorPtr)();

/* graphics routines that are called in the same way for all displays */
extern bool GrLoadStyles();
extern bool GrLoadCursors();
extern bool GrSetDisplay();
extern void GrLabelSize();
extern void GrSetStuff();
extern void GrFastBox();
extern void GrGuessDisplayType();

/* external color map routines */
extern bool GrReadCMap(), GrSaveCMap();
extern Void GrGetColor(), GrPutColor(), GrPutManyColors();
extern Void (*GrSetCMapPtr)();
extern void GrResetCMap();

/* Routines for windows, called only if non-null.  See SUN160 driver for
 * details.
 */
extern Void (*GrCreateWindowPtr)();	/* Passed a window just after it is
					 * created.
					 */
extern Void (*GrDeleteWindowPtr)();	/* Passed a window just before it
					 * is destroyed.
					 */
extern Void (*GrDamagedPtr)();		/* Called at a conventient time after
					 * receiving a SIG_WINCH signal.
					 */
extern Void (*GrUpdateIconPtr)(); 	/* Adjust text on icon.
					 */
extern Void (*GrConfigureWindowPtr)();	/* Called to reconfigure size/location
					 * of an existing window.
					 */
extern Void (*GrOverWindowPtr)();	/* Raise window to top of stack.
					 */
extern Void (*GrUnderWindowPtr)();	/* Lower window to bottom of stack.
					 */

/* Text output routine, should be called only by the textio module.  It is 
 * located here so that graphics drivers can do the text in a special way if 
 * they want to.  It is normally set to point to 'vfprintf', a varargs version
 * of fprintf.  If this doesn't exist (i.e. if the #define NOVARARGS is present)
 * then it normally points to fprintf.
 */
#ifndef	NO_VARARGS
extern int (*GrVfprintfPtr)();
#define GrVfprintf (*GrVfprintfPtr)
#else	NO_VARARGS
extern int (*GrFprintfPtr)();
#define GrFprintf (*GrFprintfPtr)
#endif	NO_VARARGS


/* Terminal state routine, called by the Textio module after it decides what
 * mode the terminal driver should be in.  This gives the graphics module a
 * chance to change it.  The pointer is normally NULL, in which case it is 
 * ignored.  */ 
extern Void (*GrTermStatePtr)();


/* Routines called by the signals module just before Magic is stopped
 * (such as via ^Z) and just after it is resumed.
 */
extern Void (*GrStopPtr)();
extern Void (*GrResumePtr)();
#define GrStop	(*GrStopPtr)
#define GrResume (*GrResumePtr)


/* The size of the screen in screen coordinates */
extern Rect GrScreenRect;

/* The size of crosses (drawn for zero-size boxes), in pixels. */
extern Rect GrCrossRect;

/* Short names for styles */
extern int GrStyleNames[];


#ifndef lint

/* Constants for easy access */

#define GrLock (*GrLockPtr)
#define GrUnlock (*GrUnlockPtr)
#define GrInit (*GrInitPtr)
#define GrClose (*GrClosePtr)
#define GrSetCMap (*GrSetCMapPtr)
#define GrTextSize (*GrTextSizePtr)
#define GrDrawGlyph (*GrDrawGlyphPtr)
#define GrBitBlt (*GrBitBltPtr)
#define	GrReadPixel (*GrReadPixelPtr)
#define GrFlush (*GrFlushPtr)

#define GrEnableTablet (*GrEnableTabletPtr)
#define GrDisableTablet (*GrDisableTabletPtr)
#define GrGetCursorPos (*GrGetCursorPosPtr)
#define GrGetButtons (*GrGetButtonsPtr)
#define GrSetCursor (*GrSetCursorPtr)

#endif


/* 4 text sizes that are specified by the graphics driver, plus a 5th one.  
 * This last one is for systems that allow the user to specify a default font 
 * (such as a sun) -- on other systems it's size is decided by the driver.
 *
 * Note: 
 *    These must be in sequential, ascending order.
 */
#define GR_TEXT_SMALL	0
#define GR_TEXT_MEDIUM	1
#define GR_TEXT_LARGE	2
#define GR_TEXT_XLARGE	3
#define GR_TEXT_DEFAULT	4

/* Default cursor position -- used on startup and if we can't read the cursor */
#define GR_CURSOR_X	100
#define GR_CURSOR_Y	100

/* Special full-screen access for window manager only */
#define GR_LOCK_SCREEN	-1
