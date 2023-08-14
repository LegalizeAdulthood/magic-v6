/* grX11su1.c -
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
 * This file contains primitive functions to manipulate an X window system
 * Included here are initialization and closing
 * functions, and several utility routines used by the other X
 * modules.
 */

#include <stdio.h>
#include <sgtty.h>
#ifdef	SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "txcommands.h"
#include "signals.h"
#include "utils.h"
#include "hash.h"
#include "grX11conf.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "grX11Int.h"
#include <signal.h>
#include "paths.h"

Display *grXdpy;
int	grXscrn;
GR_CURRENT grCurrent= {0,0,0,0,0,0};
GC grGCFill, grGCText, grGCDraw, grGCCopy, grGCGlyph, grGCStipple;
int grXWStdin();
int grXuserPC = 0;
char grXdepth[8];

Pixmap grX11Stipples[GR_NUM_STIPPLES];
HashTable	grX11WindowTable;
/* locals */

int pipeRead,pipeWrite;

typedef struct {
    char dashlist[8];
    int  dlen;
} LineStyle;

static LineStyle LineStyleTab[256];

int Xhelper;

#define grMagicToXs(n) (DisplayHeight(grXdpy,grXscrn)-(n))
#define grXsToMagic(n) (DisplayHeight(grXdpy,grXscrn)-(n))

/* This is kind of a long story, and very kludgy, but the following
 * things need to be defined as externals because of the way lint
 * libraries are made by taking this module and changing all procedures
 * names "Xxxx" to "Grxxx".  The change is only done at the declaration
 * of the procedure, so we need these declarations to handle uses
 * of those names, which don't get modified.  Check out the Makefile
 * for details on this.
 */

extern Void GrX11Close(), GrX11Flush(), GrX11Init(),GrX11Create();
extern Void GrX11Delete(),GrX11Configure(),GrX11Raise(),GrX11Lower();
extern Void GrX11Lock(),GrX11IconUpdate();


/*---------------------------------------------------------
 * grxSetWMandC:
 *	This is a local routine that resets the value of the current
 *	write mask and color, if necessary.
 *
 * Results:	None.
 *
 * Side Effects:    None.
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

Void
grx11SetWMandC (mask, c)
    int mask;			/* New value for write mask */
    int c;			/* New value for current color */
{

    c = grPixels[c];
    mask = grPlanes[mask]; 

    XSetPlaneMask(grXdpy,grGCFill,mask);
    XSetPlaneMask(grXdpy,grGCDraw,mask);
    XSetPlaneMask(grXdpy,grGCText,mask);

    XSetForeground(grXdpy,grGCFill,c);
    XSetForeground(grXdpy,grGCDraw,c);
    XSetForeground(grXdpy,grGCText,c);
}


/*---------------------------------------------------------
 * grxSetLineStyle:
 *	This local routine sets the current line style.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A new line style is output to the display.
 *
 *---------------------------------------------------------
 */

Void
grx11SetLineStyle (style)
    int style;			/* New stipple pattern for lines. */
{
    LineStyle *linestyle;
    int xstyle;

    style &= 0xFF;
    switch (style) {
    case 0xFF:
    case 0x00:
	xstyle = LineSolid;
	break;
    default:
	xstyle = LineOnOffDash;
	linestyle = &LineStyleTab[style];
	if (linestyle->dlen == 0) {

	    /* translate style to an X11 dashlist */

	    char *e;
	    int cnt,offset,cur,new,curnew,i,match;

	    e = linestyle->dashlist;
	    cnt = 0;
	    offset = 1;
	    cur = 0;
	    for (i = 7; i >= 0; i--) {
		new = (style >> i) & 1;
		curnew = (cur << 1) | new;
		switch (curnew) {
		case 0:
		case 3:
		    cnt++;
		    break;
		case 1:
		    if (cnt > 0) *e++ = cnt; else offset = 0;
		    cnt = 1;
		    break;
		case 2:
		    *e++ = cnt;
		    cnt = 1;
		    break;
		}
		cur = new;
	    }
	    *e++ = cnt;
	    cnt = e - linestyle->dashlist;
	    if (offset) {
		cur = e[0];
		for (i = 0; i < cnt-1; i++) e[i] = e[i+1];
		e[cnt-1] = cur;
	    }
	    match = 1;
	    do {
		if (cnt % 2) break;
		for (i = 0; i < cnt/2; i++) {
		    if (e[i] != e[cnt/2 + i]) match = 0;
		}
		if (match == 0) break;
		cnt = cnt/2;
	    } while (match);
	    linestyle->dlen = cnt;
	}
	XSetDashes(grXdpy, grGCDraw, 0,
		   linestyle->dashlist, linestyle->dlen);
    }
    XSetLineAttributes(grXdpy, grGCDraw, 0,
		       xstyle, CapNotLast, JoinMiter);
}


