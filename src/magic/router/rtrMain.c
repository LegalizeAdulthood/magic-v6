/* rtrMain.c -
 *
 *	Top level routing code, and glue that doesn't belong elsewhere.
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
static char rcsid[]="$Header: rtrMain.c,v 6.0 90/08/28 18:55:51 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "doubleint.h"
#include "heap.h"
#include "tile.h"
#include "database.h"
#include "gcr.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "signals.h"
#include "rtrDcmpose.h"
#include "netmenu.h"
#include "router.h"
#include "grouter.h"
#include "netlist.h"
#include "textio.h"
#include "netmenu.h"
#include "runstats.h"

extern int rtrMakeChannel();

/*
 * The origin point for the routing grid.
 * Set by the "route origin" command.
 */
Point RtrOrigin = { 0, 0 };

/*
 * ----------------------------------------------------------------------------
 *
 * Route --
 *
 * Top level procedure for the routing code.
 *
 * Route all channels in the cell routeUse->cu_def, based on net lists
 * in the net list hash table.  There currently is no check that the net
 * list belongs to the edit cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for channels is allocated and set.  The channel structure is
 *	kept around to allow flags to be examined and displayed on the screen.
 *	Leaves paint in routeUse->cu_def.
 *
 * ----------------------------------------------------------------------------
 */

void
Route(routeUse, routeArea)
    CellUse *routeUse;
    Rect *routeArea;
{
    CellDef *channelDef;
    int errs, numNets;
    NLNetList netList;
    char *netListName;

    /* Read the netlist into an internal NLNetList structure */
    if (!NMHasList())
    {
	netListName = routeUse->cu_def->cd_name;
	TxPrintf("No netlist selected yet;  using \"%s\".\n", netListName);
	NMNewNetlist(netListName);
    }
    else netListName = NMNetlistName();
    RtrMilestoneStart("Building netlist");
    numNets = NLBuild(routeUse, &netList);
    RtrMilestoneDone();
    if (numNets == 0)
    {
	TxError("No nets to route.\n");
	return;
    }

    /*
     * Create a plane whose space tiles correspond to channels.
     * See the comments in RtrDecompose() for details.
     */
    RtrMilestoneStart("Channel decomposition");
    channelDef = RtrDecompose(routeUse, routeArea, &netList);
    RtrMilestoneDone();
    if (channelDef == NULL)
    {
	TxError("Routing area (box) is too small to be of any use.\n");
	goto done;
    }
    RtrChannelPlane = channelDef->cd_planes[PL_DRC_ERROR];

    /*
     * Enumerate all space tiles in the channel plane, generating a
     * channel for each one.  Initialize each channel's dimensions,
     * but don't do anything else.
     */
    RtrChannelList = (GCRChannel *) NULL;
    (void) TiSrArea((Tile *) NULL, RtrChannelPlane, &RouteArea,
	    rtrMakeChannel, (ClientData) &RouteArea);
    if (!SigInterruptPending)
    {
	errs = GARoute(RtrChannelList, routeUse, &netList);
	if (errs == 0)
	    TxPrintf("No routing errors.\n");
	else if (errs == 1)
	    TxPrintf("There was one routing error:  see feedback.\n");
	else TxPrintf("There were %d routing errors:  see feedback.\n", errs);
    }

done:
    /* Clean up global routing information */
    NLFree(&netList);
}

/*
 * ----------------------------------------------------------------------------
 *
 * rtrMakeChannel --
 *
 * Function passed to TiSrArea to enumerate space tiles and convert them
 * to channels.  Clip all tiles against the box 'clipBox'.  Don't set
 * hazards for this channel; that has to happen after stem assignment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hashes space tile pointers from the channel cell's drc error plane
 *	to get a pointer to a malloc'ed channel structure.
 *
 *	Prepends this channel to the list being built in RtrChannelList.
 *
 * ----------------------------------------------------------------------------
 */

int
rtrMakeChannel(tile, clipBox)
    Tile *tile;		/* Potential channel tile; we create a channel whose
			 * area is equal to that of this tile if the type of
			 * this tile is TT_SPACE.
			 */
    Rect *clipBox;	/* If non-NULL, clip the channel area to this box */
{
    int length, width;
    HashEntry *entry;
    GCRChannel *ch;
    Point origin;
    Rect bbox;

    if (SigInterruptPending) return (1);
    if (TiGetBody(tile) != NULL) return (0);

    entry = HashFind(&RtrTileToChannel, (char *) tile);
    ASSERT(HashGetValue(entry) == (char *) 0, "rtrMakeChannel");
    TITORECT(tile, &bbox);
    GeoClip(&bbox, clipBox);

    /*
     * Figure out how many columns and rows will fit in the area,
     * then create and initialize a channel.
     */
    RtrChannelBounds(&bbox, &length, &width, &origin);
    ch = GCRNewChannel(length, width);
    ch->gcr_area = bbox;
    ch->gcr_origin = origin;
    ch->gcr_type = CHAN_NORMAL;

    /* Remember that we've processed it */
    HashSetValue(entry, (char *) ch);

    /* Prepend to RtrChannelList */
    ch->gcr_next = RtrChannelList;
    RtrChannelList = ch;

    return(0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * RtrRunStats --
 * RtrMilestoneStart --
 * RtrMilestoneDone --
 * RtrMilestonePrint --
 *
 * Miscellaneous procedures for debugging/timing.
 * Calling RtrRunStats() prints out the time since the last call
 * and the total amount of memory used in the heap.  The procedures
 * RtrMilestoneStart() and RtrMilestoneDone() should be used to bracket
 * calls to the various pieces of the router: the former announces
 * that the router is about to enter a given section, and the latter
 * says it is done and tells how long it took.  During a given section,
 * RtrMilestonePrint() will print '#' if no errors have occurred since
 * the last call; otherwise, it will print a '!'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output to terminal.
 *
 * ----------------------------------------------------------------------------
 */

    /* The following are both set by RtrMilestoneStart() */
static char *rtrMilestoneName;	/* Name of the current section */
static struct tms rtrStartTime;	/* Starting time of the current section */
static int rtrFeedCount;	/* Most recent # of feedback areas */

Void
RtrRunStats()
{
    char *RunStats();
    static struct tms last, delta;

    TxPrintf("%s\n", RunStats(RS_TINCR|RS_MEM, &last, &delta));
    TxFlush();
}

RtrMilestoneStart(event)
    char *event;
{
    rtrMilestoneName = event;
    TxPrintf("%s: ", event);
    TxFlush();
    (void) times(&rtrStartTime);
    rtrFeedCount = DBWFeedbackCount;
}

RtrMilestoneDone()
{
    struct tms tend;

    times(&tend);
    TxPrintf("\n%s time: %.1fu %.1fs\n", rtrMilestoneName,
		(tend.tms_utime - rtrStartTime.tms_utime) / 60.0,
		(tend.tms_stime - rtrStartTime.tms_stime) / 60.0);
}

RtrMilestonePrint()
{
    TxPrintf("%c", (DBWFeedbackCount > rtrFeedCount) ? '!' : '#');
    TxFlush();
    rtrFeedCount = DBWFeedbackCount;
}
