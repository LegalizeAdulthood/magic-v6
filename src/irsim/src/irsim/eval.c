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


/*
 * simulator can use one of several models, selectable by the user.  Which
 * model to use is kept as index into various dispatch tables, all defined
 * below.
 */
public
#define	NMODEL		2		/* number of models supported */


public	ifun   new_value[NMODEL] =	/* model dispatch tables */
  {
    cy_new_val, snew_val
  };


public	int    model = 0;	    /* which model to use during simulation */
public	int    sm_stat = NORM_SIM;  /* simulation status */
public	char   vchars[] = "0XX1D";
public	int    tdebug = 0;	    /* if <>0 report ratio errors */

private	int    firstcall = 1;	    /* set to 0 after init_nodes is called */


/*
 * find transistors with gates of VDD or GND and calculate values for source
 * and drain nodes just in case event driven calculations don't get to them.
 */
private void init_nodes()
  {
    register nptr   n;
    register tptr   t;
    register lptr   l;

    firstcall = 0;		/* initialization now taken care of */

    for( l = VDD_node->ngate; l != NULL; l = l->next )
      {
	t = l->xtor;
	t->state = compute_trans_state( t );
		/* Use the marking scheme because loops are very likely */
	n = t->source;
	if( not (n->nflags & INPUT) and n->npot == X )
	    n->nflags |= VISITED;
	n = t->drain;
	if( not (n->nflags & INPUT) and n->npot == X )
	    n->nflags |= VISITED;
      }

    for( l = GND_node->ngate; l != NULL; l = l->next )
      {
	t = l->xtor;
	t->state = compute_trans_state( t );
	n = t->source;
	if( not (n->nflags & INPUT) and n->npot == X )
	    n->nflags |= VISITED;
	n = t->drain;
	if( not (n->nflags & INPUT) and n->npot == X )
	    n->nflags |= VISITED;
      }

    for( l = VDD_node->ngate; l != NULL; l = l->next )
      {
	t = l->xtor;
	cur_node = n = t->source;
	if( n->nflags & VISITED )
	    (*new_value[model])( n );
	cur_node = n = t->drain;
	if( n->nflags & VISITED )
	    (*new_value[model])( n );
      }

    for( l = GND_node->ngate; l != NULL; l = l->next )
      {
	t = l->xtor;
	cur_node = n = t->source;
	if( n->nflags & VISITED )
	    (*new_value[model])( n );
	cur_node = n = t->drain;
	if( n->nflags & VISITED )
	    (*new_value[model])( n );
      }

    cur_node = NULL;
  }


/*
 * Run through the event list, marking all nodes that need to be evaluated.
 */
private void MarkNodes( evlist )
  evptr  evlist;
  {
    register nptr   n;
    register tptr   t;
    register lptr   l;
    register evptr  e = evlist;
    long            tmp = nevent;

    do
      {
	tmp++;
	n = e->enode;
	if( e->type == DECAY_EV )	/* Decay to X not decay to DECAY */
	  {
	    if( tdebug or n->nflags & WATCHED )
		lprintf( stdout,
		  " node %s just decayed to X (time = %2.1fns)\n",
		  pnode( n ), d2ns( e->rtime ) );
	  }

	if( n->nflags & WATCHED )
	  {
	    if( not (n->nflags & INPUT) )
		lprintf( stdout,
		  " [event #%ld] node %s: %c -> %c @ %2.1fns\n",
		  tmp, pnode( n ), vchars[n->npot],
		  vchars[e->eval], d2ns( e->ntime ) );
	    else
		lprintf( stdout,
		  " [event #%ld] input node %s: -> %c @ %2.1fns\n",
		  tmp, pnode( n ), vchars[e->eval], d2ns( e->ntime ) );
	  }


	n->npot = e->eval;

	    /* Add the new value to the history list (if they differ) */
	if( not (n->nflags & INPUT) and (n->curr->val != n->npot) )
	    (void) AddHist( n, n->npot, 0, (long) e->ntime, (long) e->delay,
	      (long) e->rtime );

	    /* for each transistor controlled by event node, mark 
	     * source and drain nodes as needing recomputation.
	     *
	     * Added MOSSIMs speed up by first checking if the 
	     * node needs to be rechecked  mh
	     *
	     * Fixed it so nodes with pending events also get
	     * re_evaluated. Kevin Karplus
	     */
	for( l = n->ngate; l != NULL; l = l->next )
	  {
	    char  oldstate;

	    t = l->xtor;
	    oldstate = t->state;

	    t->state = compute_trans_state( t );
	    if( (t->drain->npot == X) or (t->source->npot == X) or
	      ((t->drain->npot != t->source->npot) and
	      (t->state == ON)) or
	      ((t->drain->npot == t->source->npot) and
	      (t->state == OFF)) or
	      ((t->state == UNKNOWN) and
	      not (oldstate == OFF and
	      (t->drain->npot == t->source->npot))) or
	      (t->drain->events != NULL) or
	      (t->source->events != NULL) )
	      {
		if( not (t->source->nflags & INPUT) )
		    t->source->nflags |= VISITED;
		if( not (t->drain->nflags & INPUT) )
		    t->drain->nflags |= VISITED;
	      }
	  }
	free_from_node( e, n );    /* avoid punting this event (in 0 delay) */
	e = e->flink;
      }
    while( e != NULL );
  }


