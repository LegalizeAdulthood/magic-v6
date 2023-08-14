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
 * Incrmental net changes are provides by the following commands:
 * 
 *    add	type gate source drain length width [area]
 *    delete	type gate source drain length width [area]
 *    move	type gate source drain length width [area] n_gate n_src n_drn
 *    cap	node value
 *    N		node metal-area poly-area diff-area diff-perimeter
 *    M		node M2A M2P MA MP PA PP DA DP PDA PDP
 *    threshold	node low high
 *    Delay	node tplh tphl
 */

#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"

#include "net_macros.h"


#define LSIZE		2000		/* size for input line buffer */
#define	MAXARGS		20		/* max. # of arguments per line */

    /* a capacitance change > CAP_THRESH % will consider the node changed */
#define	CAP_THRESH	(0.05)		/* default is now 5% */


private	int    lineno;			/* current input file line number */
private	char   *nc_fname;		/* current input filename */

private	nptr   ndlist;			/* list of nodes that were changed */

private	FILE   *nc_logf = NULL;		/* file for recording net changes */



#define Error( A )	\
  {			\
    error A ;		\
    return;		\
  }			\


#define	CAP_CHANGE( ND, LIST, GCAP )		\
  {						\
    if( not ((ND)->nflags & VISITED) )		\
      {						\
	(ND)->n.next = (LIST);			\
	LIST = (ND);				\
	(ND)->nflags |= VISITED;		\
	(ND)->c.cap = (GCAP);			\
      }						\
    else					\
	(ND)->c.cap += (GCAP);			\
    (ND)->ncap += (GCAP);			\
  }						\


#define	NODE_CHANGE( ND, LIST )			\
  {						\
    if( not ((ND)->nflags & VISITED) )		\
      {						\
	(ND)->n.next = (LIST);			\
	LIST = (ND);				\
      }						\
    (ND)->nflags |= (VISITED | CHANGED);	\
  }						\


/*
 * node area and perimeter capacitance.
 */
private void node_cap( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;
    double         cap;

    if( targc != 8 )
	Error( ( nc_fname, lineno,
	  "Wrong number of arguments to N (%d)\n", targc));

    n = GetNode( targv[1] );

    cap = 	atof( targv[4] ) * (CMA * LAMBDA2) +
		atof( targv[5] ) * (CPA * LAMBDA2) +
		atof( targv[6] ) * (CDA * LAMBDA2) +
		atof( targv[7] ) * 2.0 * (CDP * LAMBDA);

    CAP_CHANGE( n, ndlist, cap );
  }


/*
 * new format node area and perimeter capacitance.
 */
private void nnode_cap( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;
    double         cap;

    if( targc != 14 )
	Error( ( nc_fname, lineno,
	  "Wrong number of arguments for M (%d)\n", targc) );

    n = GetNode( targv[1] );

    cap =	atof( targv[4] ) * (CM2A * LAMBDA2) +
		atof( targv[5] ) * 2.0 * (CM2P * LAMBDA) +
		atof( targv[6] ) * (CMA * LAMBDA2) +
		atof( targv[7] ) * 2.0 * (CMP * LAMBDA) +
		atof( targv[8] ) * (CPA * LAMBDA2) +
		atof( targv[9] ) * 2.0 * (CPP * LAMBDA) +
		atof( targv[10] ) * (CDA * LAMBDA) +
		atof( targv[11] ) * 2.0 * (CDP * LAMBDA) +
		atof( targv[12] ) * (CPDA * LAMBDA2) +
		atof( targv[13] ) * 2.0 * (CPDP * LAMBDA);

    CAP_CHANGE( n, ndlist, cap );
  }


/*
 * change node threshold voltages.
 */
private void change_thresh( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 4 )
	Error( ( nc_fname, lineno,
	  "wrong number of arguments for t (%d)\n", targc) );

    n = GetNode( targv[1] );
    n->vlow = atof( targv[2] );
    n->vhigh = atof( targv[3] );

    NODE_CHANGE( n, ndlist );
  }

/*
 * set user delay for a node.
 */
private void ndelay( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 4 )
	Error( ( nc_fname, lineno,
	  "wrong number of arguments for D (%d)\n", targc) );

    n = GetNode( targv[1] );
    n->nflags |= USERDELAY;
    n->tplh = ns2d( atof( targv[2] ) );
    n->tphl = ns2d( atof( targv[3] ) );

    NODE_CHANGE( n, ndlist );
  }


