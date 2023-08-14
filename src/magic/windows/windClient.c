/* windClient.c -
 *
 *	Send button pushes and commands to the window's command
 *	interpreters.
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
 */

#ifndef lint
static char rcsid[]="$Header: windClient.c,v 6.0 90/08/28 19:02:10 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "glyphs.h"
#include "windInt.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "main.h"
#include "macros.h"
#include "utils.h"
#include "malloc.h"
#include "graphics.h"
#include "styles.h"
#include "txcommands.h"
#include "undo.h"

/* The following defines are used to indicate corner positions
 * of the box:
 */

#define WIND_BL 0
#define WIND_BR 1
#define WIND_TR 2
#define WIND_TL 3
#define WIND_ILG -1

/* our window client ID */
static WindClient windClientID = NULL;

static char *windCommands[] =
    {
	"*crash			cause a core dump",
	"*files			print out currently open files",
	"*grstats		print out stats on graphics",
	"*malloc size		grab a chunk of size bytes of memory",
	"*memstats		print malloc statistics",
	"*pause	[args]		print args and wait for <cr>",
	"*profile on|off	toggle runtime profiling",
	"*runstats		print time and memory usage statistics",
	"*winddebug		set debugging mode",
	"*winddump		print out debugging info",

	"center			center window on the cursor",
	"closewindow		close a window",
	"echo [-n] [strings]	print text on the terminal",
	"grow			blow a window up to full-screen size or back again",
	"help [pattern]		print out synopses for all commands valid\n\
			in the current window (or just those\n\
			containing pattern)",
	"logcommands [file [update]]\n\
			log all commands into a file",
	"macro [char [string]]	define or print a macro called char",
	"openwindow [cell]	open a new window",
	"over			move a window over (on top of) the rest",
	"pushbutton button act	push a mouse button",
	"redo [count]		redo commands",
	"redraw			redraw the display",
	"reset			reset the display",
	"scroll dir [amount]	scroll the window",
	"send type command	send a command to a certain window type",
	"setpoint [x y [WID]]	force to cursor (point) to x,y in window WID",
	"sleep seconds		sleep for a number of seconds",
	"source filename		read in commands from file",
	"specialopen [coords] type [args]\n\
			open a special window",
	"quit			exit magic",
	"underneath		move a window underneath the rest",
	"undo [count]		undo commands",
	"updatedisplay		force the display to be updated",
	"view               	zoom window out so everything is visable",
	"windowscrollbars [on|off]\n\
			toggle scroll bars for new windows",
	"windowpositions [file]	print out window positions",
	"zoom amount		zoom window by amount",
	0
    };

extern int windCrashCmd();
extern int windFilesCmd(), windCloseCmd(), windOpenCmd();
extern int windQuitCmd(), windRedrawCmd();
extern int windResetCmd(), windSpecialOpenCmd();
extern int windOverCmd(), windUnderCmd(), windDebugCmd();
extern int windDumpCmd(), windHelpCmd();
extern int windLogCommandsCmd(), windUpdateCmd(), windSleepCmd();
extern int windSourceCmd();
extern int windSetpointCmd();
extern int windPushbuttonCmd(), windSendCmd();
extern int windMallocCmd();
extern int windMemstatsCmd(), windRunstatsCmd();
extern int windProfileCmd();
extern int windPauseCmd(), windGrstatsCmd();
extern int windEchoCmd(), windGrowCmd(), windMacroCmd();
extern int windUndoCmd(), windRedoCmd();
extern int windCenterCmd(), windScrollCmd(), windViewCmd(), windZoomCmd();
extern int windScrollBarsCmd(), windPositionsCmd();