/*---------------------------------------------------------
 * grxSetSPattern:
 *	xSetSPattern associates a stipple pattern with a given
 *	stipple number.  This is a local routine called from
 *	grStyle.c .
 *
 * Results:	None.
 *
 * Side Effects:    None.
 *---------------------------------------------------------
 */

Void
grx11SetSPattern (stipple, pattern)
    int stipple;			/* The stipple number, 1-15. */
    int pattern[8];			/* 8 8-bit patterns integers */
{
    Pixmap p;
    int x,y,pat;

    p = XCreatePixmap(grXdpy, DefaultRootWindow(grXdpy), 8, 8, 1);
    if (grGCStipple == 0) {
	grGCStipple = XCreateGC(grXdpy, p, 0, 0);
    }
    for (y = 0; y < 8; y++) {
	pat = pattern[y];
	for (x = 0; x < 8; x++) {
	    XSetForeground(grXdpy, grGCStipple, pat & 1);
	    XDrawPoint(grXdpy, p, grGCStipple, x, y);
	    pat >>= 1;
	}
    }
    grX11Stipples[stipple] = p;
}


/*---------------------------------------------------------
 * grxSetStipple:
 *	This routine sets the Xs current stipple number.
 *
 * Results: None.
 *
 * Side Effects:
 *	The current clipmask in the X is set to stipple,
 *	if it wasn't that already.
 *---------------------------------------------------------
 */

Void
grx11SetStipple (stipple)
    int stipple;			/* The stipple number to be used. */
{
    if (stipple == 0 || stipple > GR_NUM_STIPPLES) {
	XSetFillStyle(grXdpy, grGCFill, FillSolid);
    } else {
	if (grX11Stipples[stipple] == 0) MainExit(1);
	XSetStipple(grXdpy, grGCFill, grX11Stipples[stipple]);
	XSetFillStyle(grXdpy, grGCFill, FillStippled);
    }
}

bool

/*---------------------------------------------------------
 * GrXInit:
 *	GrXInit initializes the graphics display and clears its screen.
 *	Files must have been previously opened with GrSetDisplay();
 *
 * Results: TRUE if successful.
 *---------------------------------------------------------
 */

GrX11Init ()
{

    grXdpy = XOpenDisplay(NULL); 
    if (grXdpy == NULL)
    {
    	 TxError("Couldn't open display; check DISPLAY variable\n");
	 return false;
    }
    grXscrn = DefaultScreen(grXdpy);

    grCurrent.depth = DefaultDepth(grXdpy, grXscrn);
    grCurrent.window = DefaultRootWindow(grXdpy);
    if (grXuserPC > 0)
    {
    	 if (grXuserPC > grCurrent.depth)
	 {
	      TxError("You cannot specify more planes (%d) than the display has (%d).\n",grXuserPC,grCurrent.depth);
	      return false;
	 }
	 grCurrent.depth = (grXuserPC == 1)?0:grXuserPC;
    }
    else  /* default to nearest depth that we "should" have dstyles and 
    	     colormap for.
	  */
    {
         if      (grCurrent.depth < 4) grCurrent.depth = 0;
         else if (grCurrent.depth < 6) grCurrent.depth = 4;
         else if (grCurrent.depth < 7) grCurrent.depth = 6;
	 else 			       grCurrent.depth = 7;
    }

    grDStyleType = grXdepth;
    if (grCurrent.depth) 
    {
    	 sprintf(grXdepth,"%dbit",grCurrent.depth);
	 grCMapType = grXdepth;
    }
    else
    {
    	 sprintf(grXdepth,"bw");
         grCMapType = NULL;
	 /* have to call GrX11SetCmap directly here; with colormap
	       set to NULL, it doesn't get called otherwise.
         */
         GrX11SetCMap((char *)NULL);
    }

    HashInit(&grX11WindowTable,8,HT_WORDKEYS);
    return grx11LoadFont();
}