private void rm_node( node )
  nptr  node;
  {
    while( node->events )
	free_event( node->events );	    /* remove all pending events */
    if( IsInList( node->nflags ) )
      {
	idelete( node, listTbl[INPUT_NUM( node->nflags )] );
	node->nflags &= ~INPUT_MASK;
      }
    idelete( node, &wlist );		/* delete from all lists */
    rm_from_vectors( node );
    rm_from_seq( node );
    rm_from_clock( node );
    if( analyzerON )
	RemoveNode( node );
    n_delete( node );
    if( node->nflags & ALIASED )
	rm_aliases( node );
    Vfree( node->nname );
    FreeHistList( node );
    node->nlink = (nptr) freeNodes;
    freeNodes = (MList) node;
    nnodes--;
  }

/*
 * parse input line into tokens, filling up carg and setting targc
 */
private int parse_line( line, carg )
  register char  *line;
  register char  **carg;
  {
    register char  ch;
    register int   targc;

    targc = 0;
    while( ch = *line++ )
      {
	if( ch <= ' ' )
	    continue;
	targc++;
	*carg++ = line - 1;
	while( *line > ' ' )
	    line++;
	if( *line )
	    *line++ = '\0';
      }
    *carg = 0;
    return( targc );
  }


#define CHK_CAP( ND )	if( (ND)->ncap < MIN_CAP ) (ND)->ncap = MIN_CAP


private nptr changed_nodes( olist )
  nptr  olist;
  {
    register nptr  n, nextn, nlist;
    float          Cinc;			/* incremental capacitance */
    struct Node    tmp_node;

    nlist = &tmp_node;				/* use as tmp. head of list */
    for( n = olist; n != NULL; n = nextn )
      {
	nextn = n->n.next;

	if( n->ngate == NULL and n->nterm == NULL and n->nflags & MERGED == 0 )
	  {
	    rm_node( n );
	    continue;
	  }

	if( not( n->nflags & CHANGED ) )
	  {
	    Cinc = abs( (double) n->c.cap );
	    if( Cinc < CAP_THRESH * (n->ncap - Cinc) )
	      {
		n->c.time = n->curr->time;
		CHK_CAP( n );
		continue;
	      }
	    n->nflags |= CHANGED;
	    CHK_CAP( n );
	  }

	if( n->nflags & MERGED )
	  {
	    if( n->nflags & CHANGED )
	      {
		register nptr  s, d;

		s = n->t.tran->source;
		d = n->t.tran->drain;
		if( not (s->nflags & (CHANGED | POWER_RAIL) ) )
		  {
		    nlist->n.next = s;
		    nlist = s;
		    s->nflags |= CHANGED;
		  }
		if( not (d->nflags & (CHANGED | POWER_RAIL) ) )
		  {
		    nlist->n.next = d;
		    nlist = d;
		    d->nflags |= CHANGED;
		  }
	      }
	    continue;
	  }
	if( n->nflags & POWER_RAIL )
	    continue;
	nlist->n.next = n;
	nlist = n;
	CHK_CAP( n );
      }
    nlist->n.next = NULL;
    nlist = tmp_node.n.next;
    VDD_node->nflags &= ~CHANGED;	/* just in case they were touched */
    GND_node->nflags &= ~CHANGED;
    return( nlist );
  }


#define TransType( ch )		\
  ( ( ch == 'n' ) ? NCHAN :	\
    ( ch == 'p' ) ? PCHAN :	\
    ( ch == 'd' ) ? DEP :	\
    -1 )


#define	HASH_TERMS( SRC, DRN )		( (long)(SRC) ^ (long)(DRN) )


private	nptr    src_found;

