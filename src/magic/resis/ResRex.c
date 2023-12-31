
#ifndef lint
static char rcsid[] = "$Header: ResRex.c,v 6.0 90/08/28 18:54:43 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* It is hard to get the value of MAXFLOAT in a portable manner. */
#ifdef	ibm032
#define MAXFLOAT        ((float)3.40282346638528860e+38)
#else
#include <values.h>
#endif

#undef	MAXINT
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
#include "utils.h"
#include "tech.h"
#include "txcommands.h"
#include	"resis.h"
#ifdef LAPLACE
#include	"laplace.h"
#endif

#define INITFLATSIZE		1024
#define MAXNAME			1000


/* time constants are produced by multipliying attofarads by milliohms, */
/* giving milliattoseconds (or piconanoseconds, depending on how you    */
/* look at it.)  This constant is used to convert them to nanoseconds.  */
#define MA_TO_N		1e12

/* ResSimNode is a node read in from a sim file */


HashTable 	ResNodeTable;   /* Hash table of sim file nodes   */
RTran		*ResTranList;	/* Linked list of Sim transistors */
ResGlobalParams	gparams;	/* Junk passed between 		  */
					/* ResCheckSimNodes and 	  */
					/* ResExtractNet.		  */
int		Maxtnumber;     	/*maximum transistor number 	  */
ResSimNode	*ResOriginalNodes;	/*Linked List of Nodes		  */
int		resNodeNum;

#ifdef LAPLACE
int	ResOptionsFlags = ResOpt_Simplify|ResOpt_Tdi|ResOpt_DoExtFile|ResOpt_CacheLaplace;
#else
int	ResOptionsFlags = ResOpt_Simplify|ResOpt_Tdi|ResOpt_DoExtFile;
#endif
char	*ResCurrentNode;
    CellDef	*mainDef;

FILE	*ResExtFile;
FILE	*ResLumpFile;


