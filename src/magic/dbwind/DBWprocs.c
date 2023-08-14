/*
 * DBWprocs.c --
 *
 *	Procedures to interface the database with the window package
 *	for the purposes of window creation, deletion, and modification.
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
static char rcsid[] = "$Header: DBWprocs.c,v 6.0 90/08/28 18:11:27 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "windows.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "main.h"
#include "commands.h"
#include "dbwind.h"
#include "graphics.h"
#include "textio.h"
#include "txcommands.h"
#include "utils.h"
#include "undo.h"
#include "glyphs.h"
#include "malloc.h"
#include "styles.h"

global WindClient DBWclientID;
static dbwBitMask = 0;
#define MAX_BITMASK	0177777
#define MAX_BITS_IN_MASK 16

extern Void DBWredisplay();		/* Defined in DBWdisplay.c */

/* The following variable is used to allow somebody besides ourselves
 * to take over button handling for database windows.  If it is NULL
 * (which it usually is) we handle buttons.  If it isn't NULL, it gives
 * the address of a procedure to handle buttons.
 */

static Void (*dbwButtonHandler)() = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * DBWcreate
 *
 *	A new window has been created, create and initialize the needed 
 *	structures.
 *
 * Results:
 *	FALSE if we have too many windows, TRUE otherwise.
 *
 * Side effects:
 *	Load the given cell into the window.  If no cell is given, then
 *	if the box exists, load a zoomed-in view of the box.  Otherwise
 *	load a new blank cell.
 *
 * ----------------------------------------------------------------------------
 */

