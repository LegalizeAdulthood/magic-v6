/* Aed4.c -
 *
 * This file contains functions to manage the graphics tablet associated
 * with the Aed display.
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
static char rcsid[]="$Header: grAed4.c,v 6.0 90/08/28 18:40:32 mayo Exp $";
#endif  not lint

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grAedInt.h"
#include "txcommands.h"
#include "utils.h"

/* Library routines: */

extern char *fgets();
extern int sscanf();
extern int errno;

/* inports from other AED modules */
extern Void aedSetWMandC();

/* local static variables */
bool grAedMouseIsDisabled = FALSE;	/* TRUE if we had to disable the
					 * mouse (tablet) because of an error.
					 */
static Point grAedCursorPoint = {GR_CURSOR_X, GR_CURSOR_Y};
					/* The current location of the cursor.
					 * Set by button pushes and cursor
					 * position reports.
					 */

/* forward procedures */
extern bool aedReadInput();

/* Variable to keep track of whether or not the tablet is
 * enabled.
 */

bool grAedTabletOn = FALSE;


/*---------------------------------------------------------
 * AedEnableTablet:
 *	This routine enables the graphics tablet.
 *
 * Results: 
 *   	None.
 *
 * Side Effects:
 *	The tablet cursor is enabled on the Aed.  This will cause characters
 *	to be sent over the serial line whenever a button on the cursor is
 *	pushed.  
 *
 *	Design:
 *	Note:  the Aed really messes up with the cursor, because it
 *	continually overwrites CAP with cursor coordinates.  Thus the
 *	cursor must disabled before doing ANYTHING to the Aed.  Also,
 *	we have to set the write mask to all ones or else the Aed can't
 *	write the crosshair.
 *---------------------------------------------------------
 */

Void
AedEnableTablet()
{
    (void) aedSetWMandC(0377, aedColor);
    fputs("3:", grAedOutput);
    (void) fflush(grAedOutput);
    grAedTabletOn = TRUE;
}


/*---------------------------------------------------------
 * AedDisableTablet:
 *	This routine disables the graphics tablet so that other things may
 *	be done with the Aed.
 *
 *	NOTE:  This is guaranteed to be called before any graphics are
 *	drawn, and GrDisableTablet() will be called afterwards.
 *
 * Results:	
 *	None.
 *
 * Side Effects:	
 *	The tablet is disabled.
 *---------------------------------------------------------
 */

Void
AedDisableTablet()
{
    putc('3', grAedOutput);
    putc('\0', grAedOutput);
    (void) fflush(grAedOutput);
    aedCurx = aedCury = -1;
    grAedTabletOn = FALSE;
}



