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


#ifdef SYS_V

#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#include "defs.h"


	/* time and usage at program start */
private	time_t      time0;
private struct tms  ru0;

	/* time and usage before command is executed */
private	time_t      time1;
private struct tms  ru1;

private	long mem0;


public void InitUsage()
  {
    time0 = time( 0 );
    (void) times( &ru0 );
    mem0 = (long) sbrk( 0 );
  }

public void set_usage()
  {
    time1 = time( 0 );
    (void) times( &ru1 );
  }


private char *pr_secs( dst, l )
  char  *dst;
  long  l;
  {
    register int  i;

    i = l / 3600;
    if( i != 0 )
      {
	sprintf( dst, "%d:%02d", i, (l % 3600) / 60 );
	i = l % 3600;
      }
    else
      {
	i = l;
	sprintf( dst, "%d", i / 60 );
      }
    while( *++dst );

    i %= 60;
    *dst++ = ':';
    sprintf( dst, "%02d ", i );
    dst += 3;
    return( dst );
  }


private void pr_usage( dst, r0, r1, t0, t1 )
  register char         *dst;
  register struct  tms  *r0, *r1;
  register time_t       *t0, *t1;
  {
    register time_t  t, dt;
    int      mem;

    dt = r1->tms_utime - r0->tms_utime;
    sprintf( dst, "%d.%01du ", dt / HZ, dt / (HZ / 10) );
    while( *++dst );

    dt = r1->tms_stime - r0->tms_stime;
    sprintf( dst, "%d.%01ds ", dt / HZ, dt / (HZ / 10) );
    while( *++dst );

    t = *t1 - *t0;
    dst = pr_secs( dst, t );

    dt = r1->tms_utime - r0->tms_utime + r1->tms_stime - r0->tms_stime;
    t *= HZ;

    sprintf( dst, "%d%% ", (int) (dt * 100 / ( (t ? t : 1 ) )) );
    while( *++dst );

    mem = sbrk( 0 ) - mem0;
    sprintf( dst, "%dK\n", mem / 1024 );
  }


public int print_usage( partial, dest )
  int   partial;
  char  *dest;
  {
    time_t      time2;
    struct tms  ru2;

    time2 = time( 0 );
    (void) times( &ru2 );

    if( partial )
	pr_usage( dest, &ru1, &ru2, &time1, &time2 );
    else
	pr_usage( dest, &ru0, &ru2, &time0, &time2 );
    return( 0 );
  }

#else			/* BSD */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "defs.h"


	/* time and usage at program start */
private	struct timeval    time0;
private struct rusage     ru0;

	/* time and usage before command is executed */
private	struct timeval    time1;
private struct rusage     ru1;


public void InitUsage()
  {
    gettimeofday( &time0, (struct timezone *) 0 );
    getrusage( RUSAGE_SELF, &ru0 );
  }


public void set_usage()
  {
    gettimeofday( &time1, (struct timezone *) 0 );
    getrusage( RUSAGE_SELF, &ru1 );
  }


private void tvsub( tdiff, t1, t0 )
  struct timeval *tdiff, *t1, *t0;
  {
    tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
    tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
    if (tdiff->tv_usec < 0)
      {
	tdiff->tv_sec--;
	tdiff->tv_usec += 1000000;
      }
  }


private char *pr_secs( dst, l )
  char  *dst;
  long  l;
  {
    register int  i;

    i = l / 3600;
    if( i != 0 )
      {
	sprintf( dst, "%d:%02d", i, (l % 3600) / 60 );
	i = l % 3600;
      }
    else
      {
	i = l;
	sprintf( dst, "%d", i / 60 );
      }
    while( *++dst );

    i %= 60;
    *dst++ = ':';
    sprintf( dst, "%02d ", i );
    dst += 3;
    return( dst );
  }


#define	u2m( A )	( (A) / 10000 )		/* usec to msec */
#define	u2d( A )	( (A) / 100000 )	/* usec to 10th sec */

private void pr_usage( dst, r0, r1, t0, t1 )
  register char            *dst;
  register struct rusage   *r0, *r1;
  register struct timeval  *t0, *t1;
  {
    register time_t  t;
    struct timeval   dt;
    int              ms;

    tvsub( &dt, &r1->ru_utime, &r0->ru_utime );
    sprintf( dst, "%d.%01du ", dt.tv_sec, u2d( dt.tv_usec ) );
    while( *++dst );

    tvsub( &dt, &r1->ru_stime, &r0->ru_stime );
    sprintf( dst, "%d.%01ds ", dt.tv_sec, u2d( dt.tv_usec ) );
    while( *++dst );

    ms = (t1->tv_sec - t0->tv_sec) * 100 + u2m( t1->tv_usec - t0->tv_usec);
    dst = pr_secs( dst, ms / 100 );

    t = (r1->ru_utime.tv_sec - r0->ru_utime.tv_sec) * 100 +
        u2m( r1->ru_utime.tv_usec - r0->ru_utime.tv_usec ) +
        (r1->ru_stime.tv_sec - r0->ru_stime.tv_sec) * 100 +
        u2m( r1->ru_stime.tv_usec - r0->ru_stime.tv_usec );
	        
    sprintf( dst, "%d%% ", (int) (t * 100 / ( (ms ? ms : 1 ) )) );
    while( *++dst );

    sprintf( dst, "%dK\n", r1->ru_maxrss / 2 );
  }


public int print_usage( partial, dest )
  int   partial;
  char  *dest;
  {
    struct timeval  time2;
    struct rusage   ru2;

    gettimeofday( &time2, (struct timezone *) 0 );
    getrusage( RUSAGE_SELF, &ru2 );

    if( partial )
	pr_usage( dest, &ru1, &ru2, &time1, &time2 );
    else
	pr_usage( dest, &ru0, &ru2, &time0, &time2 );
    return( 0 );
  }

#endif SYS_V
