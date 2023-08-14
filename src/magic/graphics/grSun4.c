/* Sun4.c -
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
static char rcsid[]="$Header: grSun4.c,v 6.0 90/08/28 18:40:59 mayo Exp $";
#endif  not lint

#ifdef 	sun

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sunwindow/window_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */ 
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "graphics.h"
#include "graphicsInt.h"
#include "grSunInt.h"
#include "txcommands.h"

/* Library routines: */

extern char *fgets();
extern int sscanf();
extern int errno;

/* imports from other SUN modules */
extern Void sunSetWMandC();
extern int sunColor;

/* local static variables */
static bool sunHaveNullCursor = FALSE;
static struct cursor grSunNullCursor;

/* things to share */
int grSunCurCursor = 0;
bool grSunCursorOn = TRUE;



/*---------------------------------------------------------
 * SunEnableTablet:
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
SunEnableTablet()
{
    int allones = 255;

    grSunCursorOn = TRUE;
    SunSetCursor(grSunCurCursor);
    pr_putattributes(grSunCpr, &allones);
}


/*---------------------------------------------------------
 * SunDisableTablet:
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
SunDisableTablet()
{
    if (!sunHaveNullCursor)
    {
	grSunNullCursor.cur_shape = mem_create(0, 0, 1);
	sunHaveNullCursor = TRUE;
    }

    win_setcursor(grSunColorWindowFD, &grSunNullCursor);
    pr_getattributes(grSunCpr, &sunWMask);
    grSunCursorOn = FALSE;
}



/*
 * ----------------------------------------------------------------------------
 * sunGetCursorPos:
 * 	Read the cursor position from the mouse.
 *
 * Results:
 *	TRUE is returned if the coordinates were succesfully read, FALSE
 *	otherwise.
 *
 * Side effects:
 *	The parameter is filled in with the cursor position, in the form of
 *	a point in screen coordinates.
 * ----------------------------------------------------------------------------
 */

bool
sunGetCursorPos(p)
    Point *p;		/* point to be filled in with screen coordinates */
{
    int x, y;
    char str[10];
    extern bool grSunNoSuntools;

    if (grSunNoSuntools) {p->p_x = p->p_y = 100; return;};

    /* request the point */
    putc('A', grSunFileReqPoint);
    (void) fflush(grSunFileReqPoint);

    /* read the point */
    if ((fscanf(grSunFilePoint, "%3s %d %d", str, &x, &y) != 3) ||
	(strcmp(str, "P") != 0))
    {
	TxError("Could not read point from program '%s'\n",
	    BUTTON_PROG);
	return FALSE;
    }

    p->p_x = x;
    p->p_y = GrScreenRect.r_ytop - y;
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 * grSunHandleButtons:
 *	Get one button command.
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
grSunHandleButtons(fd, cdata)
    int fd;
    ClientData cdata;
{
    TxInputEvent *event;
    int x, y;
    char str[8];

    event = TxNewEvent();

    if (fscanf(grSunFileButtons, "%7s %d %d", str, &x, &y) != 3)
    {
	TxError("Funny event read from '%s' pipe.\n", BUTTON_PROG);
	goto fail;
    }
    event->txe_wid = WIND_UNKNOWN_WINDOW;
    event->txe_p.p_x = x;
    event->txe_p.p_y = GrScreenRect.r_ytop - y;
    switch (str[0])
    {
	case 'L':
	    event->txe_button = TX_LEFT_BUTTON;
	    break;
	case 'M':
	    event->txe_button = TX_MIDDLE_BUTTON;
	    break;
	case 'R':
	    event->txe_button = TX_RIGHT_BUTTON;
	    break;
	default:
	    TxError("Read %s program '%s' (button number '%c').\n",
		"funny button command from", BUTTON_PROG, str[0]);
	    goto fail;
	    break;
    }
    switch (str[1])
    {
	case 'U':
	    event->txe_buttonAction = TX_BUTTON_UP;
	    break;
	case 'D':
	    event->txe_buttonAction = TX_BUTTON_DOWN;
	    break;
	default:
	    TxError("Read %s program '%s' (button action '%c').\n",
		"funny button command from", BUTTON_PROG, str[1]);
	    goto fail;
	    break;
    }

    TxAddEvent(event);
    return;

fail:
    TxFreeEvent(event);
    return;
}


#endif 	sun
