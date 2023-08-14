
#ifndef lint
static char rcsid[] = "$Header: ResBasic.c,v 6.0 90/08/28 18:54:23 mayo Exp $";
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
#include "malloc.h"
#include "textio.h"
#include "extract.h"
#include "extractInt.h"
#include "windows.h"
#include "dbwind.h"
#include "tech.h"
#include "txcommands.h"
#include "resis.h"

int resSubTranFunc();

/*
 *--------------------------------------------------------------------------
 *
 * ResEachTile--for each tile, make a list of all possible current sources/
 *   sinks including contacts, transistors, and junctions.  Once this
 *   list is made, calculate the resistor nextwork for the tile.
 *
 *  Results: returns TRUE or FALSE depending on whether a node was 
 *           involved in a merge.
 *
 *  Side Effects: creates Nodes, transistors, junctions, and breakpoints.
 *
 *
 *--------------------------------------------------------------------------
 */
int ResEachTile(tile,startpoint)
	register Tile 	*tile;
	Point 		*startpoint;

{
      register Tile 		*tp;
      resNode			*resptr;
      cElement			*ce;
      register TileType		t1,t2;
      int			xj,yj,merged,i;
      tElement			*tcell;
      tileJunk			*tstructs= (tileJunk *)(tile->ti_client);
      
      ResTileCount++;

      /* Process startpoint, if any.  */
      t1 = TiGetType(tile);
      if (startpoint != (Point *) NULL)
      {
      	   int x = startpoint->p_x;
      	   int y = startpoint->p_y;
	   MALLOC(resNode *,resptr,sizeof(resNode));
	   InitializeNode(resptr,x,y,RES_NODE_ORIGIN);
	   resptr->rn_status = TRUE;
	   resptr->rn_noderes =0;
	   ResAddToQueue(resptr,&ResNodeQueue);
	   NEWBREAK(resptr,tile,x,y,NULL);
	   resCurrentNode = resptr;
      }
      if TTMaskHasType(&(ExtCurStyle->exts_transMask),TiGetType(tile))
      {
	   /* 
	      The transistor is put in the center of the tile. This is fine
	      for single tile transistors, but not as good for multiple ones.
	   */

           if (tstructs->tj_status & RES_TILE_TRAN)
 	   {
		if (tstructs->transistorList->rt_gate == NULL)
		{
		     int x = (LEFT(tile)+RIGHT(tile))>>1;
		     int y = (TOP(tile)+BOTTOM(tile))>>1;

		     MALLOC(resNode *,resptr,sizeof(resNode));
		     tstructs->transistorList->rt_gate = resptr;
		     MALLOC(tElement *,tcell,sizeof(tElement));
		     tcell->te_thist = tstructs->transistorList;
		     tcell->te_nextt = NULL;

		     InitializeNode(resptr,x,y,RES_NODE_JUNCTION);
		     resptr->rn_te = tcell;
		     ResAddToQueue(resptr,&ResNodeQueue);

	   	     NEWBREAK(resptr,tile,resptr->rn_loc.p_x,
		     			resptr->rn_loc.p_y,NULL);
	        }
	   }
      }
		
#ifdef ARIEL
      if (i=ExtCurStyle->exts_plugSignalNum[t1])
      {
      	   MALLOC(tElement *,tcell,sizeof(tElement));
	   
	   tcell->te_thist= ResImageAddPlug(tile,i,resCurrentNode);
           tcell->te_nextt = resCurrentNode->rn_te;
           resCurrentNode->rn_te = tcell;
      }
      if (TTMaskHasType(&ResSubsTypeBitMask,t1) &&
          			(ResOptionsFlags & ResOpt_DoSubstrate))
      {
           int	pNum;
	   Rect tileArea;
	   TileTypeBitMask  *mask = &ExtCurStyle->exts_subsTransistorTypes[t1];

	   TiToRect(tile,&tileArea);
	   for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
           {
	       if (TTMaskIntersect(&DBPlaneTypes[pNum], mask))
	       {
	            (void)DBSrPaintArea((Tile *) NULL, 
		  	ResUse->cu_def->cd_planes[pNum], 
		        &tileArea,mask,resSubTranFunc, (ClientData) tile);
	       }
           }
      }
#endif

      /* Process all the contact points */
      ce = tstructs->contactList;
      while (ce != (cElement *) NULL)
      {
	   ResContactPoint   *cp = ce->ce_thisc;
	   cElement	     *oldce;
	   if (cp->cp_cnode[0] == (resNode *) NULL)
	   {
	        ResDoContacts(cp,&ResNodeQueue,&ResResList);
	   }
	   oldce = ce;
	   ce = ce->ce_nextc;
	   FREE((char *)oldce);
     }
     tstructs->contactList = NULL;     
     
     /* 
        Walk the four sides of the tile looking for adjoining connecting
        materials.
     */
       
      /* left */
      for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp=RT(tp))
      {
	   t2 = TiGetType(tp);
	   if(TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	      TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
           /* found transistor */
	   {
			xj = LEFT(tile);
			yj = (TOP(tp)+BOTTOM(tp))>>1;
	   	        ResNewSDTransistor(tile,tp,xj,yj,RIGHTEDGE,
					   &ResNodeQueue);
	   }
	   if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),t2)
	   /* tile is junction  */
	   {
		/*junction rn_loc */
		xj = LEFT(tile);
		yj = (MAX(BOTTOM(tile),BOTTOM(tp))+MIN(TOP(tile),TOP(tp)))>>1;
		(void) ResProcessJunction(tile,tp,xj,yj,
				   &ResNodeQueue);
	   }
      }

      /*right*/
      for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp=LB(tp))
      {
      	   t2 = TiGetType(tp);
	   if(TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	      TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
           /* found transistor */
	   {
			xj = RIGHT(tile);
			yj = (TOP(tp)+BOTTOM(tp))>>1;
	   	        ResNewSDTransistor(tile,tp,xj,yj,LEFTEDGE,
					   &ResNodeQueue);
	   }
	   if TTMaskHasType(&ExtCurStyle->exts_nodeConn[t1],t2)
	   /* tile is junction  */
	   {
		/*junction rn_loc */
		xj = RIGHT(tile);
		yj = (MAX(BOTTOM(tile),BOTTOM(tp))+MIN(TOP(tile),TOP(tp)))>>1;
		(void)ResProcessJunction(tile,tp,xj,yj,
				   &ResNodeQueue);
	   }
      }

      /*top*/
      for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp=BL(tp))
      {
      	   t2 = TiGetType(tp);
	   if(TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	      TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
           /* found transistor */
	   {
		yj = TOP(tile);
		xj = (LEFT(tp)+RIGHT(tp))>>1;
	   	ResNewSDTransistor(tile,tp,xj,yj,BOTTOMEDGE,
			&ResNodeQueue);
	   }
	   if TTMaskHasType(&ExtCurStyle->exts_nodeConn[t1],t2)
	   /* tile is junction  */
	   {
		yj = TOP(tile);
		xj = (MAX(LEFT(tile),LEFT(tp))+MIN(RIGHT(tile),RIGHT(tp)))>>1;
		ResProcessJunction(tile,tp,xj,yj,&ResNodeQueue);
	   }
	   
      }

      /*bottom */
      for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp=TR(tp))
      {
      	   t2 = TiGetType(tp);
	   if(TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	      TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
           /* found transistor */
	   {
			yj = BOTTOM(tile);
			xj = (LEFT(tp)+RIGHT(tp))>>1;
	   	        ResNewSDTransistor(tile,tp,xj,yj,TOPEDGE,
					   &ResNodeQueue);
	   }
	   if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),t2)
	   /* tile is junction  */
	   {
		yj = BOTTOM(tile);
		xj = (MAX(LEFT(tile),LEFT(tp))+MIN(RIGHT(tile),RIGHT(tp)))>>1;
		ResProcessJunction(tile,tp,xj,yj,&ResNodeQueue);

	   }
      }
      tstructs->tj_status |= RES_TILE_DONE;
      
      merged = ResCalcTileResistance(tile,tstructs,
      			&ResNodeQueue,&ResNodeList);

      return(merged);
}

