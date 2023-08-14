
#ifndef lint
static char rcsid[] = "$Header: ResUtils.c,v 6.0 90/08/28 18:54:49 mayo Exp $";
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
 * ---------------------------------------------------------------------------
 *
 * ResFirst -- Checks to see if tile is a contact. If it is, allocate a 
 *	contact structure.
 * 
 *
 * Results: Always returns null.
 *
 * Side effects:
 *	Memory is allocated by ResFirst.
 *	We cons the newly allocated region onto the front of the existing
 *	region list.
 *
 *
 * -------------------------------------------------------------------------
 */
Region *
ResFirst(tile, arg)
    Tile *tile;
    FindRegion *arg;
{
    register ResContactPoint *reg;
    register TileType t;
    int i;
    
    t = TiGetType(tile);
    if ( (DBConnPlanes[t] !=0) && TTMaskHasType(&DBContactBits,t))
    {
	    MALLOC(ResContactPoint *, reg, sizeof(ResContactPoint));
	    reg->cp_center.p_x = (LEFT(tile)+RIGHT(tile))>>1;
	    reg->cp_center.p_y = (TOP(tile)+BOTTOM(tile))>>1;
	    reg->cp_status = FALSE;
	    reg->cp_type = t;
	    reg->cp_area = (RIGHT(tile)-LEFT(tile))*(TOP(tile)-BOTTOM(tile));
	    for (i=0; i< LAYERS_PER_CONTACT; i++)
	    {
	    	 reg->cp_tile[i] = (Tile *) NULL;
		 reg->cp_cnode[i] = (resNode *) NULL;
	    }
	    reg->cp_currentcontact = 0;
	    reg->cp_rect.r_ll.p_x = tile->ti_ll.p_x;
	    reg->cp_rect.r_ll.p_y = tile->ti_ll.p_y;
	    reg->cp_rect.r_ur.p_x = RIGHT(tile);
	    reg->cp_rect.r_ur.p_y = TOP(tile);
	    reg->cp_contactTile = tile;
	    /* Prepend it to the region list */
	    reg->cp_nextcontact = (ResContactPoint *) arg->fra_region;
	    arg->fra_region = (Region *) reg;
    }
return((Region *) NULL);
}

/*
 *--------------------------------------------------------------------------
 *
 * ResEach--
 *
 * ResEach calls ResFirst unless this is the first contact, in which case it 
 * has alreay been processed
 *
 * results: returns 0
 *
 * Side Effects: see ResFirst
 *
 * -------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
ResEach(tile, pNum,arg)
    Tile *tile;
    int pNum;
    FindRegion *arg;
{
   
   if ( ((ResContactPoint *)(arg->fra_region))->cp_contactTile != tile)
   {
   	(void) ResFirst(tile,arg);
   }
   return(0);
}

/*
 *-------------------------------------------------------------------------
 *
 * ResAddPlumbing-- Each tile is a tileJunk structure associated with it
 * to keep track of various things used by the extractor. ResAddPlumbing
 * adds this structure and sets the tile's ClientData field to point to it.
 * If the tile is a transistor, then a transistor structure is also added;
 * all connected transistor tiles are enumerated and their transistorList 
 * fields set to the new structure.
 *
 * Results: always returns 0
 *
 * Side Effects:see above
 *
 *-------------------------------------------------------------------------
 */
int
ResAddPlumbing(tile, arg)
     Tile	*tile;
     ClientData *arg;

