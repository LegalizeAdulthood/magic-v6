/*
 * CmdRS.c --
 *
 * Commands with names beginning with the letters R through S.
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
static char rcsid[] = "$Header: CmdRS.c,v 6.0 90/08/28 18:07:21 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "stack.h"
#include "geometry.h"
#include "utils.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "commands.h"
#include "textio.h"
#include "graphics.h"
#include "tech.h"
#include "drc.h"
#include "txcommands.h"
#include "router.h"
#include "gcr.h"
#include "doubleint.h"
#include "heap.h"
#include "grouter.h"
#include "netlist.h"
#include "netmenu.h"
#include "select.h"
#include "sim.h"
#ifdef SYSV
#include <string.h>
#endif

extern void DisplayWindow();
extern double atof();


#ifndef NO_SIM
/*
 * ----------------------------------------------------------------------------
 *
 * CmdRsim
 *
 * 	Starts Rsim under Magic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Rsim is forked.
 *
 * ----------------------------------------------------------------------------
 */
  

CmdRsim(w, cmd)
    Window *w;
    TxCommand *cmd;
{

    if ((cmd->tx_argc == 1) && (!SimRsimRunning)) {
	TxPrintf("usage: rsim [options] file\n");
	return;
    }
    if ((cmd->tx_argc != 1) && (SimRsimRunning)) {
	TxPrintf("Simulator already running.  You cannot start another.\n");
	return;
    }
    if ((w == (Window *) NULL) || (w->w_client != DBWclientID)) {
	TxError("Put the cursor in a layout window.\n");
	return;
    }
    if (cmd->tx_argc != 1) {
	cmd->tx_argv[cmd->tx_argc] = (char *) 0;
	SimStartRsim(cmd->tx_argv);
    }
    SimConnectRsim(FALSE);

}
#endif
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 *
 * CmdRoute --
 *
 * 	Route the nets in the current netlist.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the edit cell.
 *
 * ----------------------------------------------------------------------------
 */

#define ROUTERCHECKS	0
#define ROUTERDEBUG	1
#define ROUTERFILE	2
#define ROUTERSHOWMAP	3
#define ROUTERSHOWRES	4
#define ROUTERSHOWEND	5
#define ROUTEREND	6
#define ROUTERHELP	7
#define ROUTERJOG	8
#define ROUTERMMAX	9
#define ROUTERNETLIST	10
#define ROUTEROBST	11
#define	ROUTERORIGIN	12
#define ROUTERSTATS     13
#define ROUTERSETTINGS	14
#define ROUTERSTEADY	15
#define ROUTERTECHINFO	16
#define ROUTERVIAS	17
#define	ROUTERVIAMIN	18


    /* ARGSUSED */

