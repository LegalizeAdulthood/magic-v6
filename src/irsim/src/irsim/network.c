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

/* event driven mosfet simulator. Chris Terman (2/80) */

public	iptr  hinputs = NULL;	/* list of nodes to be driven high */
public	iptr  linputs = NULL;	/* list of nodes to be driven low */
public	iptr  uinputs = NULL;	/* list of nodes to be driven X */
public	iptr  xinputs = NULL;	/* list of nodes to be removed from input */

public	iptr  o_hinputs = NULL;	/* list of nodes driven high */
public	iptr  o_linputs = NULL;	/* list of nodes driven low */
public	iptr  o_uinputs = NULL;	/* list of nodes removed from input */

public	iptr  infree = NULL;	/* unused input list cells */

public	iptr  *listTbl[8];

public	FILE  *sfile;		/* current state file */


public void init_listTbl()
  {
    listTbl[ INPUT_NUM( H_INPUT ) ] = &hinputs;
    listTbl[ INPUT_NUM( L_INPUT ) ] = &linputs;
    listTbl[ INPUT_NUM( U_INPUT ) ] = &uinputs;
    listTbl[ INPUT_NUM( X_INPUT ) ] = &xinputs;

    listTbl[ INPUT_NUM( H_INPUT | OLD_INPUT ) ] = &o_hinputs;
    listTbl[ INPUT_NUM( L_INPUT | OLD_INPUT ) ] = &o_linputs;
    listTbl[ INPUT_NUM( U_INPUT | OLD_INPUT ) ] = &o_uinputs;
  }


#define	pvalue( node_name, node )	\
    lprintf( stdout, "%s=%c ", (node_name), "0XX1"[(node)->npot] )


private void pgvalue( t )
  register tptr  t;
  {
    register nptr  n;
    static char    *states[] = { "OFF", "ON", "UKNOWN", "WEAK" };

    if( debug )
	lprintf( stdout, "[%s] ", states[t->state] );

    if( t->ttype & GATELIST )
      {
	lprintf( stdout, "( " );
	for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
	  {
	    n = t->gate;
	    pvalue( pnode( n ), n );
	  }

	lprintf( stdout, ") " );
      }
    else
      {
	n = t->gate;
	pvalue( pnode( n ), n );
      }
  }


private void ptrans( t )
  register tptr  t;
  {
    lprintf( stdout, "%s ", ttype[BASETYPE( t->ttype )] );
    if( BASETYPE( t->ttype ) != RESIST )
	pgvalue( t );

    pvalue( pnode( t->source ), t->source );
    pvalue( pnode( t->drain ), t->drain );
    lprintf( stdout, "[%2.1e, %2.1e, %2.1e]\n",
      t->r.rstatic, t->r.dynhigh, t->r.dynlow );
  }


public void idelete( n, list )
  register nptr  n;
  register iptr  *list;
  {
    register iptr  p = *list;
    register iptr  q;

    if( p == NULL )
	return;
    else if( p->inode == n )
      {
	*list = p->next;
	p->next = infree;
	infree = p;
	return;
      }
    else
      {
	for( q = p->next; q != NULL; p = q, q = p->next )
	    if( q->inode == n )
	      {
		p->next = q->next;
		q->next = infree;
		infree = q;
		return;
	      }
      }
  }


public void iinsert( n, list )
  nptr  n;
  iptr  *list;
  {
    register iptr  p = infree;
    register int   i;

    if( p == NULL )
      {		/* this is ok since iptr->next is the same as MList->next */
	p = infree = (iptr) MallocList( sizeof( struct Input ) );
	if( p == NULL)
	  {
	    fprintf( stdout, "RSIM: *** Malloc returns NULL pointer. ***\n" );
	    exit( 1 );
	  }
      }
    infree = p->next;

    p->next = *list;
    *list = p;
    p->inode = n;
  }


public void ClearInputs()
  {
    register int   i;
    register iptr  p, next;
    register nptr  n;

    for( i = 0; i < 8; i++ )
      {
	if( listTbl[ i ] == NULL )
	    continue;
	for( p = *listTbl[ i ]; p != NULL; p = next )
	  {
	    next = p->next;
	    n = p->inode;
	    p->next = infree;
	    infree = p;
	    if( not( n->nflags & POWER_RAIL ) )
		n->nflags &= ~(INPUT_MASK | INPUT);
	  }
	*(listTbl[ i ]) = NULL;
      }
  }