private tptr find_trans( args )
  char  *args[];
  {
    register lptr  l;
    register tptr  t;
    register int   ttype;
    register nptr  gate, drn, src;
    register long  hval;
    
    ttype = args[0][0];
    ttype = TransType( ttype );
    if( ttype == -1 )
	return( NULL );

    if( (gate = find( args[1] )) == NULL )
	return( NULL );
    if( (src_found = src = find( args[2] )) == NULL )
	return( NULL );
    if( (drn = find( args[3] )) == NULL )
	return( NULL );

    if( src == drn or (src->nflags & drn->nflags & POWER_RAIL) )
      {
	hval = HASH_TERMS( src, drn );
	for( t = tcap_list; t != NULL; t = t->scache.t )
	  {
	    if( gate == t->gate and 
	      HASH_TERMS( t->source, t->drain ) == hval and
	      ( BASETYPE( t->ttype ) == ttype or
	      ( (ttype & ALWAYSON) and (t->ttype & ALWAYSON) ) ) )
		break;
	  }
	if( t != NULL )
	  {
	    if( t->dcache.t )
		t->dcache.t->scache.t = t->scache.t;
	    else
		tcap_list = t->scache.t;
	    if( t->scache.t )
		t->scache.t->dcache.t = t->dcache.t;
	    return( t );
	  }
      }							/* part of a stack */
    else if( (src->nflags & MERGED) or (drn->nflags & MERGED) )
      {
	t = ((src->nflags & MERGED) ? drn : src)->t.tran;
	if( BASETYPE( t->ttype ) == ttype or
	  ( (ttype & ALWAYSON) and (t->ttype & ALWAYSON) ) )
	  {
	    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
	      {
		if( gate == t->gate )
		    return( t );
	      }
	  }
      }
    else if( ttype & ALWAYSON )			/* DEP, PULLUP or RESIST */
      {
	l = ((src->nflags & POWER_RAIL) ? drn : src)->nterm;
	for( hval = HASH_TERMS( src, drn ); l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( hval == HASH_TERMS( t->source, t->drain ) and gate == t->gate
	      and (t->ttype & ALWAYSON) )
		return( t );
	  }
      }
    else						/* NCHAN or PCHAN */
      {
	l = ((src->nflags & POWER_RAIL) ? drn : src)->nterm;
	for( hval = HASH_TERMS( src, drn ); l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( hval == HASH_TERMS( t->source, t->drain ) and gate == t->gate
	      and BASETYPE( t->ttype ) == ttype )
		return( t );
	  }
      }
    return( NULL );
  }


private void del_tran( targc, targv )
  int   targc;
  char  *targv[];
  {
    register tptr  t;
    register int   ttype;
    double         width, length, area, cap, gcap;

    if( targc < 7 or targc > 8 )
	Error( ( nc_fname, lineno,
	  "Wrong number of arguments for remove (%d)\n", targc) );

    if( (t = find_trans( &targv[1] ) ) == NULL )
	Error( ( nc_fname, lineno, "Can not find transistor\n") );

    length = atof( targv[5] ) * LAMBDA;
    width = atof( targv[6] ) * LAMBDA;
    if( targc == 8 )
	area = atof( targv[7] ) * LAMBDA2;
    else
	area = width * length;

    ttype = t->ttype;

    gcap = cap = 0.0;
    if( BASETYPE( ttype ) != PULLUP or (config_flags & CNTPULLUP) )
	gcap = -(area * CGA);

    if( config_flags != 0 )
      {
	if( config_flags & SUBPAREA )
	    gcap += area * CPA;
	if( config_flags & DIFFPERIM )
	    cap += width * CDP;
	if( config_flags & DIFFEXTF )
	    cap -= (2 * DIFFEXT + width) * CDP + (DIFFEXT * width * CDA);
      }

    if( ttype & TCAP )
      {
	tcap_cnt -= 1;
	if( cap != 0.0 )
	  {
	    CAP_CHANGE( t->source, ndlist, cap );
	    CAP_CHANGE( t->drain, ndlist, cap );
	  }
      }
    else
      {
	if( ttype & STACKED )
	    DestroyStack( t->dcache.t );

	if( not (ttype & ORED) or DestroyParallel( t, width, length ) )
	  {
	    DISCONNECT( t->gate->ngate, t );
	    DISCONNECT( t->source->nterm, t );
	    DISCONNECT( t->drain->nterm, t );
	  }
	NODE_CHANGE( t->source, ndlist );
	NODE_CHANGE( t->drain, ndlist );
	t->source->ncap += cap;
	t->drain->ncap += cap;
      }
    CAP_CHANGE( t->gate, ndlist, gcap );
    ntrans[ BASETYPE( ttype ) ] -= 1;
    FREE_TRANS( t );
  }