static int (*windFuncs[])() =
    {
	windCrashCmd,
	windFilesCmd,
	windGrstatsCmd,
	windMallocCmd,
	windMemstatsCmd,
	windPauseCmd,
	windProfileCmd,
	windRunstatsCmd,
	windDebugCmd,
	windDumpCmd,

	windCenterCmd,
	windCloseCmd,
	windEchoCmd,
	windGrowCmd,
	windHelpCmd,
	windLogCommandsCmd,
	windMacroCmd,
	windOpenCmd,
	windOverCmd,
	windPushbuttonCmd,
	windRedoCmd,
	windRedrawCmd,
	windResetCmd,
	windScrollCmd,
	windSendCmd,
	windSetpointCmd,
	windSleepCmd,
	windSourceCmd,
	windSpecialOpenCmd,
	windQuitCmd,
	windUnderCmd,
	windUndoCmd,
	windUpdateCmd,
	windViewCmd,
	windScrollBarsCmd,
	windPositionsCmd,
	windZoomCmd
    };

static Rect windFrameRect;
static Window *windFrameWindow;
static int windButton = TX_LEFT_BUTTON;
static int windCorner = WIND_ILG;	/* Nearest corner when button went
					 * down.
					 */


/*
 * ----------------------------------------------------------------------------
 *	windButtonSetCursor --
 *
 * 	Used to set the programmable cursor for a particular
 *	button state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Selects and sets a programmable cursor based on the given
 *	button (for sizing or moving) and corner.
 * ----------------------------------------------------------------------------
 */

windButtonSetCursor(button, corner)
int button;			/* Button that is down. */
int corner;			/* Corner to be displayed in cursor. */

