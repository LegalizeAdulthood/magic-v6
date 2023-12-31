#ifndef lint
static char rcsid[] = "$Header: ResMain.c,v 6.0 90/08/28 18:54:32 mayo Exp $";
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
#include "tech.h"
#include "txcommands.h"
#include "resis.h"


/* this macro should really be in the database header section */

#define MasksIntersect(m1,m2) ((m1)->tt_words[0] & (m2)->tt_words[0] | (m1)->tt_words[1] & (m2)->tt_words[1])


CellUse 		*ResUse=NULL;		/* Our use and def */
CellDef 		*ResDef=NULL;
TileTypeBitMask 	ResConnectWithSD[NT];	/* A mask that goes from  */
						/* SD's to transistors.   */
TileTypeBitMask 	ResCopyMask[NT];	/* Inidicates which tiles */
						/* are to be copied.      */
resResistor 		*ResResList=NULL;	/* Resistor list	  */
resNode     		*ResNodeList=NULL;	/* Processed Nodes 	  */
resTransistor 		*ResTransList=NULL;	/* Transistors		  */
ResContactPoint		*ResContactList=NULL;	/* Contacts		  */
resNode			*ResNodeQueue=NULL;	/* Pending nodes	  */
resNode			*ResOriginNode=NULL;	/* node where R=0	  */
resNode			*resCurrentNode;
int			ResTileCount=0;		/* Number of tiles rn_status */
extern Region 			*ResFirst();
extern Tile		*FindStartTile();
extern int			ResEachTile();
extern int			ResLaplaceTile();


/*
 *--------------------------------------------------------------------------
 *
 * ResInitializeConn--
 *
 *  Sets up mask by Source/Drain type of transistors. This is 
 *  exts_transSDtypes turned inside out.
 *
 *  Results: none
 *
 * Side Effects: Sets up ResConnectWithSD.
 *
 *-------------------------------------------------------------------------
 */
ResInitializeConn()
{
     TileType tran, diff;
 	
     for (tran = TT_TECHDEPBASE; tran < TT_MAXTYPES; tran++)
     {
     	 for (diff = TT_TECHDEPBASE; diff < TT_MAXTYPES; diff++)
	 {
	     if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[tran]),diff)
	     {
		  TTMaskSetType(&ResConnectWithSD[diff],tran);
	     }
	     if TTMaskHasType(&(ExtCurStyle->exts_transSubstrateTypes[tran]),diff)
	     {
		  TTMaskSetType(&ResConnectWithSD[diff],tran);
	     }
	          
	 }
	 TTMaskSetMask(&ResConnectWithSD[tran],&DBConnectTbl[tran]);
     }
}

/*
 * ----------------------------------------------------------------------------
 *
 *  ResGetReCell --
 *
 * 	This procedure makes sure that ResUse,ResDef
 *	have been properly initialized to refer to a cell definition
 *	named "__RESIS__".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new cell use and/or def are created if necessary.
 *
 * --------------------------------------------------------------------------
 */
ResGetReCell()
{
    if (ResUse != NULL) return;
    ResDef = DBCellLookDef("__RESIS__");
    if (ResDef == NULL)
    {
	ResDef = DBCellNewDef("__RESIS__", (char *) NULL);
	ASSERT (ResDef != (CellDef *) NULL, "ResGetReCell");
	DBCellSetAvail(ResDef);
	ResDef->cd_flags |= CDINTERNAL;   
    }
    ResUse = DBCellNewUse(ResDef, (char *) NULL);
    DBSetTrans(ResUse, &GeoIdentityTransform);
    ResUse->cu_expandMask = -1;

}
 
/*
 *--------------------------------------------------------------------------
 *
 *  ResDissolveContacts--
 *
 *  results:  none
 *
 *  Side Effects:  All contacts in the design are broken into their 
 *    constituent
 *    layers.  There should be no contacts in ResDef after this procedure
 *    runs.
 *
 *
 *------------------------------------------------------------------------
 */