CmdRoute(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int option;
    GCRChannel *ch;
    Rect area;
    NLNetList netList;
    char *netListName;

    static char *cmdRouteOption[] =
    {	
	"*checks		toggle column checking",
	"*debug		toggle router trace messages",
	"*file file		route from a channel data file",
	"*map			toggle channel obstacle map display",
	"*showcolumns		toggle channel column text display",
	"*showfinal		toggle text display of final channels",

	"end real		set channel router end constant",
	"help			print this help information",
	"jog int		set minimum jog length (grid units)",
	"metal			toggle metal maximization",
	"netlist file		set current netlist",
	"obstacle real		set obstacle constant",
	"origin [x y]		print or set routing grid origin",
	"stats			print and clear previous route statistics",
	"settings		show router parameters", 
	"steady int		set steady net constant",
	"tech			show router technology information",
	"vias int		set metal maximization via limit (grid units)",
	"viamin			via minimization",
	NULL
    };

    if (cmd->tx_argc == 1)
    {
	if (!ToolGetEditBox(&area))
	    return;
	Route(EditCellUse, &area);
	return;
    }

    option = Lookup(cmd->tx_argv[1], cmdRouteOption);
    if (option == -1)
    {
	TxError("Ambiguous routing option: \"%s\"\n", cmd->tx_argv[1]);
	goto usage2;
    }
    if (option < 0)
	goto usage;

    switch (option)
    {
	/* These first few options are for wizards only.
	 */
	case ROUTERDEBUG:
	    GcrDebug= !GcrDebug;
	    TxPrintf("Router debug tracing: %s\n", GcrDebug ? "on" : "off");
	    break;
	case ROUTERCHECKS:
	    GcrNoCheck= !GcrNoCheck;
	    TxPrintf("Router column checking: %s\n", GcrNoCheck ? "on" : "off");
	    break;
	case ROUTERFILE:
	    /*  Display routing problems from file directly on the screen.
	     */
	    if(cmd->tx_argc!=3)
		goto wrongNumArgs;
	    if((ch = GCRRouteFromFile(cmd->tx_argv[2])) == (GCRChannel *) NULL)
		 TxError("Bad channel from file %s\n", cmd->tx_argv[2]);
	    else RtrPaintBack(ch, EditCellUse->cu_def);
	    break;
	case ROUTERSHOWRES:
	    GcrShowResult= !GcrShowResult;
	    TxPrintf("Show channel columns: %s\n", GcrShowResult ? "on":"off");
	    break;
	case ROUTERSHOWEND:
	    GcrShowEnd= !GcrShowEnd;
	    TxPrintf("Show finished channels: %s\n", GcrShowEnd ? "on" : "off");
	    break;
	case ROUTERSHOWMAP:
	    GcrShowMap= !GcrShowMap;
	    TxPrintf("Show channel maps: %s\n", GcrShowMap ? "on" : "off");
	    break;

	case ROUTERHELP:
	    TxPrintf("Router commands have the form \"route option\",\n");
	    TxPrintf("where option is one of:\n\n");
	    if(cmd->tx_argc==2)
		for (option=ROUTEREND; option<= ROUTERVIAMIN; option++)
		    TxPrintf("  %s\n", cmdRouteOption[option]);
	    else
	    if((cmd->tx_argc==3) && !strcmp(cmd->tx_argv[2], "*wizard"))
		for (option=0; option !=ROUTEREND; option++)
			TxPrintf("  %s\n", cmdRouteOption[option]);
	    TxPrintf("\n");
	    break;
	case ROUTERSTATS:
	    RtrPaintStats(TT_SPACE, 0);
	    break;
	case ROUTERSETTINGS:
	    TxPrintf("Router parameter settings:\n");
	    TxPrintf("   Channel end constant... %f\n", RtrEndConst);
	    TxPrintf("   Metal maximization..... %s\n", RtrDoMMax ? "on":"off");
	    TxPrintf("   Minimum jog length..... %d\n", GCRMinJog);
	    TxPrintf("   Net list............... \"%s\"\n", NMNetlistName());
	    TxPrintf("   Obstacle constant...... %f\n", GCRObstDist);
	    TxPrintf("   Steady net constant.... %d\n", GCRSteadyNet);
	    TxPrintf("   Via limit.............. %d\n", RtrViaLimit);
	    if((cmd->tx_argc!=3) || strcmp(cmd->tx_argv[2], "*wizard"))
		break;
	    TxPrintf("\n");
	    TxPrintf("   Debug tracing.......... %s\n", GcrDebug ? "on":"off");
	    TxPrintf("   Column checking........ %s\n", GcrNoCheck?"on":"off");
	    TxPrintf("   Show channel columns... %s\n",
		    GcrShowResult ? "on" : "off");
	    TxPrintf("   Show finished channels  %s\n", GcrShowEnd?"on":"off");
	    TxPrintf("   Show channel maps...... %s\n", GcrShowMap?"on":"off");
	    break;
	case ROUTERTECHINFO:
	    TxPrintf("Router Technology Information:\n");
	    TxPrintf("   Preferred layer..... %s; width %d\n",
		    DBTypeLongName(RtrMetalType), RtrMetalWidth);
	    TxPrintf("   Alternate layer..... %s; width %d\n",
		    DBTypeLongName(RtrPolyType), RtrPolyWidth);
	    TxPrintf("   Contacts............ %s; ",
		    DBTypeLongName(RtrContactType));
	    TxPrintf("width %d; offset %d; surrounds %d, %d\n", RtrContactWidth,
		    RtrContactOffset, RtrMetalSurround, RtrPolySurround);
	    TxPrintf("   Subcell separations: %d up; %d down\n",
		    RtrSubcellSepUp, RtrSubcellSepDown);
	    TxPrintf("   Router grid spacing: %d\n", RtrGridSpacing);
	    break;
	case ROUTERMMAX:
	    RtrDoMMax= !RtrDoMMax;
	    TxPrintf("Metal maximization: %s\n", RtrDoMMax ? "on" : "off");
	    break;
	case ROUTERVIAS:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		if(!sscanf(cmd->tx_argv[2], "%d", &RtrViaLimit))
		    TxError("Bad value for via limit\n");
	    }
	    TxPrintf("Via limit is %d\n", RtrViaLimit);
	    break;
	case ROUTEREND:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		if(!sscanf(cmd->tx_argv[2], "%f", &RtrEndConst))
		    TxError("Bad value for channel end distance\n");
	    }
	    TxPrintf("Channel end constant is %f\n", RtrEndConst);
	    break;
	case ROUTERJOG:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		if(!sscanf(cmd->tx_argv[2], "%d", &GCRMinJog))
		    TxError("Bad value for minimum jog length\n");
	    }
	    TxPrintf("Minimum jog length is %d\n", GCRMinJog);
	    break;
	case ROUTEROBST:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		if(!sscanf(cmd->tx_argv[2], "%f", &GCRObstDist))
		    TxError("Bad value for obstacle constant\n");
	    }
	    TxPrintf("Obstacle constant is %f\n", GCRObstDist);
	    break;
	case ROUTERSTEADY:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		if(!sscanf(cmd->tx_argv[2], "%d", &GCRSteadyNet))
		    TxError("Bad value for steady net constant\n");
	    }
	    TxPrintf("Steady net constant is %d\n", GCRSteadyNet);
	    break;
	case ROUTERNETLIST:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		NMNewNetlist(cmd->tx_argv[2]);
	    }
	    TxPrintf("Current list is \"%s\"\n", NMNetlistName());
	    break;
	case ROUTERORIGIN:
	    if (cmd->tx_argc != 2)
	    {
		if (cmd->tx_argc != 4)
		    goto wrongNumArgs;
		if (!StrIsInt(cmd->tx_argv[2]) || !StrIsInt(cmd->tx_argv[3]))
		{
		    TxError("Origin coordinates must be integers\n");
		    break;
		}
		RtrOrigin.p_x = atoi(cmd->tx_argv[2]);
		RtrOrigin.p_y = atoi(cmd->tx_argv[3]);
		break;
	    }
	    TxPrintf("Routing grid origin = (%d,%d)\n",
			RtrOrigin.p_x, RtrOrigin.p_y);
	    break;
	case ROUTERVIAMIN:
	    if (!ToolGetEditBox(&area))
		return;
	    if (!NMHasList())
	    {
		netListName = EditCellUse->cu_def->cd_name;
		TxPrintf("No netlist selected yet;  using \"%s\".\n", netListName);
		NMNewNetlist(netListName);
	    }
	    else
		netListName = NMNetlistName();

	    if ( NLBuild(EditCellUse, &netList))
	    {
		int nvia;

		nvia = RtrViaMinimize(EditCellUse->cu_def);
		DBWAreaChanged(EditCellUse->cu_def, &area,
		    DBW_ALLWINDOWS, &DBAllButSpaceBits);
		WindUpdate();
		TxPrintf("\n%d vias removed\n",nvia);
		NLFree(&netList);
	    }
	    break;
    } /* switch*/
    return;

