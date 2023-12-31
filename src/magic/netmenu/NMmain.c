/* NMmain.c -
 *
 *	This file defines the interface between the netlist
 *	editor and the window package.  Most of the routines in
 *	here are called only by the window package.  They are
 *	used for creating, deleting, and moving windows, for
 *	redisplay, and for top-level command interpretation.
 *	All the REAL work of commands goes on in other files.
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
static char rcsid[] = "$Header: NMmain.c,v 6.0 90/08/28 18:50:36 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "windows.h"
#include "graphics.h"
#include "glyphs.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "dbwind.h"
#include "textio.h"
#include "styles.h"
#include "txcommands.h"
#include "nmInt.h"
#include "netmenu.h"
#include "undo.h"

/* Forward declaration: */

extern Void nmNotDefined();

/* The comand table defined below is empty.  This means that in
 * netlist windows the only way to invoke a command is by
 * pushing a button.
 */

global char * NMcommands[] =
{
    "add term1 term2         add term1 to net of term2",
    "cleanup                 interactively cleanup netlist",
    "cull                    remove fully-wired nets from the current netlist",
    "dnet name name ...      delete the net(s) containing name(s)\n\
                        or current net if no name(s) given",
    "dterm name name ...     delete terminals from nets",
    "extract                 generate net for terminals connected to box",
    "find pattern [layers]   find all occurrences of any labels matching\n\
                        pattern beneath the box (on layers if specified)\n\
                        and leave as feedback",
    "flush [netlist]         flush changes to netlist (current list default)",
    "join term1 term2        join the nets containing term1 and term2",
    "netlist [name]          switch current netlist to name.net (default\n\
			is edit cell name)",
    "print [name]            print all terminals in name, or in current net\n\
			if no name given",
    "ripup [netlist]         ripup edit cell paint connected to paint under\n\
			box, or ripup current netlist if \"netlist\"\n\
                        typed as argument",
    "savenetlist [file]      write out current netlist",
    "shownet                 highlight edit cell paint connected to paint\n\
			under box",
    "showterms               generate feedback for all terminals in netlist",
    "trace [name]            highlight material connected to a net's \n\
                        terminals (use current net if no name given)",
    "verify                  make sure current netlist is correctly wired",
    "writeall                write out all modified netlists",
    NULL
};


Void (*(NMrtns[]))() =
{
    NMCmdAdd,
    NMCmdCleanup,
    NMCmdCull,
    NMCmdDnet,
    NMCmdDterm,
    NMCmdExtract,
    NMCmdFindLabels,
    NMCmdFlush,
    NMCmdJoin,
    NMCmdNetlist,
    NMCmdPrint,
    NMCmdRipup,
    NMCmdSavenetlist,
    NMCmdShownet,
    NMCmdShowterms,
    NMCmdTrace,
    NMCmdVerify,
    NMCmdWriteall,
    0,
};
    
/* The declarations below define the layout of the netlist
 * window.  Everything is defined in "surface" coordinates,
 * not screen coordinates.  Within each of the three categories,
 * things are in order from top to bottom.  Note:  the order of
 * the buttons is important, since they are referred to by index
 * for specific purposes.  See nmInt.h.
 */

