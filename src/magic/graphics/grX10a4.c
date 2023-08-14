/* grX10a4.c -
 *
 * This file contains functions to manage the graphics tablet associated
 * with the X display.
 *
 * Written in 1985 by Doug Pan and Prof. Mark Linton at Stanford.  Used in the
 * Berkeley release of Magic with their permission.
 *
 */

/* X10 driver "a" (from Lawrence Livermore National Labs) */



#ifndef lint
static char rcsid[]="$Header: grX10a4.c,v 6.0 90/08/28 18:41:29 mayo Exp $";
#endif  not lint

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
#include "grX10aInt.h"

/* imports from grX10a1.c
 */
extern struct xstate grCurrent;
extern int grPixels[];
extern int grTiles[];
extern DisplayHeight, DisplayWidth;


/*---------------------------------------------------------
 * GrXDisableTablet:
 *	Turns off the cursor.
 *
 *	...otherwise, the cursor messes up the X primitive drawing
 *	functions.  Mouse buttons are not unQ'd since that could lose
 *	mouse clicks, or, even worse, mouse button transitions.  (In
 *	the latter case, the mouse would be hung.)
 *
 * Results:	None.
 *
 * Side Effects:    None.		
 *---------------------------------------------------------
 */

Void
GrXDisableTablet ()
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
GrXEnableTablet ()
{
/* not implemented */
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
grxGetCursorPos (p)
    Point *p;		/* point to be filled in with screen coordinates */
{
    int x, y;
    Window subw;
    
    XQueryMouse( grCurrent.window, &x, &y, &subw );
    p->p_x = x;
    p->p_y = grXToMagic( y );
    return true;
}