/*---------------------------------------------------------
 * GrXClose:
 *
 * Results:	None.
 *
 * Side Effects:
 *---------------------------------------------------------
 */

Void
GrX11Close ()
{
    if (grXdpy == NULL) return;
    TxDeleteInputDevice(1 << pipeRead);
    close(pipeRead);
    kill(Xhelper, SIGKILL);
    do {} while (wait(0) != Xhelper);
    grGCStipple = 0;
    XCloseDisplay(grXdpy);
}


/*---------------------------------------------------------
 * GrXFlush:
 * 	Flush output to display.
 *
 *	Flushing is done automatically the next time input is read,
 *	so this procedure should not be used very often.
 *
 * Results:	None.
 *
 * Side Effects:    None.
 *---------------------------------------------------------
 */

Void
GrX11Flush ()
{
   XFlush(grXdpy);
}


/*
 * ---------------------------------------------------------------------------
 *
 * grXStdin --
 *
 *      Handle the stdin device for the X driver.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Adds events to the data queue.
 *
 * ---------------------------------------------------------------------------
 */

Void
grX11Stdin ()
{
    TxInputEvent *event;
    XEvent	xevent;
    HashEntry	*entry;
    
    read(pipeRead, &xevent, sizeof(XEvent));
    switch (xevent.type) 
    {
	case ButtonPress:
	case ButtonRelease:
	    {
		XButtonEvent *ButtonEvent = (XButtonEvent *) &xevent;

	        event = TxNewEvent();
		switch (ButtonEvent->button) {
		case Button1:
		    event->txe_button = TX_LEFT_BUTTON;
		    break;
		case Button2:
		    event->txe_button = TX_MIDDLE_BUTTON;
		    break;
		case Button3:
		    event->txe_button = TX_RIGHT_BUTTON;
		    break;
		}
		switch(xevent.type) {
		case ButtonRelease:
		    event->txe_buttonAction = TX_BUTTON_UP;
		    break;
		case ButtonPress:
		    event->txe_buttonAction = TX_BUTTON_DOWN;
		    break;
		}

	        grCurrent.window = ButtonEvent->window;
		entry = HashLookOnly(&grX11WindowTable,grCurrent.window);
	        grCurrent.mw= (entry)?(MagicWindow *)HashGetValue(entry):0;

		event->txe_p.p_x = ButtonEvent->x;
		event->txe_p.p_y = grXToMagic(ButtonEvent->y);
		event->txe_wid = grCurrent.mw->w_wid;
		TxAddEvent(event);
	    }
	    break;
	case KeyPress:
	    {
		XKeyPressedEvent *KeyPressedEvent = (XKeyPressedEvent *) &xevent;
		char c;

	        event = TxNewEvent();

	        grCurrent.window = KeyPressedEvent->window;
		entry = HashLookOnly(&grX11WindowTable,grCurrent.window);
	        grCurrent.mw= (entry)?(MagicWindow *)HashGetValue(entry):0;

    		read(pipeRead, &c, sizeof(char));
		if (c == '\015') c = '\n';
		event->txe_button = TX_CHARACTER;
		event->txe_ch = c;
		event->txe_p.p_x = KeyPressedEvent->x;
		event->txe_p.p_y = grXToMagic(KeyPressedEvent->y);
		event->txe_wid = grCurrent.mw->w_wid;
		TxAddEvent(event);
	    } 
	    break;
	case Expose:
	    {
		    XExposeEvent *ExposeEvent = (XExposeEvent*) &xevent;
		    Rect screenRect;
		    MagicWindow	*w;
		    
	            grCurrent.window = ExposeEvent->window;
		    entry = HashLookOnly(&grX11WindowTable,grCurrent.window);
	            w = (entry)?(MagicWindow *)HashGetValue(entry):0;
	            grCurrent.mw=w;

		    screenRect.r_xbot = ExposeEvent->x;
            	    screenRect.r_xtop = ExposeEvent->x+ExposeEvent->width;
            	    screenRect.r_ytop = 
			 	w->w_allArea.r_ytop-ExposeEvent->y;
            	    screenRect.r_ybot = w->w_allArea.r_ytop - 
		    		(ExposeEvent->y + ExposeEvent->height);
		    	 
                    WindAreaChanged( w, &screenRect);
                    WindUpdate();
            }
	    break;
	case ConfigureNotify:
	    {
		    XConfigureEvent *ConfigureEvent = (XConfigureEvent*) &xevent;
		    Rect screenRect;
		    MagicWindow	*w;
		    
	            grCurrent.window = ConfigureEvent->window;
		    entry = HashLookOnly(&grX11WindowTable,grCurrent.window);
	            w = (entry)?(MagicWindow *)HashGetValue(entry):0;
	            grCurrent.mw=w;

		    screenRect.r_xbot = ConfigureEvent->x;
            	    screenRect.r_xtop = ConfigureEvent->x+
			 		ConfigureEvent->width;
            	    screenRect.r_ytop = grXsToMagic(ConfigureEvent->y);
            	    screenRect.r_ybot = 
			 	grXsToMagic(ConfigureEvent->y+
					    ConfigureEvent->height);
			 
		    WindReframe(w,&screenRect,FALSE,FALSE);
		    WindRedisplay(w);
            }
            break;
	default:
	break;

     }
}