/*
 *-------------------------------------------------------------------------
 *
 * CmdExtResis--  reads in sim file and layout, and produces patches to the
 *	.ext files and .sim files that include resistors.
 *
 * Results: returns 0 if it completes correctly.
 *
 * Side Effects: Produces .res.sim file and .res.ext file for all nets that
 *	require resistors.
 *
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
CmdExtResis(win, cmd)
	Window *win;
	TxCommand *cmd;
{
    RTran	*oldTran;
    HashSearch	hs;
    HashEntry	*entry;
    tranPtr	*tptr,*oldtptr;
    ResSimNode  *node;
    int		i,j,k,option;
    static int		init=1;
    static float	tolerance,tdiTolerance;

    static char *cmdExtresisCmd[] = 
    {
    	 "tolerance value      set ratio between resistor and transistor tol.",
	 "all 		       extract all the nets",
	 "simplify [on/off]    turn on/off simplification of resistor nets",
	 "extout   [on/off]    turn on/off writing of .res.ext file",
	 "lumped   [on/off]    turn on/off writing of updated lumped resistances",
	 "silent   [on/off]    turn on/off printing of net statistics",
	 "skip     mask        don't extract these types",
	 "box      type        extract the signal under the box on layer type",
	 "help                 print this message",
#ifdef LAPLACE
	 "laplace  [on/off]    solve Laplace's equation using FEM",
#endif
	 NULL
    };

#define RES_AMBIG		-1
#define RES_BAD			-2
#define RES_TOL			0
#define RES_ALL			1
#define RES_SIMP		2
#define RES_EXTOUT		3
#define RES_LUMPED		4
#define RES_SILENT		5
#define RES_SKIP		6
#define RES_BOX			7
#define RES_HELP		8
#ifdef LAPLACE
#define RES_LAPLACE		9
#define RES_RUN			10
#else
#define RES_RUN			9
#endif


    if (init)
    {
         for (i=0;i != NT;i++)
    	 {
              TTMaskZero(&(ResCopyMask[i]));
	      TTMaskSetMask(&ResCopyMask[i],&DBConnectTbl[i]);
     	 }
	 tolerance = 1;
	 tdiTolerance = 1;
    	 
	 init = 0;
    }
    if (ToolGetBoxWindow((Rect *) NULL, (int *) NULL) == NULL)
    {
	TxError("Sorry, the box must appear in one of the windows.\n");
	return;
    }

    option= (cmd->tx_argc >1)?Lookup(cmd->tx_argv[1],cmdExtresisCmd):RES_RUN;
    switch (option)
    {
	 case RES_TOL:
	      ResOptionsFlags |=  ResOpt_ExplicitRtol;
	      if (cmd->tx_argc >2) 
	      {
		   tolerance = atof(cmd->tx_argv[2]);
		   if (tolerance <= 0)
		   {
		      TxError("Usage:  %s tolerance [value]\n", cmd->tx_argv[0]);
			 return;
		   }
		   tdiTolerance = tolerance;
	      }
	      return;
	 case RES_ALL:
	      ResOptionsFlags |= ResOpt_ExtractAll;
	      break;
	 case RES_SIMP:
	      if (cmd->tx_argc == 2 || strcmp(cmd->tx_argv[2],"on")==0)
	      	   ResOptionsFlags |= ResOpt_Simplify|ResOpt_Tdi; 
	      else
	      	   ResOptionsFlags &= ~(ResOpt_Simplify|ResOpt_Tdi);
	      return;
	 case RES_EXTOUT:
	      if (cmd->tx_argc == 2 || strcmp(cmd->tx_argv[2],"on")==0)
	      	   ResOptionsFlags |= ResOpt_DoExtFile;
	      else
	      	   ResOptionsFlags &= ~ResOpt_DoExtFile;
	      return;
	 case RES_LUMPED:
	      if (cmd->tx_argc == 2 || strcmp(cmd->tx_argv[2],"on")==0)
	      	   ResOptionsFlags |= ResOpt_DoLumpFile;
	      else
	      	   ResOptionsFlags &= ~ResOpt_DoLumpFile;
	      return;
	 case RES_SILENT:
	      if (cmd->tx_argc == 2 || strcmp(cmd->tx_argv[2],"on")==0)
	      	   ResOptionsFlags |= ResOpt_RunSilent;
	      else
	      	   ResOptionsFlags &= ~ResOpt_RunSilent;
	      return;
	 case RES_SKIP:
	      if (cmd->tx_argc >2) 
	      {
		   j = DBTechNoisyNameType(cmd->tx_argv[2]);
		   if (j >= 0) for (k = TT_TECHDEPBASE; k < TT_MAXTYPES; k++)
		   {
			TTMaskClearType(&ResCopyMask[k],j);
		   }
		   TTMaskZero(&(ResCopyMask[j]));
	      }
	      else
	      {
    		   for (i=0;i != NT;i++)
    		   {
         		TTMaskZero(&(ResCopyMask[i]));
	 		TTMaskSetMask(&ResCopyMask[i],&DBConnectTbl[i]);
     		   }
	      }
	      return;
	 case RES_HELP:
	      {
		     for (i=0; cmdExtresisCmd[i] != NULL;i++)
		     {
		     	  fprintf(stdout,"%s\n",cmdExtresisCmd[i]);
		     }
	      }
	      return;
	 case RES_BOX:
	      {
		   TileType	tt;
		   CellDef	*def;
		   Rect		rect;
		   int		oldoptions;
		   ResFixPoint	fp;

	     	   if (cmd->tx_argc != 3) return;
		   tt = DBTechNoisyNameType(cmd->tx_argv[2]);
		   if (tt <= 0 || ToolGetBox(&def,&rect)== FALSE) return;
		   gparams.rg_tranloc = &rect.r_ll;
		   gparams.rg_ttype = tt;
		   gparams.rg_status = DRIVEONLY;
		   oldoptions = ResOptionsFlags;
		   ResOptionsFlags = ResOpt_DoSubstrate|ResOpt_Signal|ResOpt_Box;
#ifdef LAPLACE
		   ResOptionsFlags |= (oldoptions & (ResOpt_CacheLaplace|ResOpt_DoLaplace));
		   LaplaceMatchCount = 0;
		   LaplaceMissCount = 0;
#endif
		   fp.fp_ttype = tt;
		   fp.fp_loc = rect.r_ll;
		   fp.fp_next = NULL;
		   if (ResExtractNet(&fp,&gparams) != 0) return;
		   ResPrintResistorList(stdout,ResResList);
		   ResPrintTransistorList(stdout,ResTransList);
#ifdef LAPLACE
		   if (ResOptionsFlags & ResOpt_DoLaplace)
		   {
		   	fprintf(stdout,"Laplace   solved: %d matched %d\n",
				LaplaceMissCount,LaplaceMatchCount);
		   }
#endif
		   
		   ResOptionsFlags = oldoptions;
		   return;
	      }
	 case RES_RUN:
	      ResOptionsFlags &= ~ResOpt_ExtractAll;
	      break;
#ifdef LAPLACE
	 case RES_LAPLACE:
	      LaplaceParseString(cmd);
	      return;
#endif
	 case RES_AMBIG:
  	      fprintf(stdout,"Ambiguous option: %s\n",cmd->tx_argv[1]);
	      fflush(stdout);
	      return;
	 case RES_BAD:
  	      fprintf(stdout,"Unknown option: %s\n",cmd->tx_argv[1]);
	      fflush(stdout);
	      return;
	 default:
	      return;

    }
    
#ifdef LAPLACE
    LaplaceMatchCount = 0;
    LaplaceMissCount = 0;
#endif
    /* turn off undo stuff */
    UndoDisable();

    ResTranList = NULL;
    ResOriginalNodes = NULL;

    if (ToolGetBox(&mainDef,(Rect *) NULL) == NULL)
    {
    	 TxError("Couldn't find def corresponding to box\n");
	 return;
    }
    ResOptionsFlags |= 	 ResOpt_Signal;
