
#ifndef lint
static char rcsid[] = "$Header: ResMakeRes.c,v 6.0 90/08/28 18:54:34 mayo Exp $";
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
#include	"resis.h"



/*
 *--------------------------------------------------------------------------
 *
 * ResCalcTileResistance-- Given a set of partitions for a tile, the tile can
 *	be converted into resistors. To do this, nodes are sorted in the 
 *	direction of current flow. Resistors are created by counting squares
 *	between successive breakpoints. Breakpoints with the same coordinate
 *	are combined.
 *
 *   Results: returns TRUE if the startnode was involved in a merge.
 *
 *   Side Effects:  Resistor structures are produced.  Some nodes may be
 *		    eliminated.
 *--------------------------------------------------------------------------
 *
 */
int 
ResCalcTileResistance(tile,junk,pendingList,doneList)
	Tile 		*tile;
	tileJunk 	*junk;
        resNode		**pendingList,**doneList;

{
     int 		MaxX=MINFINITY,MinX=INFINITY;
     int		MaxY=MINFINITY,MinY=INFINITY;
     int		merged,transistor;
     Breakpoint 	*p1;
     
     merged = FALSE;
     transistor = FALSE;
     
     if ((p1 = junk->breakList)==NULL) return FALSE;
     for (;p1;p1=p1->br_next)
     {
     	  int	x = p1->br_loc.p_x;
     	  int	y = p1->br_loc.p_y;
	  if (x > MaxX) MaxX = x;
	  if (x < MinX) MinX = x;
	  if (y > MaxY) MaxY = y;
	  if (y < MinY) MinY = y;
	  if (p1->br_this->rn_why == RES_NODE_TRANSISTOR)
	  {
	       transistor = TRUE;
	  }
     }
     
     /* Finally, produce resistors for partition. Keep track of whether */
     /* the node was involved in a merge.			  	*/
     
     if (transistor)
     {
     	  merged |= ResCalcNearTransistor(tile,pendingList,doneList,&ResResList);
     }
     else if (MaxY-MinY > MaxX-MinX)
     {
     	  merged |= ResCalcNorthSouth(tile,pendingList,doneList,&ResResList);
     }
     else
     {
     	  merged |= ResCalcEastWest(tile,pendingList,doneList,&ResResList);
     }
     /* 
        For all the new resistors, propagate the resistance from the origin
        to the new nodes.
     */
     return(merged);
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCalcEastWest-- Makes resistors from an EastWest partition.
 *
 * Results: Returns TRUE if the sacredNode was involved in a merge.
 *
 * Side Effects: Makes resistors. Frees breakpoints.
 *
 *-------------------------------------------------------------------------
 */
int
ResCalcEastWest(tile,pendingList,doneList,resList)
	Tile		*tile;
	resNode		**pendingList,**doneList;
	resResistor	**resList;

{
     int 		height,merged;
     Breakpoint		*p1,*p2,*p3;
     resResistor	*resistor;
     resElement		*element;
     resNode		*currNode;
     float		rArea;
     tileJunk		*junk = (tileJunk *)tile->ti_client;
     
     merged = FALSE;
     height = TOP(tile)-BOTTOM(tile);

     /* 
        One Breakpoint? No resistors need to be made. Free up the first
     	breakpoint, then return.
     */
     p1 = junk->breakList;
     if   (p1->br_next == NULL) 
     {
	  p1->br_this->rn_float.rn_area += height*(LEFT(tile)-RIGHT(tile));
     	  FREE((char *)p1);
	  junk->breakList = NULL;
	  return(merged);
     }
     /* re-sort nodes left to right. */
     ResSortBreaks(&junk->breakList,TRUE);
     
     /* 
        Eliminate breakpoints with the same X coordinate and merge 
        their nodes.
     */
     p2= junk->breakList;
     /* add extra left area to leftmost node */
     p2->br_this->rn_float.rn_area += 	height*(p2->br_loc.p_x-LEFT(tile));
     while (p2->br_next != NULL)
     {
     	       p1 = p2;
	       p2=p2->br_next;
	       if (p2->br_loc.p_x == p1->br_loc.p_x)
	       {
	       	    if (p2->br_this == p1->br_this)
		    {
			 currNode = NULL;
			 p1->br_next = p2->br_next;
			 FREE((char *)p2);
			 p2 = p1;
		    }
		    else if (p2->br_this == resCurrentNode)
		    {
			 currNode = p1->br_this;
			 
		    	 ResMergeNodes(p2->br_this,p1->br_this,pendingList,doneList);
			 merged = TRUE;
			 FREE((char *)p1);
		    }
		    else if (p1->br_this == resCurrentNode)
		    {
			 currNode = p2->br_this;
			 p1->br_next = p2->br_next;
		    	 ResMergeNodes(p1->br_this,p2->br_this,pendingList,doneList);
			 merged = TRUE;
			 FREE((char *)p2);
			 p2 = p1;
		    }
		    else
		    {
			 currNode = p1->br_this;
		    	 ResMergeNodes(p2->br_this,p1->br_this,pendingList,doneList);

			 FREE((char *)p1);
		    }

		    /* 
		       Was the node used in another junk or breakpoint?
		       If so, replace the old node with the new one.
		    */
		    p3  = p2->br_next;
		    while (p3 != NULL)
		    {
			if (p3->br_this == currNode)
			{
			     p3->br_this = p2->br_this;
			}
			p3 = p3->br_next;
		    }
	       }

	       /* 
	          If the X coordinates don't match, make a resistor between
	          the breakpoints.
	       */
	       else
	       {
	            MALLOC(resResistor *, resistor, sizeof(resResistor));
	            resistor->rr_nextResistor = (*resList);
	            resistor->rr_lastResistor = NULL;
	            if ((*resList) != NULL) (*resList)->rr_lastResistor = resistor;
	            (*resList) = resistor;
	            resistor->rr_connection1 = p1->br_this;
	            resistor->rr_connection2 = p2->br_this;
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = p1->br_this->rn_re;
	            element->re_thisEl = resistor;
	            p1->br_this->rn_re = element;
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = p2->br_this->rn_re;
	            element->re_thisEl = resistor;
	            p2->br_this->rn_re = element;
		    resistor->rr_value=
		      (ExtCurStyle->exts_sheetResist[TiGetType(tile)]*
		          (p2->br_loc.p_x-p1->br_loc.p_x))/height;
#ifdef ARIEL
		    resistor->rr_csArea = height*ExtCurStyle->exts_thick[TiGetType(tile)];
#endif
		    resistor->rr_tt = TiGetType(tile);
		    rArea = ((p2->br_loc.p_x-p1->br_loc.p_x)*height)/2;
		    resistor->rr_connection1->rn_float.rn_area += rArea;
		    resistor->rr_connection2->rn_float.rn_area += rArea;
		    resistor->rr_float.rr_area = 0;
		    resistor->rr_status = 0;
		    
	            FREE((char *)p1);
	       }
    }
    p2->br_this->rn_float.rn_area +=
	  	height*(RIGHT(tile)-p2->br_loc.p_x);
    FREE((char *)p2);
    junk->breakList = NULL;
    return(merged);
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCalcNorthSouth-- Makes resistors from a NorthSouth partition
 *
 * Results: Returns TRUE if the resCurrentNode was involved in a merge.
 *
 * Side Effects: Makes resistors. Frees breakpoints
 *
 *-------------------------------------------------------------------------
 */
ResCalcNorthSouth(tile,pendingList,doneList,resList)
	Tile		*tile;
	resNode		**pendingList,**doneList;
	resResistor	**resList;

{
     int 		width,merged;
     Breakpoint		*p1,*p2,*p3;
     resResistor	*resistor;
     resElement		*element;
     resNode		*currNode;
     float		rArea;
     tileJunk		*junk = (tileJunk *)tile->ti_client;
     
     merged = FALSE;
     width = RIGHT(tile)-LEFT(tile);

     /* 
        One Breakpoint? No resistors need to be made. Free up the first
     	breakpoint, then return.
     */
     p1 = junk->breakList;
     if   (p1->br_next == NULL) 
     {
	  p1->br_this->rn_float.rn_area += width*(TOP(tile)-BOTTOM(tile));
     	  FREE((char *)p1);
	  junk->breakList= NULL;
	  return(merged);
     }

     /* re-sort nodes south to north. */
     ResSortBreaks(&junk->breakList,FALSE);

     /* 
        Eliminate breakpoints with the same Y coordinate and merge 
        their nodes.
     */

     p2= junk->breakList;
     /* add extra left area to leftmost node */
     p2->br_this->rn_float.rn_area +=
	  	width*(p2->br_loc.p_y-BOTTOM(tile));
     while (p2->br_next != NULL)
     {
     	       p1 = p2;
	       p2=p2->br_next;
	       if (p1->br_loc.p_y == p2->br_loc.p_y)
	       {
	       	    if (p2->br_this == p1->br_this)
		    {
			 currNode = NULL;
		    	 p1->br_next = p2->br_next;
			 FREE((char *)p2);
			 p2 = p1;
			 
		    }
	       	    else if (p2->br_this == resCurrentNode)
		    {
			 currNode = p1->br_this;
		    	 ResMergeNodes(p2->br_this,p1->br_this,pendingList,doneList);
			 FREE((char *)p1);
			 merged = TRUE;
		    }
		    else if (p1->br_this == resCurrentNode)
		    {
			 currNode = p2->br_this;
			 p1->br_next = p2->br_next;
		    	 ResMergeNodes(p1->br_this,p2->br_this,pendingList,doneList);
			 merged = TRUE;
			 FREE((char *)p2);
			 p2 = p1;
		    }
		    else
		    {
			 currNode = p1->br_this;
		    	 ResMergeNodes(p2->br_this,p1->br_this,pendingList,doneList);
			 FREE((char *)p1);
		    }
		    
		    /* 
		       Was the node used in another junk or breakpoint?
		       If so, replace the old node with the new one.
		    */
		    p3  = p2->br_next;
		    while (p3 != NULL)
		    {
			if (p3->br_this == currNode)
			{
			     p3->br_this = p2->br_this;
			}
			p3 = p3->br_next;
		    }
	       }

	       /* 
	          If the Y coordinates don't match, make a resistor between
	          the breakpoints.
	       */
	       else
	       {
	            MALLOC(resResistor *, resistor, sizeof(resResistor));
	            resistor->rr_nextResistor = (*resList);
	            resistor->rr_lastResistor = NULL;
	            if ((*resList) != NULL) (*resList)->rr_lastResistor = resistor;
	            (*resList) = resistor;
	            resistor->rr_connection1 = p1->br_this;
	            resistor->rr_connection2 = p2->br_this;
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = p1->br_this->rn_re;
	            element->re_thisEl = resistor;
	            p1->br_this->rn_re = element;
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = p2->br_this->rn_re;
	            element->re_thisEl = resistor;
	            p2->br_this->rn_re = element;
		    resistor->rr_value=
		      (ExtCurStyle->exts_sheetResist[TiGetType(tile)]*
		          (p2->br_loc.p_y-p1->br_loc.p_y))/width;
#ifdef ARIEL
		    resistor->rr_csArea = width*ExtCurStyle->exts_thick[TiGetType(tile)];
#endif
		    resistor->rr_tt = TiGetType(tile);
		    rArea = ((p2->br_loc.p_y-p1->br_loc.p_y)*width)/2;
		    resistor->rr_connection1->rn_float.rn_area += rArea;
		    resistor->rr_connection2->rn_float.rn_area += rArea;
		    resistor->rr_float.rr_area = 0;

		    resistor->rr_status = 0;
	            FREE((char *)p1);
	       }
    }
    p2->br_this->rn_float.rn_area +=
	  	width*(TOP(tile)-p2->br_loc.p_y);
    FREE((char *)p2);
    junk->breakList = NULL;
    return(merged);
}


/*
 *-------------------------------------------------------------------------
 *
 * ResCalcNearTransistor-- Calculating the direction of current flow near
 *	transistors is tricky because there are two adjoining regions with
 *	vastly different sheet resistances.  ResCalcNearTransistor is called
 *	whenever a diffusion tile adjoining a real tile is found.  It makes
 *	a guess at the correct direction of current flow, removes extra 
 *	breakpoints, and call either ResCalcEastWest or ResCalcNorthSouth
 *
 * Side Effects: Makes resistors. Frees breakpoints
 *
 *-------------------------------------------------------------------------
 */
ResCalcNearTransistor(tile,pendingList,doneList,resList)
	Tile		*tile;
	resNode		**pendingList,**doneList;
	resResistor	**resList;

{
     int 		merged;
     int		trancount,tranedge,deltax,deltay;
     Breakpoint		*p1,*p2,*p3;
     tileJunk		*junk = (tileJunk *)tile->ti_client;
     
     
     merged = FALSE;

     /* 
        One Breakpoint? No resistors need to be made. Free up the first
     	breakpoint, then return.
     */
     
     if   (junk->breakList->br_next == NULL) 
     {
     	  FREE((char *)junk->breakList);
	  junk->breakList = NULL;
	  return(merged);
     }
     /* count the number of transistor breakpoints  */
     /* mark which edge they connect to		    */
     trancount = 0;
     tranedge = 0;
     for (p1=junk->breakList; p1 != NULL;p1 = p1->br_next)
     {
	  if (p1->br_this->rn_why == RES_NODE_TRANSISTOR)
	  {
	       trancount++;
	       if (p1->br_loc.p_x == LEFT(tile)) tranedge |= LEFTEDGE;
	       else if (p1->br_loc.p_x == RIGHT(tile)) tranedge |= RIGHTEDGE;
	       else if (p1->br_loc.p_y == TOP(tile)) tranedge |= TOPEDGE;
	       else if (p1->br_loc.p_y == BOTTOM(tile)) tranedge |= BOTTOMEDGE;
	  }
     }
     /* use distance from transistor to next breakpoint as determinant     */
     /* if there is only one transistor or if all the transitors are along */
     /* the same edge.							   */
     if (trancount == 1 		|| 
        (tranedge & LEFTEDGE) == tranedge	||
        (tranedge & RIGHTEDGE) == tranedge 	||
        (tranedge & TOPEDGE) == tranedge 	||
        (tranedge & BOTTOMEDGE) == tranedge)
     {
	  ResSortBreaks(&junk->breakList,TRUE);
          p2 = NULL;
          for (p1=junk->breakList; p1 != NULL;p1 = p1->br_next)
          {
	       if (p1->br_this->rn_why == RES_NODE_TRANSISTOR)
	       {
	            break;
	       }
	       if (p1->br_next != NULL && 
	       	     (p1->br_loc.p_x != p1->br_next->br_loc.p_x ||
	       	      p1->br_loc.p_y != p1->br_next->br_loc.p_y))
	       
	       {
	       	    p2 = p1;
	       }
          }
	  deltax=INFINITY;
	  for (p3 = p1->br_next; 
	       p3 != NULL && 
	       p3->br_loc.p_x == p1->br_loc.p_x && 
	       p3->br_loc.p_y == p1->br_loc.p_y; p3 = p3->br_next);
	  if (p3 != NULL)
	  {
	       if (p3->br_crect)
	       {
	       	    if (p3->br_crect->r_ll.p_x > p1->br_loc.p_x)
		    {
		         deltax = p3->br_crect->r_ll.p_x-p1->br_loc.p_x;
		    }
	       	    else if (p3->br_crect->r_ur.p_x < p1->br_loc.p_x)
		    {
		         deltax = p1->br_loc.p_x-p3->br_crect->r_ur.p_x;
		    }
		    else
		    {
		    	 deltax=0;
		    }
	       }
	       else
	       {
	       deltax = abs(p1->br_loc.p_x-p3->br_loc.p_x);
	       }
	  }
	  if (p2 != NULL)
	  {
	       if (p2->br_crect)
	       {
	       	    if (p2->br_crect->r_ll.p_x > p1->br_loc.p_x)
		    {
		         deltax = MIN(deltax,p2->br_crect->r_ll.p_x-p1->br_loc.p_x);
		    }
	       	    else if (p2->br_crect->r_ur.p_x < p1->br_loc.p_x)
		    {
		         deltax = MIN(deltax,p1->br_loc.p_x-p2->br_crect->r_ur.p_x);
		    }
		    else
		    {
		    	 deltax=0;
		    }
	       }
	       else
	       {
	            deltax = MIN(deltax,abs(p1->br_loc.p_x-p2->br_loc.p_x));
	       }
	  }

          /* re-sort nodes south to north. */
	  ResSortBreaks(&junk->breakList,FALSE);
          p2 = NULL;
          for (p1=junk->breakList; p1 != NULL;p1 = p1->br_next)
          {
	       if (p1->br_this->rn_why == RES_NODE_TRANSISTOR)
	       {
	            break;
	       }
	       if (p1->br_next != NULL && 
	       	     (p1->br_loc.p_x != p1->br_next->br_loc.p_x ||
	       	      p1->br_loc.p_y != p1->br_next->br_loc.p_y))
	       
	       {
	       	    p2 = p1;
	       }
          }
	  deltay=INFINITY;
	  for (p3 = p1->br_next; 
	       p3 != NULL && 
	       p3->br_loc.p_x == p1->br_loc.p_x && 
	       p3->br_loc.p_y == p1->br_loc.p_y; p3 = p3->br_next);
	  if (p3 != NULL)
	  {
	       if (p3->br_crect)
	       {
	       	    if (p3->br_crect->r_ll.p_y > p1->br_loc.p_y)
		    {
		         deltay = p3->br_crect->r_ll.p_y-p1->br_loc.p_y;
		    }
	       	    else if (p3->br_crect->r_ur.p_y < p1->br_loc.p_y)
		    {
		         deltay = p1->br_loc.p_y-p3->br_crect->r_ur.p_y;
		    }
		    else
		    {
		    	 deltay=0;
		    }
	       }
	       else
	       {
	       deltay = abs(p1->br_loc.p_y-p3->br_loc.p_y);
	       }
	  }
	  if (p2!= NULL)
	  {
	       if (p2->br_crect)
	       {
	       	    if (p2->br_crect->r_ll.p_y > p1->br_loc.p_y)
		    {
		         deltay = MIN(deltay,p2->br_crect->r_ll.p_y-p1->br_loc.p_y);
		    }
	       	    else if (p2->br_crect->r_ur.p_y < p1->br_loc.p_y)
		    {
		         deltay = MIN(deltay,p1->br_loc.p_y-p2->br_crect->r_ur.p_y);
		    }
		    else
		    {
		    	 deltay=0;
		    }
	       }
	       else
	       {
	            deltay = MIN(deltay,abs(p1->br_loc.p_y-p2->br_loc.p_y));
	       }
	  }
	  if (deltay > deltax)
	  {
	       return(ResCalcNorthSouth(tile,pendingList,doneList,resList));
	  }
	  else
	  {
	       return(ResCalcEastWest(tile,pendingList,doneList,resList));
	  }

     }
     /* multiple transistors connected to the partition */
     else  
     {
     	  if (tranedge == 0)
	  {
	       TxError("Error in transistor current direction routine\n");
	       return(merged);
	  }
	  /* check to see if the current flow is north-south		*/
	  /* possible north-south conditions:				*/
	  /* 1. there are transistors along the top and bottom edges    */
	  /*    but not along the left or right			        */
	  /* 2. there are transistors along two sides at right angles,  */
	  /*    and the tile is wider than it is tall.			*/
	  
	  if ((tranedge & TOPEDGE)     && 
	      (tranedge & BOTTOMEDGE)  &&
	      !(tranedge & LEFTEDGE)   &&
	      !(tranedge & RIGHTEDGE)  			||
	      (tranedge & TOPEDGE || tranedge & BOTTOMEDGE) &&
	      (tranedge & LEFTEDGE || tranedge & RIGHTEDGE) &&
	      RIGHT(tile)-LEFT(tile) > TOP(tile)-BOTTOM(tile))
	 {
               /* re-sort nodes south to north. */
	       ResSortBreaks(&junk->breakList,FALSE);

	      /* eliminate duplicate S/D pointers */
	      for (p1 = junk->breakList; p1 != NULL; p1 = p1->br_next)
	      {
	      	   if  (p1->br_this->rn_why == RES_NODE_TRANSISTOR &&
		       (p1->br_loc.p_y == BOTTOM(tile) ||
		        p1->br_loc.p_y == TOP(tile)))
		   {
	      		p3 = NULL;
			p2 = junk->breakList;
			while ( p2 != NULL)
			{
			     if (p2->br_this == p1->br_this	&& 
			         p2 != p1			&&
				 p2->br_loc.p_y != BOTTOM(tile) &&
				 p2->br_loc.p_y != TOP(tile))
			     {
			     	  if (p3 == NULL)
				  {
				       junk->breakList = p2->br_next;
				       FREE((char *) p2);
				       p2 = junk->breakList;
				  }
				  else
				  {
				       p3->br_next = p2->br_next;
				       FREE((char *) p2);
				       p2 = p3->br_next;
				  }
			     }
			     else
			     {
			          p3 = p2;
				  p2 = p2->br_next;
			     }
			}
		   }
	      }
	      return(ResCalcNorthSouth(tile,pendingList,doneList,resList));
	 }
	 else
	 {
	      /* eliminate duplicate S/D pointers */
	      for (p1 = junk->breakList; p1 != NULL; p1 = p1->br_next)
	      {
	      	   if (p1->br_this->rn_why == RES_NODE_TRANSISTOR &&
		       (p1->br_loc.p_x == LEFT(tile) ||
		        p1->br_loc.p_x == RIGHT(tile)))
		   {
	      		p3 = NULL;
			p2 = junk->breakList;
			while ( p2 != NULL)
			{
			     if (p2->br_this == p1->br_this	&& 
			         p2 != p1			&&
				 p2->br_loc.p_x != LEFT(tile) &&
				 p2->br_loc.p_x != RIGHT(tile))
			     {
			     	  if (p3 == NULL)
				  {
				       junk->breakList = p2->br_next;
				       FREE((char *) p2);
				       p2 = junk->breakList;
				  }
				  else
				  {
				       p3->br_next = p2->br_next;
				       FREE((char *) p2);
				       p2 = p3->br_next;
				  }
			     }
			     else
			     {
			          p3 = p2;
				  p2 = p2->br_next;
			     }
			}
		   }
	      }
	      return(ResCalcEastWest(tile,pendingList,doneList,resList));
	 }
     }
}


/*
 *-------------------------------------------------------------------------
 *
 * ResDoContacts-- Add node (or nodes) for a contact.  If there are contact
 *	resistances, also add a resistor.
 *
 * Results: 
 *
 * Side Effects: Creates nodes and resistors
 *
 *-------------------------------------------------------------------------
 */
ResDoContacts(contact,nodes,resList)
	ResContactPoint	*contact;
	resNode		**nodes;
	resResistor	**resList;


{
     resNode		 *resptr;
     cElement		 *ccell;
     int		 tilenum,squares;
     resResistor	 *resistor;
     resElement		 *element;
     static int		 too_small = 1;
     
     
     if (ExtCurStyle->exts_viaResist[contact->cp_type] == 0 ||
         ExtCurStyle->exts_viaSize[contact->cp_type] == 0)

     {
          int x = contact->cp_center.p_x;
          int y = contact->cp_center.p_y;

      	  MALLOC(resNode *,resptr,sizeof(resNode));
	  InitializeNode(resptr,x,y,RES_NODE_CONTACT);
	  ResAddToQueue(resptr,nodes);

 	  MALLOC(cElement *, ccell, sizeof(cElement));
	  ccell->ce_nextc = resptr->rn_ce;
	  resptr->rn_ce = ccell;
	  ccell->ce_thisc = contact;
	  /* add 1 celement for each layer of contact  */
          for (tilenum=0; tilenum < contact->cp_currentcontact; tilenum++)
	  {
	       Tile	*tile = contact->cp_tile[tilenum];

	       contact->cp_cnode[tilenum] = resptr;
	       NEWBREAK(resptr,tile,
	       		contact->cp_center.p_x,contact->cp_center.p_y,
			&contact->cp_rect);
          }
     }
     else
     {
     	  if ((squares = contact->cp_area / ExtCurStyle->exts_viaSize[contact->cp_type]) <= 0)
	  {
	       if (too_small)
	       {
	            TxError("Warning: %s at %d %d smaller than extract section allows\n",
	            DBTypeLongNameTbl[contact->cp_type],
	            contact->cp_center.p_x,contact->cp_center.p_y);
	            too_small = 0;
	       }
	       squares = 1;
	  }
          for (tilenum=0; tilenum < contact->cp_currentcontact; tilenum++)
	  {
      	       int x = contact->cp_center.p_x;
      	       int y = contact->cp_center.p_y;
	       Tile	*tile = contact->cp_tile[tilenum];

	       MALLOC(resNode *,resptr,sizeof(resNode));
	       InitializeNode(resptr,x,y,RES_NODE_CONTACT);
	       ResAddToQueue(resptr,nodes);
	  
 	       /* add contact pointer to node  */
	       MALLOC(cElement *, ccell, sizeof(cElement));
	       ccell->ce_nextc = resptr->rn_ce;
	       resptr->rn_ce = ccell;
	       ccell->ce_thisc = contact;
	       
	       contact->cp_cnode[tilenum] = resptr;
	       NEWBREAK(resptr,tile,
	       		contact->cp_center.p_x,contact->cp_center.p_y,
			&contact->cp_rect);

	       /* add resistors here */
	       if (tilenum > 0)
	       {
	            MALLOC(resResistor *, resistor, sizeof(resResistor));
	            resistor->rr_nextResistor = (*resList);
	            resistor->rr_lastResistor = NULL;
	            if ((*resList) != NULL) (*resList)->rr_lastResistor = resistor;
	            (*resList) = resistor;
	            resistor->rr_connection1 = contact->cp_cnode[tilenum-1];
	            resistor->rr_connection2 = contact->cp_cnode[tilenum];
		    
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = contact->cp_cnode[tilenum-1]->rn_re;
	            element->re_thisEl = resistor;
	            contact->cp_cnode[tilenum-1]->rn_re = element;
	            MALLOC(resElement *,element,sizeof(resElement));
	            element->re_nextEl = contact->cp_cnode[tilenum]->rn_re;
	            element->re_thisEl = resistor;
	            contact->cp_cnode[tilenum]->rn_re = element;
		    resistor->rr_value= 
		    	ExtCurStyle->exts_viaResist[contact->cp_type]/squares;
#ifdef ARIEL
		    resistor->rr_csArea=
		    	ExtCurStyle->exts_thick[contact->cp_type]/squares;
#endif
		    resistor->rr_tt = contact->cp_type;
		    resistor->rr_float.rr_area= 0;
		    resistor->rr_status = 0;
	       }
	  }
     }
}

ResSortBreaks(masterlist,xsort)
	Breakpoint	**masterlist;
	int		xsort;

{
     Breakpoint		*p1,*p2,*p3,*p4;
     int changed;

     changed = TRUE;
     while (changed == TRUE)
     {
	  changed = FALSE;
	  p1 = NULL;
	  p2 = *masterlist;
	  p3 = p2->br_next;
	  while (p3 != NULL)
	  {
	       if (xsort == TRUE  && p2->br_loc.p_x > p3->br_loc.p_x ||
	           xsort == FALSE && p2->br_loc.p_y > p3->br_loc.p_y)
	       {
		      changed = TRUE;
		      if (p1 == NULL)
		      {
		      	   *masterlist = p3;
		      }
		      else
		      {
		      	   p1->br_next = p3;
		      }
		      p2->br_next = p3->br_next;
		      p3->br_next = p2;
		      p4 = p2;
		      p2=p3;
		      p3=p4;
	       }
	       else
	       {
	            p1=p2;
	            p2=p3;
	       	    p3=p3->br_next;
	       }
	  }
     }
     
}
	
