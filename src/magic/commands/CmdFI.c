/*
 * CmdFI.c --
 *
 * Commands with names beginning with the letters F through I.
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
static char rcsid[] = "$Header: CmdFI.c,v 6.0 90/08/28 18:07:14 mayo Exp $";
#endif  not lint

#include <stdio.h>
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
#include "macros.h"
#include "drc.h"
#include "txcommands.h"
#include "styles.h"
#include "extract.h"
#include "malloc.h"
#include "select.h"
#include "sim.h"
#include "gcr.h"

/* The following structure is used by CmdFill to keep track of
 * areas to be filled.
 */

struct cmdFillArea
{
    Rect cfa_area;			/* Area to fill. */
    TileType cfa_type;			/* Type of material. */
    struct cmdFillArea *cfa_next;	/* Next in list of areas to fill. */
};

/*
 * ----------------------------------------------------------------------------
 *
 * CmdFeedback --
 *
 * 	Implement the "feedback" command, which provides facilities
 *	for querying and manipulating feedback information provided
 *	by other commands when they have troubles or want to highlight
 *	certain things.
 *
 * Usage:
 *	feedback option [additional_args]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the option.
 *
 * ----------------------------------------------------------------------------
 */

#undef	CLEAR
#define ADD		0
#define CLEAR		1
#define COUNT		2
#define FIND		3
#define FEED_HELP	4
#define SAVE		5
#define WHY		6

	/* ARGSUSED */

