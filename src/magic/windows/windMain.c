/* windMain.c -
 *
 *	This package implements overlapping windows for the
 *	Magic VLSI layout system.
 *
 * Design:
 *	Windows are structures that are kept in a doubly linked list.
 *	Windows near the front of the list are on top of windows further
 *	towards the tail of the list.  Each window has some information
 *	about what is in the window, as well as the size of the window
 *	(both unclipped and clipped to accomodate windows that overlay it).
 *	Transforms control what portion of the window's contents show up
 *	on the screen, and at what magnification.
 *
 *	Each window is owned by a client (the database, a menu package, etc.).
 *	The window package is notified of a new client by the AddClient
 *	call.  The client supplies routines to redisplay the contents of
 *	a window, and to do other things with the window (such as delete it).
 *	Each window is a view onto a surface maintained by the client.  The
 *	client may be asked to redisplay any part of this surface at any time.
 *	The client must also supply each window with an ID that uniquely
 *	identifies the surface.
 *
 *	There are currently two types of window packages supported:  Magic
 *	windows (implemented here) and Sun Windows.  In Magic windows, all
 * 	windows use the screen's coordinate system.  In Sun Windows, each
 *	window has it's own coordinate system with (0, 0) being at the lower
 *	left corner.  Also, under Sun Windows some of the screen managment
 *	stuff (such as clipping to obscuring areas and drawing of the
 *	screen background color) is ignored by us.
 *
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
static char rcsid[]="$Header: windMain.c,v 6.0 90/08/28 19:02:21 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "glyphs.h"
#include "windows.h"
#include "windInt.h"
#include "stack.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "textio.h"
#include "graphics.h"
#include "malloc.h"

/* The type of windows that this package will implement */
int WindPackageType = WIND_MAGIC_WINDOWS;

/* The size of our scroll bars -- may be set externally (see windows.h)
 */
int WindScrollBarWidth = 7;

/* ------ Internal variables that are global within the window package ----- */
clientRec *windFirstClientRec = NULL;	/* the head of the linked list 
					 * of clients 
					 */
Window *windTopWindow = NULL;		/* the topmost window */
Window *windBottomWindow = NULL;	/* ...and the bottom window */
extern Plane *windRedisplayArea;	/* See windDisplay.c for details. */


/*
 * ----------------------------------------------------------------------------
 * WindInit --
 *
 *	Initialize the window package.  No windows are created, but the
 *	package will be initialized so that it can do these things in the 
 *	future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables internal to the window package are initialized.
 * ----------------------------------------------------------------------------
 */

void
WindInit()
{
    extern Stack *windGrabberStack;
    extern GrGlyphs *windGlyphs;
    Rect ts;
    char glyphName[30];

    windClientInit();
    windGrabberStack = StackNew(2);
    windRedisplayArea = DBNewPlane((ClientData) TT_SPACE);

    sprintf(glyphName, "windows%d", WindScrollBarWidth);
    if (!GrReadGlyphs(glyphName, ".", SysLibPath, &windGlyphs))
	MainExit(1);
    GrTextSize("XWyqP", GR_TEXT_DEFAULT, &ts);
    windCaptionPixels = ts.r_ytop - ts.r_ybot + 3;
    WindAreaChanged((Window *) NULL, (Rect *) NULL);
}


