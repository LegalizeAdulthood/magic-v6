/*
 * CmdLQ.c --
 *
 * Commands with names beginning with the letters L through Q.
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
static char rcsid[] = "$Header: CmdLQ.c,v 6.0 90/08/28 18:07:18 mayo Exp $";
#endif  not lint

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
#include "graphics.h"
#include "drc.h"
#include "txcommands.h"
#include "undo.h"
#include "router.h"
#include "plow.h"
#include "select.h"
#include "plot.h"
#ifdef SYSV
#include <string.h>
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * CmdLabelProc --
 *
 * 	This procedure does all the work of putting a label except for
 *	parsing argments.  It is separated from CmdLabel so it can be
 *	used by the net-list menu system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A label is added to the edit cell at the box location, with
 *	the given text position, on the given layer.  If type is -1,
 *	then there must be only one layer underneath the box, and the
 *	label is added to that layer.
 *
 * ----------------------------------------------------------------------------
 */

void
CmdLabelProc(text, pos, type)
    char *text;			/* Text for label. */
    int pos;			/* Position for text relative to text. -1
				 * means "pick a nice one for me."
				 */
    TileType type;		/* Type of material label is to be attached
				 * to.  -1 means "pick a reasonable layer".
				 */
{
    Rect editBox;

    /* Make sure the box exists */
    if (!ToolGetEditBox(&editBox)) return;

    /* Make sure there's a valid string of text. */

    if ((text == NULL) || (*text == 0))
    {
	TxError("Can't have null label name.\n");
	return;
    }
    if (CmdIllegalChars(text, " /", "Label name"))
	return;

    if (type < 0) type = TT_SPACE;

    /* Eliminate any duplicate labels at the same position, regardless
     * of type.
     */

    DBEraseLabelsByContent(EditCellUse->cu_def, &editBox, pos, -1, text);

    pos = DBPutLabel(EditCellUse->cu_def, &editBox, pos, text, type);
    DBAdjustLabels(EditCellUse->cu_def, &editBox);
    DBReComputeBbox(EditCellUse->cu_def);
    DBWLabelChanged(EditCellUse->cu_def, text, &editBox, pos, DBW_ALLWINDOWS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdLabel --
 *
 * Implement the "label" command.
 * Place a label at a specific point on a specific type in EditCell
 *
 * Usage:
 *	label text [direction [layer]]
 *
 * Direction may be one of:
 *	right left top bottom
 *	east west north south
 *	ne nw se sw
 * or any unique abbreviation.  If not specified, it defaults to a value
 * chosen to keep the label text inside the cell.
 *
 * Layer defaults to the type of material beneath the degenerate box.
 * If the box is a rectangle, then use the lower left corner to determine
 * the material.
 *
 * If more than more than one tiletype other than space touches the box,
 * then the "layer" must be specified in the command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modified EditCellUse->cu_def.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

Void
CmdLabel(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    TileType type;
    int pos;
    char *p;

    if (cmd->tx_argc < 2 || cmd->tx_argc > 4)
    {
	TxError("Usage: %s text [direction [layer]]\n", cmd->tx_argv[0]);
	return;
    }

    p = cmd->tx_argv[1];

    /*
     * Find and check validity of type parameter
     */

    if (cmd->tx_argc > 3)
    {
	type = DBTechNameType(cmd->tx_argv[3]);
	if (type < 0)
	{
	    TxError("Unknown layer: %s\n", cmd->tx_argv[3]);
	    return;
	}
    } else type = -1;

    /*
     * Find and check validity of position.
     */

    if (cmd->tx_argc > 2)
    {
	pos = GeoNameToPos(cmd->tx_argv[2], FALSE, TRUE);
	if (pos < 0)
	    return;
        pos = GeoTransPos(&RootToEditTransform, pos);
    }
    else pos = -1;
    
    CmdLabelProc(p, pos, type);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdLayers --
 *
 * Implement the "layers" command.
 *
 * Usage:
 *	layers
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Print out the names of all the layers in the current
 *	technology.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */

Void
CmdLayers(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1)
    {
	TxError("Usage: %s\n", cmd->tx_argv[0]);
	return;
    }

    DBTechPrintTypes();
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdLoad --
 *
 * Implement the "load" command.
 *
 * Usage:
 *	load [name]
 *
 * If name is supplied, then the window containing the point tool is
 * remapped so as to edit the cell with the given name.
 *
 * If no name is supplied, then a new cell with the name "(UNNAMED)"
 * is created in the selected window.  If there is already a cell by
 * that name in existence (eg, in another window), that cell gets loaded
 * rather than a new cell being created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets EditCellUse.
 *
 * ----------------------------------------------------------------------------
 */

Void
CmdLoad(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (w == (Window *) NULL)
    {
	TxError("Point to a window first.\n");
	return;
    }

    if (cmd->tx_argc > 2)
    {
	TxError("Usage: %s [name]\n", cmd->tx_argv[0]);
	return;
    }

    if (cmd->tx_argc == 2)
    {
	if (CmdIllegalChars(cmd->tx_argv[1], "[],", "Cell name"))
	    return;
	DBWloadWindow(w, cmd->tx_argv[1]);
    }
    else DBWloadWindow(w, (char *) NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdMove --
 *
 * Implement the "move" command.
 *
 * Usage:
 *	move [direction [amount]]
 *	move to x y
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves everything that's currently selected.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

Void
CmdMove(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Transform t;
    Rect rootBox, newBox;
    Point rootPoint, editPoint;
    CellDef *rootDef;

    if (cmd->tx_argc > 4)
    {
	badUsage:
	TxError("Usage: %s [direction [amount]]\n", cmd->tx_argv[0]);
	TxError("   or: %s to x y\n", cmd->tx_argv[0]);
	return;
    }

    if (cmd->tx_argc > 1)
    {
	int indx, amount;
	int xdelta, ydelta;

	if (strcmp(cmd->tx_argv[1], "to") == 0)
	{
	    if (cmd->tx_argc != 4)
		goto badUsage;
	    if (!StrIsInt(cmd->tx_argv[2]) || !StrIsInt(cmd->tx_argv[3]))
		goto badUsage;
	    editPoint.p_x = atoi(cmd->tx_argv[2]);
	    editPoint.p_y = atoi(cmd->tx_argv[3]);
	    GeoTransPoint(&EditToRootTransform, &editPoint, &rootPoint);
	    goto moveToPoint;
	}

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
		ASSERT(FALSE, "Bad direction in CmdMove");
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
	 * the point as the transform.
	 */
	
	Window *window;

	window = ToolGetPoint(&rootPoint, (Rect *) NULL);
	if ((window == NULL) ||
	    (EditRootDef != ((CellUse *) window->w_surfaceID)->cu_def))
	{
	    TxError("\"Move\" uses the point as the place to put down a\n");
	    TxError("    the selection, but the point doesn't point to the\n");
	    TxError("    edit cell.\n");
	    return;
	}

moveToPoint:
	if (!ToolGetBox(&rootDef, &rootBox) || (rootDef != SelectRootDef))
	{
	    TxError("\"Move\" uses the box lower-left corner as a place\n");
	    TxError("    to pick up the selection for moving, but the box\n");
	    TxError("    isn't in a window containing the selection.\n");
	    return;
	}
	GeoTransTranslate(rootPoint.p_x - rootBox.r_xbot,
	    rootPoint.p_y - rootBox.r_ybot, &GeoIdentityTransform, &t);
	GeoTransRect(&t, &rootBox, &newBox);
	DBWSetBox(rootDef, &newBox);
    }
    
    SelectTransform(&t);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdPaint --
 *
 * Implement the "paint" command.
 * Paint the specified layers underneath the box in EditCellUse->cu_def.
 *
 * Usage:
 *	paint [layers]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modified EditCellUse->cu_def.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

Void
CmdPaint(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect editRect;
    TileTypeBitMask mask;

    if ((w == (Window *) NULL) || (w->w_client != DBWclientID))
    {
	TxError("Put the cursor in a layout window\n");
	return;
    }

    if (cmd->tx_argc != 2)
    {
	TxError("Usage: %s layers\n", cmd->tx_argv[0]);
	return;
    }

    if (!ToolGetEditBox(&editRect)) return;

    if (!CmdParseLayers(cmd->tx_argv[1], &mask))
	return;

    if (TTMaskHasType(&mask, L_LABEL))
    {
	TxError("Label layer cannot be painted.  Use the \"label\" command\n");
	return;
    }
    if (TTMaskHasType(&mask, L_CELL))
    {
	TxError("Subcell layer cannot be painted.  Use \"getcell\".\n");
	return;
    }

    TTMaskClearType(&mask, TT_SPACE);
    DBPaintMask(EditCellUse->cu_def, &editRect, &mask);
    DBAdjustLabels(EditCellUse->cu_def, &editRect);
    SelectClear();
    DBWAreaChanged(EditCellUse->cu_def, &editRect, DBW_ALLWINDOWS, &mask);
    DBReComputeBbox(EditCellUse->cu_def);
    DRCCheckThis (EditCellUse->cu_def, TT_CHECKPAINT, &editRect);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdPaintButton --
 *
 * This is a first crack at an implementation of the "paint" button.
 * Paint the specified layers underneath the box in EditCellUse->cu_def.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modified EditCellUse->cu_def.
 *
 * ----------------------------------------------------------------------------
 */

Void
CmdPaintButton(w, butPoint)
    Window *w;
    Point *butPoint;	/* Screen location at which button was raised */
{
    Rect rootRect, editRect;
    TileTypeBitMask mask;
    DBWclientRec *crec;

    if ((w == (Window *) NULL) || (w->w_client != DBWclientID))
    {
	TxError("Put the cursor in a layout window\n");
	return;
    }
    crec = (DBWclientRec *) w->w_clientData;

    WindPointToSurface(w, butPoint, (Point *) NULL, &rootRect);

    DBSeeTypesAll(((CellUse *)w->w_surfaceID), &rootRect, 
	    crec->dbw_bitmask, &mask);
    TTMaskAndMask(&mask, &DBAllButSpaceAndDRCBits);
    TTMaskAndMask(&mask, &crec->dbw_visibleLayers);

    if (!ToolGetEditBox(&editRect)) return;

    if (TTMaskEqual(&mask, &DBZeroTypeBits))
    {
	TTMaskAndMask3(&mask, &CmdYMAllButSpace, &crec->dbw_visibleLayers);

	/* A little extra bit of cleverness:  if the box is zero size
	 * then delete all labels (this must be what the user intended
	 * since a zero size box won't delete any paint).  Otherwise,
	 * only delete labels whose paint has completely vanished.
	 */

	if (GEO_RECTNULL(&editRect))
	    TTMaskSetType(&mask, L_LABEL);

	DBEraseMask(EditCellUse->cu_def, &editRect, &crec->dbw_visibleLayers);
	(void) DBEraseLabel(EditCellUse->cu_def, &editRect, &mask);
    }
    else
    {
	DBPaintMask(EditCellUse->cu_def, &editRect, &mask);
    }
    SelectClear();
    DBAdjustLabels(EditCellUse->cu_def, &editRect);

    DRCCheckThis (EditCellUse->cu_def, TT_CHECKPAINT, &editRect);
    DBWAreaChanged(EditCellUse->cu_def, &editRect, DBW_ALLWINDOWS, &mask);
    DBReComputeBbox(EditCellUse->cu_def);
    UndoNext();
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdPath --
 *
 * Implement the "path" command:  set a global cell search path.
 *
 * Usage:
 *	path [path]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The search path used to find cells is set to the first
 *	command line argument.  If no argument is given, then the
 *	current path is printed.  At present, there is only a single
 *	global path.  Eventually, there is to be a separate path
 *	for each cell.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

int
CmdPath(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern char *strncpy();

    if (cmd->tx_argc > 2)
    {
	TxError("Usage: %s [searchpath]\n", cmd->tx_argv[0]);
	return (0);
    }

    if (cmd->tx_argc == 1)
    {
	TxPrintf("Search path for cells is \"%s\"\n", Path);
	TxPrintf("Cell library search path is \"%s\"\n", CellLibPath);
	TxPrintf("System search path is \"%s\"\n", SysLibPath);
	return 0;
    }

    (void) StrDup(&Path, cmd->tx_argv[1]);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdPlot --
 *
 * Implement the "plot" command:  generate plot output for what's
 * underneath the box.
 *
 * Usage:
 *	plot type [options]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates plot output on disk somewhere.
 *
 * ----------------------------------------------------------------------------
 */

#define GREMLIN		0
#define HELP		1
#define PARAMETERS	2
#define VERSATEC	3
#define PIXELS		4

#ifndef NO_PLOT
	/* ARGSUSED */
CmdPlot(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int option;
    char **msg;
    Window *window;
    DBWclientRec *crec;
    TileTypeBitMask mask;
    CellDef *boxRootDef;
    SearchContext scx;
    float width;
#ifdef LLNL
    int iwidth;
#endif
    extern double atof();

    static char *cmdPlotOption[] =
    {	
	"gremlin file [layers]	     generate gremlin file for what's\n\
		                     underneath the box",
	"help                        print this help information",
	"parameters [name value]     set or print out plotting parameters",
	"versatec [size [layers]]    generate plot on Versatec printer, size\n\
		                     inches wide for layers underneath box",
#ifdef	LLNL
	"pixels [width [layers]]     generate plot in pix format, width pixels\n\
		                     wide, for layers underneath box",
#endif	LLNL
	NULL
    };

    if (cmd->tx_argc < 2)
    {
	option = HELP;
	cmd->tx_argc = 2;
    }
    else
    {
	option = Lookup(cmd->tx_argv[1], cmdPlotOption);
	if (option < 0)
	{
	    TxError("\"%s\" isn't a valid plot option.\n", cmd->tx_argv[1]);
	    option = HELP;
	    cmd->tx_argc = 2;
	}
    }

    if ((option == GREMLIN) || 	(option == VERSATEC) || (option == PIXELS))
    {
	window = ToolGetPoint((Point *) NULL, (Rect *) NULL);
	if (window == NULL)
	{
	    TxError("The cursor must be over a layout window to plot.\n");
	    return;
	}
	crec = (DBWclientRec *) window->w_clientData;
	scx.scx_use = (CellUse *) window->w_surfaceID;
	if ((!ToolGetBox(&boxRootDef, &scx.scx_area)) ||
		(scx.scx_use->cu_def != boxRootDef))
	{
	    TxError("The box and cursor must appear in the same window\n");
	    TxError("    for plotting.  The box indicates the area to\n");
	    TxError("    plot, and the cursor's window tells which\n");
	    TxError("    cells are expanded and unexpanded).\n");
	    return;
	}
	scx.scx_trans = GeoIdentityTransform;
	mask = crec->dbw_visibleLayers;
	if ((crec->dbw_flags & DBW_SEELABELS) && (crec->dbw_labelSize >= 0))
	    TTMaskSetType(&mask, L_LABEL);
	else TTMaskClearType(&mask, L_LABEL);
	TTMaskSetType(&mask, L_CELL);
    }

    switch (option)
    {
	case GREMLIN:
	    if ((cmd->tx_argc != 3) && (cmd->tx_argc != 4))
	    {
		TxError("Wrong number of arguments:\n    plot %s\n",
			cmdPlotOption[GREMLIN]);
		return;
	    }
	    if (cmd->tx_argc == 4)
	    {
		if (!CmdParseLayers(cmd->tx_argv[3], &mask))
			return;
	    }
	    PlotGremlin(cmd->tx_argv[2], &scx, &mask, crec->dbw_bitmask);
	    return;
	    
	case HELP:
	    TxPrintf("The \"plot\" commands are:\n");
	    for (msg = &(cmdPlotOption[0]); *msg != NULL; msg++)
	    {
		TxPrintf("    plot %s\n", *msg);
	    }
	    return;
	
	case PARAMETERS:
	    if (cmd->tx_argc == 2)
		PlotPrintParams();
	    else if (cmd->tx_argc == 4)
		PlotSetParam(cmd->tx_argv[2], cmd->tx_argv[3]);
	    else
	    {
 		TxError("Wrong arguments:\n    plot %s\n",
			cmdPlotOption[PARAMETERS]);
	    }
	    return;
	
	case VERSATEC:
	    if (cmd->tx_argc > 4)
	    {
		TxError("Too many arguments:\n    plot %s\n",
			cmdPlotOption[VERSATEC]);
		return;
	    }
	    if (cmd->tx_argc >= 3)
		width = atof(cmd->tx_argv[2]);
	    else width = 0.0;
	    if (cmd->tx_argc == 4)
	    {
		if (!CmdParseLayers(cmd->tx_argv[3], &mask))
			return;
	    }
	    PlotVersatec( &scx, &mask, crec->dbw_bitmask, width);
	    return;
    
#ifdef	LLNL
	case PIXELS:
	    if (cmd->tx_argc > 4)
	    {
		TxError("Too many arguments:\n    plot %s\n",
			cmdPlotOption[PIXELS]);
		return;
	    }
	    if (cmd->tx_argc >=3)
	      iwidth = atoi(cmd->tx_argv[2]);
	    else
	      iwidth = 0; /* means get it from the plot parameters */
	    if (cmd->tx_argc == 4)
	    {
		if (!CmdParseLayers(cmd->tx_argv[3], &mask))
			return;
	    }
	    PlotPixels( &scx, &mask, crec->dbw_bitmask, iwidth);
	    return;
#endif	LLNL
    }
}
#endif
#ifndef NO_PLOW

/*
 * ----------------------------------------------------------------------------
 *
 * CmdPlow --
 *
 * Implement the "plow" command:  snowplow.
 * One side of the box forms the plow, which is swept through the layout
 * until it coincides with the opposite side of the box.  The direction
 * depends on that specified by the user.
 *
 * Usage:
 *	plow [options]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the edit cell.
 *
 * ----------------------------------------------------------------------------
 */

    /*
     * The order of the following must be identical to that in
     * the table cmdPlowOption[] below.
     */
#define	PLOWBOUND		0
#define	PLOWHELP		1
#define	PLOWHORIZON		2
#define PLOWJOGS		3
#define	PLOWSELECTION		4
#define	PLOWSTRAIGHTEN		5
#define	PLOWNOBOUND		6
#define	PLOWNOJOGS		7
#define	PLOWNOSTRAIGHTEN	8
#define	PLOWPLOW		9	/* Implicit when direction specified */

int
CmdPlow(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int xdelta, ydelta, absX, absY;
    int option, dir, distance;
    char *layers, **msg;
    TileTypeBitMask mask;
    Rect rootBox, editBox, newBox;
    CellDef *rootDef, *editDef;
    Point rootPoint;
    Window *window;
    Transform t;
    static char *cmdPlowOption[] =
    {	
	"boundary	set boundary around area plowing may affect",
	"help		print this help information",
	"horizon n	set the horizon for jog introduction to n lambda",
	"jogs		reenable jog insertion (set horizon to 0)",
	"selection [direction [amount]]\n\
		plow the selection",
	"straighten	automatically straighten jogs after each plow",
	"noboundary	remove boundary around area plowing may affect",
	"nojogs		disable jog insertion (infinite jog horizon)",
	"nostraighten	don't automatically straighten jogs after each plow",
	NULL
    };

    if (cmd->tx_argc < 2)
	goto usage;

    option = Lookup(cmd->tx_argv[1], cmdPlowOption);
    if (option == -1)
    {
	TxError("Ambiguous plowing option: \"%s\"\n", cmd->tx_argv[1]);
	goto usage2;
    }
    if (option < 0)
    {
	dir = GeoNameToPos(cmd->tx_argv[1], TRUE, FALSE);
	if (dir < 0)
	    goto usage;
	dir = GeoTransPos(&RootToEditTransform, dir);
	option = PLOWPLOW;
    }

    switch (option)
    {
	case PLOWBOUND:
	case PLOWSELECTION:
	case PLOWNOBOUND:
	case PLOWPLOW:
	    if (w == (Window *) NULL)
	    {
		TxError("Point to a window first\n");
		return;
	    }
	    if (EditCellUse == (CellUse *) NULL)
	    {
		TxError("There is no edit cell!\n");
		return;
	    }
	    if (!ToolGetEditBox(&editBox) || !ToolGetBox(&rootDef, &rootBox))
		return;
	    editDef = EditCellUse->cu_def;
	    break;
    }

    switch (option)
    {
	case PLOWHELP:
	    TxPrintf("Plow commands have the form \"plow option\",\n");
	    TxPrintf("where option is one of:\n\n");
	    for (msg = &(cmdPlowOption[0]); *msg != NULL; msg++)
	    {
		if (**msg == '*') continue;
		TxPrintf("  %s\n", *msg);
	    }
	    TxPrintf("\n");
	    TxPrintf("Option may also be any Manhattan direction, which\n");
	    TxPrintf("causes the plow to be moved in that direction.\n");
	    return;
	case PLOWBOUND:
	    if (cmd->tx_argc != 2)
	    {
wrongNumArgs:
		TxError("Wrong number of arguments to %s option.\n",
			cmd->tx_argv[1]);
		TxError("Type \":plow help\" for help.\n");
		return;
	    }
	    PlowSetBound(editDef, &editBox, rootDef, &rootBox);
	    break;
	case PLOWHORIZON:
	    if (cmd->tx_argc == 3) PlowJogHorizon = atoi(cmd->tx_argv[2]);
	    else if (cmd->tx_argc != 2) goto wrongNumArgs;

	    if (PlowJogHorizon == INFINITY)
		TxPrintf("Jog horizon set to infinity.\n");
	    else TxPrintf("Jog horizon set to %d units.\n", PlowJogHorizon);
	    break;
	case PLOWSTRAIGHTEN:
	    PlowDoStraighten = TRUE;
	    TxPrintf("Jogs will be straightened after each plow.\n");
	    break;
	case PLOWNOBOUND:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    PlowClearBound();
	    break;
	case PLOWNOJOGS:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    PlowJogHorizon = INFINITY;
	    TxPrintf("Jog insertion disabled.\n");
	    break;
	case PLOWJOGS:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    PlowJogHorizon = 0;
	    TxPrintf("Jog insertion re-enabled (horizon 0).\n");
	    break;
	case PLOWNOSTRAIGHTEN:
	    PlowDoStraighten = FALSE;
	    TxPrintf("Jogs will not be straightened automatically.\n");
	    break;
	case PLOWPLOW:
	    if (cmd->tx_argc > 3) goto wrongNumArgs;
	    layers = cmd->tx_argc == 2 ? "*,l,subcell,space" : cmd->tx_argv[2];
	    if (!CmdParseLayers(layers, &mask))
		break;
	    if (Plow(editDef, &editBox, mask, dir))
		break;

	    TxPrintf("Reduced plow size to stay within the boundary.\n");
	    GeoTransRect(&EditToRootTransform, &editBox, &rootBox);
	    ToolMoveBox(TOOL_BL, &rootBox.r_ll, FALSE, rootDef);
	    ToolMoveCorner(TOOL_TR, &rootBox.r_ur, FALSE, rootDef);
	    break;
	case PLOWSELECTION:
	    if (cmd->tx_argc > 2)
	    {
		dir = GeoNameToPos(cmd->tx_argv[2], TRUE, TRUE);
		if (dir < 0)
		    return;
		if (cmd->tx_argc == 4)
		{
		    if (!StrIsInt(cmd->tx_argv[3]))
		    {
			TxError("Usage: plow selection [direction [amount]]\n");
			return;
		    }
		    distance = atoi(cmd->tx_argv[3]);
		}
		else distance = 1;

		switch (dir)
		{
		    case GEO_NORTH: xdelta = 0; ydelta = distance; break;
		    case GEO_SOUTH: xdelta = 0; ydelta = -distance; break;
		    case GEO_EAST:  xdelta = distance; ydelta = 0; break;
		    case GEO_WEST:  xdelta = -distance; ydelta = 0; break;
		    default:
			ASSERT(FALSE, "Bad direction in CmdPlow");
			return;
		}
	    }
	    else
	    {
		/*
		 * Use the displacement between the box lower-left corner
		 * and the point as the transform.
		 */
		if (rootDef != SelectRootDef)
		{
		    TxError("\"plow selection\" uses the box lower-left\n");
		    TxError("corner as a place to pick up the selection\n");
		    TxError("for plowing, but the box isn't in a window\n");
		    TxError("containing the selection\n");
		    return;
		}
		window = ToolGetPoint(&rootPoint, (Rect *) NULL);
		if ((window == NULL) ||
		    (EditRootDef != ((CellUse *) window->w_surfaceID)->cu_def))
		{
		    TxError("\"plow selection\" uses the point as the\n");
		    TxError("place to plow the selection, but the point\n");
		    TxError("doesn't point to the edit cell.\n");
		    return;
		}
		xdelta = rootPoint.p_x - rootBox.r_xbot;
		ydelta = rootPoint.p_y - rootBox.r_ybot;
		if (xdelta < 0) absX = -xdelta; else absX = xdelta;
		if (ydelta < 0) absY = -ydelta; else absY = ydelta;
		if (absY <= absX)
		{
		    ydelta = 0;
		    distance = absX;
		    dir = xdelta > 0 ? GEO_EAST : GEO_WEST;
		}
		else
		{
		    xdelta = 0;
		    distance = absY;
		    dir = ydelta > 0 ? GEO_NORTH : GEO_SOUTH;
		}
	    }
	    if (!PlowSelection(editDef, &distance, dir))
	    {
		TxPrintf("Reduced distance to stay in the boundary.\n");
		switch (dir)
		{
		    case GEO_EAST:	xdelta =  distance; break;
		    case GEO_NORTH:	ydelta =  distance; break;
		    case GEO_WEST:	xdelta = -distance; break;
		    case GEO_SOUTH:	ydelta = -distance; break;
		}
	    }
	    GeoTransTranslate(xdelta, ydelta, &GeoIdentityTransform, &t);
	    GeoTransRect(&t, &rootBox, &newBox);
	    DBWSetBox(rootDef, &newBox);
	    SelectClear();
	    break;
    }
    return;

usage:
    TxError("\"%s\" isn't a valid plow option.", cmd->tx_argv[1]);

usage2:
    TxError("  Type \"plow help\" for help.\n");
    return;
}
#endif
