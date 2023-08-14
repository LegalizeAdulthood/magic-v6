/*
 * grNull.c -
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
 * This file contains dummy functions for use when there is no
 * graphics display.
 */

#include <stdio.h>
#include <sgtty.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "textio.h"
#include "txcommands.h"
#include "signals.h"


#ifndef lint
static char rcsid[]="$Header: grNull.c,v 6.0 90/08/28 18:40:52 mayo Exp $";
#endif  not lint


/* Forward declarations */
extern bool nullReturnFalse();
extern Void nullDoNothing();
extern int nullReturnZero();

/*
 *---------------------------------------------------------
 *
 * nullDoNothing --
 *
 * This procedure does nothing.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

Void
nullDoNothing()
{
}

/*
 *---------------------------------------------------------
 *
 * nullReturnFalse --
 *
 * This procedure does nothing, and returns FALSE.
 *
 * Results:
 *	Returns FALSE always.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

bool
nullReturnFalse()
{
    return (FALSE);
}

/*
 *---------------------------------------------------------
 *
 * nullReturnZero --
 *
 * This procedure does nothing, and returns 0.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

nullReturnZero()
{
    return (0);
}

/*
 *---------------------------------------------------------
 *
 * NullInit --
 *
 * NullInit doesn't do much of anything.
 *
 * Results:
 *	TRUE always.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

bool
NullInit()
{
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * NullTextSize --
 *
 *	Determine the size of a text string. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle is filled in that is the size of the text in pixels.
 *	The origin (0, 0) of this rectangle is located on the baseline
 *	at the far left side of the string.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
Void
NullTextSize(text, size, r)
    char *text;
    int size;
    Rect *r;
{
    ASSERT(r != NULL, "nullTextSize");
    r->r_xbot = 0;
    r->r_xtop = strlen(text);
    r->r_ybot = 0;
    r->r_ytop = 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * nullStdin --
 *
 *      Handle the stdin device for the NULL driver.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Adds events to the event queue.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
nullStdin(fd, cdata)
    int fd;
    ClientData cdata;
{
    int ch;
    TxInputEvent *event;

    ch = getc(stdin);
    event = TxNewEvent();
    if (ch == EOF)
	event->txe_button = TX_EOF;
    else
	event->txe_button = TX_NO_BUTTON;
    event->txe_ch = ch;
    event->txe_wid = WIND_UNKNOWN_WINDOW;
    event->txe_p.p_x = GR_CURSOR_X;
    event->txe_p.p_y = GR_CURSOR_Y;
    TxAddEvent(event);
}

/*
 * ----------------------------------------------------------------------------
 *
 * NullBitBlt --
 *
 *	A no-op BitBlt operation for devices that don't have one (such as
 *	the NULL device and the AED767).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None, for sure, for sure.
 *
 * ----------------------------------------------------------------------------
 */

Void
NullBitBlt()
{
}


/*
 *---------------------------------------------------------
 *
 * nullSetDisplay --
 *
 * This routine sets the appropriate parameters so that
 * Magic will work with the particular display type.
 *
 * Results:
 *	Returns TRUE.
 *
 * Side Effects:
 *	Depends on the display type.
 *
 *---------------------------------------------------------
 */

    /*ARGSUSED*/
bool
nullSetDisplay(dispType, outFileName, mouseFileName)
    char *dispType;
    char *outFileName;
    char *mouseFileName;
{
    TxPrintf("Using NULL graphics device.\n");

    TxAddInputDevice((1 << fileno(stdin)), nullStdin, (ClientData) NULL);
    if (TxStdinIsatty) SigWatchFile(fileno(stdin), "stdin");

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = grSimpleLock;
    GrUnlockPtr = grSimpleUnlock;
    GrInitPtr = NullInit;
    GrClosePtr = nullDoNothing;
    GrSetCMapPtr = nullDoNothing;

    GrEnableTabletPtr = nullDoNothing;
    GrDisableTabletPtr = nullDoNothing;
    GrSetCursorPtr = nullDoNothing;
    GrTextSizePtr = NullTextSize;
    GrDrawGlyphPtr = nullDoNothing;
    GrBitBltPtr = NullBitBlt;
    GrReadPixelPtr = nullReturnZero;
    GrFlushPtr = nullDoNothing;

    /* local indirections */
    grSetSPatternPtr = nullDoNothing;
    grPutTextPtr = nullDoNothing;
    grDefineCursorPtr = nullDoNothing;
    grDrawGridPtr = nullReturnFalse;
    grDrawLinePtr = nullDoNothing;
    grSetWMandCPtr = nullDoNothing;
    grFillRectPtr = nullDoNothing;
    grSetStipplePtr = nullDoNothing;
    grSetLineStylePtr = nullDoNothing;
    grSetCharSizePtr = nullDoNothing;

    GrScreenRect.r_xtop = 511;
    GrScreenRect.r_ytop = 483;

    return TRUE;
}