/*---------------------------------------------------------
 * x11suSetDisplay:
 *	This routine sets the appropriate parameters so that
 *	Magic will work with the X display.
 *
 *      Under Xlib, all input events (mouse and keyboard) are
 *	sent to one queue which has to be polled to discover
 *	whether there is any input or not.  To fit the Magic
 *	interrupt-driven input model, a helper process is
 *	spawned which reads and blocks on the event queue,
 *	sending SIGIO's to Magic when it detects input.  The
 *	input read in the helper process is then sent to Magic
 *	via a communication pipe.
 *
 * Results:  success / fail
 *
 * Side Effects:	Sets up the pipe.
 *---------------------------------------------------------
 */

bool
x11suSetDisplay (dispType, outFileName, mouseFileName)
    char *dispType;		/* arguments not used by X */
    char *outFileName;
    char *mouseFileName;
{
    int fildes[2],fildes2[2];
    char	*planecount;
    char *fullname;
    FILE* f;
    bool execFailed = false;

    WindPackageType = WIND_X_WINDOWS;

    grCursorType = "bw";
    
    if (strlen(dispType) > 5)  /* 5 == strlen("XWIND") */
    {
    	 planecount = dispType+5;
	 grXuserPC = atoi(planecount);
    }
    
    WindScrollBarWidth = 14;

    /* Set up helper process */
    pipe(fildes);
    pipe(fildes2);
    pipeRead = fildes[0];
    pipeWrite = fildes2[1];

    TxAddInputDevice((1 << pipeRead), grX11Stdin, (ClientData) NULL);

    f = PaOpen("X11Helper", "r", (char *) NULL,
		HELPER_PATH, (char *) NULL, &fullname);
    if (f == NULL) {
	int error;
	TxError("Couldn't find helper process %s in search path \"%s\"\n",
	    "X11Helper", HELPER_PATH);
	error = 0;
	write(fildes[1], &error, 4);
	return false;
    } else {
	fclose(f);
    }

#ifdef SYSV
    Xhelper = fork();
#else
    Xhelper = vfork();
#endif
    if (Xhelper == 0) {    /* Child process */
	char argv[2][100];

	sprintf(argv[0], "%s", fullname);
	sprintf(argv[1], "%d %d", fildes2[0],fildes[1]);
	if (execl(argv[0], argv[0], argv[1], 0) != 0)
	{
	    execFailed = true;
	    TxError("Couldn't execute helper process \"%s\".\n", fullname);
	    TxFlush();
	    /* we're the child process -- don't muck things up by returning */
	    _exit(656);  /* see vfork man page for reason for _exit() */
	}
    };
    sleep(1);

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = GrX11Lock;
    GrUnlockPtr = grSimpleUnlock;
    GrInitPtr = GrX11Init;
    GrClosePtr = GrX11Close;
    GrSetCMapPtr = GrX11SetCMap;

    GrEnableTabletPtr = GrX11EnableTablet;
    GrDisableTabletPtr = GrX11DisableTablet;
    GrSetCursorPtr = GrX11SetCursor;
    GrTextSizePtr = GrX11TextSize;
    GrDrawGlyphPtr = GrX11DrawGlyph;
    GrReadPixelPtr = GrX11ReadPixel;
    GrFlushPtr = GrX11Flush;

    GrCreateWindowPtr = GrX11Create;
    GrDeleteWindowPtr = GrX11Delete;
    GrConfigureWindowPtr = GrX11Configure;
    GrOverWindowPtr = GrX11Raise;
    GrUnderWindowPtr = GrX11Lower;
    GrUpdateIconPtr = GrX11IconUpdate; 

    /* local indirections */
    grSetSPatternPtr = grx11SetSPattern;
    grPutTextPtr = grx11PutText;
    grDefineCursorPtr = grx11DefineCursor;
    GrBitBltPtr = GrX11BitBlt;
    grDrawGridPtr = grx11DrawGrid;
    grDrawLinePtr = grx11DrawLine;
    grSetWMandCPtr = grx11SetWMandC;
    grFillRectPtr = grx11FillRect;
    grSetStipplePtr = grx11SetStipple;
    grSetLineStylePtr = grx11SetLineStyle;
    grSetCharSizePtr = grx11SetCharSize;
    grMaxStipples = 32;
    
    if (execFailed) {
	TxError("Execution failed!\n");
	return false;
    }

    TxAddInputDevice((1 << fileno(stdin)), grXWStdin, (ClientData) NULL);

    if(!GrX11Init()){
	return false;
    };
    GrScreenRect.r_xtop = DisplayWidth(grXdpy,grXscrn);
    GrScreenRect.r_ytop = DisplayHeight(grXdpy,grXscrn);

    return true;
}

