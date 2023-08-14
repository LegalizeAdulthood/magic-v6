/* windCmdSZ.c -
 *
 *	This file contains Magic command routines for those commands
 *	that are valid in all windows.
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
static char rcsid[]="$Header: windCmdSZ.c,v 6.0 90/08/28 19:02:15 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <math.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "tile.h"
#include "windows.h"
#include "glyphs.h"
#include "windInt.h"
#include "utils.h"
#include "signals.h"
#include "txcommands.h"


/*
 * ----------------------------------------------------------------------------
 *
 * windScrollCmd --
 *
 *	Scroll the view around
 *
 * Usage:
 *	scroll [dir [amount]]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is changed.
 *
 * ----------------------------------------------------------------------------
 */

Void
windScrollCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect r;
    int xsize, ysize;
    Point p;
    int pos;
    float amount;

    if ( (cmd->tx_argc < 1) || (cmd->tx_argc > 3) )
    {
	TxError("Usage: %s [direction [amount]]\n", cmd->tx_argv[0]);
	return 0;
    }

    if (w == NULL)
    {
	TxError("Point to a window first.\n");
	return 0;
    }

    if ((w->w_flags & WIND_SCROLLABLE) == 0) {
	TxError("Sorry, can't scroll this window.\n");
	return 0;
    };

    pos = GeoNameToPos(cmd->tx_argv[1], FALSE, TRUE);
    if (pos < 0 || pos == GEO_CENTER)
	return 0;

    if (cmd->tx_argc == 3)
    {
	if (sscanf(cmd->tx_argv[2], "%f", &amount) != 1)
	{
	    TxError("Usage: %s [direction [amount]]\n", cmd->tx_argv[0]);
	    return 0;
	}
    }
    else
	amount = 0.5;
	
    r = w->w_screenArea;
    xsize = (r.r_xtop - r.r_xbot) * amount;
    ysize = (r.r_ytop - r.r_ybot) * amount;
    p.p_x = 0;
    p.p_y = 0;

    switch (pos)
    {
	case GEO_NORTH:
	    p.p_y = -ysize;
	    break;
	case GEO_SOUTH:
	    p.p_y = ysize;
	    break;
	case GEO_EAST:
	    p.p_x = -xsize;
	    break;
	case GEO_WEST:
	    p.p_x = xsize;
	    break;
	case GEO_NORTHEAST:
	    p.p_x = -xsize;
	    p.p_y = -ysize;
	    break;
	case GEO_NORTHWEST:
	    p.p_x = xsize;
	    p.p_y = -ysize;
	    break;
	case GEO_SOUTHEAST:
	    p.p_x = -xsize;
	    p.p_y = ysize;
	    break;
	case GEO_SOUTHWEST:
	    p.p_x = xsize;
	    p.p_y = ysize;
	    break;
    }

    WindScroll(w, (Point *) NULL, &p);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 * windSetpointCmd --
 *
 *	Use the x, y specified as the location of the point tool for the
 *	next command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	global variables.
 * ----------------------------------------------------------------------------
 */

windSetpointCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int wid;

    if ((cmd->tx_argc != 4) && (cmd->tx_argc != 3) && (cmd->tx_argc != 1)) 
	goto usage;
    if ((cmd->tx_argc != 1) && !
	(StrIsInt(cmd->tx_argv[1]) && StrIsInt(cmd->tx_argv[2])) )
	goto usage;
    if ((cmd->tx_argc == 4) && !StrIsInt(cmd->tx_argv[3]))
	goto usage;

    if (cmd->tx_argc == 4) 
	wid = atoi(cmd->tx_argv[3]);
    else
	wid = WIND_UNKNOWN_WINDOW;

    if (cmd->tx_argc == 1)
    {
	if (w != (Window *) NULL) 
	{
	    TxPrintf("Point is at screen coordinates (%d, %d) in window %d.\n",
		cmd->tx_p.p_x, cmd->tx_p.p_y, w->w_wid);
	} else {
	    TxPrintf("Point is at screen coordinates (%d, %d).\n",
		cmd->tx_p.p_x, cmd->tx_p.p_y);
	}
    }
    else {
	TxSetPoint(atoi(cmd->tx_argv[1]), atoi(cmd->tx_argv[2]), wid);
    }
    return;

usage:
    TxError("Usage: %s [x y [window ID]]\n", cmd->tx_argv[0]);
}

