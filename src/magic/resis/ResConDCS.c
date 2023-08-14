/* ResConnectDCS.c --
 *
 * This contains a slightly modified version of DBTreeCopyConnect.
 */

#ifndef lint
static char rcsid[] = "$Header: ResConDCS.c,v 6.1 90/09/03 14:59:56 stark Exp $";
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
#include "signals.h"
#include "windows.h"
#include "dbwind.h"
#include "tech.h"
#include "txcommands.h"
#include	"resis.h"

struct conSrArg2
{
    Rect csa2_area;
    int csa2_type;
    struct conSrArg2 *csa2_next;
};

int dbcUnconnectFunc();
int dbcConnectFuncDCS();
int resSubSearchFunc();
int dbcReturn;
static ResTranTile  *TransList = NULL;
static TileTypeBitMask	DiffTypeBitMask;
TileTypeBitMask	ResSubsTypeBitMask;


/*
 * ----------------------------------------------------------------------------
 *
 * dbcConnectFuncDCS -- the same as dbcConnectFunc, except that it does
 *	some extra searching around diffusion tiles looking for
 *	transistors.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Adds a new record to the current check list. May also add new
 *	ResTranTile structures.
 *
 * ----------------------------------------------------------------------------
 */

int
dbcConnectFuncDCS(tile, cx)
    Tile *tile;		
    TreeContext *cx;

{
    struct conSrArg2 **pHead = (struct conSrArg2 **) cx->tc_filter->tf_arg;
    struct conSrArg2 *newCsa2;
    Rect tileArea, *srArea,tranArea;
    ResTranTile		*thisTran;
    register Tile	*tp;
    register int	t2,t1;

    t1 = TiGetType(tile);
    TiToRect(tile, &tileArea);
    srArea = &cx->tc_scx->scx_area;
    if (((tileArea.r_xbot >= srArea->r_xtop-1) ||
        (tileArea.r_xtop <= srArea->r_xbot+1)) &&
	((tileArea.r_ybot >= srArea->r_ytop-1) ||
	(tileArea.r_ytop <= srArea->r_ybot+1))) return 0;
    MALLOC(struct conSrArg2 *, newCsa2, sizeof (struct conSrArg2));
    
    if TTMaskHasType(&DiffTypeBitMask,t1)
    {
    /* left */
    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp=RT(tp))
    {
         t2 = TiGetType(tp);
	 if (TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	     TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
	     {
    	          TiToRect(tp, &tranArea);
	          MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
		  ResCalcPerimOverlap(thisTran,tp);
	          GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
	          thisTran->type = TiGetType(tp);
	          thisTran->nextTran = TransList;
	          TransList = thisTran;
	     }
    }
    /*right*/
    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp=LB(tp))
    {
         t2 = TiGetType(tp);
	 if (TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	     TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
	     {
    	          TiToRect(tp, &tranArea);
	          MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
	          GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
	          thisTran->type = TiGetType(tp);
	          thisTran->nextTran = TransList;
	          TransList = thisTran;
		  ResCalcPerimOverlap(thisTran,tp);
	     }
    }
    /*top*/
    for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp=BL(tp))
    {
         t2 = TiGetType(tp);
	 if (TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	     TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
	     {
    	          TiToRect(tp, &tranArea);
	          MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
	          GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
	          thisTran->type = TiGetType(tp);
	          thisTran->nextTran = TransList;
	          TransList = thisTran;
		  ResCalcPerimOverlap(thisTran,tp);
	     }
    }
    /*bottom */
    for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp=TR(tp))
    {
         t2 = TiGetType(tp);
	 if (TTMaskHasType(&(ExtCurStyle->exts_transMask),t2) &&
	     TTMaskHasType(&(ExtCurStyle->exts_transSDTypes[t2]),t1))
	     {
    	          TiToRect(tp, &tranArea);
	          MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
	          GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
	          thisTran->type = TiGetType(tp);
	          thisTran->nextTran = TransList;
	          TransList = thisTran;
		  ResCalcPerimOverlap(thisTran,tp);
	     }
    }
    }
    else if TTMaskHasType(&(ExtCurStyle->exts_transMask),t1)
    {
    	          TiToRect(tile, &tranArea);
	          MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
		  ResCalcPerimOverlap(thisTran,tile);
	          GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
	          thisTran->type = TiGetType(tile);
	          thisTran->nextTran = TransList;
	          TransList = thisTran;
    }
    /* in some cases (primarily bipolar technology), we'll want to extract
       transistors whose substrate terminals are part of the given region.
       The following does that check.  (10-11-88)
    */
