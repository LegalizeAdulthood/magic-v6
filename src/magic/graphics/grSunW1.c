/* grSunW1.c -
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
 * This file contains primitive functions to run Magic under SunWindows.
 * Included here are initialization and closing
 * functions, and several utility routines used by the other Sun
 * modules.
 */

#ifndef lint
static char rcsid[]="$Header: grSunW1.c,v 6.0 90/08/28 18:41:11 mayo Exp $";
#endif  not lint

#ifdef  sun

#include <stdio.h>
#include <sgtty.h>
#include <errno.h>
#include <suntool/tool_hs.h>
#include <suntool/wmgr.h>
#include <suntool/menu.h>
#include <sunwindow/cms_mono.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 

#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunWInt.h"
#include "txcommands.h"
#include "utils.h"
#include "textio.h"
#include "signals.h"
#include "malloc.h"

/* If 'grSunWDoLock' is TRUE, then we use Sun's locking primitive 'pw_lock()'.
 * Otherwise, we let Sun's pixwin layer do the locking automatically.
 */
static bool grSunWDoLock = TRUE;

/* File to the topmost window on this screen.
 */
FILE *grSunRootWindow = NULL;

/* Our colormap name (for sunwindows) */
char sunCMapName[20];

/* The current write mask and color */
int sunWriteMask = 255;
int sunWColor = 0;
struct pixrect *sunWStipple = NULL; /* Current stipple */

/* Internally used -- the type of Sun this is; default is 3/160 */
int sunWIntType = SUNTYPE_160;

/* Imports from other Sun modules: */
extern int SunWReadPixel();
extern bool sunWDrawGrid();
extern Void SunWEnableTablet(), SunWDisableTablet();
extern Void SunWSetCMap(), sunWPutText();
extern Void sunWDefineCursor();
extern Void SunWSetCursor(), SunWTextSize(), SunWDrawGlyph(), SunWBitBlt();
extern Void sunWDrawLine(), sunWSetLineStyle(), sunWSetCharSize();
extern Void sunWSetWMandC(), sunWFillRect();
extern void sunWInput();

extern Void sunWSetSPattern(), sunWSetStipple();

/* Our stipple patterns */
struct pixrect *grSunWStipples[GR_NUM_STIPPLES];

extern char *getenv();
extern FILE *fopen();
extern struct pixfont *pf_sys;

/* The current pix_win that we are drawing into */
grSunWRec *grCurSunWData = NULL;

/* The number of things drawn since the last 'DiddleLock' call. */
int grSunWNumDraws = 0;

/*
 * ----------------------------------------------------------------------------
 *
 * grSunWGetLock --
 *
 *	Lock a sun window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Locking makes redisplay go faster.
 *
 * ----------------------------------------------------------------------------
 */