{
     tileJunk		*Junk,*junk2;
     static	Stack	*resTransStack=NULL;
     int		t1;
     Tile		*tp1,*tp2,*source;
     resTransistor	*resFet;
     
     if (resTransStack == NULL)
     {
     	  resTransStack = StackNew(64);
     }
     if (tile->ti_client == (ClientData) MINFINITY)
     {
     	  junk2 = resAddField(tile);
	  if (TTMaskHasType(&(ExtCurStyle->exts_transMask),TiGetType(tile)))
	  {
   	       MALLOC(resTransistor *, resFet,sizeof(resTransistor));
	       {
	       	    int	   i;
		    for (i=0; i != RT_TERMCOUNT;i++)
		    	resFet->rt_terminals[i] = (resNode *) NULL;
	       }
	       t1 = TiGetType(tile);
               resFet->rt_tile = tile;
	       resFet->rt_inside.r_ll.p_x = LEFT(tile);
	       resFet->rt_inside.r_ll.p_y = BOTTOM(tile);
	       resFet->rt_inside.r_ur.p_x = RIGHT(tile);
	       resFet->rt_inside.r_ur.p_y = TOP(tile);
	       resFet->rt_trantype = t1;
               resFet->rt_tiles = 0;
	       resFet->rt_length = 0;
	       resFet->rt_width = 0;
	       resFet->rt_perim = 0;
	       resFet->rt_area = 0;
	       resFet->rt_status = 0;
               resFet->rt_nextTran = (resTransistor *) *arg;
	       *arg = (ClientData)resFet;
	       junk2->transistorList =  resFet;
	       junk2->tj_status |= RES_TILE_TRAN;
	       
	       source = NULL;
	       /* find diffusion (if present) to be source contact */
	       /* top */
	       for (tp2= RT(tile); RIGHT(tp2) > LEFT(tile); tp2 = BL(tp2))
	       {
	      	    if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
		    {
			  junk2->sourceEdge |= TOPEDGE;
			  source = tp2;
			  Junk = resAddField(source);
			  Junk->tj_status |= RES_TILE_SD;
			  break;
		    }
	       }
	       /*bottom*/
	       if (source == NULL)
	       for (tp2= LB(tile); LEFT(tp2) < RIGHT(tile); tp2 = TR(tp2))
	       {
	      	    if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
		    {
			  junk2->sourceEdge |= BOTTOMEDGE;
			  source = tp2;
			  Junk = resAddField(source);
			  Junk->tj_status |= RES_TILE_SD;
			  break;
		    }
	       }
	       /*right*/
	       if (source == NULL)
	       for (tp2= TR(tile); TOP(tp2) > BOTTOM(tile); tp2 = LB(tp2))
	       {
	      	    if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
		    {
			  junk2->sourceEdge |= RIGHTEDGE;
			  source = tp2;
			  Junk = resAddField(source);
			  Junk->tj_status |= RES_TILE_SD;
			  break;
		    }
	       }
	       /*left*/
	       if (source == NULL)
	       for (tp2= BL(tile); BOTTOM(tp2) < TOP(tile); tp2 = RT(tp2))
	       {
	      	    if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
		    {
			  source = tp2;
			  Junk = resAddField(source);
			  Junk->tj_status |= RES_TILE_SD;
			  junk2->sourceEdge |= LEFTEDGE;
			  break;
		    }
	       }
	       
	       /* We need to know whether a given diffusion tile connects to
	          the source or to the drain of a transistor.  A single 
		  diffusion tile is marked, and all connecting diffusion tiles
		  are enumerated and called the source.  Any other SD tiles
		  are assumed to be the drain.  BUG: this does not work 
		  correctly with multi SD structures. 
	       */
	       if (source != (Tile *) NULL)
	       {
	            STACKPUSH((ClientData) (source),resTransStack);
	       }
	       while (!StackEmpty(resTransStack))
	       {
	       	    tp1 = (Tile *) STACKPOP(resTransStack);
		    t1 = TiGetType(tp1);

		     /* top */
		     for (tp2= RT(tp1); RIGHT(tp2) > LEFT(tp1); tp2 = BL(tp2))
		     {
			  if (TiGetType(tp2) == t1)
		          {
		               tileJunk	 *j= resAddField(tp2);
			       if ((j->tj_status & RES_TILE_SD) ==0)
			       {
			            j->tj_status |= RES_TILE_SD;
	            	            STACKPUSH((ClientData)tp2,resTransStack);
			       }
		          }
		     }
		     /*bottom*/
		     for (tp2= LB(tp1); LEFT(tp2) < RIGHT(tp1); tp2 = TR(tp2))
		     {
		          if (TiGetType(tp2) == t1)
		          {
		               tileJunk	 *j= resAddField(tp2);
			       if ((j->tj_status & RES_TILE_SD) == 0)
			       {
			            j->tj_status |= RES_TILE_SD;
	            	            STACKPUSH((ClientData) (tp2),resTransStack);
			       }
		          }
		     }
		     /*right*/
		     for (tp2= TR(tp1); TOP(tp2) > BOTTOM(tp1); tp2 = LB(tp2))
		     {
		          if (TiGetType(tp2) == t1)
		          {
		               tileJunk	 *j= resAddField(tp2);
			       if ((j->tj_status & RES_TILE_SD) == 0)
			       {
			            j->tj_status |= RES_TILE_SD;
	            	            STACKPUSH((ClientData) (tp2),resTransStack);
			       }
		          }
		     }
		     /*left*/
		     for (tp2= BL(tp1); BOTTOM(tp2) < TOP(tp1); tp2 = RT(tp2))
		     {
		          if (TiGetType(tp2) == t1)
		          {
		               tileJunk	 *j= resAddField(tp2);
			       if ((j->tj_status & RES_TILE_SD) == 0)
			       {
			            j->tj_status |= RES_TILE_SD;
	            	            STACKPUSH((ClientData) (tp2),resTransStack);
			       }
		          }
		     }
	       }
	       /* find rest of transistor; search for source edges */

	       STACKPUSH((ClientData) (tile), resTransStack);
	       while (!StackEmpty(resTransStack))
	       {
	       	     tileJunk	*j0;

		     tp1= (Tile *) STACKPOP(resTransStack);
	       	     t1 = TiGetType(tp1);
		     j0 = (tileJunk *) tp1->ti_client;
		     /* top */
		     for (tp2= RT(tp1); RIGHT(tp2) > LEFT(tp1); tp2 = BL(tp2))
		     {
		     	  if ((TiGetType(tp2) == t1) &&
			      (tp2->ti_client == (ClientData) MINFINITY))
			       {
     	  			    Junk = resAddField(tp2);
				    STACKPUSH((ClientData)(tp2),resTransStack);
	       			    Junk->transistorList =  resFet;
	       			    Junk->tj_status |= RES_TILE_TRAN;
				    
			       }
	      	    	  else if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
			  {
			       Junk = resAddField(tp2);
			       if (Junk->tj_status & RES_TILE_SD)
			       {
			       	    j0->sourceEdge |= TOPEDGE;
			       }
			  }
		     }
		     /*bottom*/
		     for (tp2= LB(tp1); LEFT(tp2) < RIGHT(tp1); tp2 = TR(tp2))
		     {
		     	  if ((TiGetType(tp2) == t1) &&
			      (tp2->ti_client == (ClientData) MINFINITY))
			       {
     	  			    Junk = resAddField(tp2);
				    STACKPUSH((ClientData)(tp2),resTransStack);
	       			    Junk->transistorList =  resFet;
	       			    Junk->tj_status |= RES_TILE_TRAN;
			       }
	      	    	  else if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
			  {
			       Junk = resAddField(tp2);
			       if (Junk->tj_status & RES_TILE_SD)
			       {
			       	    j0->sourceEdge |= BOTTOMEDGE;
			       }
			  }
		     }
		     /*right*/
		     for (tp2= TR(tp1); TOP(tp2) > BOTTOM(tp1); tp2 = LB(tp2))
		     {
		     	  if ((TiGetType(tp2) == t1) &&
			      (tp2->ti_client == (ClientData) MINFINITY))
			       {
     	  			    Junk = resAddField(tp2);
				    STACKPUSH((ClientData)(tp2),resTransStack);
	       			    Junk->transistorList =  resFet;
	       			    Junk->tj_status |= RES_TILE_TRAN;
			       }
	      	    	  else if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
			  {
     	  		       Junk = resAddField(tp2);
			       if (Junk->tj_status & RES_TILE_SD)
			       {
			       	    j0->sourceEdge |= RIGHTEDGE;
			       }
			  }
		     }
		     /*left*/
		     for (tp2= BL(tp1); BOTTOM(tp2) < TOP(tp1); tp2 = RT(tp2))
		     {
		     	  if ((TiGetType(tp2) == t1) &&
			      (tp2->ti_client == (ClientData) MINFINITY))
			       {
     	  			    Junk = resAddField(tp2);
				    STACKPUSH((ClientData)(tp2),resTransStack);
	       			    Junk->transistorList =  resFet;
	       			    Junk->tj_status |= RES_TILE_TRAN;
			       }
	      	    	  else if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),TiGetType(tp2))
			  {
     	  		       Junk = resAddField(tp2);
			       if (Junk->tj_status & RES_TILE_SD)
			       {
			       	    j0->sourceEdge |= LEFTEDGE;
			       }
			  }
		     }
	       }

	       /* unmark all tiles marked as being part of source */

	       if (source != (Tile *) NULL)
	       {
	            tileJunk	*j = (tileJunk *) source->ti_client;

		    STACKPUSH((ClientData) (source),resTransStack);
		    j->tj_status &= ~RES_TILE_SD;
	       }
	       while (!StackEmpty(resTransStack))
	       {
	       	    tp1 = (Tile *) STACKPOP(resTransStack);
		    t1 = TiGetType(tp1);
		     /* top */
		     for (tp2= RT(tp1); RIGHT(tp2) > LEFT(tp1); tp2 = BL(tp2))
		     {
		          tileJunk	*j2 = (tileJunk *) tp2->ti_client;
			  if (TiGetType(tp2) == t1)
		          {
			       if (j2->tj_status & RES_TILE_SD)
			       {
			            j2->tj_status &= ~RES_TILE_SD;
	            	            STACKPUSH((ClientData) tp2,resTransStack);
			       }
		          }
		     }
		     /*bottom*/
		     for(tp2= LB(tp1); LEFT(tp2) < RIGHT(tp1); tp2 = TR(tp2))
		     {
		          tileJunk	*j2 = (tileJunk *) tp2->ti_client;
		          if (TiGetType(tp2) == t1)
		          {
			       if (j2->tj_status & RES_TILE_SD)
			       {
			            j2->tj_status &= ~RES_TILE_SD;
	            	            STACKPUSH((ClientData) tp2,resTransStack);
			       }
		          }
		     }
		     /*right*/
		     for (tp2= TR(tp1); TOP(tp2) > BOTTOM(tp1); tp2 = LB(tp2))
		     {
		          tileJunk	*j2 = (tileJunk *) tp2->ti_client;
		          if (TiGetType(tp2) == t1)
		          {
			       if (j2->tj_status & RES_TILE_SD)
			       {
			            j2->tj_status &= ~RES_TILE_SD;
	            	            STACKPUSH((ClientData) tp2,resTransStack);
			       }
		          }
		     }
		     /*left*/
		     for (tp2= BL(tp1); BOTTOM(tp2) < TOP(tp1); tp2 = RT(tp2))
		     {
		          tileJunk	*j2 = (tileJunk *) tp2->ti_client;
		          if (TiGetType(tp2) == t1)
		          {
			       if (j2->tj_status & RES_TILE_SD)
			       {
			            j2->tj_status &= ~RES_TILE_SD;
	            	            STACKPUSH((ClientData) tp2,resTransStack);
			       }
		          }
		     }
	       }
	  }
     }
     return(0);
}