/*
 * ----------------------------------------------------------------------------
 *
 * grXWStdin --
 *      Handle the stdin device for X window interface.
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
grXWStdin(fd, cdata)
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

/*
 * ----------------------------------------------------------------------------
 *
 * GrX11Create --
 *      Create a new window under the X window system.
 *	Bind X window to Magic Window w.
 *
 * Results:
 *      Window created, window ID send to Xhelper.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

GrX11Create(w)
    MagicWindow *w;
{
    Window wind;
    static int firstWindow = 1;
    XSizeHints	xsh;
    HashEntry	*entry;
    char	*windowplace;
    char	*option = (firstWindow)?"window":"newwindow";
    int		x      = w->w_frameArea.r_xbot;
    int		y      = grMagicToXs(w->w_frameArea.r_ytop);
    int		width  = w->w_frameArea.r_xtop - w->w_frameArea.r_xbot;
    int		height = w->w_frameArea.r_ytop - w->w_frameArea.r_ybot;

    WindSeparateRedisplay(w);
    if (windowplace=XGetDefault(grXdpy,"magic",option))
    {
	 XParseGeometry(windowplace,&x,&y,&width,&height);
	 w->w_frameArea.r_xbot = x;
	 w->w_frameArea.r_xtop = x+width;
	 w->w_frameArea.r_ytop = grXsToMagic(y);
	 w->w_frameArea.r_ybot = grXsToMagic(y+height);
	 WindReframe(w,&(w->w_frameArea),FALSE,FALSE);
	 xsh.flags = USPosition | USSize;
    }
    else
    {
    	 xsh.flags = PPosition|PSize;
    }
    if ( wind = XCreateSimpleWindow(grXdpy,DefaultRootWindow(grXdpy),
    		x,y,width,height,0, BlackPixel(grXdpy,grXscrn),
		    WhitePixel(grXdpy,grXscrn)))
    {
#ifdef	sun
	/* Hint's for Sun's implementation of X11 (News/X11) */
        {
	    XWMHints wmhints;
	    wmhints.flags = InputHint;
	    wmhints.input = True;
	    XSetWMHints(grXdpy, wind, &wmhints);
        }