void
grSunWGetLock()
{
    static struct rect sunrect;

    ASSERT(grCurSunWData != NULL, "grSunWGetLock");
    if (grSunWDoLock)
    {
	sunrect.r_left = 0;
	sunrect.r_top = 0;
	sunrect.r_width = (grLockedWindow->w_allArea.r_xtop - 
	    grLockedWindow->w_allArea.r_xbot + 1);
	sunrect.r_height = (grLockedWindow->w_allArea.r_ytop - 
	    grLockedWindow->w_allArea.r_ybot + 1);
	/* Make sure that we are not holding a lock on this window.  Magic 
	 * should never make nested lock calls.
	 */
	ASSERT(grCurSunWData->gr_pw->pw_clipdata->pwcd_lockcount == 0, "grSunWGetLock");
	pw_lock(grCurSunWData->gr_pw, &sunrect);
    }
    grSunWNumDraws = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * grSunWReleaseLock --
 *
 *	Release a lock on the sun window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Unlocks the window.
 *
 * ----------------------------------------------------------------------------
 */

void
grSunWReleaseLock()
{
    if (grSunWDoLock)
    {
	/* Make sure that we are holding a lock on this window.  Magic should
	 * never make nested lock calls.
	 */
	ASSERT(grCurSunWData->gr_pw->pw_clipdata->pwcd_lockcount == 1, "grSunWReleaseLock");
	pw_reset(grCurSunWData->gr_pw);
	/* Bypass undocumented SUN "feature":  releasing a window lock can
	 * reset the current write mask.  Sooo... whenever we reset a lock
	 * we reset the write mask.  Aaarrggghhh....
	 */
	pw_putattributes(grCurSunWData->gr_pw, &sunWriteMask);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * SunWLock --
 *
 *	Lock a window for redisplay
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Locks the window and sets variables up.
 *
 * ----------------------------------------------------------------------------
 */

Void
SunWLock(w, inside)
    Window *w;          /* The window to lock, or GR_LOCK_SCREEN if the
			 * whole screen.
			 */
    bool inside;        /* If TRUE, clip to inside of window, otherwise clip
			 * to outside of window.
			  */
{
    grSimpleLock(w, inside);
    if (w == (Window *) GR_LOCK_SCREEN) {
	TxError("Full screen locking not implemented on the Sun --\n");
	TxError("is the code doing something wrong?\n");
	return;
    };

    grCurSunWData = (grSunWRec *) w->w_grdata;
    pw_exposed(grCurSunWData->gr_pw);
    grSunWGetLock();
    pw_exposed(grCurSunWData->gr_pw);
}

/*
 * ----------------------------------------------------------------------------
 *
 * SunWUnlock --
 *
 *	Unlock a window for redisplay
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Unlocks the window and sets variables up.
 *
 * ----------------------------------------------------------------------------
 */

Void
SunWUnlock(w)
    Window *w;
{
    grSimpleUnlock(w);
    grSunWReleaseLock();
    grCurSunWData = NULL;
}


/*---------------------------------------------------------
 * sunWSetWMandC:
 *	This is a local routine that resets the value of the current
 *	write mask and color.  Since each window has it's own write mask,
 *	we won't try to be clever by keeping the old mask around like
 *	the AED does.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Writes in the future to this window will only affect the bitplane
 *	present in 'mask'.
 *
 *---------------------------------------------------------
 */

Void
sunWSetWMandC(mask, color)
    int mask;			/* New value for write mask */
    int color;			/* New value for current color */
{
    GR_CHECK_LOCK();
    sunWriteMask = mask;
    pw_putattributes(grCurSunWData->gr_pw, &sunWriteMask);
    sunWColor = color;
}

/* Handy proc for debugging -- call it from DBX. */
sunWPrintMask()
{
    int mask;
    if (grCurSunWData == NULL)
	printf("No write mask -- not currently drawing in a window!\n");
    else
    {
	pw_getattributes(grCurSunWData->gr_pw, &mask);
	printf("Write mask is 0x%x (%dd), should be 0x%x.\n", 
	    mask, mask, sunWriteMask);
    }
}

/*---------------------------------------------------------
 * sunWSetLineStyle:
 *	This local routine sets the current line style.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A new line style is output to the display.
 *---------------------------------------------------------
 */

Void
sunWSetLineStyle(style)
    int style;			/* New stipple pattern for lines. */
{
    /* NO SUPPORT FOR THIS, ALL LINES ARE SOLID */
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrSunFigureLocation --
 *
 *	Figure out the location for the next window.
 *
 * Results:
 *	Fills in the rectangle passed.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
GrSunFigureLocation(rect)
    Rect *rect;
{
    struct rect r, rdummy;
    rect_construct(&r, WMGR_SETPOS, WMGR_SETPOS, WMGR_SETPOS, WMGR_SETPOS);


#ifdef	notdef	/* Bypass bug in wmgr_figuretoolrect */
    wmgr_figuretoolrect(fileno(grSunRootWindow), &r);
#endif	notdef

#ifdef	stillstrange
    wmgr_get_placeholders(&r, &rdummy);	/* Should fix figuretoolrect bug */
    r.r_width = 570;
    r.r_height = 532;
    rect->r_xbot = r.r_left;
    rect->r_xtop = r.r_left + r.r_width - 1;
    rect->r_ybot = INVERTY(r.r_top + r.r_height - 1);
    rect->r_ytop = INVERTY(r.r_top);
#endif	stillstrange

/* Cheap fix for wmgr_figuretoolrect problem */
    /*
     * Use windows that are 570 wide and 532 high because
     * that size is what you get for shell windows using
     * the screen.r.12 font (which allows you to put two
     * windows side-by-side).
     */
    rect->r_xbot = 7;
    rect->r_xtop = 577;
    rect->r_ybot = GrScreenRect.r_ytop - 532;
    rect->r_ytop = GrScreenRect.r_ytop;
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrSunWCreateWindow --
 *
 *	Create a new Sun window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
GrSunWCreateWindow(w)
    Window *w;
{
    grSunWRec *gdata;
    int fd, key;
    struct rect iconrect;
    struct inputmask imask;
    char windowname[WIN_NAMESIZE];

    gdata = (grSunWRec *) mallocMagic(sizeof(grSunWRec));
    ASSERT(gdata != NULL, "GrSunWCreateWindow");

    /* 
     * Open the window 
     */
    fd = win_getnewwindow();
    if (fd < 0) {
	TxError("Sorry, could not create window, %s\n",
	    "(you probably have too many of them).");
	return FALSE;
    }

    /*
     * Set window position and redrawing parameters.
     * Location of the window is set by the window package, with
     * help from GrSunFigureLocation() -- but here we will set the icon
     * position.
     */
    rect_construct(&iconrect, WMGR_SETPOS, WMGR_SETPOS, 
	WMGR_SETPOS, WMGR_SETPOS);
    wmgr_figureiconrect(fileno(grSunRootWindow), &iconrect);
    win_setsavedrect(fd, &iconrect);

    win_setlink(fd, WL_PARENT, win_fdtonumber(fileno(grSunRootWindow)));

    /*
     * Set up input parameters -- accept up & down on buttons, down on ASCII
     */
    input_imnull(&imask);
    imask.im_flags |= (IM_ASCII | IM_NEGEVENT | IM_POSASCII);
    win_setinputcodebit(&imask, MS_LEFT);
    win_setinputcodebit(&imask, MS_MIDDLE);
    win_setinputcodebit(&imask, MS_RIGHT);
    for (key = ASCII_FIRST; key <= ASCII_LAST; key++)
	win_setinputcodebit(&imask, key);
    win_setinputmask(fd, &imask, NULL, WIN_NULLLINK);

    /* 
     * All done, insert into screen and create pixwin for drawing.
     */
    win_insert(fd);
    wmgr_open(fd, fileno(grSunRootWindow));
    gdata->gr_fd = fd;
    gdata->gr_pw = pw_open(fd);
    if (gdata->gr_pw == NULL) {
	TxError("Could not create pixwin for new window.\n");
	return FALSE;
    }
    gdata->gr_w = w;
    w->w_grdata = (ClientData) gdata;
    pw_setcmsname(gdata->gr_pw, sunCMapName);
    GrResetCMap();
    WindSeparateRedisplay(w);

    TxAddInputDevice((1 << fd), sunWInput, (ClientData) gdata);
    win_fdtoname(fd, windowname);
    SigWatchFile(fd, windowname);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrSunDeleteWindow --
 *
 *	Delete a new Sun window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

Void
GrSunWDeleteWindow(w)
    Window *w;
{
    grSunWRec *gdata;
    gdata = (grSunWRec *) w->w_grdata;
    ASSERT(gdata != NULL, "GrSunWDeleteWindow");
    TxDeleteInputDevice((1 << gdata->gr_fd));
    pw_close(gdata->gr_pw);
    close(gdata->gr_fd);
    freeMagic(gdata);
}

/*---------------------------------------------------------
 * sunWSetSPattern:
 *	sunWSetSPattern associates a stipple pattern with a given
 *	stipple number.  This is a local routine called from
 *	grStyle.c .
 *
 * Results:	None.
 *
 * Side Effects:
 *	The eight low-order bytes in pattern are set to be used
 *	whenever the given stipple number is set by the routine
 *	sunWSetStipple below.
 *---------------------------------------------------------
 */

Void
sunWSetSPattern(stipple, pattern)
    int stipple;			/* The stipple number, 1-grMaxStipples.
					 */
    int pattern[8];			/* 8 8-bit pattern integers */
{
    int x, y, pat;
    struct pixrect *pr;

    if ((stipple < 0) || (stipple >= grMaxStipples))
    {
	TxError("Can not handle stipple number %d\n", stipple);
	return;
    }

    pr = mem_create(STIPMASKSIZE, STIPMASKSIZE, 1);
    grSunWStipples[stipple] = pr;

    /* fill in 8 X 8 square */
    for (y = 0; y < 8; y++)
    {
	pat = pattern[y];
	for (x = 0; x < 8; x++)
	{
	    pr_put(pr, x, y, (pat & 1));
	    pat = pat >> 1;
	}
    }

    /*
     * Duplicate it.
     * First make a strip STIPMASKSIZE pixels wide, and then
     * copy it (STIPMASKSIZE/8 - 1) times to complete the
     * STIPMASKSIZE x STIPMASKSIZE stencil pixrect.
     *
     * pr_rop(prDst, dx, dy, width, height, op, prSrc, sx, sy);
     */
#if	(STIPMASKSIZE % 8 == 0)
    for (x = 8; x < STIPMASKSIZE; x += 8)
	pr_rop(pr, x, 0, 8, 8, PIX_SRC, pr, 0, 0);
    for (y = 8; y < STIPMASKSIZE; y += 8)
	pr_rop(pr, 0, y, STIPMASKSIZE, 8, PIX_SRC, pr, 0, 0);
#else
    this is an error -- STIPMASKSIZE is not a multiple of 8
#endif

    /****  Preview the stipples on the screen for debugging
    {
	char bla[100];
	pr_rop(grSunWCpr, 10, 10, STIPMASKSIZE, STIPMASKSIZE,
			PIX_SRC | PIX_COLOR(0),
	    NULL, 0, 0);
	pr_stencil(grSunWCpr, 10, 10, STIPMASKSIZE, STIPMASKSIZE,
			PIX_SRC | PIX_COLOR(1),
	    pr, 0, 0, NULL, 0, 0);
	TxPrintf("Stipple number %d -- hit <cr>\n", stipple);
	TxGetLine(bla, 10);
    }
    ****/
}


/*---------------------------------------------------------
 * sunWSetStipple:
 *	This routine sets the Sun's current stipple number.
 *
 * Results: None.
 *
 * Side Effects:
 *	The current stipple number in the Sun is set to stipple,
 *	if it wasn't that already.
 *---------------------------------------------------------
 */

Void
sunWSetStipple(stipple)
    int stipple;			/* The stipple number to be used. */
{
    if ((stipple == 0) || (stipple >= grMaxStipples))
	sunWStipple = NULL;
    else
	sunWStipple = grSunWStipples[stipple];
}

/*---------------------------------------------------------
 * SunWInit:
 *	SunInit initializes the graphics display and clears its screen.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The display is re-initialized and its color map is reset.
 *---------------------------------------------------------
 */

Void
SunWInit()
{
    sunWColor = 0377;
    sunWStipple = 0;
    grSunWTextInit();
}


/*---------------------------------------------------------
 * SunWFlush:
 *	Flush output to the color display.
 *
 * Results:	None.
 *
 * Side Effects:
 *	None!
 *---------------------------------------------------------
 */
SunWFlush()
{
}


/*---------------------------------------------------------
 * SunWClose:
 *	SunWClose does whatever is necessary to reset the characteristics
 *	of the Sun after the program is finished.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The graphics display modes are reset.
 *---------------------------------------------------------
 */

Void
SunWClose()
{
}

 
/*
 * ----------------------------------------------------------------------------
 * GrSunWRepairDamage --
 *
 *	We recieved a SIGWINCH -- detect any screen damage and record the
 *	areas via calls to WindAreaChanged().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates our list of redisplay areas.
 * ----------------------------------------------------------------------------
 */

Void
GrSunWRepairDamage()
{
    extern int sunWDamageFunc();
    WindSearch(NULL, NULL, NULL, sunWDamageFunc, (ClientData) NULL);
}

/* Proc for each window, reads the damage and calls WindAreaChanged() */
int
sunWDamageFunc(w, cdata)
    Window *w;
    ClientData cdata;
{
    grSunWRec *grdata;
    int xoffset, yoffset;
    struct rectnode *rn;
    Rect r;

    grdata = (grSunWRec *) w->w_grdata;
    ASSERT(grdata != NULL, "sunWDamageFunc");
    pw_damaged(grdata->gr_pw);

    xoffset = grdata->gr_pw->pw_clipdata->pwcd_clipping.rl_x;
    yoffset = grdata->gr_pw->pw_clipdata->pwcd_clipping.rl_y;

    if (w->w_flags & WIND_ISICONIC) 
	WindIconChanged(w);
    else {
	/* Loop through the damaged areas */
	for (rn = grdata->gr_pw->pw_clipdata->pwcd_clipping.rl_head; 
	  rn != NULL; rn = rn->rn_next) {
	    r.r_xbot = rn->rn_rect.r_left;
	    r.r_xtop = rn->rn_rect.r_left + rn->rn_rect.r_width - 1;
	    r.r_ybot = w->w_allArea.r_ytop - 
		(rn->rn_rect.r_top + rn->rn_rect.r_height - 1);
	    r.r_ytop = w->w_allArea.r_ytop - (rn->rn_rect.r_top);
	    WindAreaChanged(w, &r);
	}
    }

    pw_donedamaged(grdata->gr_pw);
    return 0;
}


/*---------------------------------------------------------
 * grSunWCreateScreen:
 *	Initialize the interface to sunwindows and pixrects.
 *
 * Results:	None.
 *
 * Side Effects:
 *	initialization
 *---------------------------------------------------------
 */

grSunWCreateScreen()
{
    struct screen scrn;
    char windparent[WIN_NAMESIZE];
    char *envw;
    struct rect screenRect;

    /* find the root window of the screen's window system */
    envw = getenv("WINDOW_PARENT");
    if (envw == NULL)
    {
	/* No suntools!  */
	TxError("You are not running under suntools.  Use '-d NULL' if\n");
	TxError("you don't need graphics.\n");
	MainExit(1);
    }
    else
    {
	/* Find the root window for this screen */

	strcpy(windparent, envw);

	/* Get file descriptor for root window on this screen */
	while (TRUE)
	{
	    grSunRootWindow = fopen(windparent, "r");
	    if (grSunRootWindow == NULL)
	    {
		TxError("Could not open file '%s'\n", windparent);
		MainExit(1);
	    }

	    if (win_getlink(fileno(grSunRootWindow), WL_PARENT) != WIN_NULLLINK)
	    {
		int winnum;
		winnum = win_getlink(fileno(grSunRootWindow), WL_PARENT);
		win_numbertoname(winnum, windparent);
		(void) fclose(grSunRootWindow);
	    }
	    else
		break;
	}
	
	/* Get the screen size -- Sun's coordinate system is upside down! */
	win_getrect(fileno(grSunRootWindow), &screenRect);
	GrScreenRect.r_xbot = screenRect.r_left;
	GrScreenRect.r_ybot = screenRect.r_top;
	GrScreenRect.r_xtop = rect_right(&screenRect);
	GrScreenRect.r_ytop = rect_bottom(&screenRect);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * sunWStdin --
 *
 *      Handle the stdin device for the SUN driver.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Adds events to the event queue.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
sunWStdin(fd, cdata)
    int fd;
    ClientData cdata;
{
    int ch;
    TxInputEvent *event;

    event = TxNewEvent();
    ch = getc(stdin);
    if (ch == EOF)
	event->txe_button = TX_EOF;
    else
	event->txe_button = TX_CHARACTER;
    event->txe_ch = ch;
    event->txe_wid = WIND_NO_WINDOW;
    event->txe_p.p_x = GR_CURSOR_X;
    event->txe_p.p_y = GR_CURSOR_Y;
    TxAddEvent(event);
}



/*---------------------------------------------------------
 * sunWSetDisplay:
 *	This routine sets the appropriate parameters so that
 *	Magic will work with the particular display type.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Depends on the display type.
 *---------------------------------------------------------
 */
 /*ARGSUSED*/

sunWSetDisplay(type, colorScreenName, mouseName)
    char *type;			/* Currently, must be SUNBW or SUN160 */
    char *colorScreenName;
{
    extern Void SunWFlush(), sunWStdin();

    grSunWCreateScreen();
    WindPackageType = WIND_SUN_WINDOWS;
    WindScrollBarWidth = 11;

    /* Since we use up one file descriptor for each window, only allow 7 of
     * them at one time.
     */
    WIND_MAX_WINDOWS(7);	

    if (strcmp(type, "SUNBW") == 0)
	sunWIntType = SUNTYPE_BW;
    else if (strcmp(type, "SUN60") == 0) /* code for 110 works fine on 60 */
	sunWIntType = SUNTYPE_110;
    else if (strcmp(type, "SUN110") == 0)
	sunWIntType = SUNTYPE_110;

    if (sunWIntType == SUNTYPE_BW)
	(void) strcpy(sunCMapName, CMS_MONOCHROME);
    else
	(void) sprintf(sunCMapName, "Magic.%d", getpid());

    TxAddInputDevice((1 << fileno(stdin)), sunWStdin, (ClientData) NULL);
    if (TxStdinIsatty) SigWatchFile(fileno(stdin), "stdin");

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = SunWLock;
    GrUnlockPtr = SunWUnlock;
    GrInitPtr = SunWInit;
    GrClosePtr = SunWClose;
    GrSetCMapPtr = SunWSetCMap;
    GrCreateWindowPtr = GrSunWCreateWindow;
    GrDeleteWindowPtr = GrSunWDeleteWindow;
    GrDamagedPtr = GrSunWRepairDamage;

    GrEnableTabletPtr = SunWEnableTablet;
    GrDisableTabletPtr = SunWDisableTablet;
    GrSetCursorPtr = SunWSetCursor;
    GrTextSizePtr = SunWTextSize;
    GrDrawGlyphPtr = SunWDrawGlyph;
    GrBitBltPtr = SunWBitBlt;
    GrReadPixelPtr = SunWReadPixel;
    GrFlushPtr = SunWFlush;

    /* local indirections */
    grSetSPatternPtr = sunWSetSPattern;
    grPutTextPtr = sunWPutText;
    grDefineCursorPtr = sunWDefineCursor;
    grDrawGridPtr = sunWDrawGrid;
    grDrawLinePtr = sunWDrawLine;
    grSetWMandCPtr = sunWSetWMandC;
    grFillRectPtr = sunWFillRect;
    grSetStipplePtr = sunWSetStipple;
    grSetLineStylePtr = sunWSetLineStyle;
    grSetCharSizePtr = sunWSetCharSize;
    grMaxStipples = GR_NUM_STIPPLES;

    /* display-dependent initialization */
    if (sunWIntType == SUNTYPE_BW) 
    {
	grDStyleType = "bw";
	sunWriteMask = 0x01;
    } else {
	grDStyleType = "7bit";
	sunWriteMask = 0x7F;
    }

    SunWInit();
    return TRUE;
}

#endif	 sun
