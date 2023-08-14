
#ifndef lint
static char rcsid[] = "$Header: ResPrint.c,v 6.0 90/08/28 18:54:39 mayo Exp $";
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
#include "utils.h"
#include "tech.h"
#include "txcommands.h"
#include "stack.h"
#include	"resis.h"

#define MAXNAME			1000
#define KV_TO_mV		1000000


/*
 *-------------------------------------------------------------------------
 *
 * ResPrintExtRes-- Print resistor network to output file.
 *
 * Results:none
 *
 * Side Effects:prints network.
 *
 *-------------------------------------------------------------------------
 */
ResPrintExtRes(outextfile,resistors,nodename)
	FILE	*outextfile;
	resResistor *resistors;
	char	*nodename;

{
     int	nodenum=0;
     char	newname[MAXNAME];
     HashEntry  *entry;
     ResSimNode *node,*ResInitializeNode();
     
     for (; resistors != NULL; resistors=resistors->rr_nextResistor)
     {
	  /* 
	     These names shouldn't be null; they should either be set by
	     the transistor name or by the node printing routine.  This
	     code is included in case the resistor network is printed
	     before the nodes.
	  */
	  if (resistors->rr_connection1->rn_name == NULL)
	  {
     	       (void)sprintf(newname,"%s%s%d",nodename,".r",nodenum++);
     	       entry = HashFind(&ResNodeTable,newname);
	       node = ResInitializeNode(entry);
	       resistors->rr_connection1->rn_name = node->name;
	       node->oldname = nodename;
	  }
	  if (resistors->rr_connection2->rn_name == NULL)
	  {
     	       (void)sprintf(newname,"%s%s%d",nodename,".r",nodenum++);
     	       entry = HashFind(&ResNodeTable,newname);
	       node = ResInitializeNode(entry);
	       resistors->rr_connection2->rn_name = node->name;
	       node->oldname = nodename;
	  }
	  if (ResOptionsFlags & ResOpt_DoExtFile)
	  {
     	       fprintf(outextfile, "resist \"%s\" \"%s\" %d\n",
	  			resistors->rr_connection1->rn_name,
	  			resistors->rr_connection2->rn_name,
				resistors->rr_value/ExtCurStyle->exts_resistScale);
	  }
     }
}


/*
 *-------------------------------------------------------------------------
 *
 * ResPrintExtTran-- Print out all transistors that have had at least 
 *	one terminal changed.
 *
 * Results:none
 *
 * Side Effects:prints transistor lines to output file
 *
 *-------------------------------------------------------------------------
 */
ResPrintExtTran(outextfile,transistors)
	FILE		*outextfile;
	RTran		*transistors;

{
     for (; transistors != NULL; transistors=transistors->nextTran)
     {
     	  if (transistors->status & TRUE)
	  {
	       if (ResOptionsFlags & ResOpt_DoExtFile)
	       {
	       /*  fet type xl yl xh yh area perim sub gate t1 t2 */
	            fprintf(outextfile,"fet %s %d %d %d %d %d %d %s \"%s\" %d %s \"%s\" %d %s \"%s\" %d %s\n",
	       		ExtCurStyle->exts_transName[transistors->layout->rt_trantype],
	       		transistors->layout->rt_inside.r_ll.p_x,
		        transistors->layout->rt_inside.r_ll.p_y,
	       		transistors->layout->rt_inside.r_ll.p_x+1,
		        transistors->layout->rt_inside.r_ll.p_y+1,
			transistors->layout->rt_area,
			transistors->layout->rt_perim,
	       		ExtCurStyle->exts_transSubstrateName[transistors->layout->rt_trantype],
			transistors->gate->name,
			transistors->layout->rt_length*2,
			transistors->rs_gattr,
			transistors->source->name,
			transistors->layout->rt_width,
			transistors->rs_sattr,
			transistors->drain->name,
			transistors->layout->rt_width,
			transistors->rs_dattr);
		}
	  }
     }
}


/*
 *-------------------------------------------------------------------------
 *
 * ResPrintExtNode-- Prints out all the nodes in the extracted net.
 *
 * Results:none
 *
 * Side Effects: Prints out extracted net. It may add new nodes to the
 *	node hash table.
 *
 *-------------------------------------------------------------------------
 */
ResPrintExtNode(outextfile,nodelist,nodename)
	FILE	*outextfile;
	resNode	*nodelist;
	char	*nodename;

{
     int	nodenum=0;
     char	newname[MAXNAME],tmpname[MAXNAME],*cp;
     HashEntry  *entry;
     ResSimNode *node,*ResInitializeNode();
     
     if (ResOptionsFlags & ResOpt_DoExtFile)
     {
          fprintf(outextfile,"killnode \"%s\"\n",nodename);
     }
     for (; nodelist != NULL; nodelist = nodelist->rn_more)
     {
	  if (nodelist->rn_name == NULL)
	  {
	       (void)sprintf(tmpname,"%s",nodename);
	       cp = tmpname+strlen(tmpname)-1;
               if (*cp == '!' || *cp == '#')
               {
                    *cp = '\0';
               }
     	       (void)sprintf(newname,"%s%s%d",tmpname,".n",nodenum++);
     	       entry = HashFind(&ResNodeTable,newname);
	       node = ResInitializeNode(entry);
	       nodelist->rn_name = node->name;
	       node->oldname = nodename;
	  }

	  if (ResOptionsFlags & ResOpt_DoExtFile)
	  {
	       /* rnode name R C x y  type (R is always 0) */
     	       fprintf(outextfile, "rnode \"%s\" 0 %d %d %d %d\n",
		    nodelist->rn_name,
		    (int) (nodelist->rn_float.rn_area/
		    		ExtCurStyle->exts_capScale),
		    nodelist->rn_loc.p_x,
		    nodelist->rn_loc.p_y,
		    /* the following is TEMPORARILY set to 0 */
		    0);
	  }
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * ResPrintStats -- Prints out the node name, the number of transistors,
 *	and the number of nodes for each net added.  Also keeps a running
 *	track of the totals.
 *
 * Results:
 *
 * Side Effects:
 *
 *-------------------------------------------------------------------------
 */
ResPrintStats(goodies,name)
	ResGlobalParams	*goodies;
	char	*name;
{
     static int	totalnets=0,totalnodes=0,totalresistors=0;
     int nodes,resistors;
     resNode	*node;
     resResistor *res;

     if (goodies == NULL)
     {
     	  fprintf(stderr,"nets:%d nodes:%d resistors:%d\n",
	  	  totalnets,totalnodes,totalresistors);
	  totalnets=0;
	  totalnodes=0;
	  totalresistors=0;
	  return;
     }
     nodes=0;
     resistors=0;
     totalnets++;
     for (node = ResNodeList; node != NULL; node=node->rn_more)

     {
     	  nodes++;
	  totalnodes++;
     }
     for (res = ResResList; res != NULL; res=res->rr_nextResistor)
     {
     	  resistors++;
	  totalresistors++;
     }
     fprintf(stderr,"%s %d %d\n",name,nodes,resistors);
}