int
windSetPrintProc(name, val)
    char *name;
    char *val;
{
    TxPrintf("%s = \"%s\"\n", name, val);
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 * windSleepCmd --
 *
 *	Take a nap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windSleepCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int time;

    if (cmd->tx_argc != 2) 
    {
	TxError("Usage: %s seconds\n", cmd->tx_argv[0]);
	return;
    }

    time = atoi(cmd->tx_argv[1]);
    for ( ; time > 1; time--)
    {
	sleep(1);
	if (SigInterruptPending) return;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * windSourceCmd --
 *
 * Implement the "source" command.
 * Process a file as a list of commands.
 *
 * Usage:
 *	source filename
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the commands request.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windSourceCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    FILE *f;

    if (cmd->tx_argc != 2)
    {
	TxError("Usage: %s filename\n", cmd->tx_argv[0]);
	return;
    }

    f = PaOpen(cmd->tx_argv[1], "r", (char *) NULL, ".", 
	    (char *) NULL, (char **) NULL);
    if (f == NULL)
	TxError("Couldn't read from %s.\n", cmd->tx_argv[1]);
    else {
	TxDispatch(f);
	(void) fclose(f);
    };
}

/*
 * ----------------------------------------------------------------------------
 * windSpecialOpenCmd --
 *
 *	Open a new window at the cursor position.  Give it a default size,
 *	and take the client's name from the command line.  Pass the rest of
 *	the command arguments off to the client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windSpecialOpenCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    WindClient wc;
    Rect area;
    bool haveCoords;
    char *client;

    haveCoords = FALSE;

    if (cmd->tx_argc < 2) goto usage;
    haveCoords = StrIsInt(cmd->tx_argv[1]);
    if (haveCoords && (
	(cmd->tx_argc < 6) ||
	!StrIsInt(cmd->tx_argv[2]) ||
	!StrIsInt(cmd->tx_argv[3]) ||
	!StrIsInt(cmd->tx_argv[4]) 
	)) goto usage;
    if (haveCoords)
	client = cmd->tx_argv[5];
    else
	client = cmd->tx_argv[1];

    wc = WindGetClient(client);
    /* clients whose names begin with '*' are hidden */
    if ((wc == (WindClient) NULL) || (client[0] == '*')) goto usage;

    if (haveCoords) {
	area.r_xbot = atoi(cmd->tx_argv[1]);
	area.r_ybot = atoi(cmd->tx_argv[2]);
	area.r_xtop = MAX(atoi(cmd->tx_argv[3]), area.r_xbot + WIND_MIN_WIDTH);
	area.r_ytop = MAX(atoi(cmd->tx_argv[4]), area.r_ybot + WIND_MIN_HEIGHT);
	/* Assume that the client will print an error message if it fails */
	(void) WindCreate(wc, &area, FALSE, cmd->tx_argc - 6, cmd->tx_argv + 6);
    }
    else {
	area.r_xbot = cmd->tx_p.p_x - CREATE_WIDTH/2;
	area.r_xtop = cmd->tx_p.p_x + CREATE_WIDTH/2;
	area.r_ybot = cmd->tx_p.p_y - CREATE_HEIGHT/2;
	area.r_ytop = cmd->tx_p.p_y + CREATE_HEIGHT/2;
	/* Assume that the client will print an error message if it fails */
	(void) WindCreate(wc, &area, TRUE, cmd->tx_argc - 2, cmd->tx_argv + 2);
    }

    return;

usage:
    TxPrintf("Usage: specialopen [leftx bottomy rightx topy] type [args]\n");
    TxPrintf("Valid window types are:\n");
    WindPrintClientList(FALSE);
    return;
}


