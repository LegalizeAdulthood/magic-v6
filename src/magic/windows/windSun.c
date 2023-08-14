/* windClient.c -
 *
 *	Send button pushes and commands to the window's command
 *	interpreters.
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
static char rcsid[]="$Header: windSun.c,v 6.0 90/08/28 19:02:28 mayo Exp $";
#endif  not lint

#ifdef	SUNVIEW

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/menu.h>
#include <suntool/wmgr.h>
#undef bool
#define	Rect	MagicRect	/* Sun-3 brain death */

#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "glyphs.h"
#include "windInt.h"
#include "utils.h"
#include "graphics.h"
#include "txcommands.h"
#include "grSunInt.h"

 
 
/*
 * ----------------------------------------------------------------------------
 * windSunUnder --
 *
 *	Move a window underneath all the others.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

Void
windSunUnder(w)
    Window *w;
{
    grSunRec *grdata;

    grdata = (grSunRec *) w->w_grdata;
    ASSERT(grdata != NULL, "windSunUnder");
    wmgr_bottom(grdata->gr_fd, fileno(grSunRootWindow));
}
 
 
/*
 * ----------------------------------------------------------------------------
 * windSunOver --
 *
 *	Move a window on top of all the others.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

Void
windSunOver(w)
    Window *w;
{
    grSunRec *grdata;

    grdata = (grSunRec *) w->w_grdata;
    ASSERT(grdata != NULL, "windSunOver");
    wmgr_top(grdata->gr_fd, fileno(grSunRootWindow));
}
 
/*
 * ----------------------------------------------------------------------------
 * windSunWindowHasMoved --
 *
 *	A window has moved on the Sun -- update our records.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May modify w->w_frameArea, w->w_allArea and w->w_screenArea by
 *	calling WindReframe().
 * ----------------------------------------------------------------------------
 */

Void
windSunWindowHasMoved(w, slide)
    Window *w;
    bool slide;		/* Slide the window's coordinate system too? */
{
    struct rect sunRect;
    grSunRec *grdata;
    Rect frameRect;

    grdata = (grSunRec *) w->w_grdata;
    ASSERT(grdata != NULL, "windSunWindowHasMoved");

    /* Let Magic do the move so that it knows about things */
    win_getrect(grdata->gr_fd, &sunRect);
    frameRect.r_xbot = XBOT(&sunRect);
    frameRect.r_ybot = INVERTY(YBOT(&sunRect));
    frameRect.r_xtop = XTOP(&sunRect);
    frameRect.r_ytop = INVERTY(YTOP(&sunRect));
    WindReframe(w, &frameRect, FALSE, slide);
}
 
 
/*
 * ----------------------------------------------------------------------------
 * windSunSetPosition --
 *
 *	Set the position of a window on the Sun screen according to
 *	w->w_frameArea.  Nothing in the window is modified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves the window so that it matches w->w_frameArea.
 * ----------------------------------------------------------------------------
 */

Void
windSunSetWindowPosition(w)
	Window *w;
{
    struct rect new, orig;
    grSunRec *grdata;

    grdata = (grSunRec *) w->w_grdata;
    ASSERT(grdata != NULL, "windSunSetWindowPosition");

    win_getrect(grdata->gr_fd, &orig);
    new.r_left = w->w_frameArea.r_xbot;
    new.r_width = (w->w_frameArea.r_xtop - w->w_frameArea.r_xbot + 1);
    new.r_top = INVERTY(w->w_frameArea.r_ytop);
    new.r_height = (w->w_frameArea.r_ytop - w->w_frameArea.r_ybot + 1);

    wmgr_completechangerect(grdata->gr_fd, &new, &orig, 0, 0);
    /*** win_setrect(grdata->gr_fd, &sr); ****/
}

/*
 * ----------------------------------------------------------------------------
 * windSunTopMenu --
 *
 *	Handle the top menu actions for a Sun Window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May move a window or do other things to it.
 * ----------------------------------------------------------------------------
 */

