
#ifndef lint
static char rcsid[] = "$Header: ResJunct.c,v 6.0 90/08/28 18:54:31 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifdef	SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "malloc.h"
#include "textio.h"
#include "extract.h"
#include "extractInt.h"
#include "windows.h"
#include "dbwind.h"
#include "stack.h"
#include "tech.h"
#include "txcommands.h"
#include	"resis.h"



/*
 *-------------------------------------------------------------------------
 *
 * ResNewSDTransistor-- called when a transistor is reached via a piece of
 *			 diffusion. (Transistors  reached via poly, i.e.
 *			 gates, are handled by ResEachTile.)
 *
 * Results:none
 *
 * Side Effects: determines to which terminal (source or drain) node 
 * is connected. Makes new node if node hasn't already been created .
 * Allocates breakpoint in current tile for transistor.
 *
 *-------------------------------------------------------------------------
 */
ResNewSDTransistor(tile,tp,xj,yj,direction,PendingList)
	Tile 		*tile,*tp;
	int 		xj,yj,direction;
	resNode		**PendingList;
{
	resNode		*resptr;
	resTransistor	*resFet;
	tElement	*tcell;
	int		newnode;
	tileJunk	*j;
	
	newnode = FALSE;
	j = (tileJunk *) tp->ti_client;
	resFet = j->transistorList;
	if ((j->sourceEdge & direction) != NULL)
	{
	     if (resFet->rt_source == (resNode *) NULL)
		 {
		     MALLOC(resNode *,resptr,sizeof(resNode));
		     newnode = TRUE;
		     resFet->rt_source = resptr;
		 }
		 else
		 {
		      resptr = resFet->rt_source;
		 }
	}
	else
	{
	     if (resFet->rt_drain == (resNode *) NULL)
		 {
		     MALLOC(resNode *,resptr,sizeof(resNode));
		     newnode = TRUE;
		     resFet->rt_drain = resptr;
		 }
		 else
		 {
		      resptr = resFet->rt_drain;
		 }
	}
	if (newnode)
	{
	     MALLOC(tElement *,tcell,sizeof(tElement));
	     tcell->te_nextt = NULL;
	     tcell->te_thist = j->transistorList;
	     InitializeNode(resptr,xj,yj,RES_NODE_TRANSISTOR);
	     resptr->rn_te = tcell;
	     ResAddToQueue(resptr,PendingList);
	}
	NEWBREAK(resptr,tile,xj,yj,NULL);
}

/*
 *-------------------------------------------------------------------------
 *
 * ResProcessJunction-- Called whenever a tile  connecting to the tile being
 *	worked on is found. If a junction is already present, its address is
 *      returned. Otherwise, a new junction is made. 
 *
 * Results: Returns the address of the junction between the two tiles.
 *
 * Side Effects: Junctions may be created.
 *
 *-------------------------------------------------------------------------
 */
ResProcessJunction(tile,tp,xj,yj,NodeList)
	Tile 	*tile,*tp;
	int	xj,yj;
	resNode		**NodeList;

{
	ResJunction  	*junction;
	resNode	     	*resptr;
	jElement     	*jcell;
	tileJunk	*j0 = (tileJunk *)tile->ti_client;
	tileJunk	*j2 = (tileJunk *)tp->ti_client;

#ifdef PARANOID
	if (tile == tp)
	{
	     TxError("Junction being made between tile and itself \n");
	     return;
	}
#endif

	if (j2->tj_status & RES_TILE_DONE) return;
	MALLOC(resNode *,resptr,sizeof(resNode));
	resptr->rn_te = (tElement *) NULL;
	MALLOC(ResJunction *,junction,sizeof(ResJunction));
	MALLOC(jElement *,jcell,sizeof(jElement));
	InitializeNode(resptr,xj,yj,RES_NODE_JUNCTION);
	resptr->rn_je = jcell;
	ResAddToQueue(resptr,NodeList);

	jcell->je_thisj = junction;
	jcell->je_nextj = NULL;
	junction->rj_status = FALSE;
	junction->rj_jnode = resptr;
	junction->rj_Tile[0] = tile;
	junction->rj_Tile[1] = tp;
	junction->rj_loc.p_x =xj;
	junction->rj_loc.p_y =yj;
	junction->rj_nextjunction[0] = j0->junctionList;
	j0->junctionList = junction;
	junction->rj_nextjunction[1] = j2->junctionList;
	j2->junctionList = junction;
	     
	NEWBREAK(junction->rj_jnode,tile,
     			junction->rj_loc.p_x,junction->rj_loc.p_y,NULL);

	NEWBREAK(junction->rj_jnode,tp,
     			junction->rj_loc.p_x,junction->rj_loc.p_y,NULL);

}