#endif	sun

	/*
	 * Signal xhelper to poll window.
	 */
	grCurrent.window = wind; 
	/*
	 * Define window cursor and complete initialization.
	 */
	xsh.x = w->w_frameArea.r_xbot;
	xsh.y = grMagicToXs(w->w_frameArea.r_ytop);
	xsh.width = w->w_frameArea.r_xtop - w->w_frameArea.r_xbot;
	xsh.height= w->w_frameArea.r_ytop - w->w_frameArea.r_ybot;
	XSetStandardProperties(grXdpy, wind, "magic", "magic", None,
			       0, 0, &xsh);
	XSetWindowColormap(grXdpy,grCurrent.window,grXcmap);
        XMapWindow(grXdpy, grCurrent.window);
	XSync(grXdpy,1);

	if (firstWindow)
	{
	     firstWindow = 0;
             grGCFill = XCreateGC(grXdpy, grCurrent.window, 0, 0);
             grGCDraw = XCreateGC(grXdpy, grCurrent.window, 0, 0);
             grGCText = XCreateGC(grXdpy, grCurrent.window, 0, 0);
             grGCCopy = XCreateGC(grXdpy, grCurrent.window, 0, 0);
             grGCGlyph = XCreateGC(grXdpy, grCurrent.window, 0, 0);
	}
	XSetPlaneMask(grXdpy,grGCGlyph,AllPlanes);
	grCurrent.window = wind; 
	grCurrent.mw = w;
	w->w_grdata = (ClientData) wind;
	
	entry = HashFind(&grX11WindowTable,grCurrent.window);
	HashSetValue(entry,w);

        XDefineCursor(grXdpy, grCurrent.window,grCurrent.cursor);
	XSync(grXdpy,0);
	GrX11IconUpdate(w,w->w_caption); 

        write( pipeWrite, (char *) &wind, sizeof(Window));
	kill( Xhelper, SIGTERM);
	sleep(1); /* wait for Xhelper to register for Expose Events; */
		  /* the window new doesn't get painted initially    */
		  /* otherwise.					     */
	return 1;
    }
    else
	TxError("Could not open new X window\n");

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXDelete --
 *      Destroy an X window.
 *
 * Results:
 *      Window destroyed.
 *
 *
 * ----------------------------------------------------------------------------
 */

GrX11Delete(w)
    MagicWindow *w;
{
    Window xw;
    HashEntry	*entry;

    xw = (Window) w->w_grdata;
    entry = HashLookOnly(&grX11WindowTable,xw);
    HashSetValue(entry,NULL);
    
    XDestroyWindow(grXdpy,xw);
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXConfigure --
 *      Resize/ Move an existing X window.
 *
 * Results:
 *      Window reconfigured to w->w_frameArea.
 *
 * Side Effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

GrX11Configure(w)
    MagicWindow *w;
{
    XMoveResizeWindow(grXdpy,(Window) w->w_grdata,
	    w->w_frameArea.r_xbot, grMagicToXs(w->w_frameArea.r_ytop),
		w->w_frameArea.r_xtop - w->w_frameArea.r_xbot,
		    w->w_frameArea.r_ytop - w->w_frameArea.r_ybot);
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXRaise --
 *      Raise a window to the top of the screen such that nothing
 *	obscures it.
 *
 * Results:
 *      Window raised.
 *
 * Side Effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

GrX11Raise(w)
    MagicWindow *w;
{
    XRaiseWindow(grXdpy, (Window) w->w_grdata );
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXLower --
 *      Lower a window below all other X windows.
 *	obscures it.
 *
 * Results:
 *      Window lowered.
 *
 * Side Effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

GrX11Lower(w)
    MagicWindow *w;
{
    XLowerWindow(grXdpy, (Window) w->w_grdata );
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrX11Lock --
 *      Lock a window and set global variables "grCurrent.window"
 *	and "grCurrent.mw" to reference the locked window.
 *
 * Results:
 *      Window locked.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

GrX11Lock(w, flag)
    MagicWindow *w;
    bool flag;
{
    grSimpleLock(w, flag);
    if ( w != (MagicWindow *) GR_LOCK_SCREEN )
    {
	grCurrent.mw = w;
	grCurrent.window = (Window) w->w_grdata;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * GrX11IconUpdate -- updates the icon text with the window script
 *
 * Results: none
 *
 * Side Effects: changes the icon text
 *
 *-------------------------------------------------------------------------
 */
Void GrX11IconUpdate(w,text)
	MagicWindow	*w;
	char		*text;

{
     Window	wind = (Window)(w->w_grdata);
     char	*brack;
     
     if (wind == NULL) return;
     if (brack = index(text,'['))
     {
     	  brack--;
	  *brack = 0;
          XSetIconName(grXdpy,wind,text);
	  XStoreName(grXdpy,wind,text);
     	  *brack = ' ';
	  return;
     }
     if (brack = rindex(text,' ')) text = brack+1;
     XSetIconName(grXdpy,wind,text);
     XStoreName(grXdpy,wind,text);
}