private void add_trans( targc, targv )
  int   targc;
  char  *targv[];
  {
    register int   ttype;
    register tptr  t;
    register nptr  gate, drn, src;
    double         length, width, area, cap, gcap;

    if( targc < 7 or targc > 8 )
	Error(( nc_fname, lineno,
	  "wrong number of arguments for add (%d)\n", targc) );

    ttype = targv[1][0];
    ttype = TransType( ttype );
    if( ttype == -1 )
	Error( ( nc_fname, lineno, "unknown transistor type (%c)\n", targv[1]) );

    cap = gcap = 0.0;
    gate = GetNode( targv[2] );
    src = GetNode( targv[3] );
    drn = GetNode( targv[4] );
    length = atof( targv[5] ) * LAMBDA;
    width = atof( targv[6] ) * LAMBDA;
    area = ( targc == 7 ) ? (width * length) : (atof( targv[7] ) * LAMBDA2);

    ntrans[ ttype ]++;
    NEW_TRANS( t );

    if( ttype == DEP )
      {
	if( (drn == VDD_node and gate == src) or
	  (src == VDD_node and gate == drn ) )
	  {
	    ttype = PULLUP;
	    if( not (config_flags & CNTPULLUP) )
		gcap = - area * CGA;
	  }
	else if( gate == src or gate == drn )
	    ttype = RESIST;
	if( ttype == RESIST )
	  {
	    requiv( width, length, DEP, &(t->r) );
	    t->r.dynlow = t->r.dynhigh = t->r.rstatic;
	  }
	else
	    requiv( width, length, ttype, &(t->r) );
      }
    else
	requiv( width, length, ttype, &(t->r) );

    gcap += (config_flags & SUBPAREA) ? (area * (CGA - CPA)) : (area * CGA);
    if( config_flags & DIFFPERIM)
	cap -= width * CDP;
    if( config_flags & DIFFEXTF )
	cap += (2 * DIFFEXT + width) * CDP + (DIFFEXT * width * CDA);

    t->ttype = ttype;
    t->gate = gate;
    t->source = src;
    t->drain = drn;
    t->tflags = 0;

    if( gate->nflags & MERGED )
      {
	DestroyStack( gate->t.tran );
	NODE_CHANGE( gate, ndlist );
	gate->ncap += gcap;
      }
    else
	CAP_CHANGE( gate, ndlist, gcap );

    if( src == drn or src->nflags & drn->nflags & POWER_RAIL )
      {
	t->ttype |= TCAP;
	LINK_TCAP( t );
	if( cap != 0.0 )
	  {
	    CAP_CHANGE( src, ndlist, cap );
	    CAP_CHANGE( drn, ndlist, cap );
	  }
	return;
      }

    if( not (ttype & ALWAYSON) )
	CONNECT( gate->ngate, t );

    src->ncap += cap;
    drn->ncap += cap;
    if( not (src->nflags & POWER_RAIL) )
      {
	if( src->nflags & MERGED )
	    DestroyStack( src->t.tran );
	CONNECT( src->nterm, t );
	NODE_CHANGE( src, ndlist );
      }	
    if( not (drn->nflags & POWER_RAIL) )
      {
	if( drn->nflags & MERGED )
	    DestroyStack( drn->t.tran );
	CONNECT( drn->nterm, t );
	NODE_CHANGE( drn, ndlist );
      }	
  }


