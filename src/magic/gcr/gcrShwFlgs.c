/* gcrShowFlags.c -
 *
 *	Code to highlight areas flagged by the router.
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
static char rcsid[] = "$Header: gcrShwFlgs.c,v 6.0 90/08/28 18:38:07 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "tile.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "gcr.h"
#include "doubleint.h"
#include "heap.h"
#include "router.h"
#include "main.h"
#include "styles.h"
#include "textio.h"

#define MAXFLAGS 17

char * GCRFlagNames[]=
    {"blkb", "blkm", "blkp", "cc",  "ce",  "r",  "tc", "te", "u",
     "v2",   "vl",   "vd",   "vm",  "vr",  "vu", "x",  "z",  0};

int GCRFlagValue[]=
    {   3,  1,     2,   256,  1024,     8,  128, 512, 4,
       64, 32, 32768,  2048,  8192, 16384,   16,  4096};

char * GCRFlagDescr[]={
	"3     Both layers are blocked",
	"1     Location is blocked with metal",
	"2     Location is blocked with poly",
	"256   Column contact needed",
	"1024  Column ends beyond this point",
	"8     Connect to the right",
	"128   Track contact needed",
	"512   Track ends right of this point",
	"4     Connect from track upwards",
	"64    Vacate track due to 2-layer obstacle",
	"32    Vacate track from left",
	"32768 Vacate track from down",
	"2048  Vertical poly changed to metal",
	"8192  Vacate track from right",
	"16384 Vacate track from up",
	"16    Metal/poly contact",
	"4096  Via not deleted"
    };

CellDef * gcrShowCell = (CellDef *) NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * gcrShow --
 *
 * 	Fields commands of the form :*seeflags <FIELD> to display the router
 *	flags in the channel under the point.  If no arguments, then turn
 *	off highlighting.
 *
 * Results:	None.
 *
 * Side effects:
 *	Displays various router flags as feedback.  Existing feedback
 *	is cleared.
 *
 * ----------------------------------------------------------------------------
 */

void
GCRShow(point, arg)
    Point   * point;
    char    * arg;
{
    GCRChannel * ch;
    HashEntry  * he;
    Rect	 box;
    int		 dx, dy, track, col, mask;
    short      * colBits;
    char	msg[100];
    Tile * tile;
    void	 gcrDumpChannel();

/* Figure out which channel is selected */

    if(RtrChannelPlane == NULL)
    {
	TxError("Sorry.  You must route before looking at flags!\n");
	return;
    }
    tile = TiSrPoint((Tile *) NULL, RtrChannelPlane, point);
    if(TiGetType(tile) != NULL)
    {
	TxError("Point to the channel you want to highlight.\n");
	return;
    }
    he = HashLookOnly(&RtrTileToChannel, (char *) tile);
    if(he == (HashEntry *) NULL)
    {
	TxError("No channel under point.  Have you already routed?\n");
	return;
    }
    ch = (GCRChannel *) HashGetValue(he);

/* Translate the command argument to the bit mask for the flag */
    mask = Lookup(arg, GCRFlagNames);
    if ( mask < 0 )
    {
	if(strcmp(arg, "dump") == 0)
	{
	    gcrDumpChannel(ch);
	    return;
	}
	if(strcmp(arg, "help") == 0)
	    TxError("Legal values are:\n");
	else
	if (mask == -1)
	    TxError("%s:  ambiguous.  Legal values are:\n", arg);
	else
	    TxError("%s:  not found.  Legal values are:\n", arg);
	for(col=0; col<MAXFLAGS; col++)
	    TxError("	%s	%s\n", GCRFlagNames[col], GCRFlagDescr[col]);
	return;
    }
    mask =  GCRFlagValue[mask];
    (void) sprintf(msg, "Channel flag \"%s\"", arg);

    if(ch->gcr_result == (short **) NULL)
    {
	TxError("Oops.  Somebody deleted the results array.\n");
	return;
    }

/* Scan the routing grid, create feedback areas for each grid location
 * where the mask bit is set.
 */

    dx = ch->gcr_origin.p_x - 2;
    for(col = 0; col <= ch->gcr_length; col++)
    {
	if((colBits = ch->gcr_result[col]) == (short *) NULL)
	{
	    TxError("Oops.  Result array column %d is missing.\n", col);
	    return;
	}
	dy = ch->gcr_origin.p_y - 2;
	for(track = 0; track <= ch->gcr_width; track++)
	{
	    if((colBits[track] & mask) == mask)
	    {
		box.r_xbot = dx;
		box.r_ybot = dy;
		box.r_xtop = dx + RtrGridSpacing;
		box.r_ytop = dy + RtrGridSpacing;
		DBWFeedbackAdd(&box, msg, EditCellUse->cu_def, 1,
		    STYLE_PALEHIGHLIGHTS);
	    }
	    dy += RtrGridSpacing;
	}
	dx += RtrGridSpacing;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrDumpChannel --
 *
 * 	Create a file called "channel.XXXX" where XXXX is the hex address
 *	of the channel.  The file contains the routing problem for the
 *	channel, complete with obstacles.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a file.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrDumpChannel(ch)
    GCRChannel * ch;
{
    char name[20];
    int track, col, netCount = 0, gcrNetName();
    short res;
    GCRNet * net, * netNames[500];
    FILE * fp, * fopen();

    netNames[0]=(GCRNet *) 0;
    (void) sprintf(name, "channel.%x", ch);
    if((fp = fopen(name, "w")) == NULL)
    {
	TxError("Can't open file %s to dump channel.\n", name);
	return;
    }
    (void) fprintf(fp, "* %d %d\n", ch->gcr_width, ch->gcr_length);
    for(track=1; track<=ch->gcr_width; track++)
    {
	net = ch->gcr_lPins[track].gcr_pId;
	(void) fprintf(fp, "%4d", gcrNetName(netNames, &netCount, net));
    }
    (void) fprintf(fp, "\n");
    for(col=1; col<=ch->gcr_length; col++)
    {
	net = ch->gcr_bPins[col].gcr_pId;
	(void) fprintf(fp, "%4d", gcrNetName(netNames, &netCount, net));
	for(track=1; track<=ch->gcr_width; track++)
	{
	    res = ch->gcr_result[col][track];
	    if((res & GCRBLKM) && (res & GCRBLKP))
		(void) fprintf(fp, "  X");
	    else
	    if(res & GCRBLKM)
		(void) fprintf(fp, "  M");
	    else
	    if(res & GCRBLKP)
		(void) fprintf(fp, "  P");
	    else
		(void) fprintf(fp, "  .");
	}
	net = ch->gcr_tPins[col].gcr_pId;
	(void) fprintf(fp, "%4d", gcrNetName(netNames, &netCount, net));
	(void) fprintf(fp, "\n");
    }
    for(track=1; track<=ch->gcr_width; track++)
    {
	net = ch->gcr_rPins[track].gcr_pId;
	(void) fprintf(fp, "%4d", gcrNetName(netNames, &netCount, net));
    }
    (void) fprintf(fp, "\n");
}

int
gcrNetName(netNames, netCount, net)
    GCRNet * netNames[];
    int * netCount;
    GCRNet * net;
{
    int i;
    for(i=0; i<= *netCount; i++)
	if(netNames[i]==net) return(i);
    *netCount = *netCount + 1;
    netNames[*netCount]=net;
    return(*netCount);
}