NetButton NMButtons[] =
{
    NULL,	STYLE_ORANGE1,		0,	200,	80,	210,
	NMGetLabels, NULL, NMNextLabel, NULL, NMNextLabel, NULL,
    NULL,	STYLE_ERASEALL,		0,	174,	24,	198,
	NMPutLabel, NULL, NMReOrientLabel, NULL, NMReOrientLabel, NULL,
    NULL,	STYLE_ORANGE1,		42,	188,	59,	198,
	NMChangeNum, NULL, NMChangeNum, NULL, NMChangeNum, NULL,
    NULL,	STYLE_ORANGE1,		63,	188,	80,	198,
	NMChangeNum, NULL, NMChangeNum, NULL, NMChangeNum, NULL,
    "Find",	STYLE_ORANGE2,		42,	174,	80,	184,
	NMFindLabel, NULL, NMFindLabel, NULL, NMFindLabel, NULL,
    NULL,	STYLE_GREEN1,	0,	150,	80,	160,
	NMButtonNetList, NULL, NMButtonNetList, NULL, NMButtonNetList, NULL,
    "Verify",	STYLE_BLUE1,	0,	138,	38,	148,
	NMCmdVerify, NULL, NMCmdVerify, NULL, NMCmdVerify, NULL,
    "Print",	STYLE_BLUE2,	42,	138,	80,	148,
	NMCmdPrint, NULL, NMCmdPrint, NULL, NMCmdPrint, NULL,
    "Terms",	STYLE_RED1,		0,	126,	38,	136,
	NMCmdShowterms, NULL, NMCmdShowterms, NULL, NMCmdShowterms, NULL,
    "Cleanup",	STYLE_RED2,		42,	126,	80,	136,
	NMCmdCleanup, NULL, NMCmdCleanup, NULL, NMCmdCleanup, NULL,
    "No Net",STYLE_GRAY1,		0,	114,	38,	124,
	NMCmdDnet, NULL, NMCmdDnet, NULL, NMCmdDnet, NULL,
    "Show",STYLE_YELLOW1,		42,	114,	80,	124,
	NMShowUnderBox, NULL, NMShowUnderBox, NULL, NMShowUnderBox, NULL,
    NULL,	-1	/* -1 Signals end of list. */
};

NetLabel nmLabels[] =
{
    "Label",	STYLE_WHITE,	0,	212,	80,	222,
    "Netlist",	STYLE_WHITE,	0,	162,	80,	172,
    NULL,	-1 	/* -1 signals end of list. */
};

NetRect nmRects[] =
{
    STYLE_BBOX,		8,	174,	16,	198,
    STYLE_BBOX,		0,	182,	24,	190,
    STYLE_BBOX,		12,	186,	12,	186,
    -1			/* -1 signals end of list. */
};


/* The following definitions are for the total surface area of the
 * netlist menu, and the initial screen location of netlist menus.
 */

Rect nmSurfaceArea = {-4, 110, 84, 226};
Rect nmScreenArea = {0, 0, 140, 190};

/* Only one netlist window is allowed to be open at once.  This is it. */

Window *NMWindow = NULL;


