/* Sun1.c -
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
 * This file contains primitive functions to manipulate a Sun
 * color display.  Included here are initialization and closing
 * functions, and several utility routines used by the other Sun
 * modules.
 */


#ifndef lint
static char rcsid[]="$Header: grSun1.c,v 6.0 90/08/28 18:40:53 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <stdio.h>
#include <sgtty.h>
#include <errno.h>
#include <sunwindow/window_hs.h>
#undef bool
#define	Rect	MagicRect	/* Avoid Sun's definition of Rect. */
#include <signal.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grSunInt.h"
#include "txcommands.h"
#include "utils.h"
#include "textio.h"
#include "signals.h"
#include "paths.h"


/* IO files and stuff */
int grSunColorWindowFD = 0;
FILE *grSunFileReqPoint = NULL;	
FILE *grSunFilePoint = NULL; 
FILE *grSunFileButtons = NULL; 
struct pixrect *grSunCpr = NULL;
int grSunButtonProgID = 0;
bool grSunNoSuntools = FALSE;

/* Imports from other Sun modules: */
extern int SunGetButtons(), SunReadPixel();
extern bool sunGetCursorPos(), sunDrawGrid();
extern Void SunEnableTablet(), SunDisableTablet();
extern Void SunSetCMap(), sunPutText(), sunDefineCursor();
extern Void SunSetCursor(), SunTextSize(), SunDrawGlyph(), SunBitBlt();
extern Void sunDrawLine(), sunSetLineStyle(), sunSetCharSize();
extern Void sunSetWMandC(), sunFillRect();


/* The following variables are used to hold state that we keep around
 * between procedure calls in order to reduce the amount of information
 * that must be shipped to the terminal.
 */

extern sunCurCharSize;		/* Current character size */
int sunWMask;			/* Current write mask value */
int sunColor;			/* Current color value */
struct pixrect *sunStipple = NULL; /* Current stipple */

/* The following variables hold the terminal state so we can restore
 * it when we are finished.
 */

static struct sgttyb sunsgttyb;	/* Used to save terminal control bits */
static int sunsgflags;		/* Used to save flags from Sunsgttyb */
static char sunsgispeed,	/* Used to save baud rates */
    sunsgospeed;
static int sunlocalmode;	/* Used to save terminal local mode word */

/* Our stipple patterns */
struct pixrect *grSunStipples[GR_NUM_STIPPLES];

extern char *getenv();
extern FILE *fopen();


/*---------------------------------------------------------
 * sunSetWMandC:
 *	This is a local routine that resets the value of the current
 *	write mask and color, if necessary.
 *
 * Results:	None.
 *
 * Side Effects:
 *	If SunWMask is different from mask, then the new mask is output to
 *	the display and stored in SunWMask.  The same holds for SunColor
 *	and color.
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

Void
sunSetWMandC(mask, color)
    int mask;			/* New value for write mask */
    int color;			/* New value for current color */
{
    if (sunWMask != mask)
    {
	sunWMask = mask;
	pr_putattributes(grSunCpr, &sunWMask);
    }
    sunColor = color;
}




/*---------------------------------------------------------
 * sunSetLineStyle:
 *	This local routine sets the current line style.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A new line style is output to the display.
 *---------------------------------------------------------
 */

Void
sunSetLineStyle(style)
    int style;			/* New stipple pattern for lines. */
{
    /* NO SUPPORT FOR THIS, ALL LINES ARE SOLID */
}


/*---------------------------------------------------------
 * sunSetSPattern:
 *	sunSetSPattern associates a stipple pattern with a given
 *	stipple number.  This is a local routine called from
 *	grStyle.c .
 *
 * Results:	None.
 *
 * Side Effects:
 *	The eight low-order bytes in pattern are set to be used
 *	whenever the given stipple number is set by the routine
 *	sunSetStipple below.
 *---------------------------------------------------------
 */

Void
sunSetSPattern(stipple, pattern)
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
    grSunStipples[stipple] = pr;

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
	pr_rop(grSunCpr, 10, 10, STIPMASKSIZE, STIPMASKSIZE,
			PIX_SRC | PIX_COLOR(0),
	    NULL, 0, 0);
	pr_stencil(grSunCpr, 10, 10, STIPMASKSIZE, STIPMASKSIZE,
			PIX_SRC | PIX_COLOR(1),
	    pr, 0, 0, NULL, 0, 0);
	TxPrintf("Stipple number %d -- hit <cr>\n", stipple);
	TxGetLine(bla, 10);
    }
    ****/
}