ResDissolveContacts(contacts)
	ResContactPoint *contacts;
{
    TileType t,oldtype;
    Tile *tp;
    LayerInfo	*lp;

    for (; contacts != (ResContactPoint *) NULL; contacts = contacts->cp_nextcontact)

    {
          oldtype=contacts->cp_type;
#ifdef PARANOID
	  if (oldtype == TT_SPACE)
	  {
	       TxError("Error in Contact Dissolving for %s \n",ResCurrentNode);
	  }
#endif
	  lp = dbLayerInfo+oldtype;
	  if (TTMaskHasType(&ExtCurStyle->exts_transMask,lp->l_images[0])) 
	  							   continue;
	  if (TTMaskHasType(&ExtCurStyle->exts_transMask,lp->l_images[1])) 
	  							   continue;
	  if (TTMaskHasType(&ExtCurStyle->exts_transMask,lp->l_images[2])) 
	  							   continue;
	  DBErase(ResUse->cu_def,&(contacts->cp_rect),oldtype);
	  DBPaint(ResUse->cu_def,&(contacts->cp_rect),lp->l_residue);	
	  if ((t=lp->l_rbelow) != TT_SPACE)
	       DBPaint(ResUse->cu_def,&(contacts->cp_rect),t);
	  if ((t=lp->l_rabove) != TT_SPACE)
	       DBPaint(ResUse->cu_def,&(contacts->cp_rect),t);
	  tp = ResDef->cd_planes[DBPlane(contacts->cp_type)]->pl_hint;
	  GOTOPOINT(tp,&(contacts->cp_rect.r_ll));
#ifdef PARANOID
	  if (TiGetType(tp) == contacts->cp_type)
	  {
	       TxError("Error in Contact Preprocess Routines\n");
	  }
#endif
    }
     
}

/*
 *---------------------------------------------------------------------------
 *
 *  ResFindNewContactTiles --
 *
 *
 *  Results:  none
 *
 *  Side Effects:  dissolving contacts eliminated the tiles that 
 *  contacts->nextcontact pointed to. This procedure finds the tile now under
 *  center and sets that tile's ti_client field to point to the contact.  The
 *  old value of clientdata is set to nextTilecontact.
 *
 *----------------------------------------------------------------------------
 */
ResFindNewContactTiles(contacts)
	ResContactPoint *contacts;
{
     int pNum;
     register Tile *tile;
     TileTypeBitMask *mask;
     
     for (; contacts != (ResContactPoint *) NULL; contacts = contacts->cp_nextcontact)
     {
          mask = &DBComponentTbl[contacts->cp_type];
     	  for (pNum=PL_TECHDEPBASE; pNum<DBNumPlanes; pNum++)
	  {
		  tile = ResDef->cd_planes[pNum]->pl_hint;
		  GOTOPOINT(tile, &(contacts->cp_center));
#ifdef PARANOID
		  if (tile == (Tile *) NULL)
		  {
		       TxError("Error: setting contact tile to null\n");
		  }
#endif
		  if TTMaskHasType(mask, TiGetType(tile))
		  {
		       tileJunk	*j = (tileJunk *)tile->ti_client;
		       cElement *ce;
		       
		       MALLOC(cElement *,ce,sizeof(cElement));
		       contacts->cp_tile[contacts->cp_currentcontact] = tile;
		       ce->ce_thisc = contacts;
		       ce->ce_nextc = j->contactList;
		       (contacts->cp_currentcontact) += 1;
		       j->contactList = ce;
		  }
	  }
#ifdef PARANOID
	  if (contacts->cp_currentcontact > LAYERS_PER_CONTACT)
	  {
	       TxError("Error: Not enough space allocated for contact nodes\n");
	  }
#endif
     }
}

/*
 *--------------------------------------------------------------------------
 *
 * ResProcessTiles--Calls ResEachTile with processed tiles belonging to
 *		nodes in ResNodeQueue.  When all the tiles corresponding
 *		to a node have been processed, the node is moved to
 *		ResNodeList.
 *
 *  Results: none
 *
 *  Side Effects: Cleans extraneous linked lists from nodes. 
 *
 *--------------------------------------------------------------------------
 */
 ResProcessTiles(goodies,origin)
	Point		*origin;
	ResGlobalParams	*goodies;

