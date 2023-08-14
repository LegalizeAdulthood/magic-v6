/*
 *     ********************************************************************* 
 *     * Copyright (C) 1988, 1990 Stanford University.                     * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  Stanford University                 * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 */

#include "ana.h"
#include "ana_glob.h"


private void MoveToT( str )
  char  *str;
  {
    TimeType  start;
    double    tmp;

    if( str == NULL )
      {
	XBell( display, 0 );
	return;
      }
    tmp = atof( str );
    start = (int) ns2d( tmp );
    if( start < tims.first or start > tims.last or start == tims.start )
	return;

    tims.start = start;
    tims.end = start + tims.steps;
    RedrawTimes();
    UpdateScrollBar();
    DrawTraces( start, tims.end );
  }


public void MoveToTime( s )
   char  *s;				/* the menu string => ignore it */
  {
    Query( "\nEnter Time > ", MoveToT );
  }