/*---------------------------------------------------------
 * sunSetStipple:
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
sunSetStipple(stipple)
    int stipple;			/* The stipple number to be used. */
{
    if ((stipple == 0) || (stipple >= grMaxStipples))
	sunStipple = NULL;
    else
	sunStipple = grSunStipples[stipple];
}


/*---------------------------------------------------------
 * SunInit:
 *	SunInit initializes the graphics display and clears its screen.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The display is re-initialized and its color map is reset.
 *---------------------------------------------------------
 */

Void
SunInit()
{
    sunStipple = 0;
    sunSetWMandC(0377, 0);
    sunFillRect(&GrScreenRect);
    grSunTextInit();
}


/*---------------------------------------------------------
 * SunFlush:
 *	Flush output to the color display.
 *
 * Results:	None.
 *
 * Side Effects:
 *	None!
 *---------------------------------------------------------
 */
SunFlush()
{
}


/*---------------------------------------------------------
 * SunClose:
 *	SunClose does whatever is necessary to reset the characteristics
 *	of the Sun after the program is finished.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The graphics display modes are reset.
 *---------------------------------------------------------
 */

Void
SunClose()
{
    close(grSunColorWindowFD);
    if (grSunButtonProgID != 0)
    {
	kill(SIGKILL, grSunButtonProgID);
	grSunButtonProgID == 0;
    }
    if (grSunFileReqPoint != NULL) (void) fclose(grSunFileReqPoint);
    if (grSunFilePoint != NULL) (void) fclose(grSunFilePoint);
    if (grSunFileButtons != NULL) (void) fclose(grSunFileButtons);
}


/*---------------------------------------------------------
 * grSigWinch:
 *	Catch SIGWINCH signals (sent when the window size changes).
 *
 * Results:	None.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */
grSunSigWinch()
{
}



/*---------------------------------------------------------
 * grSunMakeButtonProg:
 *	Run a program to watch the color window.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Starts another process.
 *---------------------------------------------------------
 */
