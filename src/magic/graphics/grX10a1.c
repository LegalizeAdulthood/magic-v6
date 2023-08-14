/* grX10a1.c
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 *
 * This file contains primitive functions to manipulate an X window system
 * on a microVax color display.  Included here are initialization and closing
 * functions, and several utility routines used by the other X
 * modules.
 */

/* X10 driver "a" (from Lawrence Livermore National Labs) */

#include <stdio.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/socket.h>
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
#include "grX10aInt.h"
#include "paths.h"
#include <signal.h>

#ifndef lint
static char rcsid[]="$Header: grX10a1.c,v 6.0 90/08/28 18:41:23 mayo Exp $";
#endif  not lint

/* Imports from other X modules: */
extern bool grxGetCursorPos(), grxDrawGrid();
extern Void GrXEnableTablet(), GrXDisableTablet();
extern Void GrXSetCMap(), grxPutText(), grxDefineCursor();
extern Void GrXSetCursor(), GrXTextSize(), GrXDrawGlyph();
extern Void GrXBitBlt(), NullBitBlt();
extern int GrXReadPixel();
extern Void grxDrawLine(), grxSetLineStyle(), grxSetCharSize();
extern Void grxSetWMandC(), grxFillRect();
extern Void GrXCreate();
extern Void GrXDelete();
extern Void GrXConfigure();
extern Void GrXRaise();
extern Void GrXLower();
extern Void GrXLock();

/* Exports */
struct	xstate grCurrent;
int grPixels[ 256 ];
int grxBasePixelMask;
Pixmap grTiles[ 256 ];
extern HashTable	grX10WindowTable;
global bool grInited = false;		/* This insures that
					 * no graphics routine is
					 * called before ginit().
					 */
Pattern grStipples[  GR_NUM_STIPPLES ] [ 8 ];	/* Storage for stipple masks
					 */
int grCurrent_stipple;
int DisplayWidth, DisplayHeight;
int pipeRead, pipeWrite, xEventSize = sizeof(XEvent);
int Xhelper;
int grXuserPC = 0;

/* locals */
static int saveX, saveY;	/* Coordinates of the mouse in the
				 * root window when Magic is started.
				 */
static Bitmap grBitmap[  GR_NUM_STIPPLES ];	/* Storage for stipple masks
				 */

/* This is kind of a long story, and very kludgy, but the following
 * things need to be defined as externals because of the way lint
 * libraries are made by taking this module and changing all procedures
 * names "Xxxx" to "Grxxx".  The change is only done at the declaration
 * of the procedure, so we need these declarations to handle uses
 * of those names, which don't get modified.  Check out the Makefile
 * for details on this.
 */