private void move_tran( targc, targv )
  int   targc;
  char  *targv[];
  {
    register tptr  t;
    char           *gstr, *sstr, *dstr;
    register nptr  gate, src, drn;
    double         length, width, area, cap;
    int            was_tcap;

    if( targc < 10 or targc > 11 )
	Error( ( nc_fname, lineno, "Wrong number of args for move (%d)\n",
	  targc) );

    if( (t = find_trans( &targv[1] )) == NULL )
	Error( ( nc_fname, lineno, "Can not find transistor to move\n") );

    length = atof( targv[5] ) * LAMBDA;
    width = atof( targv[6] ) * LAMBDA;
    if( targc == 11 )
      {
	gstr = targv[ 8 ]; sstr = targv[ 9 ]; dstr = targv[ 10 ];
	area = atof( targv[7] ) * LAMBDA2;
      }
    else
      {
	gstr = targv[ 7 ]; sstr = targv[ 8 ]; dstr = targv[ 9 ];
	area = width * length;
      }

    if( t->source != src_found )
	SWAP( char *, sstr, dstr );

    gate = (*gstr == '*') ? t->gate : GetNode( gstr );
    src = (*sstr == '*') ? t->source : GetNode( sstr );
    drn = (*dstr == '*') ? t->drain : GetNode( dstr );

    if( t->ttype & TCAP )
      {
	was_tcap = TRUE;
	tcap_cnt -= 1;
      }
    else
	was_tcap = FALSE;

    if( src == drn or src->nflags & drn->nflags & POWER_RAIL )
	t->ttype |= TCAP;
    else
	t->ttype &= ~TCAP;

    if( t->ttype & STACKED )
	DestroyStack( t->dcache.t );

    if( t->ttype & ORED )
      {
	if( DestroyParallel( t, width, length ) )
	    t->ttype &= ~ORED;
	else
	  {
	    register tptr newt;

	    NEW_TRANS( newt );
	    newt->ttype = t->ttype & ~ORED;
	    newt->gate = gate;
	    newt->source = t->source;
	    newt->drain = t->drain;
	    newt->tflags = 0;
	    requiv( width, length, BASETYPE( newt->ttype ), &(newt->r) );
	    t = newt;
	  }
      }

    if( t->ttype & ALWAYSON )
      {
	int   newtype = t->ttype;

	if( drn == VDD_node and gate == src )
	    newtype = PULLUP;
	else if( src == VDD_node and gate == drn )
	    newtype = PULLUP;
	else if( gate == drn or gate == src )
	    newtype = RESIST;

	if( BASETYPE( t->ttype ) != newtype )
	  {
	    requiv( width, length, newtype, &(t->r) );
	    if( newtype == RESIST )
		t->r.dynlow = t->r.dynhigh = t->r.rstatic;
	    else if( not (config_flags & CNTPULLUP) )
	      {
		cap = area * CGA;
		if( newtype == PULLUP )
		    CAP_CHANGE( t->gate, ndlist, -cap )
		else if( BASETYPE( t->ttype ) == PULLUP )
		    if( gate == t->gate )
			CAP_CHANGE( gate, ndlist, cap )
		    else
			CAP_CHANGE( t->gate, ndlist, cap )
	      }
	    t->ttype = (t->ttype & ~BASETYPE( t->ttype ) ) | newtype;
	  }
      }

    if( gate != t->gate )
      {
	if( gate->nflags & MERGED )
	  {
	    DestroyStack( gate->t.tran );
	    NODE_CHANGE( gate, ndlist );
	  }
	if( BASETYPE( t->ttype ) != PULLUP or (config_flags & CNTPULLUP) )
	  {
	    cap = (config_flags & SUBPAREA) ? area * (CGA - CPA) : area * CGA;
	    CAP_CHANGE( gate, ndlist, cap );
	    CAP_CHANGE( t->gate, ndlist, -cap );
	  }
	DISCONNECT( t->gate->ngate, t );
	if( t->ttype & (ALWAYSON | TCAP) == 0 )
	    CONNECT( gate->ngate, t );
	t->gate = gate;
      }

    cap = ( config_flags & DIFFPERIM ) ? -width * CDP : 0.0;
    if( config_flags & DIFFEXTF )
	cap += (2 * DIFFEXT + width * CDP) + (DIFFEXT * width * CDA);

    if( t->ttype & TCAP )
      {
	LINK_TCAP( t );
	if( was_tcap )
	  {
	    if( cap != 0.0 and (src != t->source or drn != t->drain ) )
	      {
		CAP_CHANGE( src, ndlist, cap );
		CAP_CHANGE( drn, ndlist, cap );
		CAP_CHANGE( t->source, ndlist, -cap );
		CAP_CHANGE( t->drain, ndlist, -cap );
	      }
	  }
	else	/* just became a tcap => implies src and/or drn were moved */
	  {
	    NODE_CHANGE( t->source, ndlist );
	    DISCONNECT( t->source->nterm, t );
	    NODE_CHANGE( t->drain, ndlist );
	    DISCONNECT( t->drain->nterm, t );
	    if( gate == t->gate )	/* otherwise taken care above */
		DISCONNECT( t->gate->ngate, t );
	    if( cap != 0.0 )
	      {
		t->source->ncap -= cap;
		t->drain->ncap -= cap;
		CAP_CHANGE( src, ndlist, cap );
		CAP_CHANGE( drn, ndlist, cap );
	      }
	  }
	t->source = src;
	t->drain = drn;
	return;
      }

    if( src != t->source )
      {
	src->ncap += cap;
	DISCONNECT( t->source->nterm, t );
	if( not (src->nflags & POWER_RAIL) )
	  {
	    if( src->nflags & MERGED )
		DestroyStack( src->t.tran );
	    CONNECT( src->nterm, t );
	  }
      }

    if( drn != t->drain )
      {
	drn->ncap += cap;
	DISCONNECT( t->drain->nterm, t );
	if( not (drn->nflags & POWER_RAIL) )
	  {
	    if( drn->nflags & MERGED )
		DestroyStack( drn->t.tran );
	    CONNECT( drn->nterm, t );
	  }
      }

    NODE_CHANGE( src, ndlist );
    NODE_CHANGE( drn, ndlist );

    if( was_tcap )
      {
	if( t->drain == drn and not (drn->nflags & POWER_RAIL) )
	    CONNECT( drn->nterm, t );
	if( t->source == src and not (src->nflags & POWER_RAIL) )
	    CONNECT( drn->nterm, t );
	if( cap != 0.0 )
	  {
	    CAP_CHANGE( t->source, ndlist, -cap );
	    CAP_CHANGE( t->drain, ndlist, -cap );
	  }
      }
    else
      {
	NODE_CHANGE( t->source, ndlist );
	NODE_CHANGE( t->drain, ndlist );
	if( cap != 0.0 )
	  {
	    t->source->ncap -= cap;
	    t->drain->ncap -= cap;
	  }
      }
    t->source = src;
    t->drain = drn;
  }