DBWcreate(window, argc, argv)
    Window *window;
    int argc;
    char *argv[];
{
    int bitMask, newBitMask, expand;
    DBWclientRec *crec;
    CellDef *boxDef;
    Rect box;

    /*
     * See if we can create a new window without running out
     * of bits in our bitMask.
     */

    newBitMask = (dbwBitMask + 1) | dbwBitMask;
    if (newBitMask > MAX_BITMASK) 
	return FALSE;
    bitMask = newBitMask ^ dbwBitMask;
    dbwBitMask = newBitMask;

    crec = (DBWclientRec *) mallocMagic(sizeof(DBWclientRec));
    crec->dbw_flags = DBW_SEELABELS;
    crec->dbw_watchPlane = -1;
    crec->dbw_watchDef = (CellDef *) NULL;
    crec->dbw_bitmask = bitMask;
    crec->dbw_expandAmounts.r_xbot = 0;
    crec->dbw_expandAmounts.r_ybot = 0;
    crec->dbw_expandAmounts.r_xtop = 0;
    crec->dbw_expandAmounts.r_ytop = 0;
    crec->dbw_gridRect.r_xbot = 0;
    crec->dbw_gridRect.r_ybot = 0;
    crec->dbw_gridRect.r_xtop = 1;
    crec->dbw_gridRect.r_ytop = 1;
    crec->dbw_visibleLayers = DBAllTypeBits;
    crec->dbw_hlErase = DBNewPlane((ClientData) TT_SPACE);
    crec->dbw_hlRedraw = DBNewPlane((ClientData) TT_SPACE);
    crec->dbw_labelSize = 0;
    crec->dbw_scale = -1;

    window->w_clientData = (ClientData) crec;
    if (argc > 0)
	DBWloadWindow(window, argv[0]);
    else if (ToolGetBox(&boxDef, &box))
    {
	DBWloadWindow(window, boxDef->cd_name);

	/* Zoom in on the box, leaving a 10% border or at least 2 units
	 * on each side.
	 */
	
	expand = (box.r_xtop - box.r_xbot)/20;
	if (expand < 2) expand = 2;
	box.r_xtop += expand;
	box.r_xbot -= expand;
	expand = (box.r_ytop - box.r_ybot)/20;
	if (expand < 2) expand = 2;
	box.r_ytop += expand;
	box.r_ybot -= expand;
	WindMove(window, &box);
    }
    else
	DBWloadWindow(window, (char *) NULL);
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 * DBWdelete --
 *
 *	Clean up the data structures before deleting a window.
 *
 * Results:
 *	TRUE if we really want to delete the window, FALSE otherwise.
 *
 * Side effects:
 *	A DBWclientRec is freed.
 * ----------------------------------------------------------------------------
 */

DBWdelete(window)
    Window *window;
{
    DBWclientRec *cr;

    cr = (DBWclientRec *) window->w_clientData;
    dbwBitMask &= ~(cr->dbw_bitmask);
    DBFreePaintPlane(cr->dbw_hlErase);
    DBFreePaintPlane(cr->dbw_hlRedraw);
    TiFreePlane(cr->dbw_hlErase);
    TiFreePlane(cr->dbw_hlRedraw);
    freeMagic( (char *) cr);
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * dbwLoadFunc --
 *
 * 	This is a utility function passed to WindSearch by
 *	DBWloadWindow.  Its job is to see if the edit cell is
 *	present in any window except a given one.
 *
 * Results:
 *	1 is returned if the window doesn't match the ClientData
 *	but contains the Edit Cell.  0 is returned otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
dbwLoadFunc(w, clientData)
    Window *w;			/* A window found in the search. */
    Window *clientData;		/* Window to ignore (passed as ClientData). */
{
    if (w == clientData) return 0;
    if (((CellUse *) w->w_surfaceID)->cu_def == EditRootDef)
	return 1;
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *
 * DBWreload --
 *
 * Re-load all windows to contain the named cell as a root.
 * This is intended to be called during startup or when restarting a saved 
 * image of Magic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Loads windows with the named cell.
 *
 * ----------------------------------------------------------------------------
 */

DBWreload(name)
    char *name;
{
    int dbwReloadFunc();

    (void) WindSearch(DBWclientID, (ClientData) NULL, (Rect *) NULL,
		dbwReloadFunc, (ClientData) name);
}

dbwReloadFunc(w, name)
    Window *w;
    char *name;
{
    DBWloadWindow(w, name);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBWloadWindow
 *
 *	Replace the root cell of a window by the specified cell.
 *
 *	A cell name of NULL causes the cell with name "(UNNAMED)" to be
 *	created if it does not already exist, or used if it does.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is currently no edit use, or if this is the only
 *	window containing the edit cell, then set the edit cell
 *	to the topmost cell in the new window.  Otherwise, the edit
 *	cell doesn't change.
 *
 * ----------------------------------------------------------------------------
 */

void
DBWloadWindow(window, name)
    Window *window;	/* Identifies window to which cell is to be bound */
    char *name;		/* Name of new cell to be bound to this window */
{
    CellDef *newEditDef;
    CellUse *newEditUse;
    void DisplayWindow();
    int res, newEdit;
    int xadd, yadd;
    Rect loadBox;

    loadBox.r_xbot = loadBox.r_ybot = 0;
    loadBox.r_xtop = loadBox.r_ytop = 1;

    /* See if we're to change the edit cell */
    newEdit = !WindSearch((WindClient) DBWclientID, (ClientData) NULL,
			    (Rect *) NULL, dbwLoadFunc, (ClientData) window);

    if (name == (char *) NULL)
    {
	/*
	 * If there is an existing unnamed cell, we use it.
	 * Otherwise, we create one afresh.
	 */
	newEditDef = DBCellLookDef(UNNAMED);
	if (newEditDef == (CellDef *) NULL)
	{
	    newEditDef = DBCellNewDef(UNNAMED, (char *) NULL);
	    DBCellSetAvail(newEditDef);
	}
    }
    else
    {
	/*
	 * Name specified.
	 * First try to find it in main memory, then try to
	 * read it from disk.
	 */
	newEditDef = DBCellLookDef(name);
	if (newEditDef == (CellDef *) NULL)
	    newEditDef = DBCellNewDef(name, (char *) NULL);

	if (!DBCellRead(newEditDef, (char *) NULL, TRUE))
	{
	    TxPrintf("Creating new cell\n");
	    DBCellSetAvail(newEditDef);
	}
	else
	{
	    /* DBCellRead doesn't dare to change bounding boxes, so
	     * we have to call DBReComputeBbox here (we know that it's
	     * safe).
	     */
	    DBReComputeBbox(newEditDef);
	    loadBox = newEditDef->cd_bbox;
	}
    }

    /*
     * Attach the new cell to the selected window.
     */

    newEditUse = DBCellNewUse(newEditDef, (char *) NULL);
    (void) StrDup(&(newEditUse->cu_id), "Topmost cell in the window");
    DBExpand(newEditUse,
	((DBWclientRec *)window->w_clientData)->dbw_bitmask, TRUE);
    if (newEdit)
    {
	if (EditCellUse && EditRootDef)
	{
	    DBWUndoOldEdit(EditCellUse, EditRootDef,
		    &EditToRootTransform, &RootToEditTransform);
	    DBWUndoNewEdit(newEditUse, newEditDef,
		    &GeoIdentityTransform, &GeoIdentityTransform);
	}
	EditCellUse = newEditUse;
	EditRootDef = newEditDef;
	EditToRootTransform = GeoIdentityTransform;
	RootToEditTransform = GeoIdentityTransform;
    }

    /* enforce a minimum size of 60 and a border of 10% around the sides */
    xadd = MAX(0, (60 - (loadBox.r_xtop - loadBox.r_xbot)) / 2) + 
	(loadBox.r_xtop - loadBox.r_xbot + 1) / 10;
    yadd = MAX(0, (60 - (loadBox.r_ytop - loadBox.r_ybot)) / 2) +
	(loadBox.r_ytop - loadBox.r_ybot + 1) / 10;
    loadBox.r_xbot -= xadd;  loadBox.r_xtop += xadd;
    loadBox.r_ybot -= yadd;  loadBox.r_ytop += yadd;

    window->w_bbox = &(newEditUse->cu_def->cd_bbox);
    res = WindLoad(window, DBWclientID, (ClientData) newEditUse, &loadBox);
    ASSERT(res, "DBWcreate");

    /* Update the captions in all windows to reflect the new
     * edit cell.  Also, if we've got a new edit cell, we need
     * to explicitly ask for redisplay, because there could be
     * another window on this cell somewhere else, and it needs
     * to be redisplayed too (if it's just the new window, that
     * is taken care of during WindLoad).
     */
    CmdSetWindCaption(EditCellUse, EditRootDef);
    if (newEdit)
	DBWAreaChanged(newEditDef, &newEditDef->cd_bbox, DBW_ALLWINDOWS,
	    &DBAllButSpaceBits);
}


/*
 * ----------------------------------------------------------------------------
 * DBWexit --
 *
 *	Magic is about to exit.  Check to see if any cells need to be written.
 *
 * Results:
 *	TRUE if it is OK to exit.
 *	FALSE otherwise.
 *
 * Side effects:
 *	The user is asked if he wants to write out any cells that are left.
 * ----------------------------------------------------------------------------
 */

bool
DBWexit()
{
    return (CmdWarnWrite() == 1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBWcommands --
 *
 * 	This procedure is called by the window package whenever a
 *	button is pushed or command is typed while the cursor is over
 *	a database window.  This procedure dispatches to the handler
 *	for the command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the command procedure does.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBWcommands(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int cmdNum;

    /* If it was a keyboard command, just dispatch to the proper
     * command routine.
     */

    if (cmd->tx_button == TX_NO_BUTTON)
    {
	if (cmd->tx_argc > 0)
	{
	    cmdNum = Lookup(cmd->tx_argv[0], CmdLongCommands);
	    ASSERT(cmdNum >= 0, "DBWcommands");
	    (void) (*CmdFuncs[cmdNum])(w, cmd);
	}
    }
    else
    {
	/* It's a button. */

	(*DBWButtonCurrentProc)(w, cmd);
    }

    UndoNext();
    DBFixMismatch();		/* Update timestamps now that it's safe. */
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBWNewButtonHandler --
 *
 * 	This procedure permits anyone else in Magic to take over
 *	handling of button pushes in database windows.  One example
 *	of this is the netlist editor.
 *
 * Results:
 *	The return value is the old button handler.  NULL means the
 *	default handler (moving the box and painting) was the old
 *	handler.  The caller must restore the old handler when it is
 *	finished.
 *
 * Side effects:
 *	From now on, all button pushes within database windows are
 *	passed to buttonProc, in the following form:
 *
 *	    Void
 *	    buttonProc(w, cmd)
 *		Window *w;
 *		TxCommand *cmd;
 *	    {
 *	    }
 *
 * ----------------------------------------------------------------------------
 */

Void (*(DBWNewButtonHandler(buttonProc)))()
    Void (*buttonProc)();		/* New button handler. */
{
    Void (*result)();

    result = dbwButtonHandler;
    dbwButtonHandler = buttonProc;
    return result;
}

/*
 * ----------------------------------------------------------------------------
 * DBWupdate--
 *
 *	This procedure is called once during each WindUpdate call.  It
 *	takes care of redisplay stuff that's not already handled by
 *	DBWAreaChanged (e.g. highlight redisplay).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff gets redisplayed.
 * ----------------------------------------------------------------------------
 */

Void
DBWupdate()
{
    DBWFeedbackShow();
    DBWHLUpdate();
}

/*
 * ----------------------------------------------------------------------------
 * DBWinit --
 *
 *	Initialize this module and open an initial window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A client is added, and an initial empty window is made 
 * ----------------------------------------------------------------------------
 */

DBWinit()
{
    Window *initialWindow;
    static char *doc =
"You are currently using the \"box\" tool.  The button actions are:\n\
   left    - move the box so its lower-left corner is at cursor position\n\
   right   - resize box by moving upper-right corner to cursor position\n\
   middle  - paint box area with material underneath cursor\n\
You can move or resize the box by different corners by pressing left\n\
    or right, holding it down, moving the cursor near a different corner\n\
    and clicking the other (left or right) button down then up without\n\
    releasing the initial button.\n";

    /* Initialize */
    DBWclientID = WindAddClient("layout", DBWcreate, DBWdelete, 
	    DBWredisplay, DBWcommands, DBWupdate, DBWexit,
	    ( Void (*)() ) NULL, CmdLongCommands, (GrGlyph *) NULL);
    
    DBWHLAddClient(DBWDrawBox);
    DBWAddButtonHandler("box", DBWBoxHandler, STYLE_CURS_NORMAL, doc);
    (void) DBWChangeButtonHandler("box");

    UndoDisable();
    DBCellInit();
    DBUndoInit();
    dbwUndoInit();

    /* Create initial window, but don't load anything into it. */
    WIND_MAX_WINDOWS(MAX_BITS_IN_MASK);
    initialWindow = WindCreate(DBWclientID, (Rect *) NULL, TRUE,
	0, (char **) NULL);
    ASSERT(initialWindow != (Window *) NULL, "DBWinit");

    UndoEnable();
}
