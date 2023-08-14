/* grSunW4.c -
 *
 * This file contains functions to manage the SUN's mouse.
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
 */

#ifndef lint
static char rcsid[]="$Header: grSunW4.c,v 6.0 90/08/28 18:41:17 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <suntool/tool_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 

#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "graphics.h"
#include "graphicsInt.h"
#include "grSunWInt.h"
#include "txcommands.h"

/* Library routines: */

extern char *fgets();
extern int sscanf();
extern int errno;

/* local static variables */
static bool sunWHaveNullCursor = FALSE;
static struct cursor grSunWNullCursor;

/* things to share */
extern bool grSunWCursorOn;


/*---------------------------------------------------------
 * SunWEnableTablet:
 *	This routine enables the graphics cursor.
 *
 * Results: 
 *   	None.
 *
 * Side Effects:
 *	The Sun cursor is turned on.
 *
 *---------------------------------------------------------
 */

Void
SunWEnableTablet()
{
    grSunWCursorOn = TRUE;
    SunWSetCursor(grSunWCurCursor);
}


/*---------------------------------------------------------
 * SunWDisableTablet:
 *	This routine disables the graphics tablet so that other things may
 *	be done with the Sun.
 *
 * Results:	
 *	None.
 *
 * Side Effects:	
 *	The tablet is disabled.
 *---------------------------------------------------------
 */

Void
SunWDisableTablet()
{
    SunWSetCursor(-1);
    grSunWCursorOn = FALSE;
}


/*
 * ----------------------------------------------------------------------------
 * grSunWinput
 *	Get one input event.
 *
 * Results:
 *	returns a command, or NULL if there was none.
 *
 * Side effects:
 *	The input is read.
 *		
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
sunWInput(fd, cdata)
    int fd;
    ClientData cdata;
{
    struct inputevent sunEvent;
    TxInputEvent *event;
    grSunWRec *grdata;

    grdata = (grSunWRec *) cdata;
    ASSERT(grdata != NULL, "sunWInput");
    event = TxNewEvent();

    if (input_readevent(fd, &sunEvent) != 0) {
	perror("Magic, sunWInput");
	goto fail;
    };

    event->txe_wid = grdata->gr_w->w_wid;
    event->txe_p.p_x = sunEvent.ie_locx;
    event->txe_p.p_y = grdata->gr_w->w_allArea.r_ytop - sunEvent.ie_locy;

    if (sunEvent.ie_code >= ASCII_FIRST && sunEvent.ie_code <= ASCII_LAST) {
	event->txe_button = TX_NO_BUTTON;
	if (sunEvent.ie_code == 13)
	    event->txe_ch = '\n';
	else
	    event->txe_ch = sunEvent.ie_code;
    } else {
	if (sunEvent.ie_code == MS_LEFT) 
	    event->txe_button = TX_LEFT_BUTTON; 
	else if (sunEvent.ie_code == MS_MIDDLE) 
	    event->txe_button = TX_MIDDLE_BUTTON; 
	else if (sunEvent.ie_code == MS_RIGHT) 
	    event->txe_button = TX_RIGHT_BUTTON; 
	else {
	    TxError("Strange event number '%d', ignored.\n", sunEvent.ie_code);
	    goto fail;
	}
	if (win_inputnegevent(&sunEvent))
	    event->txe_buttonAction = TX_BUTTON_UP;
	else
	    event->txe_buttonAction = TX_BUTTON_DOWN;
    }

    TxAddEvent(event);
    return;

fail:
    TxFreeEvent(event);
    return;

}


#endif 	sun