/*
 *-------------------------------------------------------------------------
 *
 * resSubTranFunc -- called when DBSrPaintArea finds a transistor within
 *	a substrate area.
 *
 * Results: always returns 0 to keep search going.
 *
 * Side Effects: allocates substrate node.
 *
 *-------------------------------------------------------------------------
 */
resSubTranFunc(tile,tp)
	Tile	*tile,*tp;
	

{
     tileJunk	*junk = (tileJunk *)(tile->ti_client);
     resNode	*resptr;
     tElement	*tcell;
     int	x,y;

     if (junk->transistorList->rt_subs== NULL)
     {
          MALLOC(resNode *,resptr,sizeof(resNode));
	  junk->transistorList->rt_subs = resptr;
	  junk->tj_status |= RES_TILE_TRAN;
          MALLOC(tElement *,tcell,sizeof(tElement));
	  tcell->te_thist = junk->transistorList;
	  tcell->te_nextt = NULL;
	  x = (LEFT(tile)+RIGHT(tile))>>1;
	  y = (TOP(tile)+BOTTOM(tile))>>1;

	  InitializeNode(resptr,x,y,RES_NODE_JUNCTION);
	  resptr->rn_te = tcell;
	  ResAddToQueue(resptr,&ResNodeQueue);

	  NEWBREAK(resptr,tp,x,y,NULL);
    }
    return 0;
}
