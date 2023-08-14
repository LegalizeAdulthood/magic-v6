/* grX11su4.c -
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
 * This file contains functions to manage the graphics tablet associated
 * with the X display.
 *
 */

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "txcommands.h"
#include "grX11conf.h"
#include <X11/Xlib.h>
#include "grX11Int.h"


/*---------------------------------------------------------
 * GrXDisableTablet:
 *	Turns off the cursor.
 *
 * Results:	None.
 *
 * Side Effects:    None.		
 *---------------------------------------------------------
 */

Void
GrX11DisableTablet ()
{
}


/*---------------------------------------------------------
 * GrXEnableTablet:
 *	This routine enables the graphics tablet.
 *
 * Results: 
 *   	None.
 *
 * Side Effects:
 *	Simply turn on the crosshair.
 *---------------------------------------------------------
 */

Void
GrX11EnableTablet ()
{
}


/*
 * ----------------------------------------------------------------------------
 * grxGetCursorPos:
 * 	Read the cursor position in magic coordinates.
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
grx11GetCursorPos (p)
    Point *p;		/* point to be filled in with screen coordinates */
{
    int x, y, x1, y1, buttons;
    Window win1, win2;
    
    XQueryPointer(grXdpy, DefaultRootWindow(grXdpy),
		  &win1, &win2, &x1, &y1,
		  &x, &y, &buttons);
    p->p_x = x;
    p->p_y = grMagicToX(y);
    return true;
}
