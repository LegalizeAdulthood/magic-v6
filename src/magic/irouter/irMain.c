/*
 * irMain.c --
 *
 * Global data, and initialization code for the irouter.
 *
 * OTHER ENTRY POINTS FOR MODULE  (not in this file):
 *     `:iroute' command  - IRCommand() in irCommand.c
 *     `:*iroute' command - IRTest() in irTestCmd.c
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1987, 1990 Michael H. Arnold, Walter S. Scott, and  *
 *     * the Regents of the University of California.                      *
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
static char rcsid[] = "$Header: irMain.c,v 6.1 90/08/28 19:23:56 mayo Exp $";
#endif  not lint

/*--- includes --- */

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "drc.h"
#include "select.h"
#include "signals.h"
#include "textio.h"
#include "windows.h"
#include "dbwind.h"
#include "styles.h"
#include "debug.h"
#include "undo.h"
#include "txcommands.h"
#include "malloc.h"
#include "list.h"
#include "doubleint.h"
#include "../mzrouter/mzrouter.h"
#include "irouter.h"
#include "irInternal.h"

/*------------------------------ Global Data ------------------------------*/

/* Debug flags */
ClientData irDebugID;
int irDebEndPts;	/* values identify flags to debug module */
int irDebNoClean;
int irRouteWid = -1;	/* if >=0, wid of window to use for determining
			 * subcell expansion, and route cell.
			 */

MazeParameters *irMazeParms;  /* parameter settings passed to maze router */

/* the following RouteEtc pointers should have the same value as the
 * corresponding fileds in irMazeParms.  They exist for historical
 * reasons.
 */
RouteLayer *irRouteLayers;   
RouteContact *irRouteContacts;
RouteType *irRouteTypes;


/*
 * ----------------------------------------------------------------------------
 *
 * IRInit --
 *
 * This procedure is called when Magic starts up, after 
 * technology initialization.  NOTE: MZInit() MUST BE CALLED PRIOR TO IRInit()
 * to complete setup of maze routing parameters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Register ourselves with debug module
 *	Setup some internal datastructures.
 *	
 * ----------------------------------------------------------------------------
 */

Void
IRInit()
{
    int n;
    /* debug struct */
    static struct
    {
	char	*di_name;
	int	*di_id;
    } dflags[] = {
	"endpts",	&irDebEndPts,
	"noclean",	&irDebNoClean,
	0
    };

   /* Register with debug module */
    irDebugID = DebugAddClient("irouter", sizeof dflags/sizeof dflags[0]);
    for (n = 0; dflags[n].di_name; n++)
	*(dflags[n].di_id) = DebugAddFlag(irDebugID, dflags[n].di_name);

    /* Initialize the irouter maze parameters with a copy of the "irouter"
     * style (default) parameters.
     */
    irMazeParms = MZCopyParms(MZFindStyle("irouter"));
    if(irMazeParms==NULL)
    {
	return;
    }

    /* set global type lists from current irouter parms.
     * These lists are often referenced directly rather than through
     * the parameter structure for historical reasons.
     */
    irRouteLayers = irMazeParms->mp_rLayers;
    irRouteContacts = irMazeParms->mp_rContacts;
    irRouteTypes = irMazeParms->mp_rTypes;

    return;
}
