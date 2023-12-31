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
#include "ASSERT.h"


#define	TSIZE		1024	/* size of event array, must be power of 2 */
#define	TMASK		(TSIZE - 1)

typedef struct
  {
    evptr    flink, blink;	/* pointers in doubly-linked list */
  } evhdr;


public	int    debug = 0;	    /* if <>0 print lotsa interesting info */

public	long   cur_delta;	    /* current simulated time */
public	nptr   cur_node;	    /* node that belongs to current event */
public	long   nevent;		    /* number of current event */

public	evptr  evfree = NULL;	    /* list of free event structures */
public	int    npending;	    /* number of pending events */

public	int    queue_final = TRUE;  /* whether to queue final values or not */


private	evhdr  ev_array[TSIZE];	    /* used as head of doubly-linked lists */


/*
 * return the time of the latest event.
 */
public long last_event_time()
  {
    evhdr  *ev;
    long   maxt;

    maxt = 0;
    for( ev = ev_array; ev != &ev_array[TSIZE]; ev++ )
      {
	if( ev->blink->ntime > maxt )
	    maxt = ev->blink->ntime;
      }
    return( maxt );
  }


/*
 * Sets the parameter "list" to contain a pointer to the list of pending 
 * events at time "cur_delta + delta".  "end_of_list" contains a pointer to
 * the end of the list. If there are no events at the indicated time "list"
 * is set to NULL.  The function returns 0 if there are no events past the
 * "cur_delta + delta", otherwise it returns the delta at which the next
 * events are found;
 */

public int pending_events( delta, list, end_of_list )
  long   delta;
  evptr  *list, *end_of_list;
  {
    evhdr           *hdr;
    register evptr  ev;

    *list = NULL;

    delta += cur_delta;
    hdr = &ev_array[ delta & TMASK ];

    if( hdr->flink == (evptr) hdr or hdr->blink->ntime < delta )
	goto find_next;

    for( ev = hdr->flink; ev->ntime < delta; ev = ev->flink );
    if( ev->ntime != delta )
	goto find_next;

    *list = ev;
    if( hdr->blink->ntime == ev->ntime )
	*end_of_list = hdr->blink;
    else
      {
	for( delta = ev->ntime; ev->ntime == delta; ev = ev->flink );
	*end_of_list = ev->blink;
      }

  find_next:
     {
	register long  i, limit, time;

	time = max_time;
	for( i = ++delta, limit = i + TSIZE; i < limit; i++ )
	  {
	    ev = (evptr) &ev_array[ i & TMASK ];
	    if( ev != ev->flink )
	      {
		if( ev->blink->ntime < delta )
		    continue;
		for( ev = ev->flink; ev->ntime < delta; ev = ev->flink );
		if( ev->ntime < limit )
		  {
		    time = ev->ntime;
		    break;
		  }
		else if( ev->flink->ntime < time )
		    time = ev->flink->ntime;
	      }
	  }
	delta = ( time != max_time ) ? time - cur_delta : 0;
      }

    return( delta );
  }


/*
 * find the next event to be processed by scanning event wheel.  Return
 * the list of events to be processed at this time, removing it first
 * from the time wheel.
 */