grSunMakeButtonProg(windName)
    char *windName;		/* The name (/dev/winXX) to read events
				 * from.
				 */
{
    FILE *dum;
    char *progName;
    int pipeReq[2], pipePoint[2], pipeButtons[2];
    int vforkres;
    extern bool mainDebug;
    char notifyPID[40];  /* PID of the process to recieve SIGIO signals */

    /* Do path expansion on the program name. */
    dum = PaOpen(BUTTON_PROG, "r", (char *) NULL, HELPER_PATH, 
	SysLibPath, &progName);
    if (dum == NULL) 
    {
	TxError("Could not find program '%s'\n", BUTTON_PROG);
	MainExit(1);
    }
    (void) fclose(dum);

    if ( (pipe(pipeReq) == -1) || (pipe(pipePoint) == -1) || 
	 (pipe(pipeButtons) == -1) )
    {
	TxError("Could not create pipe to program '%s'\n", BUTTON_PROG);
	MainExit(1);
    }


    if (!mainDebug) 
	sprintf(notifyPID, "%d", getpid());
    else	
	strcpy(notifyPID, "0"); /* don't send signals if we are debugging */

    vforkres = vfork();
    if (vforkres == -1)
    {
	perror("Magic:");
	TxError("Could not execute 'vfork'\n");
	MainExit(1);
    }

    if (vforkres == 0)
    {
	char *win;
	char request[10], point[10], button[10];

	/* child process */
	sprintf(request, "%d", pipeReq[0]);
	sprintf(point, "%d", pipePoint[1]);
	sprintf(button, "%d", pipeButtons[1]);
	close(pipeReq[1]);
	close(pipePoint[0]);  
	close(pipeButtons[0]);

	/****
	TxError("Executing '%s %s %s %s %s %s %s'\n", 
	    progName, windName, getenv("WINDOW_ME"),  notifyPID, 
	    request, point, button);
	****/
	execl(progName, progName, windName, getenv("WINDOW_ME"),  notifyPID, 
	    request, point, button, 0);
	/* execl never returns unless something is wrong */
	perror("Magic: grSunMakeButtonProg execl returned!");
	_exit(1);
    }
    else
    {
	/* parent process */
	grSunFileReqPoint = fdopen(pipeReq[1], "w");
	setbuf(grSunFileReqPoint, NULL);
	grSunFilePoint = fdopen(pipePoint[0], "r");
	setbuf(grSunFilePoint, NULL);
	grSunFileButtons = fdopen(pipeButtons[0], "r");
	setbuf(grSunFileButtons, NULL);

	close(pipeReq[0]);
	close(pipePoint[1]);
	close(pipeButtons[1]);

	if ((grSunFileReqPoint == NULL) || (grSunFilePoint == NULL) ||
	    (grSunFileButtons == NULL))
	{
	    TxError("Could not set up files to program '%s'\n", BUTTON_PROG);
	    MainExit(1);
	}

	/* Check to make sure that we don't already have a process running! */
	/* If there is it should have been killed by GrClose() first. */
	ASSERT(grSunButtonProgID == 0, "grSunMakeButtonProg");
	grSunButtonProgID = vforkres;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * sunStdin --
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
sunStdin(fd, cdata)
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
    event->txe_wid = WIND_UNKNOWN_WINDOW;
    if (!sunGetCursorPos(&(event->txe_p))) {
        TxError("Could not read cursor position from Sun, using (%d, %d) instead.\n",
	GR_CURSOR_X, GR_CURSOR_Y);
	event->txe_p.p_x = GR_CURSOR_X;
	event->txe_p.p_y = GR_CURSOR_Y;
    };
    TxAddEvent(event);
}


/*---------------------------------------------------------
 * grSunCreateDevices:
 *	Determine what input devices to use, and initialize them.
 *
 * Results:	None.
 *
 * Side Effects:
 *	initialization
 *---------------------------------------------------------
 */

grSunCreateDevices(mouseName)
    char *mouseName;
{
    extern int grSunHandleButtons();

    /* set up the keyboard */
    TxAddInputDevice((1 << fileno(stdin)), sunStdin, (ClientData) NULL);
    if (TxStdinIsatty) SigWatchFile(fileno(stdin), "stdin");

    /* Set up the color window. */
    if (!grSunNoSuntools) {
	TxAddInputDevice(1 << fileno(grSunFileButtons), grSunHandleButtons,
	    (ClientData) NULL);
	SigWatchFile(fileno(grSunFileButtons), "pipe");
    };
}


/*---------------------------------------------------------
 * grSunCreateScreen:
 *	Initialize the interface to sunwindows and pixrects.
 *
 * Results:	None.
 *
 * Side Effects:
 *	initialization
 *---------------------------------------------------------
 */

grSunCreateScreen(devname)
    char *devname;
{
    int bwrootfd;
    struct screen scrn;
    char windparent[WIN_NAMESIZE];
    char *envw;
    FILE *bwrootfile;

    if (grSunCpr != NULL)
	return;

    signal(SIGWINCH, grSunSigWinch);

    /* create a root window on the color screen */
    strcpy(scrn.scr_rootname, "");
    strcpy(scrn.scr_kbdname, "");
    strcpy(scrn.scr_msname, "");
    strcpy(scrn.scr_fbname, devname);
    scrn.scr_background.red = 0;
    scrn.scr_background.green = 0;
    scrn.scr_background.blue = 0;
    scrn.scr_background.red = 255;
    scrn.scr_background.green = 255;
    scrn.scr_background.blue = 255;
    scrn.scr_flags = 0;
    scrn.scr_rect.r_left = scrn.scr_rect.r_top = 0;
    scrn.scr_rect.r_width = scrn.scr_rect.r_height = 0;

    grSunColorWindowFD = win_screennew(&scrn);
    /* note: contrary to manual, win_screennew returns -1 if error */
    if ((grSunColorWindowFD == NULL) || (grSunColorWindowFD == -1))
    {
	TxError("Magic could not open the color screen.\n");
	TxError("(Maybe some program is already using it?)\n");
	MainExit(1);
    }

    /* find the root window of the black & white window system */
    envw = getenv("WINDOW_PARENT");
    if (envw == NULL)
    {
	char *nameoftty;
	extern char *ttyname();

	nameoftty = ttyname(fileno(stdin));
	if ((nameoftty == NULL) || (strcmp(nameoftty, "/dev/console") == 0)) {
	    /* Don't ask my why, but we get hung up when on the console, so
	     * we will just avoid the situation.  Probably indicates a
	     * sun bug.
	     */
	    TxError("Magic can't run from the console directly.  Use suntools\n");
	    TxError("or the '-d NULL' switch if you don't need the graphics.\n");
	    MainExit(1);
	};
	/* No suntools!  This means we can't forward commands from the color
	 * window over to the text window, because there is no text window!
	 */
	TxError("Warning:  You are not running under suntools.  You won't be able to type\n");
	TxError("          commands while pointing at the graphics display.\n");
	grSunNoSuntools = TRUE;
    }
    else
    {
	{
	    char str[WIN_NAMESIZE];
	    win_fdtoname(grSunColorWindowFD, str);
	    grSunMakeButtonProg(str);
	}

	strcpy(windparent, envw);

	/* get file descriptor for root window on black & white screen */
	while (TRUE)
	{
	    bwrootfile = fopen(windparent, "r");
	    if (bwrootfile == NULL)
	    {
		TxError("Could not open file '%s'\n", windparent);
		MainExit(1);
	    }

	    bwrootfd = fileno(bwrootfile);

	    if (win_getlink(bwrootfd, WL_PARENT) != WIN_NULLLINK)
	    {
		int winnum;
		winnum = win_getlink(bwrootfd, WL_PARENT);
		win_numbertoname(winnum, windparent);
		(void) fclose(bwrootfile);
	    }
	    else
		break;
	}

	/* declare that the 2 screens are adjacent */
	{
	    int neighbors[SCR_POSITIONS];

	    win_getscreenpositions(bwrootfd, neighbors);
	    neighbors[SCR_EAST] = win_fdtonumber(grSunColorWindowFD);
	    neighbors[SCR_WEST] = win_fdtonumber(grSunColorWindowFD);
	    win_setscreenpositions(bwrootfd, neighbors);

	    win_getscreenpositions(grSunColorWindowFD, neighbors);
	    neighbors[SCR_EAST] = win_fdtonumber(bwrootfd);
	    neighbors[SCR_WEST] = win_fdtonumber(bwrootfd);
	    win_setscreenpositions(grSunColorWindowFD, neighbors);
	}

	(void) fclose(bwrootfile);
    }


    /* set up a pixrect for the whole screen */
    if (strcmp(devname, "/dev/null") == 0)
    {
	/* substitute an in-core pixrect instead */
	grSunCpr = mem_create(1024, 1024, 8);
    }
    else
	grSunCpr = pr_open(devname);
    if (grSunCpr == NULL) 
    {
	TxError("Could not open display from '%s'\n", devname);
	MainExit(1);
    }
}



/*---------------------------------------------------------
 * sunSetDisplay:
 *	This routine sets the appropriate parameters so that
 *	Magic will work with the particular display type.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Depends on the display type.
 *---------------------------------------------------------
 */

sunSetDisplay(type, colorScreenName, mouseName)
    char *type;			/* must be SUN2 */
    char *colorScreenName;
{
    grSunCreateScreen(colorScreenName);
    grSunCreateDevices(mouseName);

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = grSimpleLock;
    GrUnlockPtr = grSimpleUnlock;
    GrInitPtr = SunInit;
    GrClosePtr = SunClose;
    GrSetCMapPtr = SunSetCMap;

    GrEnableTabletPtr = SunEnableTablet;
    GrDisableTabletPtr = SunDisableTablet;
    GrSetCursorPtr = SunSetCursor;
    GrTextSizePtr = SunTextSize;
    GrDrawGlyphPtr = SunDrawGlyph;
    GrBitBltPtr = SunBitBlt;
    GrReadPixelPtr = SunReadPixel;
    GrFlushPtr = SunFlush;

    /* local indirections */
    grSetSPatternPtr = sunSetSPattern;
    grPutTextPtr = sunPutText;
    grDefineCursorPtr = sunDefineCursor;
    grDrawGridPtr = sunDrawGrid;
    grDrawLinePtr = sunDrawLine;
    grSetWMandCPtr = sunSetWMandC;
    grFillRectPtr = sunFillRect;
    grSetStipplePtr = sunSetStipple;
    grSetLineStylePtr = sunSetLineStyle;
    grSetCharSizePtr = sunSetCharSize;

    GrScreenRect.r_xtop = 639;
    GrScreenRect.r_ytop = 474;

    SunInit();
    return TRUE;
}

#endif	sun