private	char  setpot[] = "luuh";


/*
 * set/clear input status of node and add/remove it to/from corresponding list.
 */
public int setin( n, which )
  register nptr  n;
  char           which;
  {
    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & (POWER_RAIL | MERGED) )	/* Gnd, Vdd, or merged node */
      {
	if( (n->nflags & MERGED) or setpot[ n->npot ] != which )
	    lprintf( stdout, "Can't set `%s' to `%c'\n", pnode( n ), which );
      }
    else
      {
	iptr  *list = listTbl[ INPUT_NUM( n->nflags ) ];

	switch( which )
	  {
	    case 'h' :
		if( list == &hinputs or list == &o_hinputs )
		    break;
		if( list )
		    idelete( n, list );
		n->nflags = (n->nflags & ~INPUT_MASK) | H_INPUT;
		iinsert( n, &hinputs );
		break;

	    case 'l' :
		if( list == &linputs or list == &o_linputs )
		    break;
		if( list )
		    idelete( n, list );
		n->nflags = (n->nflags & ~INPUT_MASK) | L_INPUT;
		iinsert( n, &linputs );
		break;

	    case 'u' :
		if( list == &uinputs or list == &o_uinputs )
		    break;
		if( list )
		    idelete( n, list );
		n->nflags = (n->nflags & ~INPUT_MASK) | U_INPUT;
		iinsert( n, &uinputs );
		break;

	    case 'x' :
		if( list == &xinputs )
		    break;
		if( list )
		    idelete( n, list );
		if( n->nflags & INPUT )
		  {
		    n->nflags = (n->nflags & ~INPUT_MASK) | X_INPUT;
		    iinsert( n, &xinputs );
		  }
		break;
	  }
      }
    return( 1 );
  }


private int wr_value( n )
  register nptr  n;
  {
    int  ch;

    if( n->nflags & ALIAS )
	ch = 'a';
    else if( n->nflags & INPUT )
	ch = '4' + n->npot;
    else
	ch = '0' + n->npot;
    putc( ch, sfile );
  }


public int wr_state( fname )
  char  *fname;
  {
    if( (sfile = fopen( fname, "w" )) == NULL )
	return( 1 );

    fprintf( sfile, "%d\n", nnodes );
    walk_net( wr_value );
    fclose( sfile );
    return( 0 );
  }


public int rd_restore( n )
  register nptr  n;
  {
    int  ch;

    FreeHistList( n );
    while( n->events != NULL )		/* remove any pending events */
	free_event( n->events );

    ch = (sfile == NULL) ? (X + '0') : getc( sfile );
    if( ch == EOF )
      {
	ch = X + '0';
	fclose( sfile );
	sfile = NULL;
      }

    if( n->nflags & (POWER_RAIL | ALIAS) )
	return( 0 );

    if( ch < '0' or ch > '7' or ch == '2' or ch == '6' )
      {
	error( "state file", 0, "bad node value (%c)\n", ch );
	ch = X + '0';
      }

    if( ch < '4' )		/* The node was not an input before. */
      {
	ch = ch - '0';
	n->npot = ch;
	n->head.val = ch;
	n->head.inp = 0;
      }
    else
      {
	iptr  *list;

	ch = ch - '4';
	switch( ch )
	  {
	    case LOW :   n->nflags |= (L_INPUT | OLD_INPUT | INPUT);  break;
	    case HIGH :  n->nflags |= (H_INPUT | OLD_INPUT | INPUT);  break;
	    case X :     n->nflags |= (U_INPUT | OLD_INPUT | INPUT);  break;
	  }
	n->npot = ch;
	n->head.val = ch;
	n->head.inp = 1;
	list = listTbl[ INPUT_NUM( n->nflags ) ];
	iinsert( n, list );
      }
    return( 0 );
  }


