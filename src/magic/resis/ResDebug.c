
#ifndef lint
static char rcsid[] = "$Header: ResDebug.c,v 6.0 90/08/28 18:54:29 mayo Exp $";
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
#include "resis.h"

#define MAXNAME			1000
#define KV_TO_mV		1000000


/*
 *-------------------------------------------------------------------------
 *
 * ResPrintNodeList--  Prints out all the nodes in nodelist.
 *
 *
 *  Results: node
 *
 *  Side effects: prints out the 'nodes' in list to fp.
 *
 *
 *-------------------------------------------------------------------------
 */
ResPrintNodeList(fp,list)
	FILE *fp;
	resNode *list;

{

     for (; list != NULL; list = list->rn_more)
     {
     	  fprintf(fp, "node %x: (%d %d) r= %d\n",
	  	list,list->rn_loc.p_x,list->rn_loc.p_y,list->rn_noderes);
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * ResPrintResistorList--
 *
 *
 * results: none
 *
 *
 * side effects: prints out Resistors in list to file fp.
 *
 *-------------------------------------------------------------------------
 */
ResPrintResistorList(fp,list)
	FILE *fp;
	resResistor *list;

{
     
     for (; list != NULL; list = list->rr_nextResistor)
     {
     	  fprintf(fp, "r (%d,%d) (%d,%d) r=%d\n",
	          list->rr_connection1->rn_loc.p_x,
	          list->rr_connection1->rn_loc.p_y,
	          list->rr_connection2->rn_loc.p_x,
	          list->rr_connection2->rn_loc.p_y,
		  list->rr_value);
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * ResPrintTransistorList--
 *
 *
 *  Results: none
 *
 *  Side effects: prints out transistors in list to file fp.
 *
 *-------------------------------------------------------------------------
 */
ResPrintTransistorList(fp,list)
	FILE *fp;
	resTransistor *list;

{
     static char termtype[] = {'g','s','d','c'};
     int i;
     for (; list != NULL; list = list->rt_nextTran)
     {
     	  if (list->rt_status & RES_TRAN_PLUG) continue;
	  fprintf(fp, "t w %d l %d ",
			list->rt_width,
			list->rt_length);
	  for (i=0; i!= RT_TERMCOUNT;i++)
	  {
	       if (list->rt_terminals[i] == NULL) continue;
	       fprintf(fp, "%c (%d,%d) ",termtype[i],
	       		list->rt_terminals[i]->rn_loc.p_x,
			list->rt_terminals[i]->rn_loc.p_y);
	       
	  }
	  fprintf(fp,"\n");
     }
}
