/*
 * grX10aInt.h --
 *
 * Internal definitions for grX10a[1..5].c.
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 *
 * rcsid "$Header: grX10aInt.h,v 6.0 90/08/28 18:41:31 mayo Exp $"
 */

/* X10 driver "a" (from Lawrence Livermore National Labs) */

#define _GRAPHINT
#ifndef	_MAGIC
    int err = Need_to_include_magic_h
#endif
#ifndef	_WINDOWS
    int err = Need_to_include_windows_h
#endif

/*
 * Special for the X window system
 */
#define MAX_CURSORS	32	/* Maximum number of programmable cursors */
#define MBLACK 0172		/* code for black after color is mapped */
#define MWHITE 0174		/* code for white after color is mapped */
#define CHARPAD		1	/* 1 pixel for intercharacter spacing */
#define TRANSPARENTPIXEL 000	/* No color at all */
#define BRUSHHEIGHT	1	/* line height */
#define BRUSHWIDTH	1	/* line width */

#define true		1
#define false		0

#undef TRUE

/* Macros for conversion between X and Magic coordinates
 */
/*
 * Conversions relative to the co-ordinates
 * of the current window.
 */
#define grMagicToX(y) ( grCurrent.mw->w_allArea.r_ytop - ( y ) )
#define grXToMagic(y) ( grCurrent.mw->w_allArea.r_ytop - ( y ) )
/*
 * Conversion relative to the co-ordinates
 * of the screen's display.
 */
#define	grXsToMagic(y) ( DisplayHeight - (y))
#define	grMagicToXs(y) ( DisplayHeight - (y))
 
/* Redefine data types so that Magic's names don't conflict with X's names.
 */
typedef Window MagicWindow;	/* Name for Magic's def of a window. */
#define Window XWindow		/* Name for X's def of a window. */
				/* From now on, refering to "Window" will
				 * use X's definition.
				 */

/* NOTE:  In order for the above defs to work correctly, this file (grX10aInt.h)
 * must be included after all the Magic .h files and before the X .h files.
 */

/*** Must be undefined because of <X/Xlib.h> declaration of malloc ***/
#undef	malloc
#undef	free
#undef	calloc

#include <X/Xlib.h>

struct 	xstate	{ /* Current settings for X function parameters */
    Bitmap  clipmask;
    Cursor  cursor;
    Font    font;
    int	    fontSize;
    Pattern pattern;
    int     pixel;
    int     planes;
    Pixmap  tile;
    Window  window;
    MagicWindow *mw;
};
