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

#include <stdio.h>
#include <defs.h>


/*
 * My version of fgets, fread, and fwrite.  These routines provide the same
 * functionality as the stdio ones; taking care of restarting the operation
 * upon an interrupt condition.  This is mostly for system V. 
 */


public char *fgetline( bp, len, fp )
  char           *bp;
  register int   len;
  register FILE  *fp;
  {
    register char  *buff = bp;
    register int  c;

    while( --len > 0 )
      {
      again :
	c = getc( fp );
	if( c == EOF )
	  {
	    if( feof( fp ) == 0 )
	      {
		clearerr( fp );
		goto again;
	      }
	    *buff = '\0';
	    return( NULL );
	  }
	*buff++ = c;
	if( c == '\n' )
	    break;
      }
    *buff = '\0';
    return( bp );
  }


public int Fread( ptr, size, fp )
  char  *ptr;
  int   size;
  FILE  *fp;
  {
    register int  ret;

  again :
    ret = fread( ptr, 1, size, fp );
    if( ret <= 0 and feof( fp ) == 0 )
      {
	clearerr( fp );
	goto again;
      }
    return( ret );
  }


public int Fwrite( ptr, size, fp )
  char  *ptr;
  int   size;
  FILE  *fp;
  {
    register int  ret;

  again :
    ret = fwrite( ptr, 1, size, fp );
    if( ret <= 0 and feof( fp ) == 0 )
      {
	clearerr( fp );
	goto again;
      }
    return( ret );
  }