#ifdef ARIEL
    if (TTMaskHasType(&ResSubsTypeBitMask,t1) && (ResOptionsFlags & ResOpt_DoSubstrate))
    {
         int	pNum;
	 TileTypeBitMask  *mask = &ExtCurStyle->exts_subsTransistorTypes[t1];

	 for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
         {
	     if (TTMaskIntersect(&DBPlaneTypes[pNum], mask))
	     {
	          (void)DBSrPaintArea((Tile *) NULL, 
		  	cx->tc_scx->scx_use->cu_def->cd_planes[pNum], 
		        &tileArea,mask,resSubSearchFunc, (ClientData) cx);
	     }
         }
    }
#endif
    GeoTransRect(&cx->tc_scx->scx_trans, &tileArea, &newCsa2->csa2_area);
    newCsa2->csa2_type = TiGetType(tile);
    newCsa2->csa2_next = *pHead;
    *pHead = newCsa2;
    return dbcReturn;
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCalcPerimOverlap--
 *
 * Results:
 *
 * Side Effects:
 *
 *-------------------------------------------------------------------------
 */
ResCalcPerimOverlap(trans,tile)
	ResTranTile	*trans;
	Tile		*tile;

{
    Tile	*tp;
    int		t1;
    int		overlap;
    
    trans->perim = (TOP(tile)-BOTTOM(tile)-LEFT(tile)+RIGHT(tile))<<1;
    overlap =0;
    
    t1 = TiGetType(tile);
    /* left */
    for (tp = BL(tile); BOTTOM(tp) < TOP(tile); tp=RT(tp))
    {
	if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),TiGetType(tp))
	{
	      overlap += MIN(TOP(tile),TOP(tp))-
	   		  MAX(BOTTOM(tile),BOTTOM(tp));
	}
    	 
    }
    /*right*/
    for (tp = TR(tile); TOP(tp) > BOTTOM(tile); tp=LB(tp))
    {
	if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),TiGetType(tp))
	{
	      overlap += MIN(TOP(tile),TOP(tp))-
	   		  MAX(BOTTOM(tile),BOTTOM(tp));
	}
    	 
    }
    /*top*/
    for (tp = RT(tile); RIGHT(tp) > LEFT(tile); tp=BL(tp))
    {
	if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),TiGetType(tp))
	{
	      overlap += MIN(RIGHT(tile),RIGHT(tp))-
	   		  MAX(LEFT(tile),LEFT(tp));
	}
    	 
    }
    /*bottom */
    for (tp = LB(tile); LEFT(tp) < RIGHT(tile); tp=TR(tp))
    {
	if TTMaskHasType(&(ExtCurStyle->exts_nodeConn[t1]),TiGetType(tp))
	{
	      overlap += MIN(RIGHT(tile),RIGHT(tp))-
	   		  MAX(LEFT(tile),LEFT(tp));
	}
    	 
    }
    trans->overlap = overlap;
}
	

/*
 * ----------------------------------------------------------------------------
 *
 * DBTreeCopyConnectDCS --
 *
 * 	Basically the same as DBTreeCopyConnect, except it calls 
 *	dbcConnectFuncDCS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the result cell are modified.
 *
 * ----------------------------------------------------------------------------
 */

ResTranTile *
DBTreeCopyConnectDCS(scx, mask, xMask, connect, area, destUse)
    SearchContext *scx;
    TileTypeBitMask *mask;
    int xMask;	
    TileTypeBitMask *connect;
    Rect *area;	
    CellUse *destUse;