extern Void GrXClose(), GrXFlush(), GrXInit();


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
grxSetWMandC (mask, c)
    int mask;			/* New value for write mask */
    int c;			/* New value for current color */
{
    if (strcmp(grDStyleType,"bw") == 0)
    {
          if (c == 0) {
              grCurrent.pixel = grPixels[0];
              grCurrent.tile = grTiles[0];
          } else {
              grCurrent.pixel = grPixels[1];
              grCurrent.tile = grTiles[1];
          };
          grCurrent.planes = AllPlanes;
    }
    else
    {
         grCurrent.pixel = grPixels[ c ];
         grCurrent.tile = grTiles[ c ];
         grCurrent.planes = mask | grxBasePixelMask;
    }
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
 * Note: Almost all of Magic's linestyles are 0x00 or 0xff.
 *	That's why we keep the two patterns around.
 *---------------------------------------------------------
 */

Void
grxSetLineStyle (style)
    int style;			/* New stipple pattern for lines. */
{
    static Pattern style_0x00;
    static Pattern style_0xff;
    static bool styleInited = false;

    if( !styleInited) {
	style_0x00 = XMakePattern( 0x00, 8, 1 );
	style_0xff = XMakePattern( 0xff, 8, 1 );
	styleInited = true;
    };
    switch( style ) {
    case 0x00: grCurrent.pattern = style_0x00; break;
    case 0xff: grCurrent.pattern = style_0xff; break;
    default:   grCurrent.pattern = XMakePattern( style, 8, 1 ); break;
    }
}


/*---------------------------------------------------------
 * Using stipples under the X window system:
 *
 * The stipple patterns are stored as bitmaps which are used
 * as the cmask argument in drawing rectangles with the 
 * XTileFill() routine.  The bitmap resource numbers are
 * kept in the grBitmap[] array.  The index to the array
 * corresponds to the stipple numbers in Magic.
 * 
 * The current version of Xlib does not support clipmasks
 * in the XTileFill routine.  Instead, we modify the dstyle
 * file to show different materials.
 *
 * Stippling is done currently by drawing multiple dashed lines.
 * This is noticeably slower than without stippling.
 *
 * Even if Xlib finally does support clipmasks, we might still
 * have a problem:  The size of the clipmask has to match that
 * of the tile being drawn.  Defining a new clipmask for each
 * rectangle drawn seems an awefully high price to me.
 *---------------------------------------------------------
 */

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
grxSetSPattern (stipple, pattern)
    int stipple;			/* The stipple number, 1-15. */
    int pattern[8];			/* 8 8-bit patterns integers */
{
    int i;

    for( i = 0; i < 8; i++ ) {
	grStipples[ stipple ] [ i ] = 
	    XMakePattern( pattern[ i ], 8, 1 );
	if( grStipples[ stipple ] [ i ] == NULL ) {
	    TxError( "%s\n", "grX1.grxSetSPattern: Unable to create pattern");
	    return false;
	};
    };
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
grxSetStipple (stipple)
    int stipple;			/* The stipple number to be used. */
{
    grCurrent_stipple = stipple;
}


/*---------------------------------------------------------
 * GrXInit:
 *	GrXInit initializes the graphics display and clears its screen.
 *	Files must have been previously opened with GrSetDisplay();
 *
 * Results: TRUE if successful.
 *---------------------------------------------------------
 */

bool
GrXInit(displayName)
    char *displayName;
{
    char readdata[10];
    char *display;
    int *intdata;

    display = NULL;
    if (strcmp(displayName, "/dev/null") != 0)
	display = displayName;
    if (!grInited) 
    {
	Window subw;	/* Just a dummy variable */
	WindowInfo info;

	if (XOpenDisplay(display) == 0)
	{
    	      TxError("Couldn't open display; check DISPLAY variable\n");
	      return false;
	}
	if (grXuserPC == 1)
	{
	     grDStyleType = "bw";
	     grCMapType = NULL;
	     GrXSetCMap((char *)NULL);
	}
	else
	{
	     grDStyleType = "7bit";
	     grCMapType = grDStyleType;
	}

	XQueryWindow( RootWindow, &info);
	DisplayWidth = info.width;
	DisplayHeight = info.height;

	XQueryMouse( RootWindow, &saveX, &saveY, &subw );
	grInited = true;
    }
    grxLoadFont();
    return true;
}

/*---------------------------------------------------------
 * GrXClose:
 *	Return cursor to original position. (Doesn't seem to work)
 *	X resources are released automatically upon exit. 
 *
 * Results:	None.
 *
 * Side Effects:
 *---------------------------------------------------------
 */

Void
GrXClose ()
{
    XWarpMouse( RootWindow, saveX, saveY );
    XSync();	/* Without this, XWarpMouse() won't have time to
		 * finish before Magic exits.
		 */
    kill(Xhelper, SIGKILL);
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
GrXFlush ()
{
    XFlush();
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
grXStdin ()
{
    XEvent data;
    TxInputEvent *event;
    WindowInfo windowInfo;
    MagicWindow *w;
    Window xw;
    HashEntry	*entry;
    
    read(pipeRead, &data, xEventSize);
    xw = data.window;
    entry = HashLookOnly(&grX10WindowTable,xw);
    if ( w = (entry)?(MagicWindow *)HashGetValue(entry):0)
    {
	switch( data.type )
	{
	case ButtonPressed:
	case ButtonReleased:
	  {
	    XButtonEvent *ButtonEvent;

	    grCurrent.mw = w;
	    grCurrent.window = xw;

	    event = TxNewEvent();
	    ButtonEvent = (XButtonEvent *) &data;
	    switch( ButtonEvent->detail & 0xff ) {
	    case LeftButton:	event->txe_button = TX_LEFT_BUTTON; break;
	    case MiddleButton:	event->txe_button = TX_MIDDLE_BUTTON; break;
	    case RightButton:	event->txe_button = TX_RIGHT_BUTTON; break;
	    };
	    switch( data.type ) {
	      case ButtonReleased:
		event->txe_buttonAction = TX_BUTTON_UP;
		    break;
	      case ButtonPressed:
		event->txe_buttonAction = TX_BUTTON_DOWN;
		    break;
	    };
	    event->txe_p.p_x = ButtonEvent->x;
	    event->txe_p.p_y = grXToMagic(ButtonEvent->y);
	    event->txe_wid = w->w_wid;
	    TxAddEvent( event );
	    break;
	  };

	case KeyPressed:
	  {
	    /* Need to filter out non-ascii characters ( Shift, Lock, Ctrl )
	     * which are counted as Keys in X.
	     */
	    XKeyPressedEvent *KeyPressedEvent;
	    char inChar;
	    int nbytes;
	    
	    grCurrent.mw = w;
	    grCurrent.window = xw;

	    KeyPressedEvent = (XKeyPressedEvent *) &data;
	    inChar =  XLookupMapping( KeyPressedEvent, &nbytes )[0];
	    if( inChar >= 0 && inChar <= 127 && inChar != 14) {
		event = TxNewEvent();
		event->txe_button = TX_CHARACTER;
		if( inChar == 012 || inChar == '\015' ) {
		    event->txe_ch = '\n';
		} else {
		    event->txe_ch = inChar;
		};
		event->txe_p.p_x = KeyPressedEvent->x;
		event->txe_p.p_y = grXToMagic( KeyPressedEvent->y );
		event->txe_wid = w->w_wid;
		/* Cursoroff not implemented. 
		 * Might need to define a blank cursor
		 */
		/* Signal to user that mouse input is no
		 * longer expected.  Cursor will get 
		 * turned back on by GrEnableTablet()
		 * in txCommands.TxDispatch()
		 */
		TxAddEvent( event );
	    };
	    break;
	  };

	case ExposeRegion:
	  {
	    Rect screenRect;
	    XExposeEvent *ExposeEvent;

	    ExposeEvent = (XExposeEvent *) &data;

	    /*
	     * Let Magic re-display a portion of the window.
	     */

	    screenRect.r_xbot = ExposeEvent->x;
	    screenRect.r_xtop = ExposeEvent->x + ExposeEvent->width;
	    screenRect.r_ytop = w->w_allArea.r_ytop - (ExposeEvent->y);
	    screenRect.r_ybot = w->w_allArea.r_ytop - (ExposeEvent->y + ExposeEvent->height);

	    WindAreaChanged( w, &screenRect);
	    WindUpdate();
	  };
	  break;

	case ExposeWindow:
	  {
	    WindowInfo info;
	    XExposeEvent *ExposeEvent;

	    ExposeEvent = (XExposeEvent*) &data;
	    if ( XQueryWindow(xw, &info) )
	    {
		Rect frameRect;

		/*
		 * Notify Magic of re-exposure.
		 */

		frameRect.r_xbot = info.x;
		frameRect.r_xtop = info.x + info.width;
		frameRect.r_ytop = grXsToMagic(info.y);
		frameRect.r_ybot = grXsToMagic(info.y + info.height);

		WindReframe(w, &frameRect, FALSE, FALSE);
		WindRedisplay(w);
		WindUpdate();
	    }
	    else
		TxPrintf("XQueryWindow failed\n");
	  };
	}; /* switch */
    }
}

issueCommand (x, y, command)
    int x,y;
    char *command;
{
    int commandLength;
    commandLength = strlen(command);
    while (commandLength--) {
	makeTxEvent(x, y, *command++);
    };
    makeTxEvent(x,y, '\n');
}

makeTxEvent (x, y, eventChar)
    int x,y;
    char eventChar;
{
    TxInputEvent *event;
    event = TxNewEvent();
    event->txe_button = TX_CHARACTER;
    event->txe_ch = eventChar;
    event->txe_p.p_x = x;
    event->txe_p.p_y = y;
    TxAddEvent(event);
}


/*---------------------------------------------------------
 * x10aSetDisplay:
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
x10aSetDisplay (dispType, outFileName, mouseFileName)
    char *dispType;		/* arguments not used by X */
    char *outFileName;
    char *mouseFileName;
{
    int fildes[2],fildes2[2];
    int grXWStdin();

    WindPackageType = WIND_X_WINDOWS;

    grCursorType = "bw";

    if (strlen(dispType) > 5)  /* 5 == strlen("XWIND") */
    {
    	 char *planecount = dispType+5;
	 grXuserPC = atoi(planecount);
    }
    
    /* Set up helper process */
    pipe(fildes);
    pipe(fildes2);
    pipeRead = fildes[0];
    pipeWrite = fildes2[1];

    TxAddInputDevice((1 << pipeRead), grXStdin, (ClientData) NULL);

#ifdef SYSV
    Xhelper = fork();
#else
    Xhelper = vfork();
#endif
    if (Xhelper == 0) {    /* Child process */
	char *fullname;
	char argv[2][100];
	FILE* f;
	int error;

	f = PaOpen("X10helper", "r", (char *) NULL,
		    HELPER_PATH, (char *) NULL, &fullname);
	if (f == NULL) {
	    TxError("Couldn't find helper process %s in search path %s\n",
		"X10helper", HELPER_PATH);
	    error = 0;
	    write(fildes[1], &error, 4);
	    return false;
	} else {
	    fclose(f);
	    sprintf(argv[0], "%s", fullname);
	    sprintf(argv[1], "%d %d", fildes2[0],fildes[1]);
	    if (strcmp(outFileName, "/dev/null") == 0)
	    {
		/* Normal display */
		execlp(argv[0], "X10helper", argv[1], argv[2], 0);
	    }
	    else
	    {
		/* Use non-default display name */
		execlp(argv[0], "X10helper", argv[1], argv[2], outFileName, 0);
	    }
	}

	/* Only gets here if execlp fails */
	fprintf(stderr, "Unable to locate ~cad/bin/X10helper.\n");
	error = 0;
	write(fildes[1], &error, 4);
	exit();
    };

    HashInit(&grX10WindowTable,8,HT_WORDKEYS);

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = GrXLock;
    GrUnlockPtr = grSimpleUnlock;
    GrInitPtr = GrXInit;
    GrClosePtr = GrXClose;
    GrSetCMapPtr = GrXSetCMap;

    GrEnableTabletPtr = GrXEnableTablet;
    GrDisableTabletPtr = GrXDisableTablet;
    GrSetCursorPtr = GrXSetCursor;
    GrTextSizePtr = GrXTextSize;
    GrDrawGlyphPtr = GrXDrawGlyph;
    GrReadPixelPtr = GrXReadPixel;
    GrFlushPtr = GrXFlush;
    GrCreateWindowPtr = GrXCreate;
    GrDeleteWindowPtr = GrXDelete;
    GrConfigureWindowPtr = GrXConfigure;
    GrOverWindowPtr = GrXRaise;
    GrUnderWindowPtr = GrXLower;

    /* local indirections */
    grSetSPatternPtr = grxSetSPattern;
    grPutTextPtr = grxPutText;
    grDefineCursorPtr = grxDefineCursor;
    GrBitBltPtr = GrXBitBlt;
    grDrawGridPtr = grxDrawGrid;
    grDrawLinePtr = grxDrawLine;
    grSetWMandCPtr = grxSetWMandC;
    grFillRectPtr = grxFillRect;
    grSetStipplePtr = grxSetStipple;
    grSetLineStylePtr = grxSetLineStyle;
    grSetCharSizePtr = grxSetCharSize;
    grMaxStipples = 32;

    TxAddInputDevice((1 << fileno(stdin)), grXWStdin, (ClientData) NULL);

    /* Make X and magic have the same coordinate convention:
     * make the origin to be at the upper lower left corner.
     */
    if(!GrXInit(outFileName)){
	return false;
    };

    GrScreenRect.r_xtop = DisplayWidth;
    GrScreenRect.r_ytop = DisplayHeight;

    return true;
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXCreate --
 *      Create a new window under the X window system.
 *	Bind X window to Magic Window w.
 *
 * Results:
 *      Window created, window ID send to X10helper.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

GrXCreate(w)
    MagicWindow *w;
{
    Window wind;
    static int firstWindow = 1;
    HashEntry	*entry;

    if ( wind = XCreateWindow(RootWindow,
		w->w_frameArea.r_xbot, grMagicToXs(w->w_frameArea.r_ytop),
		    w->w_frameArea.r_xtop - w->w_frameArea.r_xbot,
		    w->w_frameArea.r_ytop - w->w_frameArea.r_ybot,
			0, BlackPixmap, WhitePixmap) )
    {
	/*
	 * Signal xhelper to poll window.
	 */
	write( pipeWrite, (char *) &wind, sizeof(wind));
	kill( Xhelper, SIGTERM);

	/*
	 * Define window cursor and complete initialization.
	 */

	if ( firstWindow )
	{
	    firstWindow = 0;
	    XWarpMouse(wind, 100, 100);
	}

	XDefineCursor(wind, grCurrent.cursor);
	entry = HashFind(&grX10WindowTable,wind);
	HashSetValue(entry,w);
	w->w_grdata = (ClientData) wind;
	WindSeparateRedisplay(w);
	grCurrent.window = wind;
	grCurrent.mw = w;
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
 * Side Effects:
 *      Association removed from Xassoc table.
 *
 * ----------------------------------------------------------------------------
 */

GrXDelete(w)
    MagicWindow *w;
{
    Window xw;
    HashEntry	*entry;

    xw = (Window) w->w_grdata;
    entry = HashLookOnly(&grX10WindowTable,xw);
    HashSetValue(entry,NULL);

    XDestroyWindow(xw);
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

GrXConfigure(w)
    MagicWindow *w;
{
    XConfigureWindow( (Window) w->w_grdata,
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

GrXRaise(w)
    MagicWindow *w;
{
    XRaiseWindow( (Window) w->w_grdata );
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

GrXLower(w)
    MagicWindow *w;
{
    XLowerWindow( (Window) w->w_grdata );
}

/*
 * ----------------------------------------------------------------------------
 *
 * GrXLock --
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

GrXLock(w, flag)
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