#ifdef ARIEL
    ResOptionsFlags &= 	~ResOpt_Power;
#endif

    /* read in .sim file */
    Maxtnumber = 0;
    HashInit(&ResNodeTable, INITFLATSIZE, HT_STRINGKEYS);
	 /* read in .nodes file   */
	 {
   	      if (ResReadSim(mainDef->cd_name,
	      		ResSimTransistor,ResSimCapacitor,ResSimResistor,
			ResSimAttribute,ResSimMerge) == NULL)
	      {
	           if (ResReadNode(mainDef->cd_name) == NULL)
	           {
	                /* Extract networks for nets that require it. */
	                ResCheckSimNodes(tolerance,tdiTolerance,mainDef->cd_name);
	                if (ResOptionsFlags & ResOpt_Stat)
	                {
	                     ResPrintStats((ResGlobalParams *)NULL,"");
	                }
	           }
	      }
	 }
    HashStartSearch(&hs);
    while((entry = HashNext(&ResNodeTable,&hs)) != NULL)
    {
	  node=(ResSimNode *) HashGetValue(entry);
	  tptr = node->firstTran;
	  while (tptr != NULL)
	  {
	       oldtptr = tptr;
	       tptr = tptr->nextTran;
	       FREE((char *)oldtptr);
	  }
	  FREE((char *) node);
    }
    HashKill(&ResNodeTable);
    while (ResTranList != NULL)
    {
    	 oldTran = ResTranList;
	 ResTranList = ResTranList->nextTran;
	 if (oldTran->layout != NULL)
	 {
	      FREE((char *)oldTran->layout);
	      oldTran->layout = NULL;
	 }
	 FREE((char *)oldTran);
    }
#ifdef MALLOCTRACE
    mallocTraceDone();
#endif

    /* turn back on undo stuff */
    UndoEnable();
#ifdef LAPLACE
    if (ResOptionsFlags & ResOpt_DoLaplace)
    {
	fprintf(stdout,"Laplace   solved: %d matched %d\n",
				LaplaceMissCount,LaplaceMatchCount);
    }
#endif

    return;
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCheckSimNodes-- check to see if lumped resistance is greater than the
 *		      transistor resistance; if it is, Extract the net 
 *		      resistance. If the maximum point to point resistance
 *		      in the extracted net is still creater than the 
 *		      tolerance, then output the extracted net.
 *
 * Results: none
 *
 * Side Effects: Writes networks to .res.ext and .res.sim files.
 *
 *-------------------------------------------------------------------------
 */
