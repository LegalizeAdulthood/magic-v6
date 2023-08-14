/* windSend.c -
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
static char rcsid[]="$Header: windSend.c,v 6.0 90/08/28 19:02:26 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "glyphs.h"
#include "windInt.h"
#include "stack.h"
#include "utils.h"
#include "signals.h"
#include "txcommands.h"

clientRec *windClient = NULL;
clientRec *defaultClient = NULL;

bool windPrintCommands = FALSE;	/* debugging flag */

global TxCommand *WindCurrentCmd; /* The current command.
				   */
global Window *WindCurrentWindow; /* The window at which the current command
				   * was invoked.
				   */

global int WindOldButtons;	/* The buttons for the last command */
global int WindNewButtons;	/* The buttons this time */

static WindClient windGrabber =  (WindClient) NULL;
				/* If this variable is non-null then send
				 * all commands to it
				 */

Stack *windGrabberStack = NULL;


/*
 * ----------------------------------------------------------------------------
 * WindSendCommand --
 *
 *	Send a command to a window to be executed.  If the window passed is
 *	NULL then whatever window is at the point given in the command is
 *	used.
 *
 * Results:
 *	TRUE if the command was able to be processed.
 *
 * Side effects:
 *	Whatever the window wishes to do with the command.
 * ----------------------------------------------------------------------------
 */

bool
WindSendCommand(requestedClient, cmd)
    WindClient requestedClient;	 /* If non-NULL the command will be 
				  * sent here 
				  */
    TxCommand *cmd;	/* A pointer to a command */
{
    Window *w;
    int windCmdNum, clientCmdNum;
    clientRec *rc;
    bool inside;	/* tells us if we are inside of a window */

    if (defaultClient == (clientRec *) NULL)
	defaultClient = (clientRec *) WindGetClient(DEFAULT_CLIENT);
    if (windClient == (clientRec *) NULL)
	windClient = (clientRec *) WindGetClient(WINDOW_CLIENT);

    /* ignore no-op commands */
    if ( (cmd->tx_button == TX_NO_BUTTON) && (cmd->tx_argc == 0) )
    {
	return TRUE;
    }

    w = NULL;
    inside = FALSE;
    ASSERT( (cmd->tx_button == TX_NO_BUTTON) || (cmd->tx_argc == 0), 
	"WindSendCommand");

    WindOldButtons = WindNewButtons;
    if (cmd->tx_button == TX_NO_BUTTON)
    {
	windCmdNum = Lookup(cmd->tx_argv[0], windClient->w_commandTable);
    }
    else
    {
	if (cmd->tx_buttonAction == TX_BUTTON_DOWN)
	    WindNewButtons |= cmd->tx_button;
	else
	    WindNewButtons &= ~(cmd->tx_button);
    }

    if (requestedClient == (WindClient) NULL)
    {
	if (cmd->tx_wid == WIND_UNKNOWN_WINDOW)
	{
	    w = windSearchPoint( &(cmd->tx_p), &inside);
	    if (w != NULL) cmd->tx_wid = w->w_wid;
	} else if (cmd->tx_wid >= 0) {
	    w = WindSearchWid(cmd->tx_wid);
	    if (w != NULL) 
		inside = GEO_ENCLOSE(&cmd->tx_p, &w->w_screenArea);
	}

	if (w != (Window *) NULL) 
	{
	    if (inside && ((w->w_flags & WIND_ISICONIC) == 0))
		rc = (clientRec *) w->w_client;
	    else
		rc = windClient;
	}
	else
	    rc = (clientRec *) WindGetClient(DEFAULT_CLIENT);
    }
    else
	rc = (clientRec *) requestedClient;

    if (windGrabber != NULL)
    {
	/* this client wants to hog all commands */
	rc = (clientRec *) windGrabber;
    }

    /* At this point, the command is all set up and ready to send to
     * the client.
     */
    ASSERT(rc != (clientRec *) NULL, "WindSendCommand");
    if (windPrintCommands)
    {
	TxPrintf("Sending command:\n");
	windPrintCommand(cmd);
    }
    WindCurrentCmd = cmd;
    WindCurrentWindow = w;

    if (cmd->tx_button == TX_NO_BUTTON) 
    {
	clientCmdNum = Lookup(cmd->tx_argv[0], rc->w_commandTable);

	if ( (clientCmdNum == -1) || (windCmdNum == -1) )
	{
	    TxError("That command abbreviation is ambiguous.\n");
	    return FALSE;
	}
	if ( (windCmdNum == -2) && (clientCmdNum == -2) )
	{
	    /* Not a valid command.  Help the user out by telling him
	     * what might be wrong.
	     */
	    char clue[100];
	    if (WindNewButtons != 0) 
	    {
		sprintf(clue, 
		    "'%s' window is waiting for a button to be released.",
		    rc->w_clientName);
	    }
	    else if (windGrabber != NULL)
	    {
		sprintf(clue, 
		    "'%s' window is grabbing all input.", rc->w_clientName);
	    }
	    else
		sprintf(clue, "Did you point to the correct window?");
	    TxError("Unknown command.  (%s)\n", clue);
	    return FALSE;
	}

	/* intercept 'help' */
	if ((windCmdNum >= 0) &&  
		(strncmp(windClient->w_commandTable[windCmdNum], 
		"help", 4) == 0) )
	{
	    TxUseMore();
	    windHelp(cmd, "Global", windClient->w_commandTable);
	    if (rc != windClient)
		windHelp(cmd, rc->w_clientName, rc->w_commandTable);
	    TxStopMore();
	    return TRUE;
	}

	/* only do the command once, silence the client */
	if (rc == windClient) clientCmdNum = -2;

	if ((windCmdNum < 0) && (clientCmdNum >= 0))
	    (*(rc->w_command))(w, cmd);
	else if ((windCmdNum >= 0) && (clientCmdNum < 0))
	    (*(windClient->w_command))(w, cmd);
	else if ((windCmdNum >= 0) && (clientCmdNum >= 0))
	{
	    char *(ownTable[3]);
	    int ownCmdNum;

	    ownTable[0] = windClient->w_commandTable[windCmdNum];
	    ownTable[1] = rc->w_commandTable[clientCmdNum];
	    ownTable[2] = NULL;
	    ownCmdNum = Lookup(cmd->tx_argv[0], ownTable);
	    ASSERT(ownCmdNum != -2, "WindSendCommand");
	    if (ownCmdNum == -1)
	    {
		TxError("That command abbreviation is ambiguous\n");
		return FALSE;
	    }
	    if (ownCmdNum == 0)
		(*(windClient->w_command))(w, cmd);
	    else
		(*(rc->w_command))(w, cmd);
	}
    }
    else
    {
	/* A button has been pushed.
	 * If there were no buttons pressed on the last command
	 * now there are, and direct all future button pushes to this
	 * client until all buttons are up again.
	 */
        if (WindOldButtons == 0) WindGrabInput((WindClient) rc);
	else if (WindNewButtons == 0) WindReleaseInput((WindClient) rc);

	(*(rc->w_command))(w, cmd);
    }

    /* A client may modify WindNewButtons & WindOldButtons in rare cases,
     * so we better check again.
     */
    if ((WindNewButtons == 0) && (windGrabber != NULL))
	WindReleaseInput((WindClient) rc);

    return TRUE;
}