public int rd_value( n )
  register nptr  n;
  {
    int  ch;

    FreeHistList( n );
    while( n->events != NULL )		/* remove any pending events */
	free_event( n->events );

    ch = (sfile == NULL) ? (X + '0') : getc( sfile );
    if( ch == EOF )
      {
	ch = X + '0';
	fclose( sfile );
	sfile = NULL;
      }

    if( n->nflags & (POWER_RAIL | ALIAS) )
	return( 0 );

    if( ch < '0' or ch > '7' or ch == '2' or ch == '6' )
      {
	error( "state file", 0, "bad node value (%c)\n", ch );
	ch = X;
      }
    else
	ch = (ch - '0') & (LOW | X | HIGH);

    n->npot = ch;
    n->head.val = ch;
    n->head.inp = 0;
    return( 0 );
  }


private void fix_trans_state( n )
  nptr  n;
  {
    register tptr  t;
    register lptr  l;

    if( n->nflags & ALIAS )
	return;

    for( l = n->ngate; l != NULL; l = l->next )
      {
	t = l->xtor;
	t->state = compute_trans_state( t );
      }
  }


public int rd_state( fname, fun )
  char  *fname;
  int   (*fun)();
  {
    char  rline[25];
    int   ch;

    if( (sfile = fopen( fname, "r" )) == NULL )
	return( 1 );

    fgetline( rline, 100, sfile );
    if( atoi( rline ) != nnodes )
      {
	fclose( sfile );
	return( 2 );
      }
    while( (ch = getc( sfile )) < '0' );
    ungetc( ch, sfile );
    ClearInputs();
    if( analyzerON )
	StopAnalyzer();
    sim_time0 = cur_delta = 0;

    walk_net( fun );

    walk_net( fix_trans_state );

    if( analyzerON )
	RestartAnalyzer( sim_time0, cur_delta, FALSE );

    if( sfile == NULL )
	return( 3 );
    fclose( sfile );
    NoInit();
    return( 0 );
  }


public int info( n, which )
  register nptr  n;
  {
    register tptr  t;
    register lptr  l;
    char           *name;

    if( n == NULL )
	return( 0 );

    name = pnode( n );
    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & MERGED )
      {
	lprintf( stdout, "%s => node is inside a transistor stack\n", name );
	return( 1 );
      }

    pvalue( name, n );
    if( n->nflags & INPUT )
	lprintf( stdout, "[NOTE: node is an input] " );
    lprintf( stdout, "(vl=%.2f vh=%.2f) ", n->vlow, n->vhigh );
    if( n->nflags & USERDELAY )
	lprintf( stdout, "(tplh=%d, tphl=%d) ", n->tplh, n->tphl );
    lprintf( stdout, "(%4.3f pf) ", n->ncap );

    lprintf( stdout, "%s:\n", which ? "affects" : "is computed from" );
    if( which == 0 )
      {
	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    lprintf( stdout, "  " );
	    if( debug == 0 )
	      {
		nptr  rail;

		rail = (t->drain->nflags & POWER_RAIL) ? t->drain : t->source;
		if( BASETYPE( t->ttype ) == NCHAN and rail == GND_node )
		  {
		    lprintf( stdout, "pulled down by " );
		    pgvalue( t );
		    lprintf( stdout, " [%2.1e, %2.1e, %2.1e]\n",
		      t->r.rstatic, t->r.dynhigh, t->r.dynlow );
		  }
		else if( BASETYPE( t->ttype ) == PCHAN and rail == VDD_node )
		  {
		    lprintf( stdout, "pulled up by " );
		    pgvalue( t );
		    lprintf( stdout, " [%2.1e, %2.1e, %2.1e]\n",
		      t->r.rstatic, t->r.dynhigh, t->r.dynlow );
		  }
		else if( t->ttype == PULLUP )
		    lprintf( stdout, "pulled up [%2.1e, %2.1e, %2.1e]\n",
		      t->r.rstatic, t->r.dynhigh, t->r.dynlow );
		else
		    ptrans( t );
	      }
	    else
		ptrans( t );
	  }
      }
    else
      {
	for( l = n->ngate; l != NULL; l = l->next )
	    ptrans( l->xtor );
      }

    if( n->events != NULL )
      {
	register evptr  e;

	lprintf( stdout, "Pending events:\n" );
	for( e = n->events; e != NULL; e = e->nlink )
	    lprintf( stdout, "   transition to %c at %2.1fns\n",
	      "0XX1"[e->eval], d2ns( e->ntime ) );
      }

    return( 1 );
  }
