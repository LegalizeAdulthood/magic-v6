/*
 * undo.h --
 *
 * Exported definitions for the undo/redo module.
 * The undo/redo module is designed to be as client-independent
 * as possible.  Communication to and from clients is by means
 * of objects, allocated by the undo package, known as UndoEvents.
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
 *
 * rcsid "$Header: undo.h,v 6.0 90/08/28 18:58:35 mayo Exp $"
 */

#define	_UNDO

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC

/* -------------------- Exported definitions -------------------------- */

typedef int	 UndoType;	/* Type of undo event */
typedef char	 UndoEvent;	/* Externally visible undo event */

#define	UNDOLINESIZE	300	/* Maximum number of characters in external
				 * representation of undo event.
				 */

/*
 * Procedures for manipulating undo events.
 *
 *	UndoInit	-- start up the undo package and assign a log file.
 *	UndoAddClient	-- used by a client to inform the undo package of
 *			   its existence and to obtain an UndoType used in
 *			   all future interactions with undo.
 *	UndoIsEnabled	-- returns TRUE if the undo package is turned on,
 *			   and FALSE if it is disabled.
 *	UndoNewEvent	-- returns a new UndoEvent which the client may load
 *			   with its own data.  The event is appended to the
 *			   undo log.  The client should not retain this
 *			   new event past the next call to the undo package.  
 *			   If undoing is disabled, returns NULL.
 *	UndoNext	-- used by a client to inform the undo package that
 *			   all events since the last call to UndoNext are
 *			   to be treated as a single unit by UndoForward()
 *			   and UndoBackward().
 *	UndoBackward	-- play the undo log backward N units (until the prev-
 *			   ious call to UndoNext()).
 *	UndoForward	-- play the undo log forward N units.
 *	UndoDisable	-- turn off the undo package until the next UndoEnable.
 *	UndoEnable	-- turn the undo package back on.
 *	UndoFlush	-- throw away all undo information.
 */

extern bool UndoInit();
extern UndoType UndoAddClient();
extern UndoEvent *UndoNewEvent();
extern void UndoNext();
extern int UndoBackward(), UndoForward();
extern void UndoDisable(), UndoEnable();
extern void UndoFlush();

/*
 * ----------------------------------------------------------------------------
 *
 * UndoIsEnabled --
 *
 * Test whether the undo package is enabled.
 *
 * Results:
 *	Returns TRUE if undoing is enabled, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

extern bool UndoDisableCount;
#define	UndoIsEnabled()	(UndoDisableCount == 0)