{
     Tile 		*startTile;
     int 		tilenum,merged;
     resNode		*resptr2;
     register jElement	*workingj;
     register cElement	*workingc;
     ResFixPoint	*fix;
     resNode		*resptr;
     int		(*tilefunc)();
     
#ifdef LAPLACE
     tilefunc = (ResOptionsFlags & ResOpt_DoLaplace)?ResLaplaceTile:ResEachTile;
#else
     tilefunc = ResEachTile;
#endif

    if (ResOptionsFlags & ResOpt_Signal)
    {
         startTile = FindStartTile(goodies,origin);
         if (startTile == NULL) return(1);
	 resCurrentNode = NULL;
	 (void) (*tilefunc)(startTile,origin);
    }
#ifdef ARIEL
    else if (ResOptionsFlags & ResOpt_Power)
    {
    	 for (fix = ResFixList; fix != NULL;fix=fix->fp_next)
	 {
      	      Tile	*tile = fix->fp_tile;
	      if (tile == NULL)
	      {

		   tile = ResDef->cd_planes[DBPlane(fix->fp_ttype)]->pl_hint;
		   GOTOPOINT(tile,&(fix->fp_loc));
		   if (TiGetType(tile) != TT_SPACE)
		   {
		   	fix->fp_tile = tile;
		   } 
		   else
		   {
		   	tile = NULL;
		   }
	      }
	      if (tile != NULL)
	      {
	           int x = fix->fp_loc.p_x;
	           int y = fix->fp_loc.p_y;
		   MALLOC(resNode *,resptr,sizeof(resNode));
		   InitializeNode(resptr,x,y,RES_NODE_ORIGIN);
	           resptr->rn_status = TRUE;
	           resptr->rn_noderes =0;
	           ResAddToQueue(resptr,&ResNodeQueue);
		   fix->fp_node = resptr;
		   NEWBREAK(resptr,tile,x,y,NULL);
	      }
	 }
    	 for (fix = ResFixList; fix != NULL;fix=fix->fp_next)
	 {
      	      Tile	*tile = fix->fp_tile;

	      if (tile != NULL && (((tileJunk *)tile->ti_client)->tj_status & 
			RES_TILE_DONE) == 0)
	      {
	           resCurrentNode = fix->fp_node;
	      	   (void) (*tilefunc)(tile,(Point *)NULL);
	      }
	 }
    }
#endif
#ifdef PARANOID
    else
    {
    	 TxError("Unknown analysis type in ResProcessTiles\n");
    }
#endif

    /* Process Everything else   */
    while (ResNodeQueue != NULL)
    {
	 /* 
	    merged keeps track of whether another node gets merged into
	    the current one.  If it does, then the node must be processed
	    because additional junctions or contacts were added
	 */
	 
	 resptr2 = ResNodeQueue;
	 merged = FALSE;

	 /* Process all junctions associated with node   */
	 
	 for(workingj = resptr2->rn_je;workingj != NULL;workingj = workingj->je_nextj)

	 {
	      ResJunction	*rj = workingj->je_thisj;
	      if (rj->rj_status == FALSE)
	      {
		   for (tilenum = 0; tilenum <TILES_PER_JUNCTION; tilenum++)
		   {
	      	        Tile	*tile = rj->rj_Tile[tilenum];
			tileJunk *j = (tileJunk *) tile->ti_client;
			
			if ((j->tj_status & RES_TILE_DONE) == 0)
			{
			     resCurrentNode = resptr2;
			     merged |= (*tilefunc)(tile,(Point *)NULL);
			}
		        if (merged & ORIGIN) break;
		   }
		   if (merged & ORIGIN) break;
		   rj->rj_status = TRUE;
	      }
	 }

	 /* Next, Process all contacts.  */

	 for (workingc = resptr2->rn_ce;workingc != NULL;workingc = workingc->ce_nextc)
	 {
	      ResContactPoint	*cp = workingc->ce_thisc;

	      if (merged & ORIGIN) break;
	      if (cp->cp_status == FALSE)
	      {
		   int newstatus = TRUE;
		   for (tilenum = 0; tilenum < cp->cp_currentcontact; tilenum++)
		   {
	      	        Tile	 *tile = cp->cp_tile[tilenum];
			tileJunk *j    = (tileJunk *) tile->ti_client;

			if ((j->tj_status & RES_TILE_DONE) == 0)
			{
			     if (cp->cp_cnode[tilenum] == resptr2)
			     {
			          resCurrentNode = resptr2;
			     	  merged |= (*tilefunc)(tile,(Point *)NULL);
			     }
			     else
			     {
			     	  newstatus = FALSE;
			     }
			}
			if (merged & ORIGIN) break;
		   }
		   if (merged & ORIGIN) break;
		   cp->cp_status = newstatus;
	      }
	 }
	 /* 
	    If nothing new has been added via a merge, then the node is
	    finished. It is removed from the pending queue, added to the
	    done list, cleaned up, and passed to ResDoneWithNode
	 */
	 
	 if (merged == FALSE)
	 {
		 ResRemoveFromQueue(resptr2,&ResNodeQueue);
		 resptr2->rn_more = ResNodeList;
		 resptr2->rn_less = NULL;
		 resptr2->rn_status &= ~PENDING;
		 resptr2->rn_status |= FINISHED | MARKED;
		 if (ResNodeList != NULL)
		 {
		    ResNodeList->rn_less = resptr2;
		 }
		 if (resptr2->rn_noderes == 0)
		 {
		      ResOriginNode=resptr2;
		 }
		 ResNodeList = resptr2;
	     	 ResCleanNode(resptr2,FALSE,&ResNodeList,&ResNodeQueue);
		 ResDoneWithNode(resptr2);
	 }
     }
     return(0);
}