/*
 * ----------------------------------------------------------------------------
 * WindGrabInput --
 *
 *	Grab all input -- that is, send all further commands to the
 *	specified client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	pushes old grabber onto a stack.
 * ----------------------------------------------------------------------------
 */

WindGrabInput(client)
    WindClient client;
{
    ASSERT( client != NULL, "WindGrabInput");
    StackPush( (ClientData) windGrabber, windGrabberStack);
    windGrabber = client;
}



/*
 * ----------------------------------------------------------------------------
 * WindReleaseInput --
 *
 *	Stop grabbing the input (the inverse of WindGrabInput).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The previous grabber (if any) is restored.
 * ----------------------------------------------------------------------------
 */

WindReleaseInput(client)
    WindClient client;
{
      ASSERT( client == windGrabber, "WindReleaseInput");
      windGrabber = (WindClient) StackPop(windGrabberStack);
}


/*
 * ----------------------------------------------------------------------------
 * windHelp --
 *
 *	Print out help information for a client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windHelp(cmd, name, table)
    TxCommand *cmd;		/* Information about command options. */
    char *name;			/* Name of client for whom help is being
				 * printed.
				 */
    char *table[];		/* Client's command table. */
{
    static char *capName = NULL;
    static char patString[200], *pattern;
    bool wiz;
    char **tp;
#define WIZARD_CHAR	'*'

    if (cmd->tx_argc > 2)
    {
	TxError("Usage:  help [pattern]\n");
	return;
    }

    if (SigInterruptPending) return;
    (void) StrDup(&capName, name);
    if (islower(capName[0])) capName[0] += 'A' - 'a';

    TxPrintf("\n");
    if ((cmd->tx_argc == 2) && strcmp(cmd->tx_argv[1], "wizard") == 0)
    {
	pattern = "*";
	wiz = TRUE;
	TxPrintf("Wizard %s Commands\n", capName);
	TxPrintf("----------------------\n");
    }
    else 
    {
	if (cmd->tx_argc == 2)
	{
	    pattern = patString;
	    (void) sprintf(patString, "*%.195s*", cmd->tx_argv[1]);
	}
	else
	    pattern = "*";
	wiz = FALSE;
	TxPrintf("%s Commands\n", capName);
	TxPrintf("---------------\n");
    }
    for (tp = table; *tp != (char *) NULL; tp++)
    {
	if (SigInterruptPending) return;
	if (Match(pattern, *tp) && (wiz ^ (**tp != WIZARD_CHAR)) )
	    TxPrintf("%s\n", *tp);
    }
}
