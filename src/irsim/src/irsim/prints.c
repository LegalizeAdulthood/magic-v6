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

/*
 * Prints various messages but deals with varargs correctly.
 */

#include <stdio.h>
#include <varargs.h>
#include "defs.h"


extern	FILE  *logfile;


private void logprint( s )
  register char  *s;
  {
    static int  docomment = 1;

    while( *s != '\0' )
      {
	if( docomment )
	  {
	    putc( '|', logfile );
	    putc( ' ', logfile );
	    docomment = 0;
	  }
	putc( *s, logfile );
	if( *s++ == '\n' )
	    docomment = 1;
      }
  }


/* VARARGS */
public void lprintf( va_alist )
  va_dcl
  {
    va_list  args;
    char     *fmt;
    FILE     *fp;
    char     buff[ 300 ];

    va_start( args );
    fp = va_arg( args, FILE * );
    fmt = va_arg( args, char * );
    (void) vsprintf( buff, fmt, args );
    va_end( args );

    fputs( buff, fp );

    if( logfile != NULL )
	logprint( buff );
  }


/* VARARGS */
public void error( va_alist )
  va_dcl
  {
    va_list  args;
    char     *filename;
    int      lineno;
    char     *fmt;
    char     buf1[ 100 ], buf2[ 200 ];

    va_start( args );
    filename = va_arg( args, char * );
    lineno = va_arg( args, int );
    fmt = va_arg( args, char * );
    (void) sprintf( buf1, "(%s,%d): ", filename, lineno );
    (void) vsprintf( buf2, fmt, args );
    va_end( args );
    fputs( buf1, stderr );
    fputs( buf2, stderr );
    if( logfile != NULL )
      {
	logprint( buf1 );
	logprint( buf2 );
      }
  }
