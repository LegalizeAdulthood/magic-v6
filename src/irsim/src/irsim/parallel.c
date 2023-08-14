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
#include "defs.h"
#include "net.h"
#include "globals.h"

#include "net_macros.h"


private int    nored[ NTTYPES ];

#define	hash_terms( T )		( (Ulong)((T)->source) ^ (Ulong)((T)->drain) )

#define	COMBINE( R1, R2 )	( ((R1) * (R2)) / ((R1) + (R2)) )

#define	SMALL_R			0.1


/*
 * Run through the list of nodes, collapsing all transistors with the same
 * gate/source/drain into a compound transistor.  As a side effect, clear
 * flag bits set in 'cl' (usually VISITED) on all nodes on the list.
 */
public void make_parallel( nlist, cl )
  register nptr  nlist;
  register long  cl;
  {
    register nptr   n;
    register lptr   l1, l2, prev;
    register tptr   t1, t2;
    register Ulong  hval;
    register int    ttype;

    for( cl = ~cl; nlist != NULL; nlist->nflags &= cl, nlist = nlist->n.next )
      {
	for( l1 = nlist->nterm; l1 != NULL; l1 = l1->next )
	  {
	    t1 = l1->xtor;
	    ttype = t1->ttype;
	    if( ttype & (GATELIST | TCAP | ORED) )
		continue;	/* ORED implies processed, so skip as well */

	    hval = hash_terms( t1 );
	    prev = l1;
	    for( l2 = l1->next; l2 != NULL; prev = l2, l2 = l2->next )
	      {
		t2 = l2->xtor;
		if( t1->gate != t2->gate or hash_terms( t2 ) != hval or 
		  ttype != (t2->ttype & ~ORED) )
		    continue;

		t1->r.rstatic = COMBINE( t1->r.rstatic, t2->r.rstatic );
		t1->r.dynlow = COMBINE( t1->r.dynlow, t2->r.dynlow );
		t1->r.dynhigh = COMBINE( t1->r.dynhigh, t2->r.dynhigh );
		t1->ttype |= ORED;

		DISCONNECT( t2->gate->ngate, t2 );	/* disconnect gate */
		if( t2->source == nlist )		/* disconnect term1 */
		    DISCONNECT( t2->drain->nterm, t2 )
		else
		    DISCONNECT( t2->source->nterm, t2 );

		prev->next = l2->next;			/* disconnect term2 */
		FREE_LINK( l2 );
		l2 = prev;

/*		FREE_TRANS( t2 );		/* get rid of transistor */
		/* do not free, so we can still use its x, y position */

		t2->ttype |= (ORED | TCAP);		/* mark deleted */
		t2->dcache.t = t1;		/* this is the real txtor */

		t2->scache.t = ored_list;
		ored_list = t2;

		nored[ BASETYPE( t1->ttype ) ]++;
	      }
	  }
      }
  }



public int DestroyParallel( t, width, length )
  tptr   t;
  float  width, length;
  {
    Resists  r;
    float    tmp;

    nored[ BASETYPE( t->ttype ) ] -= 1;
    requiv( width, length, BASETYPE( t->ttype ), &r );
    tmp = r.rstatic - t->r.rstatic;
    if( tmp > SMALL_R )			/* avoid division by zero */
      {
	t->r.rstatic = t->r.rstatic * r.rstatic / tmp;
	tmp = r.dynlow - t->r.dynlow;
	t->r.dynlow = t->r.dynlow * r.dynlow / tmp;
	tmp = r.dynhigh - t->r.dynhigh;
	t->r.dynhigh = t->r.dynhigh * r.dynhigh / tmp;
	return( FALSE );
      }
    else
	return( TRUE );
  }


public int pParallelTxtors()
  {
    int  i, any;

    lprintf( stdout, "parallel txtors:" );
    for( i = any = 0; i < NTTYPES; i++ )
      {
	if( nored[i] != 0 )
	  {
	    lprintf( stdout, " %s=%d", ttype[i], nored[i] );
	    any = TRUE;
	  }
      }
    lprintf( stdout, "%s\n", (any) ? "" : "none" );
  }


public void WriteParallel( f )
  FILE  *f;
  {
    int  i;

    for( i = 0; i < NTTYPES; i++ )
	fprintf( f, "%d\n", nored[i] );
  }


public void ReadParallel( f )
  FILE  *f;
  {
    int   i;
    char  s[15];

    for( i = 0; i < NTTYPES; i++ )
      {
	if( fgetline( s, 15, f ) == NULL )
	  {
	    fprintf( stderr, "premature eof\n" );
	    exit( 1 );
	  }
	nored[i] = ntrans[i] = atoi( s );
      }
  }


#define	HASH_TERMS( S, D )		( (Ulong)(S) ^ (Ulong)(D) )


#define	SAME_TYPE( TRAN, TTYP )	\
   ( BASETYPE( (TRAN)->ttype ) == BASETYPE( TTYP ) or \
   ( ((TRAN)->ttype & (TTYP) & ALWAYSON) == ALWAYSON ) )
    

private tptr Find_Trans( ttype, gate, src, drn )
  unsigned int   ttype;
  register nptr  gate, src, drn;
  {
    register lptr  l;
    register tptr  t;
    register long  hval;

    if( src == drn or (src->nflags & drn->nflags & POWER_RAIL) )
      {
	hval = HASH_TERMS( src, drn );
	for( t = tcap_list; t != NULL; t = t->scache.t )
	  {
	    if( t->gate == gate and hval == hash_terms( t ) and 
	      SAME_TYPE( t, ttype ) )
		return( t );
	  }
      }
    else if( (src->nflags & MERGED) or (drn->nflags & MERGED) )
      {
	t = ((src->nflags & MERGED) ? drn : src)->t.tran;
	if( SAME_TYPE( t, ttype ) )
	  {
	    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
	      {
		if( gate == t->gate )
		    return( t );
	      }
	  }
      }
    else
      {
	l = ((src->nflags & POWER_RAIL) ? drn : src)->nterm;
	for( hval = HASH_TERMS( src, drn ); l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( hval == hash_terms( t ) and gate == t->gate and 
	      SAME_TYPE( t, ttype ) )
		return( t );
	  }
      }
    return( NULL );
  }


public void AssignOredTrans()
  {
    tptr  tran, t;

    for( tran = ored_list; tran != NULL; tran = tran->scache.t )
      {
	if( tran->dcache.t != NULL )
	    continue;

	t = Find_Trans( tran->ttype, tran->gate, tran->source, tran->drain );
	if( t == NULL )
	    lprintf( stderr, "warning: could not find OR'D transistor\n" );
	else
	    tran->dcache.t = t;
      }
  }