/*
 * ----------------------------------------------------------------------------
 *
 * NMcreate --
 *
 * 	Called by the window package when the user tries to create
 *	a new netlist window.
 *
 * Results:
 *	FALSE if there's already a netlist window:  we only allow
 *	one at a time.  TRUE is returned otherwise.
 *
 * Side effects:
 *	Sets the size and location of the window.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
bool
NMcreate(window, argc, argv)
    Window *window;		/* Partially-created window. */
    int argc;			/* Count of additional arguments. */
    char *argv[];		/* Pointers to additional arguments. */
{
    if (argc > 0)
        TxError("Ignoring extra argments for netlist menu creation.\n");
    if (NMWindow != NULL)
    {
	TxError("Sorry, can't have more than one netlist menu at a time.\n");
	return FALSE;
    }
    NMWindow = window;
    WindCaption(window, "NETLIST MENU");
    window->w_frameArea = nmScreenArea;
    window->w_flags &= ~(WIND_SCROLLABLE | WIND_SCROLLBARS);
    WindSetWindowAreas(window);
    WindMove(window, &nmSurfaceArea);
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * NMdelete --
 *
 * 	This procedure is invoked by the window package just before
 *	the window is deleted, so we can clean things up before they
 *	go away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Nothing happens except to record that there's no longer an
 *	active netlist menu.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
bool
NMdelete(window)
    Window *window;		/* The window that's about to disappear. */
{
    NMWindow = NULL;
    NMClearPoints();
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * NMreposition --
 *
 * 	This procedure is called by the window package when the netlist
 *	menu is being repositioned or resized.  It gives us a chance to
 *	modify the final position, or at least clean ourselves up in
 *	accordance with the move.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window's view is changed so that the menu fits comfortably
 *	within it.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
Void
NMreposition(window, newScreenArea, final)
    Window *window;			/* Window being repositioned. */
    Rect *newScreenArea;		/* New screen area of window. */
    bool final;				/* FALSE means the new area is
					 * tentative, and we can modify it
					 * (we never do).  TRUE means this
					 * is our opportunity for final
					 * cleanup.
					 */
{
    if (final) WindMove(window, &nmSurfaceArea);
}

/*
 * ----------------------------------------------------------------------------
 *
 * NMredisplay --
 *
 * 	This window is called, either by the window package or by
 *	other routines in this module, when part or all of the netlist
 *	menu needs to be redisplayed.  It sets and clears its own
 *	window lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given area of the window is redisplayed.
 *
 * ----------------------------------------------------------------------------
 */

Void
NMredisplay(w, rootArea, clipArea)
    Window *w;			/* The window containing the area. */
    Rect *rootArea;		/* Redisplay area in surface coordinates. */
    Rect *clipArea;		/* Screen area to clip to.  If NULL, use
				 * screen area of window. */

{
    Rect clip, screenR;
    Point screenP;
    NetButton *nb;
    NetLabel *nl;
    NetRect *nr;

    /* Make sure that there's really a netlist window!  (This procedure
     * can be called before the window exists).
     */
    
    if (NMWindow == (Window *) NULL) return;

    GrLock(w, TRUE);

    /* Transform root area to the screen, then erase previous stuff
     * in that area.
     */

    if (clipArea != NULL) clip = *clipArea;
    else clip = GrScreenRect;
    GrClipTo(&clip);
    WindSurfaceToScreen(w, rootArea, &screenR);
    GrClipBox(&screenR, STYLE_ERASEALL);
    GrClipBox(&screenR, STYLE_PURPLE);

    /* Redisplay each of the buttons. */

    for (nb = NMButtons; nb->nmb_style >= 0; nb++)
    {
	if (GEO_TOUCH(&nb->nmb_area, rootArea))
	{
	    WindSurfaceToScreen(w, &nb->nmb_area, &screenR);

	    /* Erase the area of the button before redrawing.  This is
	     * needed on monochrome displays, or else the purple will
	     * OR in with the button color.
	     */

	    GrClipBox(&screenR, STYLE_ERASEALL);
	    GrClipBox(&screenR, nb->nmb_style);
	    GrClipBox(&screenR, STYLE_BBOX);
	    if (nb->nmb_text != NULL)
	    {
		screenP.p_x = (screenR.r_xbot + screenR.r_xtop)/2;
		screenP.p_y = (screenR.r_ybot + screenR.r_ytop)/2;
		screenR.r_xbot += 1;
		screenR.r_ybot += 1;
		screenR.r_xtop -= 1;
		screenR.r_ytop -= 1;
		GrClipTo(&GrScreenRect); 
		GrPutText(nb->nmb_text, STYLE_BBOX, &screenP,
		    GEO_CENTER, GR_TEXT_MEDIUM, TRUE, &screenR, (Rect *) NULL);
		GrClipTo(&clip);
	    }
	}
    }

    /* Redisplay each of the decorative labels. */

    GrClipTo(&GrScreenRect); 
    for (nl = nmLabels; nl->nml_style >= 0; nl++)
    {
	if (GEO_TOUCH(&nl->nml_area, rootArea))
	{
	    WindSurfaceToScreen(w, &nl->nml_area, &screenR);
	    screenP.p_x = (screenR.r_xbot + screenR.r_xtop)/2;
	    screenP.p_y = (screenR.r_ybot + screenR.r_ytop)/2;
	    screenR.r_xbot += 1;
	    screenR.r_ybot += 1;
	    screenR.r_xtop -= 1;
	    screenR.r_ytop -= 1;
	    GrPutText(nl->nml_text, nl->nml_style, &screenP,
		GEO_CENTER, GR_TEXT_MEDIUM, TRUE, &screenR, (Rect *) NULL);
	}
    }
    GrClipTo(&clip);

    /* Redisplay each of the decorative rectangles. */

    for (nr = nmRects; nr->nmr_style >= 0; nr++)
    {
	if (GEO_TOUCH(&nr->nmr_area, rootArea))
	{
	    WindSurfaceToScreen(w, &nr->nmr_area, &screenR);
	    GrClipBox(&screenR, nr->nmr_style);
	}
    }

    GrUnlock(w);
}

/*
 * ----------------------------------------------------------------------------
 *
 * NMcommand --
 *
 * 	This procedure is called by the window package whenever a
 *	button is pressed with the cursor inside the netlist menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the cursor is over one of the buttons, the corresponding
 *	command routine (if any) is called.  Otherwise, nothing is
 *	done.
 *
 * ----------------------------------------------------------------------------
 */

Void
NMcommand(w, cmd)
    Window *w;			/* Net-list menu window. */
    TxCommand *cmd;
{
    NetButton *nb;
    Point surfacePoint;
    Void (*proc)();
    int cmdNum;

    if (cmd->tx_button == TX_NO_BUTTON)
    {
	if (cmd->tx_argc > 0)
	{
	    cmdNum = Lookup(cmd->tx_argv[0], NMcommands);
	    (void) (*NMrtns[cmdNum])(w, cmd);
	}
	goto done;
    }

    if (w == NULL) return;

    WindPointToSurface(w, &cmd->tx_p, &surfacePoint, (Rect *) NULL);

    /* Since some of the command routines are invoked both in
     * response to buttons and in response to typed commands,
     * fake the presence of a single command keyword.
     */
    
    cmd->tx_argc = 1;
    cmd->tx_argv[0] = "";

    /* See if the cursor is over any of the buttons. */

    for (nb = NMButtons; nb->nmb_style >= 0; nb++)
    {
	if (GEO_ENCLOSE(&surfacePoint, &nb->nmb_area))
	{
	    /* Pick the appropriate procedure for this button action,
	     * then invoke it (if it exists).
	     */

	    proc = NULL;
	    switch (cmd->tx_buttonAction)
	    {
		case TX_BUTTON_DOWN:
		    switch (cmd->tx_button)
		    {
			case TX_LEFT_BUTTON:
			    proc = nb->nmb_leftDown;
			    break;
			case TX_MIDDLE_BUTTON:
			    proc = nb->nmb_middleDown;
			    break;
			case TX_RIGHT_BUTTON:
			    proc = nb->nmb_rightDown;
			    break;
		    }
		    break;
		case TX_BUTTON_UP:
		    switch (cmd->tx_button)
		    {
			case TX_LEFT_BUTTON:
			    proc = nb->nmb_leftUp;
			    break;
			case TX_MIDDLE_BUTTON:
			    proc = nb->nmb_middleUp;
			    break;
			case TX_RIGHT_BUTTON:
			    proc = nb->nmb_rightUp;
			    break;
		    }
		    break;
	    }
	    if (proc != NULL) (void) (*proc)(w, cmd, nb, &surfacePoint);
	}
    }

    done:
    UndoNext();
    return;
}
	    

/*
 * ----------------------------------------------------------------------------
 *
 * NWinit --
 *
 * 	Called by Magic's main program once at the very beginning
 *	so this module can initialize itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Add this module to the window package's list of registered
 *	clients, and to the undo package.
 *
 * ----------------------------------------------------------------------------
 */

void
NMinit()
{
    static char *doc =
"You are currently using the \"netlist\" tool.  The button actions are:\n\
   left    - select the net containing the terminal nearest the cursor\n\
   right   - toggle the terminal nearest the cursor into/out of current net\n\
   middle  - join current net and net containing terminal nearest the cursor\n\
";

    (void) WindAddClient("netlist", NMcreate, NMdelete,
	NMredisplay, NMcommand, (Void (*)()) NULL,
	NMCheckWritten, NMreposition, NMcommands,
	(GrGlyph *) NULL);
    DBWHLAddClient(NMRedrawPoints);
    DBWHLAddClient(NMRedrawCell);
    DBWAddButtonHandler("netlist", NMButtonProc, STYLE_CURS_NET, doc);
    NMUndoInit();
}

/*
 * ----------------------------------------------------------------------------
 *
 * nmNotDefined --
 *
 * 	This is a dummy command procedure for things that aren't
 *	implemented yet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Just prints out an error message that this command isn't implemented.
 *
 * ----------------------------------------------------------------------------
 */

Void
nmNotDefined()
{
    TxError("Sorry, no code has been written for that button yet.\n");
}