Void
windSunTopMenu(w, event, grdata)
    Window *w;
    struct inputevent *event;
    grSunRec *grdata;
{
#define NUM_TOPMENU1	7
#define NUM_TOPMENU2	2
    static struct menu topmenu1 = {MENU_IMAGESTRING, "Tool Mgr", 
	NUM_TOPMENU1, NULL, NULL, NULL};
    static struct menu topmenu2 = {MENU_IMAGESTRING, "Magic", 
	NUM_TOPMENU2, NULL, NULL, NULL};
    static struct menuitem topitems1[NUM_TOPMENU1] = {
	{MENU_IMAGESTRING, "?", (caddr_t) 1},
	{MENU_IMAGESTRING, "Move", (caddr_t) 2},
	{MENU_IMAGESTRING, "Stretch", (caddr_t) 3},
	{MENU_IMAGESTRING, "Expose", (caddr_t) 4},
	{MENU_IMAGESTRING, "Hide", (caddr_t) 5},
	{MENU_IMAGESTRING, "Redisplay", (caddr_t) 6},
	{MENU_IMAGESTRING, "Quit", (caddr_t) 7} };
    static struct menuitem topitems2[NUM_TOPMENU2] = {
	{MENU_IMAGESTRING, "?", (caddr_t) 8},
	{MENU_IMAGESTRING, "View", (caddr_t) 9} };
    static struct menu *topmenu = NULL;
    static char **openClose, **growShrink;
    struct menuitem *item;
    bool iconic;
    
    iconic = ((w->w_flags & WIND_ISICONIC) != 0);

    if (topmenu == NULL) {
	/* initalize */
	openClose = &(topitems1[0].mi_imagedata);
	growShrink = &(topitems2[0].mi_imagedata);
	topmenu1.m_items = topitems1;
	topmenu1.m_next = &topmenu2;
	topmenu2.m_items = topitems2;
	topmenu2.m_next = NULL;
	topmenu = &topmenu1;
    }

    if (iconic) {
	*openClose = "Open";
	topmenu1.m_next = NULL;
	topmenu2.m_next = NULL;
	topmenu = &topmenu1;
    }
    else {
	*openClose = "Close";
	if (w->w_flags & WIND_FULLSCREEN)
	    *growShrink = "Shrink";
	else
	    *growShrink = "Grow";
	if (topmenu->m_next == NULL) {
	    topmenu1.m_next = &topmenu2;
	    topmenu2.m_next = NULL;
	    topmenu = &topmenu1;
	}
    }

    item = menu_display(&topmenu, event, grdata->gr_fd);

    if (item != NULL) {
	switch (item->mi_data) {
	    case 1: {
		if (iconic) {
		    /* open */
		    wmgr_open(grdata->gr_fd, fileno(grSunRootWindow)); 
		    w->w_flags &= ~WIND_ISICONIC;
		    windSunWindowHasMoved(w, FALSE);
		} else {
		    /* close */
		    wmgr_close(grdata->gr_fd, fileno(grSunRootWindow)); 
		    w->w_flags |= WIND_ISICONIC;
		    windSunWindowHasMoved(w, FALSE);
		}
		break;
	    };
	    case 2: {
		/* move */
		GrEnableTablet();
		wmgr_move(grdata->gr_fd); 
		windSunWindowHasMoved(w, FALSE);
		break;
	    };
	    case 3: {
		/* stretch */
		GrEnableTablet();
		wmgr_stretch(grdata->gr_fd); 
		windSunWindowHasMoved(w, FALSE);
		WindAreaChanged(w, &w->w_allArea);
		break;
	    };
	    case 4: {
		wmgr_top(grdata->gr_fd, fileno(grSunRootWindow)); 
		break;
	    };
	    case 5: {
		wmgr_bottom(grdata->gr_fd, fileno(grSunRootWindow)); 
		break;
	    };
	    case 6: {
		wmgr_refreshwindow(grdata->gr_fd, fileno(grSunRootWindow)); 
		break;
	    };
	    case 7: { /* quit (this window only) */
		if (wmgr_confirm(grdata->gr_fd,
		    "Press the left button to confirm destruction of this window.  To cancel, press the right button now.  In either case Magic will continue to run.") == 0) 
		    break;
		(void) WindDelete(w);
		break;
	    };
	    case 8: { /* grow */
		WindFullScreen(w);
		break;
	    };
	    case 9: { /* view */
		WindView(w);
		break;
	    };
	}
    };
	
}

 
/*
 * ----------------------------------------------------------------------------
 * windSunHandleCaptionInput --
 *
 *	Handle the commands sent to the top of a Sun Window
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

Void
windSunHandleCaptionInput(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern int TxCurButtons;

    if (cmd->tx_buttonAction == TX_BUTTON_DOWN) {
	struct inputevent ourEvent;
	struct timezone tz;
	grSunRec *grdata;
	
	grdata = (grSunRec *) w->w_grdata;
	ASSERT(grdata != NULL, "windSunHandleInput");

	/* Construct an input event, for wmgr_changrect */
        switch (cmd->tx_button) {
            case TX_LEFT_BUTTON: {ourEvent.ie_code = MS_LEFT; break;}
            case TX_MIDDLE_BUTTON: {ourEvent.ie_code = MS_MIDDLE; break;}
            case TX_RIGHT_BUTTON: {ourEvent.ie_code = MS_RIGHT; break;}
        }
	ourEvent.ie_flags = 0;
	ourEvent.ie_shiftmask = 0;
        ourEvent.ie_locx = cmd->tx_p.p_x;
        ourEvent.ie_locy = w->w_allArea.r_ytop - cmd->tx_p.p_y;
	gettimeofday(&ourEvent.ie_time, &tz);

	switch(cmd->tx_button) {
	    case TX_RIGHT_BUTTON: {
		/* menu */
		windSunTopMenu(w, &ourEvent, grdata);
		break;
	    };
	    case TX_MIDDLE_BUTTON: {
		/* move */
		GrEnableTablet();
		wmgr_changerect(grdata->gr_fd, grdata->gr_fd, &ourEvent, 
		    TRUE, TRUE);
		windSunWindowHasMoved(w, FALSE);
		break;
	    }
	    case TX_LEFT_BUTTON: {
		if (wmgr_iswindowopen(grdata->gr_fd)) {
		    /* expose */
		    wmgr_top(grdata->gr_fd, fileno(grSunRootWindow));
		} else {
		    /* open */
		    wmgr_open(grdata->gr_fd, fileno(grSunRootWindow)); 
		    w->w_flags &= ~WIND_ISICONIC;
		    windSunWindowHasMoved(w, FALSE);
		}
		break;
	    }
	}
    }
    /* The following is a hack, caused by the fact that the above commands
     * read directly from Sun's event queue.  Consequently, we will still 
     * think that the button is down when in reality it is up.
     */
    TxReleaseButton(cmd->tx_button);
    WindOldButtons &= ~(cmd->tx_button);
    WindNewButtons = WindOldButtons;
}

#endif	SUNVIEW