/*
 *-------------------------------------------------------------------------
 *
 * ResRemovePlumbing-- Removes and deallocates all the tileJunk fields.
 *
 * Results: returns 0
 *
 * Side Effects: frees up memory; resets tile->ti_client fields to MINFINITY
 *
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
ResRemovePlumbing(tile, arg)
     Tile	*tile;
     ClientData *arg;

{
     
     if (tile->ti_client != (ClientData) MINFINITY)
     {
          FREE(((char *)(tile->ti_client)));
	  tile->ti_client = (ClientData) MINFINITY;
     }
     return(0);
}


/*
 *-------------------------------------------------------------------------
 *
 * ResPreprocessTransistors-- Given a list of all the transistor tiles and 
 * a list of all the transistors, this procedure calculates the width and 
 * length.. The width is set equal to the sum of all edges that touch 
 * diffusion divided by 2. The length is the remaining perimeter divided by 
 * 2*tiles.  The perimeter and area fields of transistor structures are also
 * fixed.
 *
 * Results: none
 *
 * Side Effects: sets length and width of transistors. "ResTransTile" 
 * structures are freed.
 *
 *-------------------------------------------------------------------------
 */
ResPreProcessTransistors(TileList,TransistorList,Def)
	ResTranTile		*TileList;
	resTransistor		*TransistorList;
	CellDef			*Def;

