/* selUndo.c -
 *
 *	This file provides routines for undo-ing and redo-ing the
 *	the selection.  Most of the undo-ing is handled automatically
 *	by enabling undo-ing when the selection cell is modified.
 *	All this file does is record things to be redisplayed, since
 *	the normal undo package won't handle that.
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
static char rcsid[]="$Header: selUndo.c,v 6.0 90/08/28 18:56:50 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "undo.h"
#include "textio.h"
#include "select.h"
#include "selInt.h"

/* Each selection modification causes two records of the following
 * format to be added to the undo event list.  The first record
 * is added BEFORE the modification (sue_before is TRUE), and the
 * second is added afterwards.  The reason for doubling the events
 * is that we can't redisplay selection information until after the
 * selection has been modified.  This requires events to be in
 * different places depending on whether we're undo-ing or redo-ing.
 */

typedef struct
{
    CellDef *sue_def;		/* Definition in which selection must be
				 * redisplayed.
				 */
    Rect sue_area;		/* Area of sue_def in which selection info
				 * must be redisplayed.
				 */
    bool sue_before;		/* TRUE means this entry was made before
				 * the selection modifications.  FALSE
				 * means afterwards.
				 */
} SelUndoEvent;

/* Identifier for selection undo records: */

UndoType SelUndoClientID;

/*
 * ----------------------------------------------------------------------------
 *
 * SelUndoInit --
 *
 * 	Adds us as a client to the undo package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds a new client to the undo package, and sets SelUndoClientID.
 *
 * ----------------------------------------------------------------------------
 */

void
SelUndoInit()
{
    extern void SelUndoForw(), SelUndoBack();

    SelUndoClientID = UndoAddClient((void (*)()) NULL, (void (*)()) NULL,
	(UndoEvent *(*)()) NULL, (int (*)()) NULL, SelUndoForw,
	SelUndoBack, "selection redisplay");
    if (SelUndoClientID == NULL)
	TxError("Couldn't add selection as an undo client!\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelRememberForUndo--
 *
 * 	This routine is called twice whenever the selection is modified.
 *	It must be called once before modifying the selection.  In this
 *	case before is TRUE and the other arguments are arbitrary.  It
 *	must also be called again after the selection is modified.  In
 *	this case before is FALSE and the other fields indicate exactly
 *	which area was modified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds information to the undo list.
 *
 * ----------------------------------------------------------------------------
 */

void
SelRememberForUndo(before, def, area)
    bool before;		/* TRUE means caller is about to modify
				 * the given area of the selection.  FALSE
				 * means the caller has just modified
				 * the area.
				 */
    CellDef *def;		/* Root definition on top of whom selection
				 * information was just modified.
				 */
    Rect *area;			/* The area of def where selection info
				 * changed.  This pointer may be NULL, even
				 * on the second call, if there's no need
				 * to do redisplay during undo's.  This is
				 * the case if layout information is being
				 * modified over the area of the selection:
				 * when layout is redisplayed, selection info
				 * will automatically be redisplayed too.
				 */
{
    SelUndoEvent *sue;
    static SelUndoEvent *beforeEvent = NULL;
    static Rect nullRect = {0, 0, -1, -1};

    sue = (SelUndoEvent *) UndoNewEvent(SelUndoClientID, sizeof(SelUndoEvent));
    if (sue == NULL) return;

    /* We don't have complete information when the "before" event is
     * created, so save around its address and fill in the event when
     * the "after" event is created.
     */
    
    if (before)
    {
	sue->sue_before = TRUE;
	sue->sue_def = NULL;

	ASSERT(beforeEvent == NULL, "Forgot to call SelRememberForUndo after");
	beforeEvent = sue;
    }
    else
    {
	if (area == NULL) area = &nullRect;
	sue->sue_def = def;
	sue->sue_area = *area;
	sue->sue_before = before;

	ASSERT(beforeEvent != NULL, "Forgot to call SelRememberForUndo before");
	beforeEvent->sue_def = def;
	beforeEvent->sue_area = *area;
	beforeEvent = NULL;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * SelUndoForw --
 * SelUndoBack --
 *
 * 	Called to process undo redisplay events.  The two procedures
 *	are identical except that each one looks at different events.
 *	The idea is to do the selection redisplay only AFTER the selection
 *	has actually been modified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Highlights (including the selection) are redisplayed.
 *
 * ----------------------------------------------------------------------------
 */

void
SelUndoForw(sue)
    SelUndoEvent *sue;		/* Event to be redone. */
{
    if (sue->sue_before) return;
    if (sue->sue_def == NULL) return;
    SelSetDisplay(SelectUse, sue->sue_def);
    SelectRootDef = sue->sue_def;
    DBReComputeBbox(SelectDef);
    if (sue->sue_area.r_xbot <= sue->sue_area.r_xtop)
	DBWHLRedraw(sue->sue_def, &sue->sue_area, TRUE);
    DBWAreaChanged(SelectDef, &sue->sue_area, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
}

void
SelUndoBack(sue)
    SelUndoEvent *sue;		/* Event to be undone. */
{
    if (!sue->sue_before) return;
    if (sue->sue_def == NULL) return;
    SelSetDisplay(SelectUse, sue->sue_def);
    SelectRootDef = sue->sue_def;
    DBReComputeBbox(SelectDef);
    if (sue->sue_area.r_xbot <= sue->sue_area.r_xtop)
	DBWHLRedraw(sue->sue_def, &sue->sue_area, TRUE);
    DBWAreaChanged(SelectDef, &sue->sue_area, DBW_ALLWINDOWS,
	(TileTypeBitMask *) NULL);
}