/*-------------------------------------------------------------------------
 *
 * ResExtractNet-- extracts the resistance net at the specified 
 *	rn_loc. If the resulting net is greater than the tolerance,
 *	simplify and return the resulting network.
 *
 * Results:  0 iff it worked.
 *
 * Side effects: Produces a resistance network for the node.
 *
 *
 *-------------------------------------------------------------------------
 */
bool
ResExtractNet(startlist,goodies) 
	ResFixPoint	*startlist;
	ResGlobalParams	*goodies;
{
    Window		*w;
    SearchContext 	scx;
    int			pNum;
    ResTranTile		*TranTiles,*lasttile;
    TileTypeBitMask	FirstTileMask;
    Point		startpoint;
    ResFixPoint		*fix;
    static int		first = 1;

    /* Make sure all global network variables are reset */
    
    ResResList=NULL;
    ResNodeList=NULL;
    ResTransList=NULL;
    ResNodeQueue=NULL;
    ResContactList = NULL;
    ResOriginNode = NULL;

    /* Pass back network pointers	*/
    
    goodies->rg_maxres = 0;
    goodies->rg_tilecount = 0;

    /*set up internal stuff if this is the first time through */

    if (first) 
    {
    	 ResInitializeConn(); 
         first = 0;
         ResGetReCell();
    }

    /* Initialize Cell */

    w = ToolGetBoxWindow(&scx.scx_area, (int *) NULL);
    if (w == (Window *) NULL)
    {
	TxError("Sorry, the box must appear in one of the windows.\n");
	return 1;
    }

    scx.scx_use = (CellUse *) w->w_surfaceID;
    scx.scx_trans = GeoIdentityTransform;   

    DBCellClearDef(ResUse->cu_def);  


    /* Copy Paint     */
    TranTiles = NULL;
    lasttile = NULL;
    for (fix = startlist; fix != NULL;fix=fix->fp_next)
    {
	 ResTranTile	*newtrantiles,*tmp;

#ifdef ARIEL
	 if ((ResOptionsFlags & ResOpt_Power) &&
	 		strcmp(fix->fp_name,goodies->rg_name) != 0) continue;
#endif

         scx.scx_area.r_ll.p_x = fix->fp_loc.p_x-2;
         scx.scx_area.r_ll.p_y = fix->fp_loc.p_y-2;
         scx.scx_area.r_ur.p_x = fix->fp_loc.p_x+2;
         scx.scx_area.r_ur.p_y = fix->fp_loc.p_y+2;
	 startpoint = fix->fp_loc;
	 TTMaskSetOnlyType(&FirstTileMask,fix->fp_ttype);

         newtrantiles = DBTreeCopyConnectDCS(&scx,&FirstTileMask, 0,
	         			ResCopyMask, &TiPlaneRect,ResUse);
	 for (tmp = newtrantiles;tmp && tmp->nextTran;tmp=tmp->nextTran);
	 if (newtrantiles) 
	 {
	      if (TranTiles)
	      {
	      	   lasttile->nextTran = newtrantiles;
	      }
	      else
	      {
	      	   TranTiles = newtrantiles;
	      }
	      lasttile = tmp;
	 }
    }

    ExtResetTiles(scx.scx_use->cu_def,extUnInit);

    /* find all contacts in design and  note their position */

    ResContactList = (ResContactPoint *) ExtFindRegions(ResUse->cu_def,
				     &(ResUse->cu_def->cd_bbox),
				     &DBAllButSpaceAndDRCBits,
				     ResConnectWithSD, extUnInit, ResFirst, 
				     ResEach);
    ExtResetTiles(ResUse->cu_def,extUnInit);
    
    /* 
      dissolve the contacts and find which tiles now cover the point 
      where the tile used to be.
    */

    ResDissolveContacts(ResContactList);
    
    /* Add "junk" fields to tiles */

    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
    	 Plane	*plane = ResUse->cu_def->cd_planes[pNum];
	 Rect	*rect  = &ResUse->cu_def->cd_bbox;
	 ResFracture(plane,rect);
	 (void) DBSrPaintClient((Tile *) NULL,plane,rect,
	 		&DBAllButSpaceAndDRCBits,
			(ClientData) MINFINITY,ResAddPlumbing,
			(ClientData) &ResTransList);
			
    }

    /* Finish preprocessing.		*/
    
    ResFindNewContactTiles(ResContactList);
    ResPreProcessTransistors(TranTiles,ResTransList,ResUse->cu_def);
