/*
 * CmdAB.c --
 *
 * Commands with names beginning with the letters A through B.
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
static char rcsid[] = "$Header: CmdAB.c,v 6.0 90/08/28 18:07:06 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "commands.h"
#include "utils.h"
#include "textio.h"
#include "drc.h"
#include "graphics.h"
#include "txcommands.h"
#include "malloc.h"
#include "netlist.h"


/* ---------------------------------------------------------------------------
 *
 * CmdAddPath --
 *
 * Implement the "addpath" command:  append to the global cell search path.
 *
 * Usage:
 *	addpath path
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The search path used to find cells is appended to with the first
 *	command line argument.  See CmdLQ.CmdPath for more information.
 *
 * History:
 *	Contributed by Doug Pan and Prof. Mark Linton at Stanford.
 *
 * ---------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
CmdAddPath( w, cmd )
    Window *w;
    TxCommand *cmd;
{
    int oldlength, addlength;
    char *new;

    if (cmd->tx_argc != 2) {
	TxError("Usage: %s appended_search_path\n", cmd->tx_argv[0]);
	return;
    }

    oldlength = strlen(Path);
    addlength = strlen(cmd->tx_argv[1]);
    new = mallocMagic((unsigned) (oldlength + addlength + 2));
    (void) strcpy(new, Path);
    new[oldlength] = ' ';
    (void) strcpy(new + oldlength + 1, cmd->tx_argv[1]);
    freeMagic(Path);
    Path = new;
}

/* ---------------------------------------------------------------------------
 *
 * CmdArt --
 *
 * Implement the "art" command: area router
 *
 * Usage:
 *	art
 *
 * Results:
 *	Area routed.
 *
 * Side effects:
 *
 * ---------------------------------------------------------------------------
 */

#ifdef	LLNL
#define	ART_HELP	0
#define	ART_MODEL	1
#define	ART_NETLIST	2
#define	ART_OBSTACLES	3
#define	ART_ORIENT	4
#define	ART_REGIONS	5
#define	ART_SET		6
#define	ART_SHOW	7
#define	ART_STRATEGY	8

 /*ARGSUSED*/
CmdArt(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect area;
    int	option;
    NLNetList netList;
    char *netListName;

    static char *cmdArtOption[] =
    {	
	"help			Display art command options",
	"model			Display planes in routing model",
	"netlist <file>	set netlist file",
	"obstacles <layers>	Display obstacleson layers <layer>",
	"orientation		Toggle orientation",
	"regions <layer>	Display routing regions on <layer>",
	"set			Set routing options",
	"show			Display routing options",
	"strategy		Set/Display routing strategy",
	0,
    };

    if ( cmd->tx_argc == 1 )
    {
	if (!ToolGetEditBox(&area))
	    return;
	ArtRoute(EditCellUse, &area);
	return;
    }

    option = Lookup(cmd->tx_argv[1], cmdArtOption);
    if (option == -1)
    {
	TxError("Ambiguous subcommand: \"%s\"\n", cmd->tx_argv[1]);
	option = ART_HELP;
    }
    if (option < 0)
    {
	TxError("Unrecognized subcommand: \"%s\"\n", cmd->tx_argv[1]);
	option = ART_HELP;
    }

    switch (option)
    {
	case ART_HELP:
	    TxPrintf("Area router commands have the form \"art [subcommand [parameters]]\",\n");
	    TxPrintf("where subcommand is one of:\n\n");
	    for (option=0; cmdArtOption[option]; option++)
		TxPrintf("  %s\n", cmdArtOption[option]);
	    TxPrintf("\n");
	    break;

	case ART_MODEL:
	    ArtTechDisplay();
	    break;

	case ART_NETLIST:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc == 3)
		    NMNewNetlist(cmd->tx_argv[2]);
		else
		    TxPrintf("Usage: art netlist <filename>\n");
	    }
	    TxPrintf("Current list is \"%s\"\n", NMNetlistName());
	    break;

	case ART_OBSTACLES:
	    if (!ToolGetEditBox(&area))
		return;
	    ArtDisplayObstacles(cmd->tx_argc - 1, &cmd->tx_argv[1], EditCellUse, &area);
	    break;

	case ART_ORIENT:
	    ArtToggleOrientation();
	    break;

	case ART_REGIONS:
	    if (!ToolGetEditBox(&area))
		return;
	    ArtDisplayRegions(cmd->tx_argc - 1, &cmd->tx_argv[1], EditCellUse, &area);
	    break;

	case ART_SET:
	    ArtSetOptions(cmd->tx_argc - 1, &cmd->tx_argv[1]);
	    break;

	case ART_SHOW:
	    ArtDspOptions(cmd->tx_argc - 1, &cmd->tx_argv[1]);
	    break;
	
	case ART_STRATEGY:
	    ArtStrategy(cmd->tx_argc - 1, &cmd->tx_argv[1]);
	    break;
    }
    return;

  usage:
    TxPrintf("Usage: art [subcommand [parameters]]\n");
}
#endif	LLNL