{
     Tile		*tile;
     ResTranTile	*oldTile;
     tileJunk		*tstruct;
     TileType		tt;
     int		pNum;
     
     while (TileList != (ResTranTile *) NULL)
     {
	  tt = TileList->type;
	  pNum = DBPlane(tt);
	  if (!TTMaskHasType(&ExtCurStyle->exts_transMask,tt)) 
	  /* this layer must be a contact; find which image is transistor. */
	  {
	       LayerInfo	*lp = &dbLayerInfo[tt];
	       int		origin;
	       
	       for (origin = 0; origin != lp->l_nresidues; origin++)
	       {
	       	    if (lp->l_images[origin] == tt) break;
	       }
	       pNum= (TTMaskHasType(&ExtCurStyle->exts_transMask,lp->l_images[origin+1]))?pNum+1:pNum-1;
	  }
	  tile = (Def->cd_planes[pNum])->pl_hint;
	  GOTOPOINT(tile,&(TileList->area.r_ll));
	  tt = TiGetType(tile);
	  tstruct = (tileJunk *) tile->ti_client;
	  if (!TTMaskHasType(&ExtCurStyle->exts_transMask,tt) ||
	  	tstruct->transistorList == NULL)
	  {
	       TxError("Bad Transistor Location at %d,%d\n",
	       				TileList->area.r_ll.p_x,
	       				TileList->area.r_ll.p_y);
	  }
	  else
	  if ((tstruct->tj_status & RES_TILE_MARK) ==0)
	  {
	       resTransistor	*rt = tstruct->transistorList;

	       tstruct->tj_status |= RES_TILE_MARK;
	       rt->rt_perim+=TileList->perim;
	       rt->rt_length+=TileList->overlap;
	       rt->rt_area+=
	       		(TileList->area.r_xtop-TileList->area.r_xbot)*
			(TileList->area.r_ytop-TileList->area.r_ybot);
	       rt->rt_tiles++;
	  }
	  oldTile = TileList;
	  TileList = TileList->nextTran;
	  FREE((char *)oldTile);
     }
     for(; TransistorList != NULL;TransistorList = TransistorList->rt_nextTran)
     {
     	  int width  = TransistorList->rt_perim;
	  int length = TransistorList->rt_length;
	  if (TransistorList->rt_tiles != 0)
	  {
	       if (length)
	       {
	            TransistorList->rt_length = (float) length/((float)((TransistorList->rt_tiles)<<1));
	            TransistorList->rt_width = (width-length)>>1;
	       }
	       else
	       {
	       	    double perimeter = TransistorList->rt_perim;
		    double area = TransistorList->rt_area;
		    
		    perimeter /= 4.0;

		    TransistorList->rt_width=
		    	perimeter+sqrt(perimeter*perimeter-area);
		    TransistorList->rt_length= 
		    	(TransistorList->rt_perim-2*TransistorList->rt_width)>>1;
	       }
	  }
     }
}