Void
CmdFeedback(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    static char *cmdFeedbackOptions[] =
    {
	"add text [style]		create new feedback area over box",
	"clear				clear all feedback info",
	"count				count # feedback entries",
	"find [nth]			put box over next [or nth] entry",
	"help				print this message",
	"save file			save feedback areas in file",
	"why				print all feedback messages under box",
	NULL
    };
    static char *cmdFeedbackStyleNames[] =
    {
	"dotted", "medium", "outline", "pale", "solid", NULL
    };
    static int cmdFeedbackStyles[] =
    {
	STYLE_DOTTEDHIGHLIGHTS, STYLE_MEDIUMHIGHLIGHTS,
	STYLE_OUTLINEHIGHLIGHTS, STYLE_PALEHIGHLIGHTS,
	STYLE_SOLIDHIGHLIGHTS, -1
    };
    static int nth = 0;			/* Last entry displayed in
					 * "feedback find".
					 */
    int option, i, style;
    Rect box;
    char *text, **msg;
    CellDef *rootDef;
    HashTable table;
    HashEntry *h;
    FILE *f;

    if (cmd->tx_argc < 2)
    {
	badusage:
	TxPrintf("Wrong number of arguments for \"feedback\" command.\n");
	TxPrintf("Type \":feedback help\" for help.\n");
	return;
    }
    option = Lookup(cmd->tx_argv[1], cmdFeedbackOptions);
    if (option < 0)
    {
	TxError("%s isn't a valid feedback option.  Try one of:\n",
	    cmd->tx_argv[1]);
	TxError("    add        find\n");
	TxError("    clear      help\n");
	TxError("    count      save\n");
	TxError("    save\n");
	return;
    }
    switch (option)
    {
	case ADD:
	    if (cmd->tx_argc == 3) style = STYLE_PALEHIGHLIGHTS;
	    else
	    {
		if (cmd->tx_argc != 4) goto badusage;
		i = Lookup(cmd->tx_argv[3], cmdFeedbackStyleNames);
		if (i < 0)
		{
		    TxError("%s isn't a valid display style.  Try one of:\n",
			cmd->tx_argv[3]);
		    TxError("    dotted        pale\n");
		    TxError("    medium        solid\n");
		    TxError("    outline\n");
		    break;
		}
		style = cmdFeedbackStyles[i];
	    }
	    w = ToolGetBoxWindow(&box, (int *) NULL);
	    if (w == NULL) return;
	    rootDef = ((CellUse *) w->w_surfaceID)->cu_def;
	    DBWFeedbackAdd(&box, cmd->tx_argv[2], rootDef, 1, style);
	    break;
	
	case CLEAR:
	    if (cmd->tx_argc != 2) goto badusage;
	    DBWFeedbackClear();
	    nth = 0;
	    break;
	
	case COUNT:
	    if (cmd->tx_argc != 2) goto badusage;
	    TxPrintf("There are %d feedback areas.\n", DBWFeedbackCount);
	    break;
	
	case FIND:
	    if (cmd->tx_argc > 3) goto badusage;
	    if (DBWFeedbackCount == 0)
	    {
		TxPrintf("There are no feedback areas right now.\n");
		break;
	    }
	    if (cmd->tx_argc == 3)
	    {
		nth = atoi(cmd->tx_argv[2]);
		if ((nth > DBWFeedbackCount) || (nth <= 0))
		{
		    TxError("Sorry, but only feedback areas 1-%d exist.\n",
			DBWFeedbackCount);
		    nth = 1;
		}
	    }
	    else
	    {
		nth += 1;
		if (nth > DBWFeedbackCount) nth = 1;
	    }
	    text = DBWFeedbackNth(nth-1, &box, &rootDef, (int *) NULL);
	    ToolMoveBox(TOOL_BL, &box.r_ll, FALSE, rootDef);
	    ToolMoveCorner(TOOL_TR, &box.r_ur, FALSE, rootDef);
	    TxPrintf("Feedback #%d: %s\n", nth, text);
	    break;
	
	case FEED_HELP:
	    if (cmd->tx_argc > 2) goto badusage;
	    TxPrintf("Feedback commands have the form \"feedback option\",\n");
	    TxPrintf("where option is one of:\n");
	    for (msg = cmdFeedbackOptions; *msg != NULL; msg++)
		TxPrintf("%s\n", *msg);
	    break;
	
	case SAVE:
	    if (cmd->tx_argc != 3) goto badusage;
	    f = PaOpen(cmd->tx_argv[2], "w", (char *) NULL, ".",
	        (char *) NULL, (char **) NULL);
	    if (f == NULL)
	    {
		TxError("Can't open file %s.\n", cmd->tx_argv[2]);
		break;
	    }
	    for (i = 0; i < DBWFeedbackCount; i++)
	    {
		int j, style;
		text = DBWFeedbackNth(i, &box, (CellDef **) NULL, &style);
		(void) fprintf(f, "box %d %d %d %d\n", box.r_xbot, box.r_ybot,
		    box.r_xtop, box.r_ytop);
		(void) fprintf(f, "feedback add \"");

		/* Be careful to backslash any quotes in the text! */

		for ( ; *text != 0; text += 1)
		{
		    if (*text == '"') fputc('\\', f);
		    fputc(*text, f);
		}
		fputc('"', f);
		for (j = 0; cmdFeedbackStyles[j] >= 0; j++)
		{
		    if (cmdFeedbackStyles[j] == style)
		    {
			(void) fprintf(f, " %s", cmdFeedbackStyleNames[j]);
			break;
		    }
		}
		(void) fprintf(f, "\n");
	    }
	    (void) fclose(f);
	    break;
	
	case WHY:
	    if (cmd->tx_argc > 2) goto badusage;
	    w = ToolGetBoxWindow(&box, (int *) NULL);
	    if (w == NULL) return;
	    rootDef = ((CellUse *) w->w_surfaceID)->cu_def;
	    HashInit(&table, 16, 0);
	    for (i=0; i<DBWFeedbackCount; i++)
	    {
		Rect area;
		CellDef *fbRootDef;

		text = DBWFeedbackNth(i, &area, &fbRootDef, (int *) NULL);
		if (rootDef != fbRootDef) continue;
		if (!GEO_OVERLAP(&box, &area)) continue;
		h = HashFind(&table, text);
		if (HashGetValue(h) == 0) TxPrintf("%s\n", text);
		HashSetValue(h, 1);
	    }
	    HashKill(&table);
	    break;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdFill --
 *
 * Implement the "fill" command.  Find all paint touching one side
 * of the box, and paint it across to the other side of the box.  Can
 * operate in any of four directions.
 *
 * Usage: 
 *	fill direction [layers]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the edit cell definition.
 *
 * ----------------------------------------------------------------------------
 */

/* Data passed between CmdFill and cmdFillFunc: */

int cmdFillDir;				/* Direction in which to fill. */
Rect cmdFillRootBox;			/* Root coords of box. */
struct cmdFillArea *cmdFillList;	/* List of areas to fill. */

Void
CmdFill(w, cmd)
    Window *w;		/* Window in which command was invoked. */
    TxCommand *cmd;	/* Describes the command that was invoked. */
{
    TileTypeBitMask maskBits;
    Rect editBox;
    SearchContext scx;
    extern int cmdFillFunc();

    if (cmd->tx_argc < 2 || cmd->tx_argc > 3)
    {
	TxError("Usage: %s direction [layers]\n", cmd->tx_argv[0]);
	return;
    }

    if ( w == (Window *) NULL )
    {
	TxError("Point to a window\n");
	return;
    }

    /* Find and check validity of position argument. */

    cmdFillDir = GeoNameToPos(cmd->tx_argv[1], TRUE, TRUE);
    if (cmdFillDir < 0)
	return;

    /* Figure out which layers to fill. */

    if (cmd->tx_argc < 3)
	maskBits = DBAllButSpaceAndDRCBits;
    else
    {
	if (!CmdParseLayers(cmd->tx_argv[2], &maskBits))
	    return;
    }

    /* Figure out which material to search for and invoke a search
     * procedure to find it.
     */

    if (!ToolGetEditBox(&editBox)) return;
    GeoTransRect(&EditToRootTransform, &editBox, &cmdFillRootBox);
    scx.scx_area = cmdFillRootBox;
    switch (cmdFillDir)
    {
	case GEO_NORTH:
	    scx.scx_area.r_ytop = scx.scx_area.r_ybot + 1;
	    scx.scx_area.r_ybot -= 1;
	    break;
	case GEO_SOUTH:
	    scx.scx_area.r_ybot = scx.scx_area.r_ytop - 1;
	    scx.scx_area.r_ytop += 1;
	    break;
	case GEO_EAST:
	    scx.scx_area.r_xtop = scx.scx_area.r_xbot + 1;
	    scx.scx_area.r_xbot -= 1;
	    break;
	case GEO_WEST:
	    scx.scx_area.r_xbot = scx.scx_area.r_xtop - 1;
	    scx.scx_area.r_xtop += 1;
	    break;
    }
    scx.scx_use = (CellUse *) w->w_surfaceID;
    scx.scx_trans = GeoIdentityTransform;
    cmdFillList = (struct cmdFillArea *) NULL;

    (void) DBTreeSrTiles(&scx, &maskBits,
	    ((DBWclientRec *) w->w_clientData)->dbw_bitmask,
	    cmdFillFunc, (ClientData) NULL);

    /* Now that we've got all the material, scan over the list
     * painting the material and freeing up the entries on the list.
     */
    while (cmdFillList != NULL)
    {
	DBPaint(EditCellUse->cu_def, &cmdFillList->cfa_area,
		cmdFillList->cfa_type);
	freeMagic((char *) cmdFillList);
	cmdFillList = cmdFillList->cfa_next;
    }

    SelectClear();
    DBAdjustLabels(EditCellUse->cu_def, &editBox);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKPAINT, &editBox);
    DBWAreaChanged(EditCellUse->cu_def, &editBox, DBW_ALLWINDOWS, &maskBits);
    DBReComputeBbox(EditCellUse->cu_def);
}

/* Important note:  these procedures can't paint the tiles directly,
 * because a search is in progress over the same planes and if we
 * paint here it may mess up the search.  Instead, the procedures
 * save areas on a list.  The list is post-processed to paint the
 * areas once the search is finished.
 */

int
cmdFillFunc(tile, cxp)
    Tile *tile;			/* Tile to fill with. */
    TreeContext *cxp;		/* Describes state of search. */
{
    Rect r1, r2;
    struct cmdFillArea *cfa;

    TiToRect(tile, &r1);
    GeoTransRect(&cxp->tc_scx->scx_trans, &r1, &r2);
    GeoClip(&r2, &cmdFillRootBox);
    switch (cmdFillDir)
    {
	case GEO_NORTH:
	    r2.r_ytop = cmdFillRootBox.r_ytop;
	    break;
	case GEO_SOUTH:
	    r2.r_ybot = cmdFillRootBox.r_ybot;
	    break;
	case GEO_EAST:
	    r2.r_xtop = cmdFillRootBox.r_xtop;
	    break;
	case GEO_WEST:
	    r2.r_xbot = cmdFillRootBox.r_xbot;
	    break;
    }
    GeoTransRect(&RootToEditTransform, &r2, &r1);
    cfa = (struct cmdFillArea *) mallocMagic(sizeof(struct cmdFillArea));
    cfa->cfa_area = r1;
    cfa->cfa_type = TiGetType(tile);
    cfa->cfa_next = cmdFillList;
    cmdFillList = cfa;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdFindBox --
 *
 * Center the display on a corner of the box.  If 'zoom', then make the box
 * fill the window.
 *
 * Usage:
 *	findbox [zoom]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window underneath the cursor is moved.
 *
 * ----------------------------------------------------------------------------
 */

Void
CmdFindBox(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    CellDef *boxDef;
    Rect box;

    if (w == NULL)
    {
	TxError("Point to a window first.\n");
	return;
    };

    if (!ToolGetBox(&boxDef, &box))
    {
	TxError("Put the box in a window first.\n");
	return;
    };

    if (boxDef != (((CellUse *) w->w_surfaceID)->cu_def))
    {
	TxError("The box is not in the same coordinate %s", 
		"system as the window.\n");
	return;
    };

    if (cmd->tx_argc == 1) 
    {
	/* center view on box */
	Point rootPoint;
	Rect newArea, oldArea;

	rootPoint.p_x = (box.r_xbot + box.r_xtop)/2;
	rootPoint.p_y = (box.r_ybot + box.r_ytop)/2;

	oldArea = w->w_surfaceArea;
	newArea.r_xbot = rootPoint.p_x - (oldArea.r_xtop - oldArea.r_xbot)/2;
	newArea.r_xtop = newArea.r_xbot - oldArea.r_xbot + oldArea.r_xtop;
	newArea.r_ybot = rootPoint.p_y - (oldArea.r_ytop - oldArea.r_ybot)/2;
	newArea.r_ytop = newArea.r_ybot - oldArea.r_ybot + oldArea.r_ytop;

	WindMove(w, &newArea);
	return;
    }
    else if (cmd->tx_argc == 2)
    {
	int expand;

	/* zoom in to box */

	if (strcmp(cmd->tx_argv[1], "zoom") != 0) goto usage;

	/* Allow a 5% ring around the box on each side. */

	expand = (box.r_xtop - box.r_xbot)/20;
	if (expand < 2) expand = 2;
	box.r_xtop += expand;
	box.r_xbot -= expand;
	expand = (box.r_ytop - box.r_ybot)/20;
	if (expand < 2) expand = 2;
	box.r_ytop += expand;
	box.r_ybot -= expand;

	WindMove(w, &box);
	return;
    };

usage:
    TxError("Usage: findbox [zoom]\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdFlush --
 *
 * Implement the "flush" command.
 * Throw away all changes made within magic to the specified cell,
 * and re-read it from disk.  If no cell is specified, the default
 * is the current edit cell.
 *
 * Usage:
 *	flush [cellname]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	THIS IS NOT UNDO-ABLE!
 *	Modifies the specified CellDef.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
CmdFlush(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    CellDef *def;
    int action;
    static char *actionNames[] = { "no", "yes", 0 };
    char answer[100];

    if (cmd->tx_argc > 2)
    {
	TxError("Usage: flush [cellname]\n");
	return;
    }

    if (cmd->tx_argc == 1)
	def = EditCellUse->cu_def;
    else
    {
	def = DBCellLookDef(cmd->tx_argv[1]);
	if (def == (CellDef *) NULL)
	{
	    /* an error message has already been printed by the database */
	    return;
	}
    }

    if (def->cd_flags & (CDMODIFIED|CDSTAMPSCHANGED|CDBOXESCHANGED))
    {
	do
	{
	    TxPrintf("Really throw away all changes made to cell \"%s\"? [no] ",
			    def->cd_name);
	    if (TxGetLine(answer, sizeof answer) == NULL || answer[0] == '\0')
		return;
	} while ((action = Lookup(answer, actionNames)) < 0);
	if (action == 0)	/* No */
	    return;
    }

    cmdFlushCell(def);
    SelectClear();
    TxPrintf("[Flushed]\n");
}
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 *
 * CmdGaRoute --
 *
 * Command interface for gate-array routing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command; see below.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
Void
CmdGaRoute(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    typedef enum { CHANNEL, GEN, HELP, NOWARN, RESET, ROUTE, WARN } cmdType;
    static char *chanTypeName[] = { "NORMAL", "HRIVER", "VRIVER" };
    extern bool GAStemWarn;
    char *name, *channame;
    int n, chanType;
    Rect editArea;
    FILE *f;
    static struct
    {
	char	*cmd_name;
	cmdType	 cmd_val;
    } cmds[] = {
	"channel xl yl xh yh [type]\n\
channel	[type]		define a channel",			CHANNEL,
	"generate type [file]	generate channel definition commands",
								GEN,
	"help			print this message",		HELP,
	"nowarn			only warn if all locations of a terminal\n\
			are unreachable",			NOWARN,
	"route [netlist]		route the current cell",ROUTE,
	"reset			clear all channel definitions",	RESET,
	"warn			leave feedback for each location of a\n\
			terminal that is unreachable",		WARN,
	0
    };

    GAInit();
    if (cmd->tx_argc == 1)
	goto doRoute;

    n = LookupStruct(cmd->tx_argv[1], (LookupTable *) cmds, sizeof cmds[0]);
    if (n < 0)
    {
	if (n == -1)
	    TxError("Ambiguous option: \"%s\"\n", cmd->tx_argv[1]);
	else
	    TxError("Unrecognized routing command: %s\n", cmd->tx_argv[1]);
	TxError("    Type \"garoute help\" for help.\n");
	return;
    }

    switch (cmds[n].cmd_val)
    {
	case HELP:
	    TxPrintf("Gate-array router commands have the form:\n");
	    TxPrintf("\"garoute option\", where option is one of:\n\n");
	    for (n = 0; cmds[n].cmd_name; n++)
		TxPrintf("%s\n", cmds[n].cmd_name);
	    TxPrintf("\n");
	    break;
	case WARN:
	    if (cmd->tx_argc != 2) goto badWarn;
	    GAStemWarn = TRUE;
	    TxPrintf(
    "Will leave feedback for each unusable terminal loc.\n");
	    break;
	case NOWARN:
	    if (cmd->tx_argc != 2)
	    {
badWarn:
		TxError("Usage: \"garoute warn\" or \"garoute nowarn\"\n");
		return;
	    }
	    GAStemWarn = FALSE;
	    TxPrintf(
    "Will only leave feedback if all locs for a terminal are unusable.\n");
	    break;
	case RESET:
	    TxPrintf("Clearing all channel information.\n");
	    GAClearChannels();
	    break;
	case GEN:
	    if (cmd->tx_argc < 3 || cmd->tx_argc > 4)
	    {
		TxError("Usage: garoute generate type [file]\n");
		return;
	    }
	    if (!ToolGetEditBox(&editArea))
		return;
	    channame = cmd->tx_argv[2];
	    f = stdout;
	    if (cmd->tx_argc == 4)
	    {
		f = fopen(cmd->tx_argv[3], "w");
		if (f == NULL)
		{
		    perror(cmd->tx_argv[3]);
		    return;
		}
	    }
	    if (channame[0] == 'h') GAGenChans(CHAN_HRIVER, &editArea, f);
	    else if (channame[0] == 'v') GAGenChans(CHAN_VRIVER, &editArea, f);
	    else
	    {
		TxError("Unrecognized channel type: %s\n", channame);
		TxError("Legal types are \"h\" or \"v\"\n");
	    }
	    if (f != stdout)
		(void) fclose(f);
	    break;
	case CHANNEL:
	    channame = (char *) NULL;
	    if (cmd->tx_argc == 2 || cmd->tx_argc == 3)
	    {
		if (!ToolGetEditBox(&editArea))
		    return;
		if (cmd->tx_argc == 3)
		    channame = cmd->tx_argv[2];
	    }
	    else if (cmd->tx_argc == 6 || cmd->tx_argc == 7)
	    {
		if (!StrIsInt(cmd->tx_argv[2])
			|| !StrIsInt(cmd->tx_argv[3])
			|| !StrIsInt(cmd->tx_argv[4])
			|| !StrIsInt(cmd->tx_argv[5])) goto badChanCmd;
		editArea.r_xbot = atoi(cmd->tx_argv[2]);
		editArea.r_ybot = atoi(cmd->tx_argv[3]);
		editArea.r_xtop = atoi(cmd->tx_argv[4]);
		editArea.r_ytop = atoi(cmd->tx_argv[5]);
		chanType = CHAN_NORMAL;
		if (cmd->tx_argc == 7)
		    channame = cmd->tx_argv[6];
	    }
	    else goto badChanCmd;
	    if (channame)
	    {
		if (channame[0] == 'h') chanType = CHAN_HRIVER;
		else if (channame[0] == 'v') chanType = CHAN_VRIVER;
		else
		{
		    TxError("Unrecognized channel type: %s\n", channame);
		    goto badChanCmd;
		}
	    }
	    TxPrintf("Channel [%s] %d %d %d %d\n", chanTypeName[chanType],
		    editArea.r_xbot, editArea.r_ybot,
		    editArea.r_xtop, editArea.r_ytop);
	    if (!GADefineChannel(chanType, &editArea))
	    {
		TxError("Channel definition failed.\n");
		break;
	    }
	    break;
	case ROUTE:
	doRoute:
	    if (cmd->tx_argc > 3)
	    {
		TxError("Usage: garoute route [netlist]\n");
		break;
	    }
	    name = (char *) NULL;
	    if (cmd->tx_argc == 3)
		name = cmd->tx_argv[2];
	    n = GARouteCmd(EditCellUse, name);
	    if (n < 0)
		TxError("Couldn't route at all.\n");
	    else if (n > 0)
		TxPrintf("%d routing error%s.\n", n, n == 1 ? "" : "s");
	    else
		TxPrintf("No routing errors.\n");
	    break;
    }
    return;

badChanCmd:
    TxError("Usage: garoute channel xlo ylo xhi yhi [type]\n");
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdGetcell --
 *
 *	Implement the ":getcell" command.
 *
 * Usage:
 *	getcell cellName [child refPointChild] [parent refPointParent]
 *
 * where the refPoints are either a label name, e.g., SOCKET_A, or an x-y
 * pair of integers, e.g., 100 200.  The words "child" and "parent" are
 * keywords, and may be abbreviated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Makes cellName a subcell of the edit cell, positioned so
 *	that refPointChild in the child cell (or the lower-left
 *	corner of its bounding box) ends up at location refPointParent
 *	in the edit cell (or the location of the box tool's lower-left).
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
CmdGetcell(w, cmd)
    Window *w;			/* Window in which command was invoked. */
    TxCommand *cmd;		/* Describes command arguments. */
{
    CellUse dummy, *newUse;
    Transform editTrans;
    SearchContext scx;
    CellDef *def;
    Rect newBox;

    /* Leaves scx.scx_trans set to the transform from the child to root */
    if (!cmdDumpParseArgs("getcell", w, cmd, &dummy, &scx))
	return;
    def = dummy.cu_def;

    /* Create the new use. */
    newUse = DBCellNewUse(def, (char *) NULL);
    if (!DBLinkCell(newUse, EditCellUse->cu_def))
    {
	(void) DBCellDeleteUse(newUse);
	TxError("Could not link in new cell\n");
	return;
    }

    GeoTransTrans(&scx.scx_trans, &RootToEditTransform, &editTrans);
    DBSetTrans(newUse, &editTrans);
    if (DBCellFindDup(newUse, EditCellUse->cu_def) != NULL)
    {
	DBCellDeleteUse(newUse);
	TxError("Can't place a cell on an exact copy of itself.\n");
	return;
    }
    DBPlaceCell(newUse, EditCellUse->cu_def);

    /*
     * Reposition the box tool to around the gotten cell to show
     * that it has become the current cell.
     */
    GeoTransRect(&EditToRootTransform, &newUse->cu_bbox, &newBox);
    DBWSetBox(EditRootDef, &newBox);

    /* Select the new use */
    SelectClear();
    SelectCell(newUse, EditRootDef, &scx.scx_trans, FALSE);

    /* Redisplay and mark for design-rule checking */
    DBReComputeBbox(EditCellUse->cu_def);
    DBWAreaChanged(EditCellUse->cu_def, &newUse->cu_bbox,
	DBW_ALLWINDOWS, &DBAllButSpaceBits);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKSUBCELL, &newUse->cu_bbox);
}
#ifndef NO_SIM

/*
 * ----------------------------------------------------------------------------
 *
 * CmdGetnode --
 *
 * Implement the "getnode" command.
 * Returns the name of the node pointed by the mouse
 *
 * Usage:
 *  	getnode
 *	getnode abort
 *	getnode abort string
 *      getnode alias on
 *      getnode alias off
 *      getnode fast
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The GetNode hash tables may be modified.
 * ----------------------------------------------------------------------------
 */
Void
CmdGetnode(w, cmd)
    Window *w;
    TxCommand *cmd;
{
#define TBLSIZE 50
#define STRINGS 0

    bool  is_fast = FALSE;

    /* check arguments to command */

    switch (cmd->tx_argc) {
	case 1 : 
	    break;

	case 2 : 
	    if (strcmp("abort", cmd->tx_argv[1]) == 0) {
		if (!SimInitGetnode) {
		    HashKill(&SimGetnodeTbl);
		    SimInitGetnode = TRUE;
   		    SimRecomputeSel = TRUE;
		}
		return;
	    }
	    else if (strcmp("fast", cmd->tx_argv[1]) == 0) {
		is_fast = TRUE;
	    }
	    else {
		goto badusage;
	    }
	    break;

	case 3 : 
	    if (strcmp("alias", cmd->tx_argv[1]) == 0) {
		if (strcmp("on", cmd->tx_argv[2]) == 0) {
		    if (!SimGetnodeAlias) {
			HashInit(&SimGNAliasTbl, 120, STRINGS);
		    }
		    SimGetnodeAlias = TRUE;
		    return;
		}
		else if (strcmp("off", cmd->tx_argv[2]) == 0) {
		    if (SimGetnodeAlias) {
			HashKill(&SimGNAliasTbl);
		    }
		    SimGetnodeAlias = FALSE;
		    return;
		}
		else
		    goto badusage;
	    }
	    else if (strcmp("abort", cmd->tx_argv[1]) == 0) {
		if (SimInitGetnode) {
		    HashInit(&SimGetnodeTbl, TBLSIZE, STRINGS);
		    SimInitGetnode = FALSE;
		}
		SimRecomputeSel = TRUE;
		HashFind(&SimGetnodeTbl, cmd->tx_argv[2]);
		return;
	    }
	    else {
		goto badusage;
	    }
	    break;

	default :
	    goto badusage;
    }

    if ((w == (Window *) NULL) || (w->w_client != DBWclientID))
    {
	TxError("Put the cursor in a layout window\n");
	return;
    }
    if( is_fast == TRUE )
    {
	SimRecomputeSel = TRUE;
	SimGetsnode();
    }
    else
	SimGetnode();

    if (SimGetnodeAlias) {			/* "erase" the hash table */
	HashKill(&SimGNAliasTbl);
	HashInit(&SimGNAliasTbl, 120, STRINGS);
    }
    return;


badusage:
    TxError("Usage: getnode [abort [str]]\n");
    TxError("   or: getnode alias on\n");
    TxError("   or: getnode alias off\n");
    TxError("   or: getnode fast\n");
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdGrid --
 *
 * Implement the "grid" command.
 * Toggle the grid on or off in the selected window.
 *
 * Usage:
 *	grid [spacing [spacing [xorig yorig]]]
 *	grid off
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None, except to enable or disable grid display.
 *
 * ----------------------------------------------------------------------------
 */

Void
CmdGrid(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int xSpacing, ySpacing, xOrig, yOrig;
    DBWclientRec *crec;

    if (w == (Window *) NULL) return;
    crec = (DBWclientRec *) w->w_clientData;

    if ((cmd->tx_argc == 4) || (cmd->tx_argc > 5))
    {
	TxError("Usage: %s [xSpacing [ySpacing [xOrig yOrig]]]]\n",
		cmd->tx_argv[0]);
	return;
    }

    if ((cmd->tx_argc == 2) &&
	    ((strcmp(cmd->tx_argv[1], "off") == 0)
	    || (strcmp(cmd->tx_argv[1], "0") == 0)))
	crec->dbw_flags &= ~DBW_GRID;
    else if (cmd->tx_argc >= 2)
    {
	if (!StrIsInt(cmd->tx_argv[1]))
	{
	    TxError("Grid spacing must be a positive integer.\n");
	    return;
	}
	xSpacing = atoi(cmd->tx_argv[1]);
	if (xSpacing <= 0)
	{
	    TxError("Grid spacing must be greater than zero.\n");
	    return;
	}
	ySpacing = xSpacing;
	xOrig = yOrig = 0;

	if (cmd->tx_argc >= 3)
	{
	    if (!StrIsInt(cmd->tx_argv[2]))
	    {
		TxError("Grid spacing must be a positive integer.\n");
		return;
	    }
	    ySpacing = atoi(cmd->tx_argv[2]);
	    if (ySpacing <= 0)
	    {
		TxError("Grid spacing must be greater than zero.\n");
		return;
	    }

	    if (cmd->tx_argc == 5)
	    {
		if (!StrIsInt(cmd->tx_argv[3]) || !StrIsInt(cmd->tx_argv[4]))
		{
		    TxError("Grid origin coordinates must be integers.\n");
		    return;
		}
		xOrig = atoi(cmd->tx_argv[3]);
		yOrig = atoi(cmd->tx_argv[4]);
	    }
	}

	crec->dbw_gridRect.r_xbot = xOrig;
	crec->dbw_gridRect.r_ybot = yOrig;
	crec->dbw_gridRect.r_xtop = xOrig + xSpacing;
	crec->dbw_gridRect.r_ytop = yOrig + ySpacing;
	crec->dbw_flags |= DBW_GRID;
    }
    else crec->dbw_flags ^= DBW_GRID;
    WindAreaChanged(w, (Rect *) NULL);
}
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 *
 * CmdIRoute --
 *
 * Interactive route command.
 * Calls command entry point in irouter/irCommand.c
 *
 * Usage:
 *	iroute [subcmd [args]]
 *	(See irCommand.c for details.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on subcommand, see irCommand.c
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
CmdIRoute(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    IRCommand(w,cmd);

    return;
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdIdentify --
 *
 * Implement the "identify" command.
 * Sets the instance identifier for the currently selected cell.
 *
 * Usage:
 *	identify use_id
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the instance identifier for the selected cell (the
 *	first selected cell, if there are many).
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
CmdIdentify(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern int cmdIdFunc();		/* Forward reference. */

    if (cmd->tx_argc != 2)
    {
	TxError("Usage: identify use_id\n");
	return;
    }

    if (CmdIllegalChars(cmd->tx_argv[1], "[],/", "Cell use id"))
	return;

    if (SelEnumCells(FALSE, (int *) NULL, (SearchContext *) NULL,
	    cmdIdFunc, (ClientData) cmd->tx_argv[1]) == 0)
    {
	TxError("There isn't a selected subcell;  can't change ids.\n");
	return;
    }
}

    /* ARGSUSED */
int
cmdIdFunc(selUse, use, transform, newId)
    CellUse *selUse;		/* Use from selection cell. */
    CellUse *use;		/* Use from layout that corresponds to
				 * selUse.
				 */
    Transform *transform;	/* Not used. */
    char *newId;		/* New id for cell use. */
{
    if (!DBIsChild(use, EditCellUse))
    {
	TxError("Cell %s (%s) isn't a child of the edit cell.\n",
	    use->cu_id, use->cu_def->cd_name);
	TxError("    Cell identifier not changed.\n");
	return 1;
    }

    if (!DBReLinkCell(use, newId))
    {
	TxError("New name isn't unique within its parent definition.\n");
	TxError("    Cell identifier not changed.\n");
	return 1;
    }

    /* Change the id of the cell in the selection too, so that they
     * stay in sync.
     */

    (void) DBReLinkCell(selUse, newId);

    DBWAreaChanged(use->cu_parent, &use->cu_bbox,
	(int) ~(use->cu_expandMask), &DBAllButSpaceBits);
    DBWHLRedraw(EditRootDef, &selUse->cu_bbox, TRUE);
    return 1;
}