/*
 * ----------------------------------------------------------------------------
 * aedGetCursorPos:
 * 	Read the cursor position from the tablet.  This procedure should
 *	not be called unless the tablet is enabled:  it will return a
 *	bogus value.
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
aedGetCursorPos(p)
    Point *p;		/* point to be filled in with screen coordinates */
{
    ASSERT(grAedTabletOn, "Tried to read cursor without tablet on");

    if (grAedInput == NULL) return FALSE;

    fputs("G1DDDNj", grAedOutput);  /* Set encoding scheme to decimal, then 
				     * report cursor position.
				     */
    fputs("G18D8N", grAedOutput);   /* Reset encoding scheme. */
    (void) fflush(grAedOutput);

    /* Call aedReadInput to process all input from the device.  If it finds
     * any button pushes it will add them to Magic's event queue.  If it
     * finds an error it will disable the tablet.  It sets 'grAedCursorPoint'
     * when it learns of a new cursor position.
     */
    if (!aedReadInput(TRUE)) return FALSE;

    *p = grAedCursorPoint;
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 * grAedMouseDevice:
 *
 *	Read the input stream associated with the mouse, and create input
 *	event records to push into our input event queue.
 *
 * Results:
 *	returns a command.
 *
 * Side effects:
 *	reads the mouse.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
grAedMouseDevice(fd, cdata)
    int fd;
    ClientData cdata;
{
    ASSERT(fd == fileno(grAedInput), "grAedMouseDevice");
    if (grAedMouseIsDisabled) return;

    /* Call aedReadInput to process all input from the device.  If it finds
     * any button pushes it will add them to Magic's event queue.  If it
     * finds an error it will disable the tablet.
     */
    (void) aedReadInput(FALSE);
}

/*---------------------------------------------------------------------------
 * aedRecordButtons:
 *
 *	Record any buttons that have changed.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	May put events into the event queue.
 *
 *----------------------------------------------------------------------------
 */

void
aedRecordButtons(buttons)
    int buttons;
{
    static int oldButtons = 0;
    static int actualButtons = 0;
    TxInputEvent *event;
    int maskDiff;

    ASSERT(oldButtons == actualButtons, "aedRecordButtons");
    actualButtons = buttons;

    while (oldButtons != actualButtons) {
	maskDiff = (oldButtons ^ actualButtons); /* get the bits that changed */
	ASSERT(maskDiff != 0, "aedRecordButtons");
	event = TxNewEvent();
	/* Remove all but the last '1' bit -- see macro in utils.h */
	event->txe_button = LAST_BIT_OF(maskDiff);
	if ((actualButtons & event->txe_button) != 0)
	{
	    event->txe_buttonAction = TX_BUTTON_DOWN;
	    oldButtons |= event->txe_button;
	}
	else
	{                     
	    event->txe_buttonAction = TX_BUTTON_UP;
	    oldButtons &= ~(event->txe_button);
	}                     
	event->txe_p = grAedCursorPoint;
	event->txe_wid = WIND_UNKNOWN_WINDOW;
	TxAddEvent(event);
    }

}


/*
 * ----------------------------------------------------------------------------
 * aedReadInput:
 *	Reads information that has been sent from the tablet and records
 *	any button pushes and the cursor location.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Some input in the tablet input stream will be consumed.
 *	Button pushes found will be added to the input queue.
 *	grAedCursorPoint will be updated to relect the newest known cursor
 *	position.
 *
 * Design:
 *	This must be the only proc that reads from the graphics terminal, 
 *	because multiple readers won't be able to always predict what is
 *	coming back over the line.  For instance, if you ask the terminal
 *	for the cursor location the next thing back from the terminal may not
 *	be the cursor position report because the user might have hit a
 *	button at an inopportune moment.
 *	
 * ----------------------------------------------------------------------------
 */

bool
aedReadInput(needCursorReport)
    bool needCursorReport;	/* If TRUE, loop until the AED reports the 
			 	 * cursor position.
				 */
{
    int button, x, y;
    bool haveButtons;
    static char line1[20];
    int inbits, selres;
    struct timeval zeroTime;

    /* Loop until all input is gone. */
    zeroTime.tv_sec = 0;
    zeroTime.tv_usec = 0;
    while(TRUE)
    {
	inbits = (1 << fileno(grAedInput));
	selres = select(20, &inbits, (int *) NULL, (int *) NULL, &zeroTime);
	if (selres < 0)
	{
	    /* Got an error or an interrupt (signal) */
	    if (errno != EINTR)
	    {
		perror("magic");
		goto disable;
	    }
	    else
		continue;
	    
	}
	else if (selres == 0)
	{
	    if (!needCursorReport) return TRUE;
	}

	if (grFgets(line1, 20, grAedInput, "tablet") == NULL) goto eof;

	/*
	 * Format of button push:
	 *		:b\nx\ny\n      -- where b is a mask of the buttons
	 *				-- x and y are decimal integers
	 *
	 * Format of a cursor position report:
	 *		x\ny\n 		-- x and y are decimal integers
	 */
	haveButtons = (line1[0] == ':');	/* Which type of input? */
	if (!haveButtons) needCursorReport = FALSE; /* Got a cursor report. */
	if (haveButtons)
	{
	    /* Read the button mask. */
	    if (sscanf(line1, ":%d", &button) != 1) goto disable;
	    if (grFgets(line1, 20, grAedInput, "tablet") == NULL) goto eof;
	}

	/* Read the x position. */
	if (sscanf (line1, "%d", &x) != 1) goto disable;

	/* Read the y position. */
	if (grFgets(line1, 20, grAedInput, "tablet") == NULL) goto eof;
	if (sscanf (line1, "%d", &y) != 1) goto disable;

	if (haveButtons)
	{
	    /* If the cursor is off-screen, set it to the nearest edge
	     * of the screen (if it's off-screen, the coordinates get
	     * reported in tablet units rather than screen units, so this
	     * code translates them back to screen units).  See the AED manual
	     * for the 'DTM' command for details on the mapping function.
	     */
	    
	    /* NOTE:  If you have a GTCO bitpad you might want to mask out
	     * the high order bits via 'button = button % 16'.  If you do this
	     * on a bitpad then pointing off-screen will screw things up.
	     * (Thanks to John Charles Scott of the University of Iowa for the 
	     * GTCO changes.)
	     */
#ifdef	GTCO
	    button = button % 16;
#endif
	    if (button & 16)
	    {
		extern int aedDTMxscale, aedDTMyscale;
		extern int aedDTMxorigin, aedDTMyorigin;
		extern int aedCursOriginX, aedCursOriginY;

		button &= ~16;
		/* Adjust for tablet mapping, as well as the location of the
		 * hot-spot on the programmable cursor relative to the center of
		 * the cursor.
		 */
		x -= (8 - aedCursOriginX);
		x = ((x - aedDTMxorigin) * GrScreenRect.r_xtop)
		    / (65536 / aedDTMxscale);
		y -= (8 - aedCursOriginY);
		y = ((y - aedDTMyorigin) * GrScreenRect.r_ytop)
		    / (65536 / aedDTMyscale);

	    }
	    else
	    {
		/* There's a bug in the AED where when the cursor is just
		 * off-screen to the bottom, it reports it on-screen but too
		 * high.  This code compensates.
		 */
		 if (y > GrScreenRect.r_ytop) y = 0;
	    }
	}

	/* Move the cursor to the edge of the screen, if it's still
	 * off-screen.
	 */
	
	if (x < 0) x = 0;
	if (x > GrScreenRect.r_xtop) x = GrScreenRect.r_xtop;
	if (y < 0) y = 0;
	if (y > GrScreenRect.r_ytop) y = GrScreenRect.r_ytop;

	grAedCursorPoint.p_x = x;
	grAedCursorPoint.p_y = y;

	if (haveButtons)
	{
	    /* Is the button info valid? */
#ifdef	GTCO
	    button = button % 16;
#endif
	    if ((button >= 0) && (button <= 15))
	    {
		int buttonMask;

		buttonMask = 0;
		switch (aedButtonOrder) {
		    case BUT124: {
			if (button & 1) buttonMask |= TX_LEFT_BUTTON;
			if (button & 2) buttonMask |= TX_MIDDLE_BUTTON;
			if (button & 4) buttonMask |= TX_RIGHT_BUTTON;
			break;
			}
		    case BUT421: {
			if (button & 4) buttonMask |= TX_LEFT_BUTTON;
			if (button & 2) buttonMask |= TX_MIDDLE_BUTTON;
			if (button & 1) buttonMask |= TX_RIGHT_BUTTON;
			break;
			}
		    case BUT842: {
			if (button & 8) buttonMask |= TX_LEFT_BUTTON;
			if (button & 4) buttonMask |= TX_MIDDLE_BUTTON;
			if (button & 2) buttonMask |= TX_RIGHT_BUTTON;
			break;
			}
		    case BUT248: 
		    default: {
			if (button & 2) buttonMask |= TX_LEFT_BUTTON;
			if (button & 4) buttonMask |= TX_MIDDLE_BUTTON;
			if (button & 8) buttonMask |= TX_RIGHT_BUTTON;
			break;
			}
		}
		aedRecordButtons(buttonMask);
	    }
	    else
	    {
		goto disable;
	    }
	}
    }

eof:
    TxError("EOF enountered in tablet input stream.\n");
    /* Fall through to 'disable' */

disable:
    TxError("Error in reading tablet, tablet is now disabled.\n");
    TxDeleteInputDevice(1 << fileno(grAedInput));
    grAedMouseIsDisabled = TRUE;
    return FALSE;
}