{
    switch (corner)
    {
	case WIND_BL:
	    if (button == TX_LEFT_BUTTON)
		GrSetCursor(STYLE_CURS_LLWIND);
	    else
		GrSetCursor(STYLE_CURS_LLWINDCORN);
	    break;
	case WIND_BR:
	    if (button == TX_LEFT_BUTTON)
		GrSetCursor(STYLE_CURS_LRWIND);
	    else
		GrSetCursor(STYLE_CURS_LRWINDCORN);
	    break;
	case WIND_TL:
	    if (button == TX_LEFT_BUTTON)
		GrSetCursor(STYLE_CURS_ULWIND);
	    else
		GrSetCursor(STYLE_CURS_ULWINDCORN);
	    break;
	case WIND_TR:
	    if (button == TX_LEFT_BUTTON)
		GrSetCursor(STYLE_CURS_URWIND);
	    else
		GrSetCursor(STYLE_CURS_URWINDCORN);
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 * windGetCorner --
 *
 * 	Returns the corner of the window closest to a given screen location.
 *
 * Results:
 *	An integer value is returned, indicating the corner closest to
 *	the given screen location.  
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

int
windGetCorner(screenPoint, screenRect)
    Point *screenPoint;
    Rect *screenRect;

{
    Rect r;

    /* Find out which corner is closest.  Consider only the
     * intersection of the box with the window (otherwise it
     * may not be possible to select off-screen corners.
     */

    r = *screenRect;
    GeoClip(&r, &GrScreenRect);
    if (screenPoint->p_x < ((r.r_xbot + r.r_xtop)/2))
    {
	if (screenPoint->p_y < ((r.r_ybot + r.r_ytop)/2))
	    return WIND_BL;
	else 
	    return WIND_TL;
    }
    else
    {
	if (screenPoint->p_y < ((r.r_ybot + r.r_ytop)/2))
	    return WIND_BR;
	else 
	    return WIND_TR;
    }
}

/*
 * ----------------------------------------------------------------------------
 * windMoveRect --
 *
 * 	Repositions a rectangle by one of its corners.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The rectangle is changed so that the given corner is at the
 *	given position.  
 * ----------------------------------------------------------------------------
 */

void
windMoveRect(wholeRect, corner, p, rect)
    bool wholeRect;		/* move the whole thing?  or just a corner? */
    int corner;			/* Specifies a corner in the format
				 * returned by ToolGetCorner.
				 */
    Point *p;			/* New position of corner, in screen
				 * coordinates.
				 */
    Rect *rect;
{
    int x, y, tmp;

    /* Move the rect.  If an illegal corner is specified, then
     * move by the bottom-left corner.
     */

    if (wholeRect)
    {
	switch (corner)
	{
	    case WIND_BL:
		x = p->p_x - rect->r_xbot;
		y = p->p_y - rect->r_ybot;
		break;
	    case WIND_BR:
		x = p->p_x - rect->r_xtop;
		y = p->p_y - rect->r_ybot;
		break;
	    case WIND_TR:
		x = p->p_x - rect->r_xtop;
		y = p->p_y - rect->r_ytop;
		break;
	    case WIND_TL:
		x = p->p_x - rect->r_xbot;
		y = p->p_y - rect->r_ytop;
		break;
	    default:
		x = p->p_x - rect->r_xbot;
		y = p->p_y - rect->r_ybot;
		break;
	}
	rect->r_xbot += x;
	rect->r_ybot += y;
	rect->r_xtop += x;
	rect->r_ytop += y;
    }
    else
    {
	switch (corner)
	{
	    case WIND_BL:
		rect->r_xbot = p->p_x;
		rect->r_ybot = p->p_y;
		break;
	    case WIND_BR:
		rect->r_xtop = p->p_x;
		rect->r_ybot = p->p_y;
		break;
	    case WIND_TR:
		rect->r_xtop = p->p_x;
		rect->r_ytop = p->p_y;
		break;
	    case WIND_TL:
		rect->r_xbot = p->p_x;
		rect->r_ytop = p->p_y;
		break;
	}

	/* If the movement turned the box inside out, turn it right
	 * side out again.
	 */
	
	if (rect->r_xbot > rect->r_xtop)
	{
	    tmp = rect->r_xtop;
	    rect->r_xtop = rect->r_xbot;
	    rect->r_xbot = tmp;
	}
	if (rect->r_ybot > rect->r_ytop)
	{
	    tmp = rect->r_ytop;
	    rect->r_ytop = rect->r_ybot;
	    rect->r_ybot = tmp;
	}

    }
}


/*
 * ----------------------------------------------------------------------------
 * Button Routines --
 *
 * 	This page contains a set of routines to handle the puck
 *	buttons within the window border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Left button:  used to move the whole window by the lower-left corner.
 *	Right button: used to re-size the window by its upper-right corner.
 *		If one of the left or right buttons is pushed, then the
 *		other is pushed, the corner is switched to the nearest
 *		one to the cursor.  This corner is remembered for use
 *		in box positioning/sizing when both buttons have gone up.
 *	Bottom button: Center the view on the point where the crosshair is
 *		at when the button is released.
 * ----------------------------------------------------------------------------
 */

windFrameDown(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (WindOldButtons == 0)
    {
	windFrameRect = w->w_frameArea;
	windFrameWindow = w;
	windButton = cmd->tx_button;
    }
#define BOTHBUTTONS (TX_LEFT_BUTTON | TX_RIGHT_BUTTON)
    if ((WindNewButtons & BOTHBUTTONS) == BOTHBUTTONS)
    {
	windCorner = windGetCorner(&(cmd->tx_p), &(windFrameWindow->w_frameArea));
    }
    else if (cmd->tx_button == TX_LEFT_BUTTON) 
    {
	windCorner = WIND_BL;
	windButtonSetCursor(windButton, windCorner);
    }
    else if (cmd->tx_button == TX_RIGHT_BUTTON) 
    {
	windCorner = WIND_TR;
	windButtonSetCursor(windButton, windCorner);
    }
}

    /*ARGSUSED*/
void
windFrameUp(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (WindNewButtons == 0)
    {
	GrSetCursor(STYLE_CURS_NORMAL);
	switch (cmd->tx_button)
	{
	    case TX_LEFT_BUTTON:
	    case TX_RIGHT_BUTTON:
		windMoveRect( (windButton == TX_LEFT_BUTTON),
			windCorner, &(cmd->tx_p), &windFrameRect);
		WindReframe(windFrameWindow, &windFrameRect, FALSE,
			(windButton == TX_LEFT_BUTTON) );
		break;
	}
    }
    else
    {
	/* If both buttons are down and one is released, we just change
	 * the cursor to reflect the current corner and the remaining
	 * button (i.e. move or size window).
	 */

	windCorner = windGetCorner(&(cmd->tx_p), &(windFrameWindow->w_frameArea));
	windButtonSetCursor(windButton, windCorner);
    }
}



/*
 * ----------------------------------------------------------------------------
 * windClientButtons --
 *
 *	Handle button pushes to the window border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	depends upon where the button was pushed.
 * ----------------------------------------------------------------------------
 */

void
windClientButtons(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern void windBarLocations();

    /*
     * Is this an initial 'down push' in a non-iconic window?  If so, we
     * will initiate some user-interaction sequence, such as moving the corner
     * of the window or growing it to full-screen size.
     *
     * (An 'iconic' window is one that is closed down to an icon -- this
     * currently only happens when we are using the Sun window package, but
     * in the future it might happen in other cases too.)
     *
     */
    if ((WindOldButtons == 0) && ((w->w_flags & WIND_ISICONIC) == 0)) {   
	/* single button down */
	Point p;
	Rect caption, leftBar, botBar, up, down, right, left, zoom;

	windFrameWindow = NULL;
	if (w == NULL) return;
	p.p_x = w->w_screenArea.r_xtop - w->w_screenArea.r_xbot;
	p.p_y = w->w_screenArea.r_ytop - w->w_screenArea.r_ybot;
	caption = w->w_allArea;
	caption.r_ybot = caption.r_ytop - TOP_BORDER(w) + 1;

	/* Handle 'grow' for our window package. */
	if (WindPackageType == WIND_MAGIC_WINDOWS) {
	    if ((cmd->tx_button == TX_MIDDLE_BUTTON) && 
		GEO_ENCLOSE(&cmd->tx_p, &caption)) {
		    WindFullScreen(w);
		    return;
	    };
	}

	/* Always handle scroll bars ourselves, even if there is an external
	 * window package.
	 */
	if ((w->w_flags & WIND_SCROLLBARS) != 0) {
	    windBarLocations(w, &leftBar, &botBar, &up, &down, 
		&right, &left, &zoom);
	    if (cmd->tx_button == TX_MIDDLE_BUTTON) {
		if (GEO_ENCLOSE(&cmd->tx_p, &leftBar)) {
		    /* move elevator */
		    p.p_x = 0;
		    p.p_y = 
			w->w_bbox->r_ybot + 
			((w->w_bbox->r_ytop - w->w_bbox->r_ybot) * 
			(cmd->tx_p.p_y - leftBar.r_ybot))
			/ (leftBar.r_ytop - leftBar.r_ybot) -
			(w->w_surfaceArea.r_ytop + w->w_surfaceArea.r_ybot)/2;
		    WindScroll(w, &p, (Point *) NULL);
		    return;
		}
		else if (GEO_ENCLOSE(&cmd->tx_p, &botBar)) {
		    /* move elevator */
		    p.p_y = 0;
		    p.p_x = 
			w->w_bbox->r_xbot + 
			((w->w_bbox->r_xtop - w->w_bbox->r_xbot) *
			(cmd->tx_p.p_x - botBar.r_xbot))
			/ (botBar.r_xtop - botBar.r_xbot) -
			(w->w_surfaceArea.r_xtop + w->w_surfaceArea.r_xbot)/2;
		    WindScroll(w, &p, (Point *) NULL);
		    return;
		}
		else if (GEO_ENCLOSE(&cmd->tx_p, &up)) {
		    /* scroll up */
		    p.p_y = -p.p_y;
		    p.p_x = 0;
		    WindScroll(w, (Point *) NULL, &p);
		    return;
		}
		else if (GEO_ENCLOSE(&cmd->tx_p, &down)) {
		    /* scroll down */
		    p.p_x = 0;
		    WindScroll(w, (Point *) NULL, &p);
		    return;
		}
		else if (GEO_ENCLOSE(&cmd->tx_p, &right)) {
		    /* scroll right */
		    p.p_x = -p.p_x;
		    p.p_y = 0;
		    WindScroll(w, (Point *) NULL, &p);
		    return;
		}
		else if (GEO_ENCLOSE(&cmd->tx_p, &left)) {
		    /* scroll left */
		    p.p_y = 0;
		    WindScroll(w, (Point *) NULL, &p);
		    return;
		}
	    }
	    if (GEO_ENCLOSE(&cmd->tx_p, &zoom)) {
		/* zoom in, out, or view */
		switch (cmd->tx_button)
		{
		    case TX_LEFT_BUTTON:
			WindZoom(w, 2.0);
			break;
		    case TX_MIDDLE_BUTTON:
			WindView(w);
			break;
		    case TX_RIGHT_BUTTON:
			WindZoom(w, 0.5);
			break;
		};
		return;
	    }
	};

	/* else fall through */
    };

    /*
     * At this point, we have decided that the button was not an initial
     * button push for Magic's window package.  Maybe an external window
     * package wants it, or maybe it is a continuation of a previous Magic
     * sequence (such as moving a corner of a window).
     */
    switch ( WindPackageType )
    {
	case WIND_X_WINDOWS:
	    break;
	
	case WIND_SUN_WINDOWS:
#ifdef	SUNVIEW
	    windSunHandleCaptionInput(w, cmd);
#endif	SUNVIEW
	    break;
	
	default:
	    /* Magic Windows */
	    if (cmd->tx_button == TX_MIDDLE_BUTTON) 
		return;
	    if ((cmd->tx_buttonAction == TX_BUTTON_UP) && 
		(windFrameWindow == NULL)) 
		return;

	    /* no special area or else an up push -- reframe window */
	    switch (cmd->tx_buttonAction)
	    {
		case TX_BUTTON_DOWN:
		    windFrameDown(w, cmd);
		    break;
		case TX_BUTTON_UP:
		    windFrameUp(w, cmd);
		    break;
		default:
		    TxError("windClientButtons failed!!.\n");
		    break;
	    }
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * windClientInterp
 *
 * Window's command interpreter.
 * Dispatches to long commands, providing them with the window and command
 * we are passed.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Whatever occur as a result of executing the long
 *	command supplied.
 *
 * ----------------------------------------------------------------------------
 */

Void
windCmdInterp(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int cmdNum;

    switch (cmd->tx_button)
    {
        case TX_LEFT_BUTTON:
        case TX_RIGHT_BUTTON:
        case TX_MIDDLE_BUTTON:
	    windClientButtons(w, cmd);
	    break;
	case TX_NO_BUTTON:
	    if (cmd->tx_argc > 0)
	    {
		cmdNum = Lookup(cmd->tx_argv[0], windCommands);
		ASSERT(cmdNum >= 0, "windCmdInterp");

		(void) (*windFuncs[cmdNum])(w, cmd);
		UndoNext();
	    }
	    break;
	default:
	    ASSERT(FALSE, "windCmdInterp");
	    break;
    }
}


/*
 * ----------------------------------------------------------------------------
 *  windCmdInit --
 *
 *	Initialize the window client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

void
windClientInit()
{
    windClientID = WindAddClient(WINDOW_CLIENT, ( Void (*)() ) NULL,
	( Void (*)() ) NULL, ( Void (*)() ) NULL, windCmdInterp, 
	( Void (*)() ) NULL, ( Void (*)() ) NULL, ( Void (*)() ) NULL,
	windCommands, (GrGlyph *) NULL);
}