wrongNumArgs:
    TxError("Wrong number of arguments to %s option.\n", cmd->tx_argv[1]);
    TxError("Type \":route help\" for help.\n");
	    return;
usage:
    TxError("\"%s\" isn't a valid router option.", cmd->tx_argv[1]);

usage2:
    TxError("  Type \"route help\" for help.\n");
    return;
}
#else
     /* these guys are used in a couple of places */
     int RtrMetalWidth=2, RtrPolyWidth=2, RtrContactWidth=2;
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSave --
 *
 * Implement the "save" command.
 * Writes the EditCell out to a disk file.
 *
 * Usage:
 *	save [file]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes the cell out to file, if specified, or the file
 *	associated with the cell otherwise.
 *	Updates the caption in the window if the name of the edit
 *	cell has changed.
 *	Clears the modified bit in the cd_flags.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdSave(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc > 2)
    {
	TxError("Usage: %s [file]\n", cmd->tx_argv[0]);
	return;
    }

    ASSERT(EditCellUse != (CellUse *) NULL, "CmdSave");
    DBUpdateStamps();
    if (cmd->tx_argc == 2)
    {
	if (CmdIllegalChars(cmd->tx_argv[1], "[],", "Cell name"))
	    return;
	cmdSaveCell(EditCellUse->cu_def, cmd->tx_argv[1], FALSE, TRUE);
    }
    else cmdSaveCell(EditCellUse->cu_def, (char *) NULL, FALSE, TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSee --
 *
 * 	This procedure is used to enable or disable display of certain
 *	things on the screen.
 *
 * Usage:
 *	see [no] stuff
 *
 *	Stuff consists of mask layers or the keyword "allSame"
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The indicated mask layers are enabled or disabled from being
 *	displayed in the current window.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdSee(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int flags;
    bool off;
    char *arg;
    TileTypeBitMask mask;
    DBWclientRec *crec;

    if ((w == NULL) || (w->w_client != DBWclientID))
    {
	TxError("Point to a layout window first.\n");
	return;
    }
    crec = (DBWclientRec *) w->w_clientData;

    arg = (char *) NULL;
    off = FALSE;
    flags = 0;
    if (cmd->tx_argc > 1)
    {
	if (strcmp(cmd->tx_argv[1], "no") == 0)
	{
	    off = TRUE;
	    if (cmd->tx_argc > 2) arg = cmd->tx_argv[2];
	}
	else arg = cmd->tx_argv[1];
	if ((cmd->tx_argc > 3) || ((cmd->tx_argc == 3) && !off))
	{
	    TxError("Usage: see [no] layers|allSame\n");
	    return;
	}
    }

    /* Figure out which things to set or clear.  Don't ever make space
     * invisible:  that doesn't make any sense.
     */

    if (arg != NULL)
    {
	if (strcmp(arg, "allSame") == 0)
	{
	    mask = DBZeroTypeBits;
	    flags = DBW_ALLSAME;
	}
	else
	{
	    if (!CmdParseLayers(arg, &mask))
		return;
	}
    }
    else mask = DBAllTypeBits;

    if (TTMaskHasType(&mask, L_LABEL))
	flags |= DBW_SEELABELS;
    TTMaskClearType(&mask, L_LABEL);
    TTMaskClearType(&mask, L_CELL);
    TTMaskClearType(&mask, TT_SPACE);

    if (off)
    {
	int i;
	for (i = 0; i < DBNumUserLayers; i++)
	{
	    if (TTMaskHasType(&mask, i))
		TTMaskClearMask(&crec->dbw_visibleLayers,
			&DBLayerTypeMaskTbl[i]);
	}
	crec->dbw_flags &= ~flags;
    }
    else
    {
	int i;
	for (i = 0; i < DBNumUserLayers; i++)
	{
	    if (TTMaskHasType(&mask, i))
		TTMaskSetMask(&crec->dbw_visibleLayers,
			&DBLayerTypeMaskTbl[i]);
	}
	crec->dbw_flags |= flags;
    }
    WindAreaChanged(w, &w->w_screenArea);
    return;
}
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 * CmdSeeFlags --
 *
 * 	Display router-generated flags on the highlight layer.  User points
 *	to a channel and invokes the command with an argument naming the
 *	flag to be displayed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paints highlights on the screen.  Allocates a special cell def and
 *	initializes a hash table glTileToChannel the first time it is called.
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdSeeFlags(w, cmd)
    Window * w;
    TxCommand *cmd;
{
    Rect      rootRect;
    Point     point;
    Window  * window;

    window = CmdGetRootPoint(&point, &rootRect);
    if (window == (Window *) NULL)
	return;

    if(cmd->tx_argc > 2)
    {
	TxError("Useage:  %s [flag name]\n", cmd->tx_argv[0]);
	return;
    }
    if(cmd->tx_argc == 2)
    {
	GCRShow(&point, cmd->tx_argv[1]);
	TxError("%s:  flag highlights turned on.\n", cmd->tx_argv[0]);
    }
    else
    {
	NMUnsetCell();
	TxError("%s:  flag highlights turned off.\n", cmd->tx_argv[0]);
    }
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * cmdSelectArea --
 *
 * 	This is a utility procedure used by CmdSelect to do area
 *	selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is augmented to contain all the information on
 *	layers that is visible under the box, including paint, labels,
 *	and unexpanded subcells.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
void
cmdSelectArea(layers, less)
    char *layers;			/* Which layers are to be selected. */
    bool less;
{
    SearchContext scx;
    TileTypeBitMask mask;
    int windowMask, xMask;
    DBWclientRec *crec;
    Window *window;

    window = ToolGetBoxWindow(&scx.scx_area, &windowMask);
    if (window == NULL)
    {
	TxPrintf("The box isn't in a window.\n");
	return;
    }

    /* Since the box may actually be in multiple windows, we have to
     * be a bit careful.  If the box is only in one window, then there's
     * no problem.  If it's in more than window, the cursor must
     * disambiguate the windows.
     */
    
    xMask = ((DBWclientRec *) window->w_clientData)->dbw_bitmask;
    if ((windowMask & ~xMask) != 0)
    {
	window = CmdGetRootPoint((Point *) NULL, (Rect *) NULL);
        xMask = ((DBWclientRec *) window->w_clientData)->dbw_bitmask;
	if ((windowMask & xMask) == 0)
	{
	    TxPrintf("The box is in more than one window;  use the cursor\n");
	    TxPrintf("to select the one you want to select from.\n");
	    return;
	}
    }
    if (CmdParseLayers(layers, &mask))
    {
	if (TTMaskEqual(&mask, &DBSpaceBits))
	    (void) CmdParseLayers("*,label", &mask);
	TTMaskClearType(&mask, TT_SPACE);
    }
    else return;
    
    if (less)
      {
	(void) SelRemoveArea(&scx.scx_area, &mask);
	return;
      }

    scx.scx_use = (CellUse *) window->w_surfaceID;
    scx.scx_trans = GeoIdentityTransform;
    crec = (DBWclientRec *) window->w_clientData;
    SelectArea(&scx, &mask, crec->dbw_bitmask);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSelect --
 *
 * Implement the "select" command.
 *
 * Usage:
 *	select [option args]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current selection is modified.  See the user documentation
 *	for all the possible things this command can do.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */
CmdSelect(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    TileTypeBitMask mask;
    SearchContext scx;
    DBWclientRec *crec;
    Window *window;
    bool samePlace;

    /* The two tables below define the allowable selection options, and
     * also the message printed out by ":select help" to describe the
     * options (can't use the same table for both because of the presence
     * of the "more" option).  Note that there's one more entry in the
     * second table, due to a help message for ":select" with no arguments.
     */

#define AREA	0
#define CELL	1
#define SCLEAR	2
#define HELP	3
#define SAVE	4
#define DEFAULT	5

    static char *cmdSelectOption[] =
    {
	"area",
	"cell",
	"clear",
	"help",
	"save",
	NULL
    };
    static char *cmdSelectMsg[] =
    {
	"[more | less]                [de]select paint chunk/region/net under\n\
                                        cursor, or [de]select subcell if cursor\n\
		                        over space",
	"[more | less] area [layers]  [de]select all info under box in layers",
	"[more | less] cell [name]    [de]select cell under cursor, or \"name\"",
	"clear                        clear selection",
	"help                         print this message",
	"save file                    save selection on disk in file.mag",
	NULL
    };

    static TileType type = TT_SELECTBASE-1;
				/* Type of material being pointed at.
				 * Remembered across commands so that when
				 * multiples types are pointed to, consecutive
				 * selections will cycle through them.
				 */
    static Rect lastArea = {-100, -100, -200, -200};
				/* Used to remember region around what was
				 * pointed at in the last select command:  a
				 * new selection in this area causes the next
				 * bigger thing to be selected.
				 */
    static int lastCommand;	/* Serial number of last command:  the next
				 * bigger thing is only selected when there
				 * are several select commands in a row.
				 */
    static Rect chunkSelection;	/* Used to remember the size of the last chunk
				 * selected.
				 */
    static int level;		/* How big a piece to select.  See definitions
				 * below.
				 */
#define CHUNK 0
#define REGION 1
#define NET 2
    static CellUse *lastUse;	/* The last cellUse selected.  Used to step
				 * through multiple uses underneath the cursor.
				 */
    static Point lastIndices;	/* The array indices of the last cell selected.
				 * also used to step through multiple uses.
				 */
    static bool lessCycle = FALSE, lessCellCycle = FALSE;
    char path[200], *printPath, **msg, **optionArgs;
    TerminalPath tpath;
    CellUse *use;
    Transform trans, rootTrans, tmp1;
    Point p;
    Rect r;
    bool more, less;
    int option;
    
/* How close two clicks must be to be considered the same point: */

#define MARGIN 2

    if ((w == (Window *) NULL) || (w->w_client != DBWclientID))
    {
	TxError("Put the cursor in a layout window\n");
	return;
    }

    /* See if "more" was given.  If so, just strip off the "more" from
     * the argument list and set the "more" flag.
     */
    
    if ((cmd->tx_argc >= 2)
	    && (strncmp(cmd->tx_argv[1], "more", strlen(cmd->tx_argv[1]))
	    == 0))
    {
	more = TRUE;
	less = FALSE;
	optionArgs = &cmd->tx_argv[2];
	cmd->tx_argc--;
    }
    else if ((cmd->tx_argc >= 2)
	     && (strncmp(cmd->tx_argv[1], "less", strlen(cmd->tx_argv[1])) == 0))
      {
	more = FALSE;
	less = TRUE;
	optionArgs = &cmd->tx_argv[2];
	cmd->tx_argc--;
      }
    else
    {
	more = FALSE;
	less = FALSE;
	optionArgs = &cmd->tx_argv[1];
    }

    /* Check the option for validity. */

    if (cmd->tx_argc == 1)
	option = DEFAULT;
    else
    {
	option = Lookup(optionArgs[0], cmdSelectOption);
	if (option < 0)
	{
	    TxError("\"%s\" isn't a valid select option.\n", cmd->tx_argv[1]);
	    option = HELP;
	    cmd->tx_argc = 2;
	}
    }

#ifndef NO_SIM
    SimRecomputeSel = TRUE;
#endif

    switch (option)
    {
	/*--------------------------------------------------------------------
	 * Select everything under the box, perhaps looking only at
	 * particular layers.
	 *--------------------------------------------------------------------
	 */

	case AREA:
	    if (cmd->tx_argc > 3)
	    {
		usageError:
		TxError("Bad arguments:\n    select %s\n",
			cmdSelectMsg[option+1]);
		return;
	    }
	    if (!(more || less)) SelectClear();
	    if (cmd->tx_argc == 3)
		cmdSelectArea(optionArgs[1], less);
	    else cmdSelectArea("*,label,subcell", less);
	    return;
	
	/*--------------------------------------------------------------------
	 * Clear out all of the material in the selection.
	 *--------------------------------------------------------------------
	 */

	case SCLEAR:
	    if ((more) || (less) || (cmd->tx_argc > 2)) goto usageError;
	    SelectClear();
	    return;
	
	/*--------------------------------------------------------------------
	 * Print out help information.
	 *--------------------------------------------------------------------
	 */

	case HELP:
	    TxPrintf("Selection commands are:\n");
	    for (msg = &(cmdSelectMsg[0]); *msg != NULL; msg++)
		TxPrintf("    select %s\n", *msg);
	    return;

	/*--------------------------------------------------------------------
	 * Save the selection as a new Magic cell on disk.
	 *--------------------------------------------------------------------
	 */

	 case SAVE:
	    if (cmd->tx_argc != 3) goto usageError;

	    /* Be sure to paint DRC check information into the cell before
	     * saving it!  Otherwise DRC problems may not be detected.  Also
	     * be sure to adjust labels in the cell.
	     */

	    DBAdjustLabels(SelectDef, &TiPlaneRect);
	    DBPaintPlane(SelectDef->cd_planes[PL_DRC_CHECK],
		    &SelectDef->cd_bbox,
		    DBStdPaintTbl(TT_CHECKPAINT, PL_DRC_CHECK),
		    (PaintUndoInfo *) NULL);

	    DBUpdateStamps();
	    cmdSaveCell(SelectDef, cmd->tx_argv[2], FALSE, FALSE);
	    return;
	
	/*--------------------------------------------------------------------
	 * The default case (no args):  see what's under the cursor.  Select
	 * paint if there is any, else select a cell.  In both cases,
	 * multiple clicks cycle through larger and larger selections.  The
	 * CELL option also comes here (to share initialization code) but
	 * quickly branches away.
	 *--------------------------------------------------------------------
	 */

	case DEFAULT:
	case CELL:
	    if (!(more || less)) SelectClear();
	    window = CmdGetRootPoint((Point *) NULL, &scx.scx_area);
	    if (window == NULL) return;
	    scx.scx_use = (CellUse *) window->w_surfaceID;
	    scx.scx_trans = GeoIdentityTransform;
	    crec = (DBWclientRec *) window->w_clientData;
	    DBSeeTypesAll(scx.scx_use, &scx.scx_area, crec->dbw_bitmask, &mask);
	    TTMaskAndMask(&mask, &crec->dbw_visibleLayers);
	    TTMaskAndMask(&mask, &DBAllButSpaceAndDRCBits);

	    /* See if we're pointing at the same place as we were the last time
	     * this command was invoked, and if this command immediately follows
	     * another selection comand.
	     */
	
	    if (GEO_ENCLOSE(&cmd->tx_p, &lastArea)
		    && (lastCommand+1 == TxCommandNumber))
		samePlace = TRUE;
	    else samePlace = FALSE;
	    lastArea.r_xbot = cmd->tx_p.p_x - MARGIN;
	    lastArea.r_ybot = cmd->tx_p.p_y - MARGIN;
	    lastArea.r_xtop = cmd->tx_p.p_x + MARGIN;
	    lastArea.r_ytop = cmd->tx_p.p_y + MARGIN;
	    lastCommand = TxCommandNumber;

	    /* If there's material under the cursor, select some paint.
	     * Repeated selections at the same place result in first a
	     * chunk being selected, then a region of a particular type,
	     * then a whole net.
	     */

	    if (!TTMaskIsZero(&mask) && (option != CELL))
	    {
		if (samePlace && lessCycle == less)
		{
		    level += 1;
		    if (level > NET) level = CHUNK;
		}
		else level = CHUNK;

		lessCycle = less;

		if (level == CHUNK)
		{
		    /* Pick a tile type to use for selection.  If there are
		     * several different types under the cursor, pick one of
		     * them.  This code remembers which type was used to
		     * choose last time, so that consecutive selections will
		     * use different types.
		     */

		    for (type += 1; ; type += 1)
		    {
			if (type >= DBNumTypes)
			    type = TT_SELECTBASE;
			if (TTMaskHasType(&mask, type)) break;
		    }

		    SelectChunk(&scx, type, crec->dbw_bitmask, &chunkSelection, less);
		    if (!less)
		      DBWSetBox(scx.scx_use->cu_def, &chunkSelection);
		}
		if (level == REGION)
		{
		    /* If a region has the same size as the preceding chunk,
		     * then we haven't added anything to the selection, so
		     * go on immediately and select the whole net.
		     */

		    Rect area;

		    SelectRegion(&scx, type, crec->dbw_bitmask, &area, less);
		    if (GEO_SURROUND(&chunkSelection, &area))
			level = NET;
		}
		if (level == NET)
		    SelectNet(&scx, type, crec->dbw_bitmask, (Rect *) NULL, less);
		return;
	    }

	/*--------------------------------------------------------------------
	 * We get here either if the CELL option is requested, or under
	 * the DEFAULT case where there's no paint under the mouse.  In
	 * this case, select a subcell.
	 *--------------------------------------------------------------------
	 */

	    if (cmd->tx_argc > 3) goto usageError;

	    /* If an explicit cell use id is provided, look for that cell
	     * and select it.  In this case, defeat all of the "multiple
	     * click" code.
	     */
	    
	    if (cmd->tx_argc == 3)
	    {
		SearchContext scx2;

		DBTreeFindUse(optionArgs[1], scx.scx_use, &scx2);
		use = scx2.scx_use;
		if (use == NULL)
		{
		    TxError("Couldn't find a cell use named \"%s\"\n",
			    optionArgs[1]);
		    return;
		}
		trans = scx2.scx_trans;
		p.p_x = scx2.scx_x;
		p.p_y = scx2.scx_y;
		printPath = optionArgs[1];
		samePlace = FALSE;
		lastArea.r_xbot = lastArea.r_ybot = -1000;
		lastArea.r_xtop = lastArea.r_ytop = -1000;
	    }
	    else
	    {
		/* Find the cell underneath the cursor.  If this is a
		 * second or later click at the same position, select
		 * the "next" cell underneath the point (see comments
		 * in DBSelectCell() for what "next" means).
		 */

		tpath.tp_first = tpath.tp_next = path;
		tpath.tp_last = &path[(sizeof path) - 2];
		if ((lastUse == scx.scx_use) || !samePlace || (lessCellCycle != less))
		    lastUse = NULL;
		lessCellCycle = less;
		use = DBSelectCell(scx.scx_use, lastUse, &lastIndices,
			&scx.scx_area, crec->dbw_bitmask, &trans, &p, &tpath);
    
		/* Use the window's root cell if nothing else is found. */

		if (use == NULL)
		{
		    use = lastUse = scx.scx_use;
		    p.p_x = scx.scx_use->cu_xlo;
		    p.p_y = scx.scx_use->cu_ylo;
		    trans = GeoIdentityTransform;
		    printPath = scx.scx_use->cu_id;
		}
		else
		{
		    printPath = index(path, '/');
		    if (printPath == NULL)
			printPath = path;
		    else printPath++;
		}
	    }

	    lastUse = use;
	    lastIndices = p;

	    /* The translation stuff is funny, since we got one
	     * element of the array, but not necessarily the
	     * lower-left element.  To get the transform for the
	     * array as a whole, subtract off for the indx of
	     * the element.
	     */

	    GeoInvertTrans(DBGetArrayTransform(use, p.p_x, p.p_y), &tmp1);
	    GeoTransTrans(&tmp1, &trans, &rootTrans);

	    if (less)
	      SelectRemoveCellUse(use, &rootTrans);
	    else
	      SelectCell(use, scx.scx_use->cu_def, &rootTrans, samePlace);
	    GeoTransRect(&trans, &use->cu_def->cd_bbox, &r);
	    DBWSetBox(scx.scx_use->cu_def, &r);

	    TxPrintf("Selected cell is %s (%s)\n", use->cu_def->cd_name,
		    printPath);
	    return;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSideways --
 *
 * Implement the "sideways" command.
 *
 * Usage:
 *	sideways
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection and box are flipped left-to-right, using the
 *	center of the selection as the axis for flipping.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdSideways(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Transform trans;
    Rect rootBox, bbox;
    CellDef *rootDef;

    if (cmd->tx_argc != 1)
    {
	TxError("Usage: %s\n", cmd->tx_argv[0]);
	return;
    }

    /* To flip the selection sideways, first flip it around the
     * y-axis, then move it back so its lower-left corner is in
     * the same place that it used to be.
     */
    
    GeoTransRect(&GeoSidewaysTransform, &SelectDef->cd_bbox, &bbox);
    GeoTranslateTrans(&GeoSidewaysTransform,
	SelectDef->cd_bbox.r_xbot - bbox.r_xbot,
	SelectDef->cd_bbox.r_ybot - bbox.r_ybot, &trans);

    SelectTransform(&trans);

    /* Flip the box, if it exists and is in the same window as the
     * selection.
     */
    
    if (ToolGetBox(&rootDef, &rootBox) && (rootDef == SelectRootDef))
    {
	Rect newBox;

	GeoTransRect(&trans, &rootBox, &newBox);
	DBWSetBox(rootDef, &newBox);
    }

    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSgraph
 *
 * Implement the "sgraph" command.
 *
 * Usage:
 *	sgraph [off|add|delete|debug]
 *	sgraph [show|auto] [vertical|horizontal]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the 'stretchgraph' module, if present.
 *
 * ----------------------------------------------------------------------------
 */

#ifdef	LLNL
Void (*CmdStretchCmd)() = NULL;
    /* ARGSUSED */

CmdSgraph(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (CmdStretchCmd != NULL)
    {
	(*CmdStretchCmd)(w, cmd);
    }
    else
    {
	TxError("Sorry, the sgraph command doesn't work in this version.\n");
	TxError("(Magic was not linked with stretchgraph module.)\n");
    }
}
#endif	LLNL
#ifndef NO_SIM
/*
 * ----------------------------------------------------------------------------
 *
 * CmdStartRsim
 *
 * 	This command starts Rsim under Magic, escapes Rsim, and returns
 *	back to Magic.
 *
 * Results:
 *	Rsim is forked from Magic.
 * 
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

CmdStartRsim(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static char rsimstr[] = "rsim";

    if ((cmd->tx_argc == 1) && (!SimRsimRunning)) {
	TxPrintf("usage: startrsim [options] file\n");
	return;
    }
    if ((cmd->tx_argc != 1) && (SimRsimRunning)) {
	TxPrintf("Simulator already running.  You cannont start another.\n");
	return;
    }
    if ((w == (Window *) NULL) || (w->w_client != DBWclientID)) {
	TxError("Put the cursor in a layout window.\n");
	return;
    }

    /* change argv[0] to be "rsim" and send it to Rsim_start */

    cmd->tx_argv[0] = rsimstr;
    if (cmd->tx_argc != 1) {
	cmd->tx_argv[cmd->tx_argc] = (char *) 0;
	SimStartRsim(cmd->tx_argv);
    }
    SimConnectRsim(TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdSimCmd
 *
 *	Applies the given rsim command to the currently selected nodes.
 *
 * Results:
 *	Whatever rsim replys to the commands input.
 *
 * Side effects:
 *	None.
 *
 * ---------------------------------------------------------------------------- 
 */

CmdSimCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static char cmdbuf[200];
    char 	*strptr;
    char 	*nodeCmd;
    int 	i;

    if (!SimRsimRunning) {
	TxPrintf("You must first start the simulator by using the rsim command.\n");
	return;
    }
    if (cmd->tx_argc == 1) {
	TxPrintf("usage: simcmd command [options]\n");
	return;
    }
    if ((w == (Window *) NULL) || (w->w_client != DBWclientID)) {
	TxError("Put the cursor in a layout window.\n");
	return;
    }

    /* check to see whether to apply the command to each node selected,
     * or whether to just ship the command to rsim without any node
     * names.
     */
    nodeCmd = SimGetNodeCommand( cmd->tx_argv[1] );

    strcpy( cmdbuf, (nodeCmd != NULL) ? nodeCmd : cmd->tx_argv[1] );
    strptr = cmdbuf + strlen(cmdbuf);
    *strptr++ = ' ';
    *strptr = NULL;

    for (i = 2; i <= cmd->tx_argc - 1; i++) {
	strcpy(strptr, cmd->tx_argv[i]);
	strcat(strptr, " ");
	strptr += strlen(strptr) + 1;
    }

    if (nodeCmd != NULL) {
	SimSelection(cmdbuf);
    }
    else {
	SimRsimIt(cmdbuf, "");

        while (TRUE) {
	    if (!SimGetReplyLine(&strptr)) {
		break;
	    }
	    if (!strptr) {
		break;
	    }
	    TxPrintf("%s\n", strptr);
	}
    }
}
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * CmdSnap --
 *
 * Enable/disable the box snapping to the nearest user-defined grid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

CmdSnap(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static char *names[] = { "off", "on", 0 };
    int n;

    if (cmd->tx_argc != 2) goto printit;

    n = Lookup(cmd->tx_argv[1], names);
    if (n < 0)
    {
	TxPrintf("Usage: snap on   or   snap off\n");
	return;
    }
    DBWSnapToGrid = n;

printit:
    TxPrintf("Snapping to grid is %sabled\n", DBWSnapToGrid ? "en" : "dis");
}
#ifndef NO_PLOW

/*
 * ----------------------------------------------------------------------------
 *
 * CmdStraighten --
 *
 * Straighten jogs in an area by pulling in a particular direction.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the geometry of the edit cell.
 *
 * ----------------------------------------------------------------------------
 */

CmdStraighten(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect editBox;
    int dir;

    if (w == (Window *) NULL)
    {
	TxError("Point to a window first\n");
	return;
    }

    if (cmd->tx_argc != 2)
	goto usage;

    dir = GeoNameToPos(cmd->tx_argv[1], TRUE, FALSE);
    if (dir < 0)
	goto usage;
    dir = GeoTransPos(&RootToEditTransform, dir);

    if (EditCellUse == (CellUse *) NULL)
    {
	TxError("There is no edit cell!\n");
	return;
    }
    if (!ToolGetEditBox(&editBox))
    {
	TxError("The box is not in a window over the edit cell.\n");
	return;
    }

    PlowStraighten(EditCellUse->cu_def, &editBox, dir);
    return;

usage:
    TxError("Usage: straighten manhattan-direction\n");
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdStretch --
 *
 * Implement the "stretch" command.
 *
 * Usage:
 *	stretch [direction [distance]]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves everything that's currently selected, erases material that
 *	the selection would sweep over, and fills in material behind the
 *	selection.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

Void
CmdStretch(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Transform t;
    Rect rootBox, newBox;
    CellDef *rootDef;
    int xdelta, ydelta;

    if (cmd->tx_argc > 3)
    {
	badUsage:
	TxError("Usage: %s [direction [amount]]\n", cmd->tx_argv[0]);
	return;
    }

    if (cmd->tx_argc > 1)
    {
	int indx, amount;

	indx = GeoNameToPos(cmd->tx_argv[1], TRUE, TRUE);
	if (indx < 0)
	    return;
	if (cmd->tx_argc == 3)
	{
	    if (!StrIsInt(cmd->tx_argv[2])) goto badUsage;
	    amount = atoi(cmd->tx_argv[2]);
	}
	else amount = 1;

	switch (indx)
	{
	    case GEO_NORTH:
		xdelta = 0;
		ydelta = amount;
		break;
	    case GEO_SOUTH:
		xdelta = 0;
		ydelta = -amount;
		break;
	    case GEO_EAST:
		xdelta = amount;
		ydelta = 0;
		break;
	    case GEO_WEST:
		xdelta = -amount;
		ydelta = 0;
		break;
	    default:
		ASSERT(FALSE, "Bad direction in CmdStretch");
		return;
	}
	GeoTransTranslate(xdelta, ydelta, &GeoIdentityTransform, &t);

	/* Move the box by the same amount as the selection, if the
	 * box exists.
	 */

	if (ToolGetBox(&rootDef, &rootBox) && (rootDef == SelectRootDef))
	{
	    GeoTransRect(&t, &rootBox, &newBox);
	    DBWSetBox(rootDef, &newBox);
	}
    }
    else
    {
	/* Use the displacement between the box lower-left corner and
	 * the point as the transform.  Round off to a Manhattan distance.
	 */
	
	Point rootPoint;
	Window *window;
	int absX, absY;

	if (!ToolGetBox(&rootDef, &rootBox) || (rootDef != SelectRootDef))
	{
	    TxError("\"Stretch\" uses the box lower-left corner as a place\n");
	    TxError("    to pick up the selection for stretching, but the\n");
	    TxError("    box isn't in a window containing the selection.\n");
	    return;
	}
	window = ToolGetPoint(&rootPoint, (Rect *) NULL);
	if ((window == NULL) ||
	    (EditRootDef != ((CellUse *) window->w_surfaceID)->cu_def))
	{
	    TxError("\"Stretch\" uses the point as the place to put down a\n");
	    TxError("    the selection, but the point doesn't point to the\n");
	    TxError("    edit cell.\n");
	    return;
	}
	xdelta = rootPoint.p_x - rootBox.r_xbot;
	ydelta = rootPoint.p_y - rootBox.r_ybot;
	if (xdelta < 0) absX = -xdelta;
	else absX = xdelta;
	if (ydelta < 0) absY = -ydelta;
	else absY = ydelta;
	if (absY <= absX) ydelta = 0;
	else xdelta = 0;
	GeoTransTranslate(xdelta, ydelta, &GeoIdentityTransform, &t);
	GeoTransRect(&t, &rootBox, &newBox);
	DBWSetBox(rootDef, &newBox);
    }
    
    SelectStretch(xdelta, ydelta);
}