public evptr get_next_event( stop_time )
  long  stop_time;
  {
    register evptr  event;
    register long   i, limit, time;

    if( npending == 0 )
	return( NULL );

    time = max_time;
    for( i = cur_delta, limit = i + TSIZE; i < limit; i++ )
      {
	event = (evptr) &ev_array[ i & TMASK ];
	if( event != event->flink )
	  {
	    if( event->flink->ntime < limit )		/* common case */
		goto found;
	    if( event->flink->ntime < time )
		time = event->flink->ntime;
	  }
      }

    if( time != max_time )
	event = (evptr) &ev_array[ time & TMASK ];
    else
      {
	fprintf( stderr, "*** internal error: no events but npending set\n" );
	return( NULL );
      }

  found:
      {
	evptr  evlist = event->flink;

	time = evlist->ntime;

	if( time >= stop_time )
	    return( NULL );

	/* sanity check for incremental simulation mostly */
	ASSERT( time < cur_delta )
	  {
	    lprintf( stderr, "time moving back %d -> %d\n", cur_delta,
	     evlist->ntime );
	  }

	cur_delta = time;			/* advance simulation time */

	if( event->blink->ntime != time )	/* check tail of list */
	  {
	    do
		event = event->flink;
	    while( event->ntime == time );

	    event = event->blink;		/* grab part of the list */
	    evlist->blink->flink = event->flink;
	    event->flink->blink = evlist->blink;
	    evlist->blink = event;
	    event->flink = NULL;
	  }
	else
	  {
	    event = evlist->blink;		/* grab the entire list */
	    event->blink->flink = NULL;
	    evlist->blink = event->blink;
	    event->flink = event->blink = (evptr) event;
	  }
	return( evlist );
      }
  }


/* remove event from node's list of pending events */
public
#define free_from_node( ev, nd )					\
  {									\
    if( (nd)->events == (ev) )						\
	(nd)->events = (ev)->nlink;					\
    else								\
      {									\
	register evptr  evp;						\
	for( evp = (nd)->events; evp->nlink != (ev); evp = evp->nlink );\
	evp->nlink = (ev)->nlink;					\
      }									\
  }									\


/*
 * remove event from all structures it belongs to and return it to free pool
 */
public void free_event( event )
  register evptr  event;
  {
	/* unhook from doubly-linked event list */
    event->blink->flink = event->flink;
    event->flink->blink = event->blink;
    npending -= 1;

	/* add to free storage pool */
    event->flink = evfree;
    evfree = event;

    free_from_node( event, event->enode );
  }

/* get event structure.  Allocate a bunch more if we've run out. */
#define NEW_EVENT( NEW )					\
  {								\
    if( ((NEW) = evfree) == NULL )				\
      {								\
	(NEW) = (evptr) MallocList( sizeof( struct Event ) );	\
	if( (NEW) == NULL )					\
	  {							\
	    fprintf( stderr, "Out of memory for events" );	\
	    exit( 1 );						\
	  }							\
      }								\
    evfree = (NEW)->flink;					\
  }								\


/*
 * Add an event to event list, specifying transition delay and new value.
 * 0 delay transitions are converted into unit delay transitions (0.1 ns).
 */