private nptr EvalNodes( evlist )
  evptr  evlist;
  {
    register tptr   t;
    register lptr   l;
    register nptr   n;
    register evptr  event = evlist;
    nptr            brk_node = NULL;

    do
      {
	nevent += 1;		/* advance counter to that of this event */
	n = cur_node = event->enode;
	n->c.time = event->ntime;	/* set up the cause stuff */
	n->t.cause = event->p.cause;

	npending -= 1;

	  /* now calculate new value for each marked node.  Some nodes marked
	   * above may become unmarked by earlier calculations before we get
	   * to them in this loop...
	   */

	for( l = n->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->source->nflags & VISITED )
		(*new_value[model])( t->source );
	    if( t->drain->nflags & VISITED )
		(*new_value[model])( t->drain );
	  }

	    /* see if we want to halt if this node changes value */
	if( n->nflags & STOPONCHANGE )
	    brk_node = n;
	event = event->flink;
      }
    while( event != NULL );

    return( brk_node );
  }


/*
 * Mark all nodes that just became inputs by setting the INPUT flag.
 */
private void SetInputs()
  {
    register iptr  list;
    register nptr  n;

    for( list = hinputs; list != NULL; list = list->next )
      {
	n = list->inode;
	n->nflags |= INPUT;
      }
    for( list = linputs; list != NULL; list = list->next )
      {
	n = list->inode;
	n->nflags |= INPUT;
      }
    for( list = uinputs; list != NULL; list = list->next )
      {
	n = list->inode;
	n->nflags |= INPUT;
      }
  }


private void MarkInputs( list, val )
  register iptr  list;
  register int   val;
  {
    register nptr  n, other;
    register lptr  l;
    register tptr  t;

    while( list != NULL )
      {
	n = cur_node = list->inode;

	n->npot = val;
	n->nflags |= OLD_INPUT;

		/* Punt any pending events for this node. */
	while( n->events != NULL )
	    free_event( n->events );

		/* enqueue event so consequences are computed. */
	if( n->ngate != NULL )
	    enqueue_input( n, val );

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->state != OFF )
	      {
		other = other_node( t, n );
		if( not( other->nflags & INPUT ) )
		    other->nflags |= VISITED;
	      }
	  }

	if( n->curr->val != val or not (n->curr->inp) )
	    (void) AddHist( n, val, 1, cur_delta, 0L, 0L );

	list = list->next;
      }
  }


#define MarkNOinputs( LIST )					\
  {								\
    register iptr  list;					\
								\
    for( list = LIST; list != NULL; list = list->next )		\
      {								\
	list->inode->nflags &= ~(INPUT_MASK | INPUT);		\
	list->inode->nflags |= VISITED;				\
      }								\
  }								\


private void EvalInputs( listp, dest )
  iptr  *listp;
  iptr  *dest;
  {
    iptr  list, last;
    nptr  n, other;
    lptr  l;
    tptr  t;

    for( list = last = *listp; list != NULL; list = list->next )
      {
	cur_node = n = list->inode;
	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    other = other_node( t, n );
	    if( other->nflags & VISITED )
		(*new_value[model])( other );
	  }
	last = list;
      }

    if( last )
      {
	last->next = *dest;
	*dest = *listp;
      }
    *listp = NULL;
  }


	/* nodes which are no longer inputs */