{
    SearchContext scx2;
    TileTypeBitMask notConnectMask, *connectMask;
    struct conSrArg2 *list, *current;
    CellDef *def = destUse->cu_def;
    int pNum;
    ResTranTile		*CurrentT;
    static int 		first = 1;
    int			tran;

    if (first)
    {
         TTMaskZero(&DiffTypeBitMask);
	 TTMaskZero(&ResSubsTypeBitMask);
	 for (tran = TT_TECHDEPBASE; tran < TT_MAXTYPES; tran++)
	 {
	      TTMaskSetMask(&DiffTypeBitMask,
	      		&(ExtCurStyle->exts_transSDTypes[tran]));
	      TTMaskSetMask(&ResSubsTypeBitMask,
	      		&(ExtCurStyle->exts_transSubstrateTypes[tran]));
	 }
	 first = 0;

    }
    /* Get one initial tile and put it our list. */

    TransList = NULL;
    list = NULL;
    dbcReturn = 1;
    (void) DBTreeSrTiles(scx, mask, xMask, dbcConnectFuncDCS, (ClientData) &list);
    dbcReturn = 0;

    scx2 = *scx;
    while (list != NULL)
    {
	current = list;
	list = current->csa2_next;

	GeoClip(&current->csa2_area, area);
	if (GEO_RECTNULL(&current->csa2_area)) goto endOfLoop;

	connectMask = &connect[current->csa2_type];

        if (TTMaskHasType(&DBContactBits,current->csa2_type))
        {
             TTMaskSetOnlyType(&notConnectMask, current->csa2_type);
             TTMaskCom(&notConnectMask);
        }
        else
        {
             TTMaskCom2(&notConnectMask, connectMask);
        }
	pNum = DBPlane(current->csa2_type);
	if (DBSrPaintArea((Tile *) NULL, def->cd_planes[pNum],
	    &current->csa2_area, &notConnectMask,
	    dbcUnconnectFunc, (ClientData) &current->csa2_area) != 0)
	{

	    scx2.scx_area = current->csa2_area;
	    scx2.scx_area.r_xbot -= 1;
	    scx2.scx_area.r_ybot -= 1;
	    scx2.scx_area.r_xtop += 1;
	    scx2.scx_area.r_ytop += 1;
	    (void) DBTreeSrTiles(&scx2, connectMask,
		xMask, dbcConnectFuncDCS, (ClientData) &list);
	}

	DBPaintPlane(def->cd_planes[pNum], &current->csa2_area,
	    DBStdPaintTbl(current->csa2_type, pNum), (PaintUndoInfo *) NULL);
	endOfLoop: FREE((char *) current);
    }

	for (CurrentT = TransList; CurrentT != NULL; CurrentT=CurrentT->nextTran)
	{
	     TileType t = CurrentT->type;
	     TileType nt;
	     LayerInfo	*lp = &dbLayerInfo[t];
	     int origin;
	     

	     pNum = DBPlane(t);
	     DBPaintPlane(def->cd_planes[pNum], &CurrentT->area,
	     DBStdPaintTbl(t, pNum), (PaintUndoInfo *) NULL);

	     for (origin = 0; origin != lp->l_nresidues; origin++)
	     {
	       	    if (lp->l_images[origin] == t) break;
	     }
	     if ((nt=lp->l_images[origin+1]) != TT_SPACE)
	     {
	          DBPaintPlane(def->cd_planes[pNum+1], &CurrentT->area,
	          DBStdPaintTbl(nt, pNum+1), (PaintUndoInfo *) NULL);
	     }
	     if (origin && (nt=lp->l_images[origin-1]) != TT_SPACE)
	     {
	          DBPaintPlane(def->cd_planes[pNum-1], &CurrentT->area,
	          DBStdPaintTbl(nt, pNum-1), (PaintUndoInfo *) NULL);
	     }
	}

    DBReComputeBbox(def);
    return(TransList);
}

/*
 *-------------------------------------------------------------------------
 *
 * resSubSearchFunc -- called when DBSrPaintArea finds a transistor within
 *	a substrate area.
 * Results:
 *
 * Side Effects:
 *
 *-------------------------------------------------------------------------
 */
resSubSearchFunc(tile,cx)
	Tile	*tile;
	TreeContext	*cx;
	

{
     ResTranTile	*thisTran;
     Rect		tranArea;
     TileType		t = TiGetType(tile);

     /* Right now, we're only going to extract substrate terminals for 
     	devices with only one diffusion terminal, principally bipolar
	devices.
     */
     if (ExtCurStyle->exts_transSDCount[t] >1) return 0;
     TiToRect(tile, &tranArea);
     MALLOC(ResTranTile *, thisTran, sizeof(ResTranTile));
     GeoTransRect(&cx->tc_scx->scx_trans, &tranArea, &thisTran->area);
     if (t >= DBNumUserLayers)
     {
     	  Tile *tp;
	  int	pNum = DBPlane(t);
	  LayerInfo	*lp = &dbLayerInfo[t];

	  if (lp->l_rbelow == TT_SPACE) pNum++;
	  else pNum--;
     	  tp = cx->tc_scx->scx_use->cu_def->cd_planes[pNum]->pl_hint;
	  GOTOPOINT(tp,&tile->ti_ll);
	  t = TiGetType(tp);
     }
     thisTran->type = t;
     thisTran->nextTran = TransList;
     TransList = thisTran;
     ResCalcPerimOverlap(thisTran,tile);
     
     return 0;
}