/*
 * ----------------------------------------------------------------------------
 * windUnderCmd --
 *
 *	Move a window underneath the other windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Screen updates.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windUnderCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1)
    {
	TxError("Usage: %s\n", cmd->tx_argv[0]);
    }
    if (w == NULL)
    {
	TxError("Point to a window first.\n");
	return;
    }
    WindUnder(w);
}


/*
 * ----------------------------------------------------------------------------
 *
 * windUndoCmd
 *
 * Implement the "undo" command.
 *
 * Usage:
 *	undo [count]
 *
 * If a count is supplied, the last count events are undone.  The default
 * count if none is given is 1.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the undo module.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windUndoCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int count;

    if (cmd->tx_argc > 2)
    {
	TxError("Usage: undo [count]\n");
	return;
    }

    if (cmd->tx_argc == 2)
    {
	if (!StrIsInt(cmd->tx_argv[1]))
	{
	    TxError("Count must be numeric\n");
	    return;
	}
	count = atoi(cmd->tx_argv[1]);
    }
    else
	count = 1;

    if (UndoBackward(count) == 0)
	TxPrintf("Nothing more to undo\n");
}


/*
 * ----------------------------------------------------------------------------
 * windUpdateCmd --
 *
 *	Force an update of the graphics screen.  This is usually only called
 *	from command scripts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Display updates.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windUpdateCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1)
    {
	TxError("Usage: %s\n", cmd->tx_argv[0]);
	return;
    }

    WindUpdate();
}


/*
 * ----------------------------------------------------------------------------
 *
 * windViewCmd --
 *
 * Implement the "View" command.
 * Change the view in the selected window so everything is visable.
 *
 * Usage:
 *	view
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is changed.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windViewCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (w == NULL)
	return;

    if ((w->w_flags & WIND_SCROLLABLE) == 0) {
	TxError("Sorry, can't zoom out this window.\n");
	return;
    };

    WindView(w);
}


/*
 * ----------------------------------------------------------------------------
 *
 * windScrollBarsCmd --
 *
 *	Change the flag which says whether new windows will have scroll bars.
 *
 * Usage:
 *	windscrollbars [on|off]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A flag is changed.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
windScrollBarsCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int place;
    static char *onoff[] = {"on", "off", 0};
    static bool truth[] = {TRUE, FALSE};
    extern int windDefaultScrollBars;

    if (cmd->tx_argc != 2) goto usage;

    place = Lookup(cmd->tx_argv[1], onoff);
    if (place < 0) goto usage;

    windDefaultScrollBars = truth[place];
    if (windDefaultScrollBars)
	TxPrintf("New windows will have scroll bars.\n");
    else
	TxPrintf("New windows will not have scroll bars.\n");
    return;

    usage:
	TxError("Usage: %s [on|off]\n", cmd->tx_argv[0]);
	return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * windSendCmd --
 *
 *	Send a command to a certain window type.  If possible we will pass a
 *	arbitrarily chosen window of that type down to the client.
 *
 * Usage:
 *	send type command
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the client does
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
windSendCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Window *toWindow;
    WindClient client;
    TxCommand newcmd;
    extern int windSendCmdFunc();

    if (cmd->tx_argc < 3) goto usage;
    if (cmd->tx_argv[1][0] == '*') goto usage; /* hidden window client */

    client = WindGetClient(cmd->tx_argv[1]);
    if (client == (WindClient) NULL) goto usage;
    toWindow = (Window *) NULL;
    (void) WindSearch(client, (ClientData) NULL, (Rect *) NULL, 
	windSendCmdFunc, (ClientData) &toWindow);
    {
	int i;
	newcmd = *cmd;
	newcmd.tx_argc -= 2;
	for (i = 0; i < newcmd.tx_argc; i++) {
	    newcmd.tx_argv[i] = newcmd.tx_argv[i + 2];
	};
    }
    newcmd.tx_wid = WIND_UNKNOWN_WINDOW;
    if (toWindow != NULL) newcmd.tx_wid = toWindow->w_wid;
    (void) WindSendCommand(client, &newcmd);
    return;

    usage:
	TxError("Usage: send type command\n");
	TxPrintf("Valid window types are:\n");
	WindPrintClientList(FALSE);
	return;
}

int
windSendCmdFunc(w, cd)
    Window *w;
    ClientData cd;
{
    *((Window **) cd) = w;
    return 1;
}


/*
 * ----------------------------------------------------------------------------
 *
 * windPositionsCmd --
 *
 * Print out the positions of the windows.
 *
 * Usage:
 *	windowpositions [file]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A file is written.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
windPositionsCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    FILE *file;
    extern int windPositionsFunc();
    bool haveFile;

    if (cmd->tx_argc > 2) goto usage;
    haveFile = (cmd->tx_argc == 2);

    if (haveFile) {
	file = fopen(cmd->tx_argv[1], "w");
	if (file == (FILE *) NULL) {
	    TxError("Could not open file %s for writing.\n", cmd->tx_argv[1]);
	    return;
	};
    }
    else {
	file = stdout;
    }
    (void) WindSearch((WindClient) NULL, (ClientData) NULL, (Rect *) NULL, 
	windPositionsFunc, (ClientData) file);
    if (haveFile) (void) fclose(file);
    return;
    
usage:
    TxError("Usage:  windowpositions [file]\n");
    return;
}

int
windPositionsFunc(w, cdata)
    Window *w;
    ClientData cdata;
{
    fprintf((FILE *) cdata, "specialopen %d %d %d %d %s\n", 
	w->w_frameArea.r_xbot, 
	w->w_frameArea.r_ybot, 
	w->w_frameArea.r_xtop, 
	w->w_frameArea.r_ytop, 
	((clientRec *) w->w_client)->w_clientName);
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *
 * windZoomCmd --
 *
 * Implement the "zoom" command.
 * Change the view in the selected window by the given scale factor.
 *
 * Usage:
 *	zoom amount
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is changed.
 *
 * ----------------------------------------------------------------------------
 */

windZoomCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    float factor;

    if (w == NULL)
	return;

    if ((w->w_flags & WIND_SCROLLABLE) == 0) {
	TxError("Sorry, can't zoom this window.\n");
	return;
    };

    if (cmd->tx_argc != 2)
    {
	TxError("Usage: %s factor\n", cmd->tx_argv[0]);
	return;
    }

    factor = atof(cmd->tx_argv[1]);
    if ((factor <= 0) || (factor >= 20))
    {
	TxError("zoom factor must be between 0 and 20.\n");
	return;
    }

    WindZoom(w, factor);
}