/*
 * ----------------------------------------------------------------------------
 * WindAddClient --
 *
 *	Add a new client of the window package.  The client must supply a
 *	set of routines, as described below.
 *
 * Results:
 *	A unique ID (of type WindClient) is returned.
 *	This is used to identify the client in future calls to the window 
 *	package.
 *
 * Routines supplied:
 *
 *	( A new window was just created for this client.  Do things to
 *	  initialize the window, such as filling in the caption and making
 *	  the contents be empty. The client may refuse to create a new
 *	  window by returning FALSE, otherwise the client should return
 *	  TRUE.  The client will get passed argc and argv, with the command
 *	  name stripped off.  The client may do whatever it wants with this.
 *	  It may even modify parts of the window record -- such as changing
 *	  the window's location on the screen.)
 *
 *	bool
 *	create(w, argc, argv)
 *	    Window *w;
 *	    int argc;
 *	    char *argv[];
 *	{
 *	}
 *
 *	( One of the client's windows is about to be deleted.  Do whatever
 *	  needs to be done, such as freeing up dynamically allocated data
 *	  structures. Fields manipulated by the window package, such as
 *	  the caption, should not be freed by the client.  The client should
 *	  normally return TRUE.  If the client returns FALSE, the window
 *	  manager will refuse the request to delete the window.)
 *
 *	bool
 *	delete(w)
 *	    Window *w;
 *	{
 *	}
 *
 *	( Redisplay an area of the screen.  The client is passed the window,
 *	  the area in his coordinate system, and a clipping rectangle in
 *	  screen coordinates. )
 *
 *	redisplay(w, clientArea, screenArea)
 *	    Window *w;
 *	    Rect *clientArea, *screenArea;
 *	{
 *	}
 *
 *
 *	( The window is about to be moved or resized.  This procedure will
 *	  be called twice.  
 *
 *	  The first time (with 'final' == FALSE), the window 
 *	  will be passed in 'w' as it is now and a suggested new w_screenarea 
 *	  is passed in 'newpos'.  The client is free to modify 'newpos' to
 *	  be whatever screen location it desires.  The routine should not 
 *	  pass 'w' to any window procedure such as windMove since 'w' has
 *	  the old transform, etc. instead of the new one.
 *
 *	  On the second call ('final' == TRUE), the window 'w' has all of
 *	  it's fields updated, newpos is equal to w->w_frameArea, and the
 *	  client is free to do things like windMove which require a window
 *	  as an argument.  It should not modify newpos.
 *
 *	reposition(w, newpos, final)
 *	    Window *w;
 *	    Rect *newpos	-- new w_framearea (screen area of window)
 *	    bool final;
 *	{
 *	}
 *
 *
 *	( A command has been issued to this window.  The client should
 *	  process it.  It is split into Unix-style argc and argv words. )
 *
 *	command(w, cmd)
 *	    Window *w;
 *	    TxCommand *cmd;
 *	{
 *	}
 *
 *	( A command has just finished.  Update any screen info that may have
 *	  been changed as a result. )
 *
 *	update()
 *	{
 *	}
 *
 * Side effects:
 *	Internal tables are expanded to include the new client.
 * ----------------------------------------------------------------------------
 */

WindClient
WindAddClient(clientName, create, delete, redisplay, command, update, 
exitproc, reposition, table, icon)
    char *clientName;		/* A textual name for the client.  This
				 * name will be visable in the user
				 * interface as the name to use to switch
				 * a window over to a new client
				 */
    Void (*create)();
    bool (*delete)();
    Void (*redisplay)();
    Void (*command)();
    Void (*update)();
    Void (*exitproc)();
    Void (*reposition)();
    char *(table[]);		/* A table of commands that this client
				 * is willing to accept.
				 */
    GrGlyph *icon;		/* An icon to draw when the window is closed.
				 * (currently for Sun Windows only).
				 */
{
    clientRec *res;

    ASSERT( (clientName != NULL), "WindAddClient");
    ASSERT( (command != NULL), "WindAddClient");
    ASSERT( (table != NULL), "WindAddClient");

    res = (clientRec *) mallocMagic(sizeof(clientRec));
    res->w_clientName = clientName;
    res->w_create = create;
    res->w_delete = delete;
    res->w_redisplay = redisplay;
    res->w_command = command;
    res->w_update = update;
    res->w_exit = exitproc;
    res->w_reposition = reposition;
    res->w_commandTable = table;
    res->w_icon = icon;
    res->w_nextClient = windFirstClientRec;
    windFirstClientRec = res;

    return (WindClient) res;
}


/*
 * ----------------------------------------------------------------------------
 * WindGetClient --
 *
 *	Looks up the unique ID of a client of the window package.
 *
 * Results:
 *	A variable of type WindClient is returned if the client was found,
 *	otherwise NULL is returned.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

WindClient
WindGetClient(clientName)
    char *clientName;	/* the textual name of the client */
{
    clientRec *cr, *found;
    int length;

    /* Accept any unique abbreviation. */

    found = NULL;
    length = strlen(clientName);
    for (cr = windFirstClientRec; cr != (clientRec *) NULL; 
	    cr = cr->w_nextClient)
    {
	if (strncmp(clientName, cr->w_clientName, length) == 0)
	{
	    if (found != NULL) return NULL;
	    found = cr;
	}
    }

    return (WindClient) found;
}

/*
 * ----------------------------------------------------------------------------
 * WindPrintClientList --
 *
 *	Print the name of each client of the window package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

WindPrintClientList(wizard)
    bool wizard;	/* If true print the names of ALL clients, even those
			 * that don't have user-visable windows */
{
    clientRec *cr;

    for (cr = windFirstClientRec; cr != (clientRec *) NULL; 
	    cr = cr->w_nextClient) {
	if (wizard || (cr->w_clientName[0] != '*'))
	    TxError("	%s\n", cr->w_clientName);
	}
}