#ifdef LAPLACE
    if (ResOptionsFlags & ResOpt_DoLaplace)
    {
         for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
         {
    	      Plane	*plane = ResUse->cu_def->cd_planes[pNum];
	      Rect	*rect  = &ResUse->cu_def->cd_bbox;
	      Res1d(plane,rect);
         }
    }
#endif

#ifdef ARIEL
    if (ResOptionsFlags & ResOpt_Power)
    {
    	 for (fix = startlist; fix != NULL;fix=fix->fp_next)
	 {
     	      fix->fp_tile = ResUse->cu_def->cd_planes[DBTypePlaneTbl[fix->fp_ttype]]->pl_hint;
	      GOTOPOINT(fix->fp_tile,&fix->fp_loc);
	      if (TiGetType(fix->fp_tile) == TT_SPACE) fix->fp_tile = NULL;
	 }
    }
#endif

    /* do extraction */
    if (ResProcessTiles(goodies,&startpoint) != 0) return(1);
    return(0);
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCleanUpEverything--After each net is extracted by ResExtractNet,
 *	the resulting memory must be freed up, and varius trash swept under
 *	the carpet in preparation for the next extraction.
 *
 * Results: none
 *
 * Side Effects: Frees up memory formerly occupied by network elements.
 *
 *-------------------------------------------------------------------------
 */
ResCleanUpEverything()
{

    int		pNum;
    resResistor *oldRes;
    resTransistor *oldTran;
    ResContactPoint	*oldCon;

    /* check integrity of internal database. Free up lists. */

    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
    {
    	 (void) DBSrPaintClient((Tile *) NULL,ResUse->cu_def->cd_planes[pNum],
	 		&(ResUse->cu_def->cd_bbox),&DBAllButSpaceAndDRCBits,
			(ClientData) MINFINITY,ResRemovePlumbing,
			(ClientData) NULL);
			
    }

    while (ResNodeList != NULL)
    {
	ResCleanNode(ResNodeList,TRUE,&ResNodeList,&ResNodeQueue);
    }
    while (ResContactList != NULL)
    {
    	 oldCon = ResContactList;
	 ResContactList = oldCon->cp_nextcontact;
	 FREE((char *)oldCon);
    }
    while (ResResList != NULL)
    {
    	 oldRes = ResResList;
	 ResResList = ResResList->rr_nextResistor;
	 FREE((char *)oldRes);
    }
    while (ResTransList != NULL)
    {
    	 oldTran = ResTransList;
	 ResTransList = ResTransList->rt_nextTran;
	 if ((oldTran->rt_status & RES_TRAN_SAVE) == 0)
	 {
	      FREE((char *)oldTran);
	 }
    }

    DBCellClearDef(ResUse->cu_def);  
}



/*
 *-------------------------------------------------------------------------
 *
 * FindStartTile-- To start the extraction, we need to find the first driver.
 *	The sim file gives us the location of a point in  or near (within 1
 *	unit) of the transistor. FindStartTile looks for the transistor, then
 *	for adjoining diffusion. The diffusion tile is returned.
 *
 * Results: returns source diffusion tile, if it exists. Otherwise, return
 *	NULL.
 *
 * Side Effects: none
 *
 *-------------------------------------------------------------------------
 */
Tile *
FindStartTile(goodies,SourcePoint)
	Point		*SourcePoint;
	ResGlobalParams	*goodies;