/*
 * ----------------------------------------------------------------------------
 *
 * CmdArray --
 *
 * Implement the "array" command.  Make everything in the selection
 * into an array.  For paint and labels, just copy.  For subcells,
 * make each use into an arrayed use.
 *
 * Usage:
 *	array xlo xhi ylo yhi
 *	array xsize ysize
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the edit cell.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */
CmdArray(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    ArrayInfo a;
    Rect toolRect;

    if (cmd->tx_argc != 3 && cmd->tx_argc != 5) goto badusage;
    if (!StrIsInt(cmd->tx_argv[1]) || !StrIsInt(cmd->tx_argv[2])) 
	    goto badusage;
    if (cmd->tx_argc == 3)
    {
	a.ar_xlo = 0;
	a.ar_ylo = 0;
	a.ar_xhi = atoi(cmd->tx_argv[1]) - 1;
	a.ar_yhi = atoi(cmd->tx_argv[2]) - 1;
	if ( (a.ar_xhi < 0) || (a.ar_yhi < 0) ) goto badusage;
    }
    else
    {
	if (!StrIsInt(cmd->tx_argv[3]) || 
		!StrIsInt(cmd->tx_argv[4])) goto badusage;
	a.ar_xlo = atoi(cmd->tx_argv[1]);
	a.ar_xhi = atoi(cmd->tx_argv[2]);
	a.ar_ylo = atoi(cmd->tx_argv[3]);
	a.ar_yhi = atoi(cmd->tx_argv[4]);
    }

    if (!ToolGetBox((CellDef **) NULL, &toolRect))
    {
	TxError("Position the box to indicate the array spacing.\n");
	return;
    }
    a.ar_xsep = toolRect.r_xtop - toolRect.r_xbot;
    a.ar_ysep = toolRect.r_ytop - toolRect.r_ybot;

    SelectArray(&a);
    return;

badusage:
    TxError("Usage: array xlo xhi ylo yhi or array xsize ysize\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdBox --
 *
 * Box command.
 *
 * Usage:
 *	box
 *	box left [distance]
 *	box up [distance]
 *	box right [distance]
 *	box down [distance]
 *	box width [num]
 *	box height [num]
 *	box llx lly urx ury	(in edit cell coordinates)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the location of the box tool.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

CmdBox(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static char *boxDirNames[] = { "left", "right", "up", "down", 
	"width", "height", 0 };
    static int boxDirs[] = { GEO_WEST, GEO_EAST, GEO_NORTH, GEO_SOUTH,
	GEO_SOUTHWEST, GEO_NORTHWEST};
    Rect rootBox;
    CellDef *rootBoxDef;

    if ( (cmd->tx_argc > 3) &&
	(cmd->tx_argc != 5 || 
	!StrIsInt(cmd->tx_argv[1]) || !StrIsInt(cmd->tx_argv[2]) ||
	!StrIsInt(cmd->tx_argv[3]) || !StrIsInt(cmd->tx_argv[4]) ) )
    {
	goto badusage;
    }

    if (cmd->tx_argc != 5)
    {
	if (!ToolGetBox(&rootBoxDef, &rootBox))
	{
	    TxError("Box tool must be present\n");
	    return;
	}
    }
    else if (w == NULL)
    {
	TxError("Cursor not in a window.\n");
	return;
    }
    else rootBoxDef = ((CellUse *) w->w_surfaceID)->cu_def;

    if (cmd->tx_argc < 2)
    {
	Rect editbox;
	TxPrintf("Box height: %d, width: %d\n",
	    rootBox.r_ytop-rootBox.r_ybot, rootBox.r_xtop-rootBox.r_xbot);
	if (EditRootDef == rootBoxDef)
	{
	    (void) ToolGetEditBox(&editbox);
	    TxPrintf("Edit cell coordinates: ll=(%d, %d) ur=(%d, %d)\n",
		editbox.r_xbot, editbox.r_ybot,
		editbox.r_xtop, editbox.r_ytop);
	}
	else
	{
	    TxPrintf("Root cell coordinates: ll=(%d, %d) ur=(%d, %d)\n",
		rootBox.r_xbot, rootBox.r_ybot,
		rootBox.r_xtop, rootBox.r_ytop);
	}
	return;
    }

    if (cmd->tx_argc == 5)
    {
	Rect r;
	r.r_xbot = atoi(cmd->tx_argv[1]);
	r.r_ybot = atoi(cmd->tx_argv[2]);
	r.r_xtop = atoi(cmd->tx_argv[3]);
	r.r_ytop = atoi(cmd->tx_argv[4]);
	if (EditRootDef == rootBoxDef)
	    GeoTransRect(&EditToRootTransform, &r, &rootBox);
	else rootBox = r;
    }
    else
    {
	int direction, distance;

	direction = Lookup(cmd->tx_argv[1], boxDirNames);
	if (direction < 0)
	    goto badusage;

	if (cmd->tx_argc == 3)
	{
	    if (!StrIsInt(cmd->tx_argv[2]))
	    {
		TxError("Distance or size must be an integer\n");
		return;
	    }
	    distance = atoi(cmd->tx_argv[2]);
	}
	else
	{
	    switch (boxDirs[direction])
	    {
		case GEO_WEST:
		case GEO_EAST:
		    distance = rootBox.r_xtop - rootBox.r_xbot;
		    break;
		case GEO_NORTH:
		case GEO_SOUTH:
		    distance = rootBox.r_ytop - rootBox.r_ybot;
		    break;
	    }
	}

	switch (boxDirs[direction])
	{
	    case GEO_WEST:
		rootBox.r_xbot -= distance;
		rootBox.r_xtop -= distance;
		break;
	    case GEO_EAST:
		rootBox.r_xbot += distance;
		rootBox.r_xtop += distance;
		break;
	    case GEO_SOUTH:
		rootBox.r_ybot -= distance;
		rootBox.r_ytop -= distance;
		break;
	    case GEO_NORTH:
		rootBox.r_ybot += distance;
		rootBox.r_ytop += distance;
		break;
	    case GEO_SOUTHWEST:
		if (cmd->tx_argc != 3) 
		{
		    TxPrintf("Box width: %d lambda\n",
			    rootBox.r_xtop - rootBox.r_xbot);
		    return;
		}
		rootBox.r_xtop = rootBox.r_xbot + distance;
		break;
	    case GEO_NORTHWEST:
		if (cmd->tx_argc != 3) 
		{
		    TxPrintf("Box height: %d lambda\n",
			    rootBox.r_ytop - rootBox.r_ybot);
		    return;
		}
		rootBox.r_ytop = rootBox.r_ybot + distance;
		break;
	}
    }

    ToolMoveBox(TOOL_BL, &rootBox.r_ll, FALSE, rootBoxDef);
    ToolMoveCorner(TOOL_TR, &rootBox.r_ur, FALSE, rootBoxDef);
    return;

badusage:
    TxError("Usage: box [direction [distance]]\n");
    TxError("   or: box [width | height] [size]\n");
    TxError("   or: box llx lly urx ury     %s\n",
	"(location in edit cell coordinates)");
    return;
}
