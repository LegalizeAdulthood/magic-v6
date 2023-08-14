/*
 * CmdTZ.c --
 *
 * Commands with names beginning with the letters T through Z.
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
static char rcsid[] = "$Header: CmdTZ.c,v 6.0 90/08/28 18:07:26 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <math.h>
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
#include "signals.h"
#include "router.h"
#include "gcr.h"
#include "undo.h"
#include "select.h"
#include "styles.h"
#include "wiring.h"
#include "netlist.h"
#include "netmenu.h"

#ifdef	LLNL
#include "yacr.h"
#endif	LLNL

extern int atoi();

extern void DisplayWindow();

/*
 * ----------------------------------------------------------------------------
 *
 * CmdTool --
 *
 * 	Implement the "tool" command.
 *
 * Usage:
 *	tool [name|info]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current tool that's active in layout windows may be changed.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
Void
CmdTool(w, cmd)
    Window *w;			/* Window in which command was invoked. */
    TxCommand *cmd;		/* Info about command options. */
{
    if (cmd->tx_argc == 1)
    {
	(void) DBWChangeButtonHandler((char *) NULL);
	return;
    }

    if (cmd->tx_argc > 2)
    {
	TxError("Usage: %s [name|info]\n", cmd->tx_argv[0]);
	return;
    }

    if (strcmp(cmd->tx_argv[1], "info") == 0)
	DBWPrintButtonDoc();
    else (void) DBWChangeButtonHandler(cmd->tx_argv[1]);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdUnexpand --
 *
 * Implement the "unexpand" command.
 *
 * Usage:
 *	unexpand
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unexpands all cells under the box that don't completely
 *	contain the box.
 *
 * ----------------------------------------------------------------------------
 */

CmdUnexpand(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int windowMask, boxMask;
    Rect rootRect;
    int cmdUnexpandFunc();		/* Forward reference. */

    if (cmd->tx_argc != 1)
    {
	TxError("Usage: %s\n", cmd->tx_argv[0]);
	return;
    }
    
    if (w == (Window *) NULL)
    {
	TxError("Point to a window first.\n");
	return;
    }
    windowMask = ((DBWclientRec *) w->w_clientData)->dbw_bitmask;

    (void) ToolGetBoxWindow(&rootRect, &boxMask);
    if ((boxMask & windowMask) != windowMask)
    {
	TxError("The box isn't in the same window as the cursor.\n");
	return;
    }
    DBExpandAll(((CellUse *) w->w_surfaceID), &rootRect, windowMask,
	    FALSE, cmdUnexpandFunc, (ClientData) windowMask);
}

/* This function is called for each cell whose expansion status changed.
 * It forces the cells area to be redisplayed, then returns 0 to keep
 * looking for more cells to unexpand.
 */

int
cmdUnexpandFunc(use, windowMask)
    CellUse *use;		/* Use that was just unexpanded. */
    int windowMask;		/* Window where it was unexpanded. */
{
    if (use->cu_parent == NULL) return 0;
    DBWAreaChanged(use->cu_parent, &use->cu_bbox, windowMask,
	    (TileTypeBitMask *) NULL);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdUpsidedown --
 *
 * Implement the "upsidedown" command.
 *
 * Usage:
 *	upsidedown
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The box and verything in the selection are flipped upside down
 *	using the point as the axis around which to flip.
 *
 * ----------------------------------------------------------------------------
 */
    
    /* ARGSUSED */

CmdUpsidedown(w, cmd)
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

    
    /* To flip the selection upside down, first flip it around the
     * x-axis, then move it back so its lower-left corner is in
     * the same place that it used to be.
     */
    
    GeoTransRect(&GeoUpsideDownTransform, &SelectDef->cd_bbox, &bbox);
    GeoTranslateTrans(&GeoUpsideDownTransform,
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
 * CmdWhat --
 *
 * 	Print out information about what's selected.
 *
 * Usage:
 *	what
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets printed to identify the kinds of paint, plus
 *	labels and subcells, that are selected.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdWhat(w, cmd)
    Window *w;			/* Window in which command was invoked. */
    TxCommand *cmd;		/* Information about the command. */
{
    bool foundAny;
    TileTypeBitMask layers;
    extern int cmdWhatPaintFunc(), cmdWhatLabelFunc(), cmdWhatCellFunc();

    if (cmd->tx_argc > 1)
    {
	TxError("Usage: what\n");
	return;
    }

    /* Find all the selected paint and print out the layer names. */

    TTMaskZero(&layers);
    (void) SelEnumPaint(&DBAllButSpaceAndDRCBits, FALSE, (bool *) NULL,
	    cmdWhatPaintFunc, (ClientData) &layers);
    if (!TTMaskIsZero(&layers))
    {
	int i;

	TxPrintf("Selected mask layers:\n");
	for (i = TT_SELECTBASE; i < DBNumUserLayers; i++)
	{
	    if (TTMaskHasType(&layers, i))
		TxPrintf("    %s\n", DBTypeLongName(i));
	}
    }

    /* Enumerate all of the selected labels. */

    foundAny = FALSE;
    (void) SelEnumLabels(&DBAllTypeBits, FALSE, (bool *) NULL,
	    cmdWhatLabelFunc, (ClientData) &foundAny);
    

    /* Enumerate all of the selected subcells. */

    foundAny = FALSE;
    (void) SelEnumCells(FALSE, (bool *) NULL, (SearchContext *) NULL,
	    cmdWhatCellFunc, (ClientData) &foundAny);
}

/* Search function invoked for each paint tile in the selection:
 * just set a bit in a tile type mask.
 */

    /*ARGSUSED*/
int
cmdWhatPaintFunc(rect, type, mask)
    Rect *rect;			/* Not used. */
    TileType type;		/* Type of this piece of paint. */
    TileTypeBitMask *mask;	/* Place to OR in type's bit. */
{
    TTMaskSetType(mask, type);
    return 0;
}

/* Search function invoked for each label in the selection:  print
 * out information about the label.
 */

    /*ARGSUSED*/
int
cmdWhatLabelFunc(label, cellDef, transform, foundAny)
    Label *label;		/* Label that's selected. */
    CellDef *cellDef;		/* Cell definition containing label. */
    Transform *transform;	/* Not used. */
    bool *foundAny;		/* Use to print extra stuff for the first
				 * label found.
				 */
{
    if (!*foundAny)
    {
	TxPrintf("Selected label(s): \n");
	*foundAny = TRUE;
    }

    TxPrintf("    \"%s\" is attached to %s in cell %s\n", label->lab_text,
	    DBTypeLongName(label->lab_type), cellDef->cd_name);
    return 0;
}

/* Search function invoked for each selected subcell.  Just print out
 * its name and use id.
 */

    /*ARGSUSED*/
int
cmdWhatCellFunc(selUse, realUse, transform, foundAny)
    CellUse *selUse;		/* Not used. */
    CellUse *realUse;		/* Selected cell use. */
    Transform *transform;	/* Not used. */
    bool *foundAny;		/* Used to print extra stuff for the first
				 * use found.
				 */
{
    if (!*foundAny)
    {
	TxPrintf("Selected subcell(s):\n");
	*foundAny = TRUE;
    }
    TxPrintf("    Instance \"%s\" of cell \"%s\"\n", realUse->cu_id,
	    realUse->cu_def->cd_name);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdWire --
 *
 * Implement the "wire" command, which provides a wiring-style interface
 * for painting.
 *
 * Usage:
 *	wire option args
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The edit cell is modified to contain additional paint.
 *
 * ----------------------------------------------------------------------------
 */

#define HELP		0
#define HORIZONTAL	1
#define LEG		2
#define SWITCH		3
#define TYPE		4
#define VERTICAL	5

	/* ARGSUSED */
CmdWire(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int option;
    char **msg;
    TileType type;
    int width;

    static char *cmdWireOption[] =
    {	
	"help                   print this help information",
	"horizontal             add a new horizontal wire leg",
	"leg                    add a new horizontal or vertical leg",
	"switch [layer width]   place contact and switch layers",
	"type [layer width]     select the type and size of wires",
	"vertical               add a new vertical wire leg",
	NULL
    };

    if (cmd->tx_argc < 2)
    {
	option = HELP;
	cmd->tx_argc = 2;
    }
    else
    {
	option = Lookup(cmd->tx_argv[1], cmdWireOption);
	if (option < 0)
	{
	    TxError("\"%s\" isn't a valid wire option.\n", cmd->tx_argv[1]);
	    option = HELP;
	    cmd->tx_argc = 2;
	}
    }

    switch (option)
    {
	case HELP:
	    TxPrintf("Wiring commands have the form \":wire option\",");
	    TxPrintf(" where option is one of:\n");
	    for (msg = &(cmdWireOption[0]); *msg != NULL; msg++)
	    {
		TxPrintf("    %s\n", *msg);
	    }
	    return;
	
	case HORIZONTAL:
	    WireAddLeg((Rect *) NULL, (Point *) NULL, WIRE_HORIZONTAL);
	    return;
	
	case LEG:
	    WireAddLeg((Rect *) NULL, (Point *) NULL, WIRE_CHOOSE);
	    return;

	case SWITCH:
	    if (cmd->tx_argc == 2)
		WireAddContact(-1, 0);
	    else if (cmd->tx_argc != 4)
		goto badargs;
	    else
	    {
		type = DBTechNameType(cmd->tx_argv[2]);
		if (type == -2)
		{
		    TxError("Layer name \"%s\" doesn't exist.\n",
			    cmd->tx_argv[2]);
		    return;
		}
		else if (type == -1)
		{
		    TxError("Layer name \"%s\" is ambiguous.\n",
			    cmd->tx_argv[2]);
		    return;
		}
		if (!StrIsInt(cmd->tx_argv[3])) goto badargs;
		width = atoi(cmd->tx_argv[3]);
		WireAddContact(type, width);
		return;
	    }
	    break;

	case TYPE:
	    if (cmd->tx_argc == 2)
		WirePickType(-1, 0);
	    else if (cmd->tx_argc != 4)
	    {
		badargs:
		TxError("Wrong arguments.  The correct syntax is\n");
		TxError("    \"wire %s\"\n", cmdWireOption[option]);
		return;
	    }
	    else
	    {
		type = DBTechNameType(cmd->tx_argv[2]);
		if (type == -2)
		{
		    TxError("Layer name \"%s\" doesn't exist.\n",
			    cmd->tx_argv[2]);
		    return;
		}
		else if (type == -1)
		{
		    TxError("Layer name \"%s\" is ambiguous.\n",
			    cmd->tx_argv[2]);
		    return;
		}
		if (!StrIsInt(cmd->tx_argv[3])) goto badargs;
		width = atoi(cmd->tx_argv[3]);
		WirePickType(type, width);
		return;
	    }
	    break;

	case VERTICAL:
	    WireAddLeg((Rect *) NULL, (Point *) NULL, WIRE_VERTICAL);
	    return;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdWriteall --
 *
 * Implement the "writeall" command.
 * Write out all modified cells to disk.
 *
 * Usage:
 *	writeall [force]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	For each cell that has been modified since it was last written,
 *	the user is asked whether he wants to write it, flush it,
 *	skip it, or abort the "writeall" command.  If the decision
 *	is made to write, the cell is written out to disk and the
 *	modified bit in its definition's flags is cleared.  If the
 *	decision is made to flush, all paint and subcell uses are
 *	removed from the cell, and it is re-read from disk.
 *
 * ----------------------------------------------------------------------------
 */

static bool cmdWriteAllForce;  	/* If true, the user specified 'force' */

    /* ARGSUSED */

CmdWriteall(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int cmdWriteallFunc();
    bool autoWrite;
    static char *force[] = { "force", 0 };

    if ((cmd->tx_argc > 2) || 
	((cmd->tx_argc == 2) && (Lookup(cmd->tx_argv[1], force) < 0)))
    {
	TxError("Usage: %s [force]\n", cmd->tx_argv[0]);
	return;
    }

    cmdWriteAllForce = autoWrite = (cmd->tx_argc == 2);

    DBUpdateStamps();
    (void) DBCellSrDefs(CDMODIFIED|CDBOXESCHANGED|CDSTAMPSCHANGED,
	cmdWriteallFunc, (ClientData) &autoWrite);
}

/*
 * Filter function used by CmdWriteall() above.
 * This function is called for each known CellDef whose modified bit
 * is set.
 */

    /*ARGSUSED*/
int
cmdWriteallFunc(def, autoWrite)
    CellDef *def;	/* Pointer to CellDef to be saved.  This def might
			 * be an internal buffer; if so, we ignore it.
			 */
    bool *autoWrite;	/* Client data passed to DBCellSrDefs; pointer to
			 * flag:  if flag true, then write without asking.
			 * Otherwise, ask user whether or not to write
			 * dirty cells.
			 */
{
    char answer[50];
    int action;
    static char *actionNames[] =
        { "write", "flush", "skip", "abort", "autowrite", 0 };

    if (def->cd_flags & CDINTERNAL) return 0;
    if (SigInterruptPending) return 1;

    if (*autoWrite)
    {
	action = 4;
    }
    else do
    {
	TxPrintf("%s", def->cd_name);
	if (!(def->cd_flags & CDMODIFIED))
	{
	    if (!(def->cd_flags & CDSTAMPSCHANGED))
		TxPrintf("(bboxes)");
	    else if (!(def->cd_flags & CDBOXESCHANGED))
		TxPrintf("(timestamps)");
	    else TxPrintf("(bboxes/timestamps)");
	}
	TxPrintf(": write, autowrite, flush, skip, or abort command? [write] ");
	if (TxGetLine(answer, sizeof answer) == NULL || answer[0] == '\0')
	{
	    action = 0;
	    break;
	}
    } while ((action = Lookup(answer, actionNames)) < 0);

    switch (action)
    {
	case 0:		/* Write */
	    cmdSaveCell(def, (char *) NULL, cmdWriteAllForce, TRUE);
	    break;
	case 1:		/* Flush */
	    cmdFlushCell(def);
	    break;
	case 2:		/* Skip */
	    break;
	case 3:		/* Abort command */
	    return 1;
	case 4:		/* Automatically write everything */
	    *autoWrite = TRUE;
	    TxPrintf("Writing '%s'\n", def->cd_name);
	    cmdSaveCell(def, (char *) NULL, cmdWriteAllForce, TRUE);
	    break;
    }
    return 0;
}
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 *
 * CmdYacr --
 *
 * Channel decomposition and routing using the YACR2 algorithm.
 *
 * Usage:
 *	yacr [options]
 *
 * Results:
 *	Channels routed.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */

#ifdef	LLNL
#define	YCRSTART	0
#define	YCRCHANNELS	0
#define	YCRHELP		1
#define	YCRNETLIST	2
#define	YCRTECHINFO	3
#define	YCRVIA		4
#define	YCREND		4

    /* ARGSUSED */

CmdYacr(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect area;
    int	option;
    NLNetList netList;
    char *netListName;

    static char *cmdYacrOption[] =
    {	
	"channels 		Display channel structure",
	"help			Display yacr command options",
	"netlist file		set current netlist",
	"techinfo		show technology parameters", 
	"via			perform via minimization",
	0,
    };

    if ( cmd->tx_argc == 1 )
    {
	if (!ToolGetEditBox(&area))
	    return;

	YcrRoute(EditCellUse, &area);
	return;
    }

    option = Lookup(cmd->tx_argv[1], cmdYacrOption);
    if (option == -1)
    {
	TxError("Ambiguous yacr option: \"%s\"\n", cmd->tx_argv[1]);
	goto usage2;
    }
    if (option < 0)
	goto usage;

    switch (option)
    {
	case YCRCHANNELS:
	    {
		/*
		 * Decompose and display channels using feedback.
		 */

		CellDef *def;
		register GCRChannel *ch;
		int cmdYacrFunc();

		if (!ToolGetEditBox(&area))
		    return;
		RtrChannelList = (GCRChannel *) NULL;
		if ( def = YcrDecompose(EditCellUse, &area) )
		{
		    /*
		     * Display channel structure on screen.
		     */

		    (void) DBSrPaintArea((Tile *)NULL, def->cd_planes[PL_DRC_ERROR],
			&area, &DBSpaceBits, cmdYacrFunc, (ClientData) NULL);
		    /*
		     * Free channel data structures and storage. 
		     */

		    for ( ch = RtrChannelList; ch; ch = ch->gcr_next)
			GCRFreeChannel(ch);
		    TxPrintf("\n");
		}
		else
		    TxPrintf("Unusable routing area\n");
	    }
	    break;

	case YCRHELP:
	    TxPrintf("Yacr commands have the form \"yacr option\",\n");
	    TxPrintf("where option is one of:\n\n");
	    if(cmd->tx_argc==2)
		for (option=YCRSTART; option<= YCREND; option++)
		    TxPrintf("  %s\n", cmdYacrOption[option]);
	    else
	    if((cmd->tx_argc==3) && !strcmp(cmd->tx_argv[2], "*wizard"))
		for (option=0; option <=YCREND; option++)
			TxPrintf("  %s\n", cmdYacrOption[option]);
	    TxPrintf("\n");
	    break;

	case YCRNETLIST:
	    if(cmd->tx_argc!=2)
	    {
		if(cmd->tx_argc!=3)
		    goto wrongNumArgs;
		NMNewNetlist(cmd->tx_argv[2]);
	    }
	    TxPrintf("Current list is \"%s\"\n", NMNetlistName());
	    break;

	case YCRTECHINFO:
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
	
	case YCRVIA:
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
    }
    return;

wrongNumArgs:
    TxPrintf("wrong number of arguments\n");
    goto usage2;

usage:
    TxError("\"%s\" isn't a valid yacr option.", cmd->tx_argv[1]);

usage2:
    TxError("  Type \"yacr help\" for help.\n");
    return;
}

int
cmdYacrFunc(tile)
    Tile *tile;
{
    Rect area, rootArea;

    TiToRect(tile, &area);
    GeoTransRect(&EditToRootTransform, &area, &rootArea);
    DBWFeedbackAdd(&area, "Channel area", EditRootDef, 1,
	STYLE_OUTLINEHIGHLIGHTS);
    return 0;
}
#endif	LLNL
#endif