private void change_cap( targc, targv )
  int   targc;
  char  *targv[];
  {
    nptr   n;
    float  cap;
    
    if( targc != 3 )
	Error( ( nc_fname, lineno, "Wrong number of arguments for cap (%d)\n",
	  targc) );

    if( (n = find( targv[1] )) == NULL )
	Error( ( nc_fname, lineno, "can not find node '%s'\n", targv[1]) );

    cap = atof( targv[2] );
    CAP_CHANGE( n, ndlist, cap );
  }


private int input_changes( fname )
  char  *fname;
  {
    FILE  *fin;
    char  line[LSIZE];
    char  *targv[MAXARGS];
    int   targc;
    int   ttype;
    long  ltime;

    nc_fname = fname;
    lineno = 0;

    if( (fin = fopen( fname, "r" )) == NULL )
      {
	lprintf( stderr, "can not open '%s' for net changes\n", fname );
	return( FALSE );
      }

    if( nc_logf )
      {
	ltime = time( 0 );
	fprintf( nc_logf, "| changes @ %s", ctime( &ltime ) );
      }

    VDD_node->nflags |= VISITED;	/* never add these to 'change' list */
    GND_node->nflags |= VISITED;

    while( fgetline( line, LSIZE, fin ) != NULL )
      {
	lineno++;
	if( nc_logf )
	    fputs( line, nc_logf );
	targc = parse_line( line, targv );
	if( targc == 0 )
	    continue;

	switch( targv[0][0] )
	  {
	    case 'a' :
		add_trans( targc, targv );
		break;
	    case 'c' :
		change_cap( targc, targv );
		break;
	    case 'm' :
		move_tran( targc, targv );
		break;
	    case 'd' :
		del_tran( targc, targv );
		break;
	    case 't' :
		change_thresh( targc, targv );
		break;
	    case 'N' :
		node_cap( targc, targv );
		break;
	    case 'M' :
		nnode_cap( targc, targv );
		break;
	    case 'D' :
		ndelay( targc, targv );
		break;
	    case '|' :
		break;
	    default :
		error( nc_fname, lineno, "Unrecognized command (%s)\n", line );
	  }
      }
    fclose( fin );

    VDD_node->nflags &= ~VISITED;	/* restore flag */
    GND_node->nflags &= ~VISITED;
    return( TRUE );
  }


public nptr rd_changes( fname, logname )
  char  *fname;
  char  *logname;
  {
    ndlist = NULL;
    if( logname == NULL )
	nc_logf = NULL;
    else
      {
	if( (nc_logf = fopen( logname, "a" )) == NULL )
	    lprintf( stderr, "warning: can't open logfile %s\n", logname );
      }
    if( analyzerON )
	StopAnalyzer();
    if( input_changes( fname ) )
      {
	make_parallel( ndlist, (long) (stack_txtors ? VISITED : 0) );
	if( stack_txtors )
	    make_stacks( ndlist );
	ndlist = changed_nodes( ndlist );
	pTotalNodes();
	pTotalTxtors();
	pParallelTxtors();
	if( stack_txtors )
	    pStackedTxtors();
	if( analyzerON )
	    RestartAnalyzer( sim_time0, sim_time0, FALSE );
      }
    else
	if( analyzerON )
	    RestartAnalyzer( sim_time0, cur_delta, TRUE );

    if( nc_logf )
      {
	fclose( nc_logf );
	nc_logf = NULL;
      }
    return( ndlist );
  }