{
     Point	workingPoint;
     Tile	*tile,*tp;
     int	pnum,t1,t2;
     
    workingPoint.p_x = goodies->rg_tranloc->p_x;
    workingPoint.p_y = goodies->rg_tranloc->p_y;
    pnum = DBTypePlaneTbl[goodies->rg_ttype];

     /* for drivepoints, we don't have to find a transistor */
     if (goodies->rg_status & DRIVEONLY)
     {

     	  tile = ResUse->cu_def->cd_planes[pnum]->pl_hint;
	  GOTOPOINT(tile,&workingPoint);
	  SourcePoint->p_x = workingPoint.p_x;
	  SourcePoint->p_y = workingPoint.p_y;
	  return(TiGetType(tile) != TT_SPACE?tile:NULL);
     }

     workingPoint.p_x = goodies->rg_tranloc->p_x;
     workingPoint.p_y = goodies->rg_tranloc->p_y;
     
/*
     for (pnum= PL_TECHDEPBASE; pnum < DBNumPlanes; pnum++)
     {
     	  if (MasksIntersect(&ExtCurStyle->exts_transMask,&DBPlaneTypes[pnum]) == 0)
	  {
	       continue;
	  }
	  tile = ResUse->cu_def->cd_planes[pnum]->pl_hint;
	  GOTOPOINT(tile,&workingPoint);
     }
*/
     tile = ResUse->cu_def->cd_planes[pnum]->pl_hint;
     GOTOPOINT(tile,&workingPoint);
     if (TTMaskHasType(&ExtCurStyle->exts_transMask,TiGetType(tile)) == 0)
     {
     	  TxError("Couldn't find transistor at %d %d\n",goodies->rg_tranloc->p_x,goodies->rg_tranloc->p_y);
	  return(NULL);
     }
     t1 = TiGetType(tile);
      /* left */
      for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp=RT(tp))
      {
	   t2 = TiGetType(tp);
	   if  TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),t2)
	      {
	      	   SourcePoint->p_x = LEFT(tile);
	      	   SourcePoint->p_y = (MIN(TOP(tile),TOP(tp))+
		   			MAX(BOTTOM(tile),BOTTOM(tp)))>>1;
		   return(tp);
	      }
      }
      /*right*/
      for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp=LB(tp))
      {
	   t2 = TiGetType(tp);
	   if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),t2)
	      {
	      	   SourcePoint->p_x = RIGHT(tile);
	      	   SourcePoint->p_y = (MIN(TOP(tile),TOP(tp))+
		   			MAX(BOTTOM(tile),BOTTOM(tp)))>>1;
		   return(tp);
	      }
      }
      /*top*/
      for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp=BL(tp))
      {
	   t2 = TiGetType(tp);
	   if TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),t2)
	      {
	      	   SourcePoint->p_y = TOP(tile);
	      	   SourcePoint->p_x = (MIN(RIGHT(tile),RIGHT(tp))+
		   			MAX(LEFT(tile),LEFT(tp)))>>1;
		   return(tp);
	      }
      }
      /*bottom */
      for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp=TR(tp))
      {
	   t2 = TiGetType(tp);
	   if   TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t1]),t2)
	      {
	      	   SourcePoint->p_y = BOTTOM(tile);
	      	   SourcePoint->p_x = (MIN(RIGHT(tile),RIGHT(tp))+
		   			MAX(LEFT(tile),LEFT(tp)))>>1;
		   return(tp);
	      }
      }
      return((Tile *) NULL);
}

/*
 *-------------------------------------------------------------------------
 *
 * ResGetTransistor-- Once the net is extracted, we still have to equate
 *	the sim file transistors with the layout transistors. ResGetTransistor
 *	looks for a transistor at the given location.
 *
 * Results: returns transistor structure at location TransistorPoint, if it
 *	exists.
 *
 * Side Effects: none
 *
 *-------------------------------------------------------------------------
 */
resTransistor *
ResGetTransistor(pt)
	Point	*pt;

{
     Point	workingPoint;
     Tile	*tile;
     int	pnum;
     
     workingPoint.p_x = (*pt).p_x;
     workingPoint.p_y = (*pt).p_y;
     
     for (pnum= PL_TECHDEPBASE; pnum < DBNumPlanes; pnum++)
     {
     	  if (MasksIntersect(&ExtCurStyle->exts_transMask,&DBPlaneTypes[pnum]) == 0)
	  {
	       continue;
	  }
	  /*start at hint tile for transistor plane */
	  tile = ResUse->cu_def->cd_planes[pnum]->pl_hint;
	  GOTOPOINT(tile,&workingPoint);
     }
     return(((tileJunk *)tile->ti_client)->transistorList);
}
