/*
 * SWCmds.c --
 *
 * Command to build scan windows.
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
static char rcsid[] = "$Header: SWCmds.c,v 6.0 90/08/28 18:07:31 mayo Exp $";
#endif  not lint

#ifdef	LLNL
#include <stdio.h>
#include <ctype.h>
#include "magic.h"
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
#include "txcommands.h"

extern int SWHalo;
extern int SWMinSpace;
extern bool SWVerbose;

/*
 * ----------------------------------------------------------------------------
 *
 * CmdMakeSW --
 *
 * Implement the "makesw" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May create a scanwindow (.sw) file or output to one.
 *
 * Format:
 *	See the file sw.c for a description of the format
 *	of a scanwindow file.
 *
 * ----------------------------------------------------------------------------
 */

#define	SWCREATE	0
#define	SWHALO		1
#define	SWHELP		2
#define	SWMAPWINDOW	3
#define	SWMAPSPACE	4
#define	SWMINSPACE	5
#define	SWVERBOSE	6
#define	SWWINDOW	7
#define	SWWINSPACE	8

    /* ARGSUSED */

Void
CmdMakeSW(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static bool isInitialized = FALSE;
    int option, prevErrs, numErrs, n;
    char **msg, *layers;
    Rect r;
    static char *cmdSWOption[] =
    {	
	"create file		create the output file \"file\"",
	"halo [n]		yank n units around each scan window",
	"help			print this message",
	"mapwindow file mapfile [layers]\n\
			Append all scan windows defined by mapfile to\n\
			the file \"file\"",
	"mapspace file mapfile [layers]\n\
			Output space tiles for above",
	"minspace [n]		merge space tiles smaller than n units tall\n\
			or wide with their neighbors",
	"verbose [on|off]	enable/disable verbose output",
	"window file windowname xlo ylo xhi yhi [layers]\n\
			Append the scan window \"windowname\" defined by xlo,\n\
			etc. to the file \"file\"",
	"winspace file windowname xlo ylo xhi yhi [layers]\n\
			Output space tiles for above",
	NULL
    };
    static struct
    {
	char	*cmd_str;
	bool	 cmd_value;
    }
    cmdSWVerbose[] =
    {
	"disable",	FALSE,
	"enable",	TRUE,
	"false",	FALSE,
	"no",		FALSE,
	"off",		FALSE,
	"on",		TRUE,
	"true",		TRUE,
	"yes",		TRUE,
	NULL
    };

    if (w == (Window *) NULL)
    {
	TxError("Point to a window first\n");
	return;
    }

    if (!isInitialized) SWInit(), isInitialized = TRUE;
    if (cmd->tx_argc == 1)
    {
wrongnumargs:
	TxError("Wrong number of arguments in \"makesw\" command.");
	TxError("  Try \"makesw help\" for help.\n");
	return;
    }

    option = Lookup(cmd->tx_argv[1], cmdSWOption);
    if (option < 0)
    {
	TxError("\"%s\" isn't a valid makesw option.", cmd->tx_argv[1]);
	TxError("  Try \"makesw help\" for help.\n");
	return;
    }

    prevErrs = DBWFeedbackCount;
    switch (option)
    {
	case SWHALO:
	    if (cmd->tx_argc == 2)
	    {
		TxPrintf("Current halo is %d units\n", SWHalo);
		break;
	    }
	    else if (cmd->tx_argc != 3) goto wrongnumargs;
	    if (!StrIsInt(cmd->tx_argv[2]))
	    {
		TxError("Halo must be numeric.\n");
		TxError("  Try \"makesw help\" for help.\n");
		break;
	    }
	    SWHalo = atoi(cmd->tx_argv[2]);
	    TxPrintf("Halo now set to %d units\n", SWHalo);
	    break;

	case SWMINSPACE:
	    if (cmd->tx_argc == 2)
	    {
		if (SWMinSpace == 0)
		    TxPrintf("No minimum space tile size\n");
		else
		    TxPrintf("Minimum space tile size is %d units\n",
				SWMinSpace);
		break;
	    }
	    else if (cmd->tx_argc != 3) goto wrongnumargs;
	    if (!StrIsInt(cmd->tx_argv[2]))
	    {
		TxError("Minimum space tile size must be numeric.\n");
		TxError("  Try \"makesw help\" for help.\n");
		break;
	    }
	    SWMinSpace = atoi(cmd->tx_argv[2]);
	    TxPrintf("Minimum space tile size now set to %d units\n",
		    SWMinSpace);
	    break;

	case SWHELP:
	    TxPrintf("Makesw commands have the form \"makesw option\",\n");
	    TxPrintf("where option is one of:\n");
	    for (msg = &(cmdSWOption[0]); *msg != NULL; msg++)
	    {
		if (**msg == '*') continue;
		TxPrintf("  %s\n", *msg);
	    }
	    TxPrintf("If [layers] are specified, they should be CIF layers\n");
	    TxPrintf("in the current output style (\"cif ostyle\")\n");
	    TxPrintf("If [layers] are not specified, they default to all.\n");
	    break;

	case SWCREATE:
	    SWCreate( (CellUse *) w->w_surfaceID, cmd->tx_argv[2]);
	    break;
	case SWWINDOW:
	case SWWINSPACE:
	    if (cmd->tx_argc < 8 || cmd->tx_argc > 9) goto wrongnumargs;
	    layers = (char *) NULL;
	    if (cmd->tx_argc == 9) layers = cmd->tx_argv[8];
	    r.r_xbot = atoi(cmd->tx_argv[4]);
	    r.r_ybot = atoi(cmd->tx_argv[5]);
	    r.r_xtop = atoi(cmd->tx_argv[6]);
	    r.r_ytop = atoi(cmd->tx_argv[7]);
	    SWWindow((CellUse *) w->w_surfaceID, cmd->tx_argv[2],
		cmd->tx_argv[3], &r, layers, option == SWWINSPACE);
	    break;
	case SWMAPWINDOW:
	case SWMAPSPACE:
	    if (cmd->tx_argc < 4 || cmd->tx_argc > 5) goto wrongnumargs;
	    layers = (char *) NULL;
	    if (cmd->tx_argc == 5) layers = cmd->tx_argv[4];
	    SWMapWind( (CellUse *) w->w_surfaceID, cmd->tx_argv[2],
			cmd->tx_argv[3], layers, option == SWMAPSPACE);
	    break;
	case SWVERBOSE:
	    if (cmd->tx_argc == 2)
	    {
		TxPrintf("Verbose output %s\n",
			SWVerbose ? "enabled" : "disabled");
		break;
	    }
	    if (cmd->tx_argc > 3) goto wrongnumargs;
	    n = LookupStruct(cmd->tx_argv[2], cmdSWVerbose,
			sizeof cmdSWVerbose[0]);
	    if (n < 0)
	    {
		TxError("Usage: makesw verbose [on|off]\n");
		break;
	    }
	    SWVerbose = cmdSWVerbose[n].cmd_value;
	    break;
    }

    numErrs = DBWFeedbackCount - prevErrs;
    if (numErrs == 1)
	TxPrintf("There was one error; see feedback.\n");
    else if (numErrs > 1)
	TxPrintf("There were %d errors; see feedback.\n", numErrs);
}
#endif	LLNL