ResCheckSimNodes(tol,rctol,outfile)
	float		tol,rctol;
	char		*outfile;

{
     ResSimNode		*node;
     tranPtr		*ptr;
     float		ftolerance,rctolerance,minRes,cumRes;
     int		failed1=0;
     int 		failed3=0;
     int		total =0;
     char		*last4,*last3;
     
     if (ResOptionsFlags & ResOpt_DoExtFile)
     {
          ResExtFile = PaOpen(outfile,"w",".res.ext",".",(char *) NULL, (char **) NULL);
     }
     else
     {
     	  ResExtFile = NULL;
     }
     if (ResOptionsFlags & ResOpt_DoLumpFile)
     {
          ResLumpFile = PaOpen(outfile,"w",".res.lump",".",(char *) NULL, (char **) NULL);
     }
     else
     {
     	  ResLumpFile = NULL;
     }
     if (ResExtFile == NULL && (ResOptionsFlags & ResOpt_DoExtFile)
         || (ResOptionsFlags & ResOpt_DoLumpFile) && ResLumpFile == NULL)
     {
     	  TxError("Couldn't open output file\n");
	  return;
     }
     for (node = ResOriginalNodes; node != NULL; node=node->nextnode)
     {
	  /* hack !! don't extract Vdd or GND lines  */
	  last4 = node->name+strlen(node->name)-4;
	  last3 = node->name+strlen(node->name)-3;
	  ResCurrentNode = node->name;

	  if ((strncmp(last4,"Vdd!",4) == 0 || 
	      strncmp(last4,"VDD!",4) == 0 ||
	      strncmp(last4,"vdd!",4) == 0 ||
	      strncmp(last4,"Gnd!",4) == 0 ||
	      strncmp(last4,"gnd!",4) == 0 ||
	      strncmp(last4,"GND!",4) == 0 ||
	      strncmp(last3,"Vdd",3) == 0 || 
	      strncmp(last3,"VDD",3) == 0 ||
	      strncmp(last3,"vdd",3) == 0 ||
	      strncmp(last3,"Gnd",3) == 0 ||
	      strncmp(last3,"gnd",3) == 0 ||
	      strncmp(last3,"GND",3) == 0) &&
	      (node->status & FORCE) != FORCE) continue;

	  /* Has this node been merged away or is it marked as skipped? */
	  /* If so, skip it */
	  if ((node->status & FORWARD) || ((node->status & SKIP) && 
	  	(ResOptionsFlags & ResOpt_ExtractAll)==0)) continue;
	  total++;
	  
     	  ResSortByGate(&node->firstTran);
	  /* Find largest SD transistor connected to node.	*/
	  
	  minRes = MAXFLOAT;
	  gparams.rg_tranloc = (Point *) NULL;
	  gparams.rg_status = FALSE;
	  gparams.rg_nodecap = node->capacitance;

	  /* the following is only used if there is a drivepoint */
	  /* to identify which tile the drivepoint is on.	 */
	  gparams.rg_ttype = node->rs_ttype;

	  for (ptr = node->firstTran; ptr != NULL; ptr=ptr->nextTran)
	  {
	       RTran	*t1;
	       RTran	*t2;

	       if (ptr->terminal == GATE)
	       {
	       	    break;
	       }
	       else
	       {
	       	    /* get cumulative resistance of all transistors */
		    /* with same connections.			    */
		    cumRes = ptr->thisTran->resistance;
	            t1 = ptr->thisTran;
		    for (; ptr->nextTran != NULL; ptr = ptr->nextTran)
		    {
	                 t1 = ptr->thisTran;
			 t2 = ptr->nextTran->thisTran;
			 if (t1->gate != t2->gate) break;
			 if ((t1->source != t2->source ||
			     t1->drain  != t2->drain) &&
			    (t1->source != t2->drain ||
			     t1->drain  != t2->source)) break;

			 /* do parallel combination  */
			 if (cumRes != 0.0 && t2->resistance != 0.0)
			 {
			      cumRes = (cumRes*t2->resistance)/
			      	       (cumRes+t2->resistance);
			 }
			 else
			 {
			      cumRes = 0;
			 }
			 
		    }
		    if (minRes > cumRes)
		    {
		         minRes = cumRes;
		    	 gparams.rg_tranloc = &t1->location;
			 gparams.rg_ttype = t1->rs_ttype;
		    }
	       }
	  }
	  /* special handling for FORCE and DRIVELOC labels:  */
	  /* set minRes = node->minsizeres if it exists, 0 otherwise */
	  if (node->status & (FORCE|DRIVELOC))
	  {
	      if (node->status & MINSIZE)
	      {
	      	   minRes = node->minsizeres;
	      }
	      else
	      {
	      	   minRes = 0;
	      }
	      if (node->status  & DRIVELOC)
	      {
	       	    gparams.rg_tranloc = &node->drivepoint;
		    gparams.rg_status |= DRIVEONLY;
	      }
	  }
	  if (gparams.rg_tranloc == NULL && node->status & FORCE)
	  {
    	       TxError("Node %s has force label but no drive point or driving transistor\n",node->name);
	  }
	  if (minRes == MAXFLOAT || gparams.rg_tranloc == NULL)
	  {
	       continue;
	  }
	  gparams.rg_bigtranres = (int)minRes*OHMSTOMILLIOHMS;
	  if (rctol == 0.0 || tol == 0.0)
	  {
	       ftolerance = 0.0;
	       rctolerance = 0.0;
	  }
	  else
	  {
	       ftolerance =  minRes/tol;
	       rctolerance = minRes/rctol;
	  }
	  /* 
	     Is the transistor resistance greater than the lumped node 
	     resistance? If so, extract net.
	  */
	  if (node->resistance > ftolerance || node->status & FORCE ||
	      (ResOpt_ExtractAll & ResOptionsFlags))
	  {
	       ResFixPoint	fp;

	       failed1++;
	       fp.fp_loc = node->location;
	       fp.fp_ttype = node->type;
	       fp.fp_next = NULL;
	       if (ResExtractNet(&fp,&gparams) != 0)
	       {
	       	    TxError("Error in extracting node %s\n",node->name);
		    break;
	       }
	       else
	       {
		    ResDoSimplify(ftolerance,rctol,&gparams);
		    if (ResOptionsFlags & ResOpt_DoLumpFile)
		    {
		         ResWriteLumpFile(node);
		    }
		    if (gparams.rg_maxres >= ftolerance  || 
		        gparams.rg_maxres >= rctolerance || 
			(ResOptionsFlags & ResOpt_ExtractAll))
		    {
		         resNodeNum = 0;
			 failed3 += ResWriteExtFile(node,tol,rctol);
		    }
	       }
#ifdef PARANOID
	       ResSanityChecks(node->name,ResResList,ResNodeList,ResTransList);
#endif
	       ResCleanUpEverything();
	  }
     }
     
     /* 
        Print out all transistors which have had at least one terminal changed
        by resistance extraction.
     */
     if (ResOptionsFlags & ResOpt_DoExtFile)
     {
          ResPrintExtTran(ResExtFile,ResTranList);
     }

     /* Output statistics about extraction */
     if (total)
     {
           fprintf(stderr,
             "Total Nets: %d\nNets extracted: %d (%f)\nNets output: %d (%f)\n",
     		total,failed1,(float)failed1/(float)total,failed3,
		(float)failed3/(float)total);
     }
     else
     {
          fprintf(stderr,"Total Nodes: %d\n",total);
     }
     /* close output files */
     if (ResExtFile != NULL) 
     {
     	  fclose(ResExtFile);
     }
     if (ResLumpFile != NULL)
     {
     	  (void) fclose(ResLumpFile);
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * ResFixUpConnections-- Changes the connection to  a terminal of the sim 
 *	transistor.  The new name is formed by appending .t# to the old name.
 *	The new name is added to the hash table of node names.
 *
 * Results:none
 *
 * Side Effects: Allocates new ResSimNodes. Modifies the terminal connections
 *	of sim Transistors.
 *
 *-------------------------------------------------------------------------
 */
ResFixUpConnections(simTran,layoutTran,simNode,nodename)
	RTran		*simTran;
	resTransistor 	*layoutTran;
	ResSimNode	*simNode;
	char		*nodename;

{
     static char	newname[MAXNAME],oldnodename[MAXNAME];
     int		notdecremented;
     resNode		*gate,*source,*drain;
     
     /* If we aren't doing output (i.e. this is just a statistical run) */
     /* don't patch up networks.  This cuts down on memory use.		*/

     if ((ResOptionsFlags & (ResOpt_DoRsmFile | ResOpt_DoExtFile)) == 0)
     {
     	  return;
     }
     if (simTran->layout == NULL)
     {
          layoutTran->rt_status |= RES_TRAN_SAVE;
          simTran->layout = layoutTran;
     }
     simTran->status |= TRUE;
     if (strcmp(nodename,oldnodename) != 0)
     {
     	  strcpy(oldnodename,nodename);
     }
     (void)sprintf(newname,"%s%s%d",nodename,".t",resNodeNum++);
     notdecremented = TRUE;
     
     if (simTran->gate == simNode)
     {
     	  if ((gate=layoutTran->rt_gate) != NULL)
	  {
	       /* cosmetic addition: If the layout tran already has a      */
	       /* name, the new one won't be used, so we decrement resNodeNum */
	       if (gate->rn_name != NULL)
	       {
	       	    resNodeNum--;
		    notdecremented = FALSE;
	       }  

	       ResFixTranName(newname,GATE,simTran,gate);
	       gate->rn_name = simTran->gate->name;
     	       (void)sprintf(newname,"%s%s%d",nodename,".t",resNodeNum++);
	  }
	  else
	  {
	       TxError("Missing gate connection\n");
	  }
     }
     if (simTran->source == simNode)
     {
     	  if (simTran->drain == simNode)
	  {
	       if ((source=layoutTran->rt_source) && 
	       	   (drain=layoutTran->rt_drain))
	       {
	            if (source->rn_name != NULL && notdecremented)
		    {
		    	 resNodeNum--;
			 notdecremented = FALSE;
		    }  
	            ResFixTranName(newname,SOURCE,simTran,source);
	            source->rn_name = simTran->source->name;
		    (void)sprintf(newname,"%s%s%d",nodename,".t",resNodeNum++);
	            if (drain->rn_name != NULL)  resNodeNum--;
	            ResFixTranName(newname,DRAIN,simTran,drain);
	            drain->rn_name = simTran->drain->name;
	       	    /* one to each */
	       }
	       else
	       {
	       	    TxError("Missing SD connection\n");
	       }
	  }
	  else
	  {
	       if (source=layoutTran->rt_source)
	       {
	       	    if (drain=layoutTran->rt_drain)
		    {
			      if (source != drain)
			      {
			           if (drain->rn_why & RES_NODE_ORIGIN)
				   {
				        ResMergeNodes(drain,source,
					 &ResNodeQueue,&ResNodeList);
		  	                ResDoneWithNode(drain);
					source = drain;
				   }
				   else
				   {
				        ResMergeNodes(source,drain,
					 &ResNodeQueue,&ResNodeList);
		  	                ResDoneWithNode(source);
					drain = source;
				   }
			      }
			      layoutTran->rt_drain = (resNode *)NULL;
	            	      if (source->rn_name != NULL)  resNodeNum--;
	            	      ResFixTranName(newname,SOURCE,simTran,source);
	            	      source->rn_name = simTran->source->name;
		    }
		    else
		    {
             	         if (source->rn_name != NULL && notdecremented)
			 {
			      resNodeNum--;
			      notdecremented = FALSE;
			 }  
	            	 ResFixTranName(newname,SOURCE,simTran,source);
	            	 source->rn_name = simTran->source->name;
		    }
		    
	       }
	       else
	       {
	       	    TxError("missing SD connection\n");
	       }
	  }
     }
     else if (simTran->drain == simNode)
     {
       if (source=layoutTran->rt_source)
       {
       	    if (drain=layoutTran->rt_drain)
	    {
	          if (drain != source)
		  {
			if (drain->rn_why & ORIGIN)
			{
			     ResMergeNodes(drain,source,
				      &ResNodeQueue,&ResNodeList);
		             ResDoneWithNode(drain);
			     source = drain;
			}
  		      else
		      {
 		             ResMergeNodes(source,drain,
				      &ResNodeQueue,&ResNodeList);
		             ResDoneWithNode(source);
			     drain = source;
		      }
		  }
		  layoutTran->rt_source = (resNode *) NULL;
             	  if (drain->rn_name != NULL)
		  {
		       resNodeNum--;
		       notdecremented = FALSE;
		  }  
	          ResFixTranName(newname,DRAIN,simTran,drain);
	          drain->rn_name = simTran->drain->name;
	    }
	    else
	    {
             	 if (source->rn_name != NULL  && notdecremented)
		 {
		      resNodeNum--;
		      notdecremented = FALSE;
		 }  
	         ResFixTranName(newname,DRAIN,simTran,source);
	         source->rn_name = simTran->drain->name;
	    }
	    
       }
       else
       {
       	    TxError("missing SD connection\n");
       }
    }
    else
    {
    	 resNodeNum--;
    }
}


/*
 *-------------------------------------------------------------------------
 *
 *  ResFixTranName-- Moves transistor connection to new node.
 *
 * Results: returns zero if node is added correctly, one otherwise.
 *
 * Side Effects: May create a new node. Creates a new transistor pointer.
 *
 *-------------------------------------------------------------------------
 */
ResFixTranName(line,type,transistor,layoutnode)
	char 		line[];
	int		type;
	RTran		*transistor;
	resNode		*layoutnode;

{
     HashEntry		*entry;
     ResSimNode		*node,*ResInitializeNode();
     tranPtr		*tptr;
     
     if (layoutnode->rn_name != NULL)
     {
          entry = HashFind(&ResNodeTable,layoutnode->rn_name);
          node = ResInitializeNode(entry);
     	  
     }
     else
     {
          entry = HashFind(&ResNodeTable,line);
          node = ResInitializeNode(entry);
     }
     MALLOC(tranPtr *,tptr, sizeof(tranPtr));
     tptr->thisTran = transistor;
     tptr->nextTran = node->firstTran;
     node->firstTran = tptr;
     tptr->terminal = type;
     switch(type)
     {
     	  case GATE:   node->oldname = transistor->gate->name;
	  	       transistor->gate = node;
	  	       break;
     	  case SOURCE: node->oldname = transistor->source->name;
	  	       transistor->source = node;
	  	       break;
     	  case DRAIN:  node->oldname = transistor->drain->name;
	  	       transistor->drain = node;
	  	       break;
	  default:  TxError("Bad Terminal Specifier\n");
	  		break;
     }
}


/*
 *-------------------------------------------------------------------------
 *
 *  ResSortByGate--sorts transistor pointers whose terminal field is either
 *	drain or source by gate node number, then by drain (source) number.
 *	This places transistors with identical connections next to one 
 *	another.
 *
 * Results: none
 *
 * Side Effects: modifies order of transistors
 *
 *-------------------------------------------------------------------------
 */
ResSortByGate(TranpointerList)
	tranPtr	**TranpointerList;

{
     int	changed=TRUE;
     int	localchange=TRUE;
     tranPtr	*working,*last=NULL,*current,*gatelist=NULL;
     
     working = *TranpointerList;
     while (working != NULL)
     {
          if (working->terminal == GATE)
          {
	       current = working;
	       working = working->nextTran;
       	       if (last == NULL)
	       {
		    *TranpointerList = working;
	       }
	       else
	       {
	      	    last->nextTran = working;
	       }
	       current->nextTran = gatelist;
	       gatelist = current;
	  }
	  else
	  {
	       last = working;
	       working = working->nextTran;
	  }
     }
     while (changed == TRUE)
     {
          changed = localchange = FALSE;
	  working = *TranpointerList;
     	  last = NULL;
	  while (working != NULL && (current = working->nextTran) != NULL)
	  {
	       	    RTran	*w = working->thisTran;
		    RTran	*c = current->thisTran;

		    if (w->gate > c->gate)
		    {
		    	 changed = TRUE;
			 localchange = TRUE;
		    }
		    else if (w->gate == c->gate     &&
			     (working->terminal == SOURCE && 
			     current->terminal == SOURCE && 
			     w->drain > c->drain    ||
			     working->terminal == SOURCE && 
			     current->terminal == DRAIN && 
			     w->drain > c->source    ||
			     working->terminal == DRAIN && 
			     current->terminal == SOURCE && 
			     w->source > c->drain    ||
			     working->terminal == DRAIN && 
			     current->terminal == DRAIN && 
			     w->source >  c->source))
		     {
			 changed = TRUE;
			 localchange = TRUE;
		     }
		     else
		     {
		     	  last = working;
			  working = working->nextTran;
			  continue;
		     }
		     if (localchange)
		     {
			 localchange = FALSE;
			 if (last == NULL)
			 {
			      *TranpointerList = current;
			 }
			 else
			 {
			      last->nextTran = current;
			 }
			 working->nextTran = current->nextTran;
			 current->nextTran = working;
			 last = current;
		    }
	  }
     }
     if (working == NULL)
     {
     	  *TranpointerList = gatelist;
     }
     else
     {
     	  if (working->nextTran != NULL)
	  {
	       TxError("Bad Transistor pointer in sort\n");
	  }
	  else
	  {
	       working->nextTran = gatelist;
	  }
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * ResWriteLumpFile
 *
 * Results:
 *
 * Side Effects:
 *
 *-------------------------------------------------------------------------
 */
ResWriteLumpFile(node)
	ResSimNode	*node;

{
     int	lumpedres;

     if (ResOptionsFlags & ResOpt_Tdi)
     {
	  if (gparams.rg_nodecap != 0)
	  {
	      lumpedres = (int)((gparams.rg_Tdi/gparams.rg_nodecap
			-(float)(gparams.rg_bigtranres))/OHMSTOMILLIOHMS);
	  }
          else
          {
	      lumpedres = 0;
          }
     }
     else
     {
          lumpedres = gparams.rg_maxres;
     }
     fprintf(ResLumpFile,"R %s %d\n",node->name,lumpedres);
     
}

/*
 *-------------------------------------------------------------------------
 *
 * ResWriteExtFile
 *
 * Results:
 *
 * Side Effects:
 *
 *-------------------------------------------------------------------------
 */
ResWriteExtFile(node,tol,rctol)
	ResSimNode	*node;
	float		tol,rctol;

{
     float	RCtran;
     char	*cp,newname[MAXNAME];
     tranPtr	*ptr;
     resTransistor	*layoutFet,*ResGetTransistor();
     
     RCtran = gparams.rg_bigtranres*gparams.rg_nodecap;

     if (tol == 0.0 ||(node->status & FORCE) ||
	 (ResOptionsFlags & ResOpt_ExtractAll)||
	 (ResOptionsFlags & ResOpt_Simplify)==0||
	 (rctol+1)*RCtran < rctol*gparams.rg_Tdi)
     {
	  ASSERT(gparams.rg_Tdi != -1,"ResWriteExtFile");
	  (void)sprintf(newname,"%s",node->name);
          cp = newname+strlen(newname)-1;
          if (*cp == '!' || *cp == '#') *cp = '\0';
	  if ((rctol+1)*RCtran < rctol*gparams.rg_Tdi || 
	  			(ResOptionsFlags & ResOpt_Tdi) == 0)
	  {
	       if ((ResOptionsFlags & (ResOpt_RunSilent|ResOpt_Tdi)) == ResOpt_Tdi)
	       {
		     fprintf(stderr,"Adding  %s; Tnew = %.2fns,Told = %.2fns\n",
		     	    node->name,gparams.rg_Tdi/MA_TO_N, RCtran/MA_TO_N);
	       }
          }
          for (ptr = node->firstTran; ptr != NULL; ptr=ptr->nextTran)
          {
	       if (layoutFet = ResGetTransistor(&ptr->thisTran->location))
	       {
		     ResFixUpConnections(ptr->thisTran,layoutFet,node,newname);
	       }
	  }
          if (ResOptionsFlags & ResOpt_DoExtFile)
          {
		ResPrintExtNode(ResExtFile,ResNodeList,node->name);
      	        ResPrintExtRes(ResExtFile,ResResList,newname);
          }
	  return 1;
     }
     else return 0;
}