public void enqueue_event( n, newvalue, delta, rtime )
  register nptr  n;
  long           delta, rtime;
  {
    register evptr  marker, new;
    register long   etime;

	/* check against numerical errors from new_val routines */
    ASSERT( delta > 0 and delta < 60000 )
      {
	lprintf( stderr, "bad event @ %.1fns: %s ,delay=%ddeltas\n",
	  d2ns( cur_delta ), pnode( n ), delta );
	delta = rtime = 1;
      }

    if( not queue_final )	/* don't queue final value (see incsim.c) */
	return;

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = cur_delta + delta;
    new->rtime = rtime;
    new->enode = n;
    new->p.cause = cur_node;
    new->delay = delta;
    if( newvalue == DECAY )		/* change value to X here */
      {
	new->eval = X;
	new->type = DECAY_EV;
      }
    else
      {
	new->eval = newvalue;
	new->type = REVAL;		/* for incremental simulation */
      }

	/* add the new event to the event list at the appropriate entry
	 * in event wheel.  Event lists are kept sorted by increasing
	 * event time.
	 */
    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending += 1;

	/* 
	 * thread event onto list of events for this node, keeping it
	 * in sorted order
	 */
    if( (n->events != NULL) and (n->events->ntime > etime) )
      {
	for( marker = n->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = n->events;
	n->events = new;
      }
  }


/* same as enqueue_event, but assumes 0 delay and rtise/fall time */
public void enqueue_input( n, newvalue )
  register nptr  n;
  {
    register evptr  marker, new;
    register long   etime;

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = cur_delta;
    new->rtime = new->delay = 0;
    new->enode = new->p.cause = n;
    new->eval = newvalue;
    new->type = REVAL;			/* anything, doesn't matter */

     /* Add new event to HEAD of list at appropriate entry in event wheel */

    marker = (evptr) & ev_array[ etime & TMASK ];
    new->flink = marker->flink;
    new->blink = marker;
    marker->flink->blink = new;
    marker->flink = new;
    npending += 1;

	/* thread event onto (now empty) list of events for this node */
    new->nlink = NULL;
    n->events = new;
  }


/*
 * Initialize event structures
 */
public void init_event()
  {
    register int    i;
    register evhdr  *event;

    for( i = 0, event = &ev_array[0]; i < TSIZE; i += 1, event += 1 )
      {
	event->flink = event->blink = (evptr) event;
      }
    npending = 0;
    nevent = 0;
  }


public void PuntEvent( node, ev )
  nptr   node;
  evptr  ev;
  {
    if( (node->nflags & WATCHED) and debug )
	lprintf( stdout,
	  "    punting transition to %c scheduled for %2.1fns\n",
	  vchars[ev->eval], d2ns( ev->ntime ) );

    ASSERT( node == ev->enode )
      {
	lprintf( stderr, "bad punt @ %d for %s\n", cur_delta, node->nname );
	node->events = NULL;
	return;
      }

    if( ev->type != DECAY_EV )		/* don't save punted decay events */
	AddPunted( ev->enode, ev, cur_delta );
    free_event( ev );
  }


private void requeue_events( evlist )
  evptr  evlist;
  {
    register long   etime;
    register evptr  ev, next, target;

    for( ev = evlist; ev != NULL; ev = next )
      {
	next = ev->flink;

	etime = ev->ntime;
	target = (evptr) & ev_array[ etime & TMASK ];

	if( (target->blink != target) and (target->blink->ntime > etime) )
	  {
	    do { target = target->flink; } while( target->ntime <= etime );
	  }

	ev->flink = target;
	ev->blink = target->blink;
	target->blink->flink = ev;
	target->blink = ev;
      }
  }

	/* Incremental simulation routines */

/*
 * Back the event queues up to time 'btime'.  This is the opposite of
 * advancing the simulation time.  Mark all pending events as PENDING,
 * and re-enqueue them according to their creation-time (ntime - delay).
 */
public void back_sim_time( btime, is_inc )
  long  btime;
  int   is_inc;
  {
    evptr           tmplist;
    register int    i, nevents;
    register evptr  ev, next;
    register evhdr  *hdr;

    nevents = 0;
    tmplist = NULL;

	/* first empty out the time wheel onto the temporary list */
    for( i = TSIZE - 1, hdr = ev_array; i != 0; i--, hdr++ )
      {
	for( ev = hdr->flink; ev != (evptr) hdr; ev = next )
	  {
	    next = ev->flink;

	    ev->blink->flink = ev->flink;	/* remove event */
	    ev->flink->blink = ev->blink;
	    if( is_inc )
		free_from_node( ev, ev->enode );

	    if( (not is_inc) and ev->ntime - ev->delay >= btime )
	      {
		free_from_node( ev, ev->enode );
		ev->flink = evfree;
		evfree = ev;
	      }
	    else
	      {
		ev->flink = tmplist;		/* move it to tmp list */
		tmplist = ev;

		nevents++;
	      }
	  }
      }

    if( not is_inc )
      {
	requeue_events( tmplist );
	npending = nevents;
	return;
      }

	/* now move the temporary list to the time wheel */
    for( ev = tmplist; ev != NULL; ev = next )
      {
	register long   etime;
	register evptr  target;

	next = ev->flink;

	ev->ntime -= ev->delay;
	ev->type = PENDING;
	etime = ev->ntime;
	target = (evptr) & ev_array[ etime & TMASK ];

	if( (target->blink != target) and (target->blink->ntime > etime) )
	  {
	    do { target = target->flink; } while( target->ntime <= etime );
	  }

	ev->flink = target;
	ev->blink = target->blink;
	target->blink->flink = ev;
	target->blink = ev;
      }

    npending = nevents;
  }


/*
 * Enqueue event type 'type' form history entry 'hist' for node 'nd'.
 * Note that events with type > THREAD are not threaded onto a node's list
 * of pending events, since we don't want the new_val routines to look at
 * this event, instead we keep track of these events in the c.event event
 * of every node.  Return FALSE if this history entry is the sentinel
 * (last_hist), otherwise return TRUE.
 */
public int EnqueueHist( nd, hist, type )
  nptr  nd;
  hptr  hist;
  int   type;
  {
    register evptr  marker, new;
    register long   etime;

    if( hist == last_hist )			/* never queue this up */
      {
	nd->c.event = NULL;
	return( FALSE );
      }

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = hist->time;
    new->eval = hist->val;
    new->enode = nd;
    new->p.hist = hist;
    if( hist->punt )
      {
	new->delay = hist->t.p.delay;
	new->rtime = hist->t.p.rtime;
      }
    else
      {
	new->delay = hist->t.r.delay;
	new->rtime = hist->t.r.rtime;
      }

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending += 1;

    if( hist->inp )
	type |= IS_INPUT;
    else if( new->delay == 0 )
	type |= IS_XINPUT;
    new->type = type;

    if( type > THREAD )
      {
	nd->c.event = new;
	return( TRUE );
      }

    if( (nd->events != NULL) and (nd->events->ntime > etime) )
      {
	for( marker = nd->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = nd->events;
	nd->events = new;
      }
    return( TRUE );
  }


public void DequeueEvent( nd )
  register nptr nd;
  {
    register evptr  ev;

    ev = nd->c.event;
    ev->blink->flink = ev->flink;
    ev->flink->blink = ev->blink;
    ev->flink = evfree;
    evfree = ev;
    nd->c.event = NULL;

    npending -= 1;
  }


public void DelayEvent( ev, delay )
  evptr  ev;
  long   delay;
  {
    register evptr  marker, new;
    register long   etime;
    register nptr   nd;

    nd = ev->enode;
    NEW_EVENT( new );
    
	/* remember facts about this event */
    *new = *ev;
    new->delay += delay;
    new->ntime += delay;

    etime = new->ntime;

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending += 1;

    if( new->type > THREAD )
      {
	nd->c.event = new;
	return;
      }

    if( (nd->events != NULL) and (nd->events->ntime > etime) )
      {
	for( marker = nd->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = nd->events;
	nd->events = new;
      }
  }


public void EnqueueModelChange( time )
  Ulong  time;
  {
    register evptr  marker, new;
    register long   etime;

    NEW_EVENT( new );

    new->ntime = etime = time;
    new->type = CHNG_MODEL;

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending += 1;
  }


/*
 * Remove any events that may be left from the incremental simulation.
 */
public void rm_inc_events()
  {
    register int    i, nevents;
    register evptr  ev, next;
    register evhdr  *hdr;

    nevents = 0;

    for( i = TSIZE - 1, hdr = ev_array; i != 0; i--, hdr++ )
      {
	for( ev = hdr->flink; ev != (evptr) hdr; ev = next )
	  {
	    next = ev->flink;
	    if( ev->type > THREAD or ev->type == PUNTED )
	      {
		ev->blink->flink = next;		/* remove event */
		ev->flink->blink = ev->blink;
		ev->flink = evfree;
		evfree = ev;
		if( ev->type == PUNTED )
		    free_from_node( ev, ev->enode );
	      }
	    else
		nevents++;
	  }
      }

    npending = nevents;
  }