private void EvalNOinputs()
  {
    nptr  n;
    iptr  list, last;

    for( list = last = xinputs; list != NULL; list = list->next )
      {
	cur_node = n = list->inode;
	(void) AddHist( n, (int) n->curr->val, 0, cur_delta, 0L, 0L );
	if( n->nflags & VISITED )
	    (*new_value[model])( n );
	last = list;
      }
    if( last )
      {
	last->next = infree;
	infree = xinputs;
      }
    xinputs = NULL;
  }


/*
 * Reset the firstcall flag.  Usually after reading history dump or net state
 */
public void NoInit()
  {
    firstcall = 0;
  }


public nptr step( stop_time )
  long  stop_time;
  {
    evptr  evlist;
    nptr   brk_node;

    /* look through input lists updating any nodes which just become inputs */

    SetInputs();
    MarkNOinputs( xinputs );		/* nodes no longer inputs */
    MarkInputs( hinputs, HIGH );	/* HIGH inputs */
    MarkInputs( linputs, LOW );		/* LOW inputs */
    MarkInputs( uinputs, X );		/* X inputs */

	/* 
	 * on the first call to step, make sure transistors with gates
	 * of vdd and gnd are set up correctly.  Mark initial inputs first!
	 */
    if( firstcall )
	init_nodes();

    EvalNOinputs();
    EvalInputs( &hinputs, &o_hinputs );
    EvalInputs( &linputs, &o_linputs );
    EvalInputs( &uinputs, &o_uinputs );


    /* process events until we reach specified stop time or events run out. */
    while( (evlist = get_next_event( stop_time )) != NULL )
      {
	MarkNodes( evlist );
	brk_node = EvalNodes( evlist );
		/* return event list to free pool */
	evlist->blink->flink = evfree;
	evfree = evlist;

	if( brk_node )
	    return( brk_node );
	if( int_received )
	  {
	    if( analyzerON )
		UpdateWindow( cur_delta );
	    return( NULL );
	  }
      }
    cur_delta = stop_time;
    if( analyzerON )
	UpdateWindow( cur_delta );
    return( NULL );
  }


/* table to convert transistor type and gate node value into switch state
 * indexed by switch_state[transistor-type][gate-node-value].
 */
public	char  switch_state[NTTYPES][4] = 
  {
    OFF,	UNKNOWN,	UNKNOWN,	ON,	/* NCHAH */
    ON,		UNKNOWN,	UNKNOWN,	OFF,	/* PCHAN */
    WEAK,	WEAK,		WEAK,		WEAK,   /* DEP */
    WEAK,	WEAK,		WEAK,		WEAK,   /* PULLUP */
    WEAK,	WEAK,		WEAK,		WEAK,   /* RESIST */
  };


/* compute state of transistor.  If gate is a simple node, state is determined
 * by type of implant and value of node.  If gate is a list of nodes, then
 * this transistor represents a stack of transistors in the original network,
 * and we perform the logical AND of all the gate node values to see if
 * transistor is on.
 */
public
#define	 compute_trans_state( TRANS )					\
    ( ((TRANS)->ttype & GATELIST) ?					\
	ComputeTransState( TRANS ):					\
	switch_state[ BASETYPE( (TRANS)->ttype ) ][ (TRANS)->gate->npot ] )


public int ComputeTransState( t )
  register tptr  t;
  {
    register nptr  n;
    register tptr  l;
    register int   result;

    switch( BASETYPE( t->ttype ) )
      {
	case NCHAN :
	    result = ON;
	    for( l = (tptr) t->gate; l != NULL; l = l->scache.t )
	      {
		n = l->gate;
		if( n->npot == LOW )
		    return( OFF );
		else if( n->npot == X )
		    result = UNKNOWN;
	      }
	    return( result );

	case PCHAN :
	    result = ON;
	    for( l = (tptr) t->gate; l != NULL; l = l->scache.t )
	      {
		n = l->gate;
		if( n->npot == HIGH )
		    return( OFF );
		else if( n->npot == X )
		    result = UNKNOWN;
	      }
	    return( result );

	case DEP :
	case PULLUP :
	case RESIST :
	    return( WEAK );

	default :
	    fprintf( stderr,
	      "**** internal error: unrecongized transistor type (0x%x)\n",
	      BASETYPE( t->ttype ) );
	    return( UNKNOWN );
      }
  }
