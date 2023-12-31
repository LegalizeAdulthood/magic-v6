/*
 * irTestCmd.c --
 *
 * Code to process the `*iroute' command.
 * `*iroute' is a wizard command for debugging and testing.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1987, 1990 Michael H. Arnold, Walter S. Scott, and  *
 *     * the Regents of the University of California.                      *
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
static char rcsid[] = "$Header: irTestCmd.c,v 6.1 90/08/28 19:24:04 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "tile.h"
#include "database.h"
#include "signals.h"
#include "textio.h"
#include "irouter.h"
#include "graphics.h"
#include "windows.h"
#include "dbwind.h"
#include "dbwtech.h"
#include "txcommands.h"
#include "main.h"
#include "utils.h"
#include "commands.h"
#include "styles.h"
#include "malloc.h"
#include "list.h"
#include "doubleint.h"
#include "../mzrouter/mzrouter.h"
#include "irInternal.h"

/* Subcommand table - declared here since its referenced before defined */
typedef struct
{
    char	*sC_name;	/* name of iroute subcommand */
    int    (*sC_proc)();	/* Procedure implementing this 
				       subcommand */
    char 	*sC_commentString;
    char	*sC_usage;
} TestCmdTableE;
extern TestCmdTableE irTestCommands[];


/*
 * ----------------------------------------------------------------------------
 *
 * irDebugTstCmd --
 *
 * irouter wizard command (`:*iroute') to set/clear debug flags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modify debug flags.
 *	
 * ----------------------------------------------------------------------------
 */
    /*ARGSUSED*/
Void
irDebugTstCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc > 4)
    {
	TxPrintf("Too many args on '*iroute debug'\n");
	return;
    }
    else if (cmd->tx_argc == 4)
    {
	/* two args, set or clear first arg according to second */
	int value = -1;

	SetNoisyBool(&value,cmd->tx_argv[3], (FILE *) NULL);
	if(value==FALSE || value==TRUE)
	{
	    TxPrintf("\n");
	    DebugSet(irDebugID,1,&(cmd->tx_argv[2]),(bool) value);
	}
    }
    else
    {
	/* list current values of flags */
	DebugShow(irDebugID);
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * irHelpTstCmd --
 *
 * irouter wizard command (`:*iroute') to print help info on irouter wizard
 * commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *	
 * ----------------------------------------------------------------------------
 */
    /*ARGSUSED*/
Void
irHelpTstCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int n;
    int which;

    if(cmd->tx_argc == 2)
    {
	/* No arg, so print summary of commands */
	for(n=0; irTestCommands[n].sC_name!=NULL; n++)
	{
	    TxPrintf("*iroute %s - %s\n",
		    irTestCommands[n].sC_name,
		    irTestCommands[n].sC_commentString);
	}
	TxPrintf("\n*iroute help [subcmd] - ");
	TxPrintf("Print usage info for subcommand.\n");
    }
    else
    {
	/* Lookup subcommand in table, and printed associated help info */
	which = LookupStruct(
	    cmd->tx_argv[2], 
	    (char **) irTestCommands, 
	    sizeof irTestCommands[0]);

        /* Process result of lookup */
	if (which >= 0)
	{
	    /* subcommand found - print out its comment string and usage */
	    TxPrintf("*iroute %s - %s\n",
		    irTestCommands[which].sC_name,
		    irTestCommands[which].sC_commentString);
	    TxPrintf("Usage:  *iroute %s\n",
		    irTestCommands[which].sC_usage);
	}
	else if (which == -1)
	{
	    /* ambiguous subcommand - complain */
	    TxError("Ambiguous *iroute subcommand: \"%s\"\n", cmd->tx_argv[2]);
	}
	else
	{
	    /* unrecognized subcommand - complain */
	    TxError("Unrecognized iroute subcommand: \"%s\"\n", 
		    cmd->tx_argv[2]);
	    TxError("Valid *iroute subcommands are:  ");
	    for (n = 0; irTestCommands[n].sC_name; n++)
		TxError(" %s", irTestCommands[n].sC_name);
	    TxError("\n");
	}
    }

    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * irParmsTstCmd --
 *
 * irouter wizard command (`:*iroute') to dump parms.
 * commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dump routelayers and routecontacts.
 *	
 * ----------------------------------------------------------------------------
 */
    /*ARGSUSED*/
Void
irParmsTstCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{

    MZPrintRLs(irRouteLayers);
    TxMore("");
    MZPrintRCs(irRouteContacts);

    return;
}

/*--------------------------- Command Table ------------------------------ */
TestCmdTableE irTestCommands[] = {
    "debug",	irDebugTstCmd,
    "set or clear debug flags",
    "debug [flag] [value]",

    "help",	irHelpTstCmd,
    "summarize *iroute subcommands",
    "help [subcommand]",

    "parms",	irParmsTstCmd,
    "print internal data structures",
    "parms",

	0
    }, *testCmdP;


/*
 * ----------------------------------------------------------------------------
 *
 * IRTest --
 *
 * Command interface for testing the interactive router.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command; see below.
 *
 * Organization:
 *	We select a procedure based on the first keyword (argv[0])
 *	and call it to do the work of implementing the rule.  Each
 *	such procedure is of the following form:
 *
 *	Void
 *	proc(argc, argv)
 *	    int argc;
 *	    char *argv[];
 *	{
 *	}
 *
 * ----------------------------------------------------------------------------
 */
Void
IRTest(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int n;
    int which;


    if(cmd->tx_argc == 1)
    {
	/* No subcommand specified.  */
	TxPrintf("Must specify subcommand.");
	TxPrintf("  (type '*iroute help' for summary)\n");
    }
    else
    {
	/* Lookup subcommand in table */
	which = LookupStruct(
	    cmd->tx_argv[1], 
	    (char **) irTestCommands, 
	    sizeof irTestCommands[0]);

        /* Process result of lookup */
	if (which >= 0)
	{
	    /* subcommand found - call proc that implements it */
	    testCmdP = &irTestCommands[which];
	    (*testCmdP->sC_proc)(w,cmd);
	}
	else if (which == -1)
	{
	    /* ambiguous subcommand - complain */
	    TxError("Ambiguous subcommand: \"%s\"\n", cmd->tx_argv[1]);
	}
	else
	{
	    /* unrecognized subcommand - complain */
	    TxError("Unrecognized subcommand: \"%s\"\n", cmd->tx_argv[1]);
	    TxError("Valid subcommands:");
	    for (n = 0; irTestCommands[n].sC_name; n++)
		TxError(" %s", irTestCommands[n].sC_name);
	    TxError("\n");
	}
    }

    return;
}