/*
 *-------------------------------------------------------------------------
 *
 * ResAddToQueue-- adds new nodes to list of nodes requiring processing.
 *
 * Side Effects: nodes are added to list (i.e they have their linked list
 *	pointers modified.)
 *
 *-------------------------------------------------------------------------
 */
ResAddToQueue(node,list)
     resNode		*node,**list;
{

     node->rn_more = *list;
     node->rn_less = NULL;
     if (*list) (*list)->rn_less = node;
     *list = node;
}


/*
 *-------------------------------------------------------------------------
 *
 * ResRemoveFromQueue-- removes node from queue. Complains if it notices
 *	that the node isn't in the supplied list.
 *
 * Results: none
 *
 * Side Effects: modifies nodelist
 *
 *-------------------------------------------------------------------------
 */
ResRemoveFromQueue(node,list)
	resNode	*node,**list;

{
     
     if (node->rn_less != NULL)
     {
     	  node->rn_less->rn_more = node->rn_more;
     }
     else
     {
     	  if (node != (*list))
	  {
	       TxError("Error: Attempt to remove node from wrong list\n");
	  }
	  else
	  {
	       *list = node->rn_more;
	  }
     }
     if (node->rn_more != NULL)
     {
     	  node->rn_more->rn_less = node->rn_less;
     }
     node->rn_more = NULL;
     node->rn_less = NULL;
}
tileJunk *
resAddField(tile)
	Tile	*tile;

{
        tileJunk *Junk;
	if ((Junk=(tileJunk *)tile->ti_client) == (tileJunk *) MINFINITY)
	{
     	      MALLOC(tileJunk *,Junk,sizeof(tileJunk));
	      ResJunkInit(Junk);
	      tile->ti_client = (ClientData) Junk;
	}
	return Junk;
}
