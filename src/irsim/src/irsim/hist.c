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
#include "ASSERT.h"
#include "globals.h"


public	hptr   freeHist = NULL;		/* list of free history entries */
public  hptr   last_hist;		/* pointer to dummy hist-entry that
					 * serves as tail for all nodes */
public	int    num_edges = 0;
public	int    num_punted = 0;
public	int    num_cons_punted = 0;

public	hptr   first_model;		/* first model entry */
private	hptr   curr_model;		/* ptr. to current model entry */

private	char   mem_msg[] = "*** OUT OF MEMORY:Will stop collecting history\n";


public	Ulong  max_time;

public void init_hist()
  {
    static HistEnt  dummy;
    static HistEnt  dummy_model;

    max_time = MAX_TIME;

    last_hist = &dummy;
    dummy.next = last_hist;
    dummy.time = max_time;
    dummy.val = X;
    dummy.inp = 1;
    dummy.punt = 0;
    dummy.t.r.delay = dummy.t.r.rtime = 0;

    dummy_model.time = 0;
    dummy_model.val = model;
    dummy_model.inp = 0;
    dummy_model.punt = 0;
    dummy_model.next = NULL;
    first_model = curr_model = &dummy_model;
  }


#define	NEW_HIST( NEW, ACTnoMEM )					\
  {									\
    if( ((NEW) = freeHist) == NULL )					\
      {									\
	if( ((NEW) = (hptr) MallocList( sizeof( HistEnt ) )) == NULL )	\
	  {								\
	    lprintf( stderr, mem_msg );					\
	    sm_stat |= OUT_OF_MEM;					\
	    ACTnoMEM;							\
	  }								\
      }									\
    freeHist = (NEW)->next;						\
  }									\


/*
 * Add a new entry to the history list.  Update curr to point to this change.
 */
public int AddHist( node, value, inp, time, delay, rtime )
  register nptr  node;
  int            value;
  int            inp;
  long           time;
  long           delay, rtime;
  {
    register hptr  newh, curr;

    num_edges++;

    curr = node->curr;

    if( sm_stat & OUT_OF_MEM )
        return( 1 );

    while( curr->next->punt )		/* skip past any punted events */
	curr = curr->next;

    NEW_HIST( newh, return( 1 ) );

    newh->next = curr->next;
    newh->time = time;
    newh->val = value;
    newh->inp = inp;
    newh->punt = 0;
    newh->t.r.delay = delay;
    newh->t.r.rtime = rtime;
    node->curr = curr->next = newh;
    return( 0 );
  }


#define	PuntTime( H )		( (H)->time - (H)->t.p.ptime )


/*
 * Add a punted event to the history list for the node.  Consecutive punted
 * events are kept in punted-order, so that h->ptime < h->next->ptime.
 * Adding a punted event does not change the current pointer, which always
 * points to the last "effective" node change.
 */
public int AddPunted( node, ev, tim )
  register nptr  node;
  evptr          ev;
  long           tim;
  {
    register hptr  newp, h;

    h = node->curr;

    num_punted++;
    if( sm_stat & OUT_OF_MEM )
        return( 1 );

    NEW_HIST( newp, return( 1 ) );	/* allocate the punted event itself */

    newp->time = ev->ntime;
    newp->val = ev->eval;
    newp->inp = 0;
    newp->punt = 1;
    newp->t.p.delay = ev->delay;
    newp->t.p.rtime = ev->rtime;
    newp->t.p.ptime = newp->time - tim;

    if( h->next->punt )		/* there are some punted events already */
      {
	num_cons_punted++;
	do { h = h->next; } while( h->next->punt );
      }

    newp->next = h->next;
    h->next = newp;

    return( 0 );
  }


/*
 * Free up a node's history list
 */
public void FreeHistList( node )
  register nptr  node;
  {
    register hptr  h, next;

    if( (h = node->head.next) == last_hist )		/* nothing to do */
    	return;

    while( (next = h->next) != last_hist )		/* find last entry */
	h = next;

    h->next = freeHist;				/* link list to free list */
    freeHist = node->head.next;

    node->head.next = last_hist;
    node->curr = &(node->head);

    sm_stat &= ~OUT_OF_MEM;
  }



public void NoMoreIncSim()
  {
    fprintf( stderr, "can't continue incremetal simulation\n" );
    exit( 1 );		/* do something here ? */
  }


/*
 * Add a new model entry, recording the time of the change.
 */
public void NewModel( model_num )
  int  model_num;
  {
    if( curr_model->time != cur_delta )
      {
	hptr newh;

	NEW_HIST( newh, NoMoreIncSim() );

	newh->next = NULL;
	newh->time = cur_delta;
	newh->val = model_num;
	curr_model->next = newh;
	curr_model = newh;
      }
    else
	curr_model->val = model_num;
  }



#define	QTIME( H )		( (H)->time - (H)->t.r.delay )

/*
 * Add a new change to the history list of node 'nd' caused by event 'ev'.
 * Skip past any punted events already in the history and update nd->curr to
 * point to the new change.
 */
public void NewEdge( nd, ev )
  nptr   nd;
  evptr  ev;
  {
    register hptr  p, h, newh;

    NEW_HIST( newh, NoMoreIncSim() );

    newh->time = ev->ntime;
    newh->val = ev->eval;
    newh->inp = 0;		/* always true in incremental simulation */
    newh->punt = 0;
    newh->t.r.delay = ev->delay;
    newh->t.r.rtime = ev->rtime;

    for( p = nd->curr, h = p->next; h->punt; p = h, h = h->next );
    newh->next = h;
    p->next = newh;

    nd->curr = newh;		/* newh becomes the current value */
  }


/*
 * Delete the next effective change (following nd->curr) from the history.
 * Punted events before the next change (in nd->t.punts) can now be freed.
 * Punted events following the deleted edge are moved to nd->t.punts.
 */
public void DeleteNextEdge( nd )
  nptr  nd;
  {
    register hptr  a, b, c;

    if( (a = nd->t.punts) != NULL )	/* remove previously punted events */
      {
	for( b = a; b->next != NULL; b = b->next );
	b->next = freeHist;
	freeHist = a;
      }

    a = nd->curr;
    for( b = a->next; b->punt; a = b, b = b->next );
    for( c = b->next; c->punt; b = c, c = c->next );
    c = a->next;			/* c => next edge */
    a->next = b->next;
    a = c->next;			/* a => first punted event after c */

    c->next = freeHist;			/* free the next effective change */
    freeHist = c;

    if( a->punt )			/* move punted events from hist */
      {
	nd->t.punts = a;
	b->next = NULL;
      }
    else
	nd->t.punts = NULL;
  }


#define	NEXTH( H, P )	for( (H) = (P)->next; (H)->punt; (H) = (H)->next )

public void FlushHist( ftime )
  register Ulong  ftime;
  {
    register nptr  n;
    register hptr  h, p, head;

    for( n = GetNodeList(); n != NULL; n = n->n.next )
      {
	head = &(n->head);
	if( head->next == last_hist or (n->nflags & ALIAS) )
	    continue;
	p = head;
	NEXTH( h, p );
	while( h->time < ftime )
	  {
	    p = h;
	    NEXTH( h, h );
	  }
	head->val = p->val;
	head->time = p->time;
	head->inp = p->inp;
	while( p->next != h )
	    p = p->next;
	if( head->next != h )
	  {
	    p->next = freeHist;
	    freeHist = head->next;
	    head->next = h;
	  }
	if( n->curr->time < ftime )
	  {
	    n->curr = head;
	  }
      }
  }


public int backToTime( nd )
  register nptr  nd;
  {
    register hptr  h, p;

    if( nd->nflags & ALIAS )
	return( 0 );

    h = &(nd->head);
    NEXTH( p, h );
    while( p->time < cur_delta )
      {
	h = p;
	NEXTH( p, p );
      }
    nd->curr = h;

	/* queue pending events */
    for( p = h, h = p->next; ; p = h, h = h->next )
      {
	long  qtime;

	if( h->punt )
	  {
	    if( PuntTime( h ) < cur_delta )	/* already punted, skip it */
		continue;

	    qtime = h->time - h->t.p.delay;	/* pending, enqueue it */
	    if( qtime < cur_delta )
	      {
		Ulong tmp = cur_delta;
		cur_delta = qtime;
		enqueue_event( nd, h->val, (long) h->t.p.delay,
		  (long) h->t.p.rtime );
		cur_delta = tmp;
	      }
	    p->next = h->next;
	    h->next = freeHist;
	    freeHist = h;
	    h = p;
	  }
	else
	  {
	    qtime = QTIME( h );
	    if( qtime < cur_delta )		/* pending, enqueue it */
	      {
		Ulong tmp = cur_delta;
		cur_delta = qtime;
		enqueue_event( nd, h->val, (long) h->t.r.delay,
		  (long) h->t.r.rtime );
		cur_delta = tmp;

		p->next = h->next;		/* and free it */
		h->next = freeHist;
		freeHist = h;
		h = p;
	      }
	    else
		break;
	  }
      }

    p->next = last_hist;
    p = h;
	/* p now points to the 1st event in the future (to be deleted) */
    if( p != last_hist )
      {
	while( h->next != last_hist )
	    h = h->next;
	h->next = freeHist;
	freeHist = p;
      }

    h = nd->curr;
    nd->npot = h->val;
    nd->c.time = h->time;
    if( h->inp and not( nd->nflags & POWER_RAIL ) )
      {
	switch( h->val )
	  {
	    case HIGH :
		nd->nflags |= (OLD_INPUT | H_INPUT | INPUT);
		iinsert( nd, &o_hinputs );
		break;

	    case LOW :
		nd->nflags |= (OLD_INPUT | L_INPUT | INPUT);
		iinsert( nd, &o_linputs );
		break;

	    case X :
		nd->nflags |= (OLD_INPUT | U_INPUT | INPUT);
		iinsert( nd, &o_uinputs );
		break;
	  }
      }

    if( nd->ngate != NULL )		/* recompute transistor states */
      {
	register lptr  l;
	register tptr  t;

	for( l = nd->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->state = compute_trans_state( t );
	  }
      }
    return( 0 );
  }


/****** Routines to dump and read history file **********/

#include "bin_io.h"

private	char    fh_header[] = "#HDUMP#\n";	/* first line of dump file */

private	int     dumph_version = 2;

	/* define the number of bytes used to read/write history */
#define	NB_NUMBER	4
#define	NB_HEADER	( sizeof( fh_header ) - 1 )
#define	NB_MAJOR	2
#define	NB_MINOR	2
#define	NB_TIME		4
#define	NB_PVAL		1
#define	NB_RTIME	2
#define	NB_DELAY	2
#define	NB_EVAL		1
#define	NB_VERSION	2


typedef struct
  {
    char  header[ NB_HEADER ];
    char  hsize[ NB_NUMBER ];
    char  nnodes[ NB_NUMBER ];
    char  cur_delta[ NB_TIME ];
    char  magic[ NB_NUMBER ];
    char  version[ NB_VERSION ];
  } File_Head;

#define	Size_File_Head	\
    ( NB_HEADER + NB_NUMBER + NB_NUMBER + NB_TIME + NB_NUMBER + NB_VERSION )


typedef struct
  {
    char  flags[ 2 ];
    char  time0[ NB_TIME ];
    char  free[ 40 ];                   /* add extra info here */
  } File_Extra;

#define	Size_File_Extra		( 2 + NB_TIME + 40 )


#define	FLAGS_TIME0	0x1


typedef struct			/* node specifier */
  {
    char  major[ NB_MAJOR ];
    char  minor[ NB_MINOR ];
  } NDid;

#define	Size_NDid		( NB_MAJOR + NB_MINOR )

typedef struct			/* format of history header, one per node */
  {
    NDid  node;			/* node for which history follows */
    char  time[ NB_TIME ];	/* initial time (usually 0) */
    char  pval[ NB_PVAL ];	/* initial packed-value */
  } Node_Head;

#define	Size_Node_Head		( Size_NDid + NB_TIME + NB_PVAL )


typedef struct		/* format of each history entry */
  {
    char  time[ NB_TIME ];	/* time of this change */
    char  rtime[ NB_RTIME ];	/* input rise time */
    char  delay[ NB_DELAY ];	/* associated delay */
    char  pval[ NB_PVAL ];	/* packed-value (inp, val, punt) */
    char  ptime[ NB_DELAY ];	/* punt time (only punted events) */
  } File_Hist;

#define	Size_File_Hist		( NB_TIME + NB_RTIME + NB_DELAY + NB_PVAL )
#define	Size_Ptime		( NB_DELAY )
#define	Size_PuntFile_Hist	( Size_File_Hist + Size_Ptime )


	/* marks end of history.  Must be same size as Size_File_Hist! */
typedef struct
  {
    char  mark[ NB_TIME ];		/* marker, same for all nodes */
    char  npend[ NB_RTIME ];		/* # of pending events */
    char  dummy1[ NB_DELAY ];		/* just fill the space */
    char  dummy2[ NB_PVAL ];
  } EndHist;


typedef struct		/* format for pending events */
  {
    NDid  cause;
    char  time[ NB_TIME ];
    char  delay[ NB_DELAY ];
    char  rtime[ NB_RTIME ];
    char  eval[ NB_EVAL ];
  } File_Pend;

#define	Size_File_Pend	( Size_NDid + NB_TIME + NB_DELAY + NB_RTIME + NB_EVAL)


	/* macros to pack/unpack a value/inp/punt triplet from/to a byte */

#define	PACK_VAL( H )		( (H->inp << 5) | (H->punt << 4) | H->val )
#define	GET_INP( PV )		( (PV) >> 5 & 1 )
#define	GET_VAL( PV )		( (PV) & 0x7 )
#define	IS_PUNT( PV )		( (PV) & 0x10 )


private	FILE       *fd;
private	EndHist    h_end;
private	char       marker[] = "\0\0\0\040";


private int DumpNodeHist( nd, major, minor )
  nptr  nd;
  int   major, minor;
  {
    register hptr  h;
    Node_Head      header;
    File_Hist      hist;

    if( nd->nflags & (POWER_RAIL | ALIAS) )
	return;

    PackBytes( header.node.major, major, NB_MAJOR );
    PackBytes( header.node.minor, minor, NB_MINOR );

    h = &(nd->head);
    PackBytes( header.time, h->time, NB_TIME );
    PackBytes( header.pval, PACK_VAL( h ), NB_PVAL );
    if( Fwrite( &header, Size_Node_Head, fd ) == 0 )
	goto abort;
    
    for( h = h->next; h != last_hist; h = h->next )
      {
	int  nb;

	PackBytes( hist.time, h->time, NB_TIME );
	PackBytes( hist.pval, PACK_VAL( h ), NB_PVAL );
	if( h->punt )
	  {
	    PackBytes( hist.delay, h->t.p.delay, NB_DELAY );
	    PackBytes( hist.rtime, h->t.p.rtime, NB_RTIME );
	    PackBytes( hist.ptime, h->t.p.ptime, NB_DELAY );
	    nb = Size_PuntFile_Hist;
	  }
	else
	  {
	    PackBytes( hist.delay, h->t.r.delay, NB_DELAY );
	    PackBytes( hist.rtime, h->t.r.rtime, NB_RTIME );
	    nb = Size_File_Hist;
	  }
	if( Fwrite( &hist, nb, fd ) == 0 )
	    goto abort;
      }

    if( nd->events != NULL )
      {
	register evptr  ev;
	register int    n;
	File_Pend       pending;

	for( n = 0, ev = nd->events; ev != NULL; ev = ev->nlink, n++ );
	PackBytes( h_end.npend, n, NB_RTIME );
	if( Fwrite( &h_end, Size_File_Hist, fd ) == 0 )
	    goto abort;
	for( ev = nd->events; ev != NULL; ev = ev->nlink )
	  {
	    Node2index( ev->p.cause, &major, &minor );
	    PackBytes( pending.cause.major, major, NB_MAJOR );
	    PackBytes( pending.cause.minor, minor, NB_MINOR );
	    PackBytes( pending.time, ev->ntime, NB_TIME );
	    PackBytes( pending.delay, ev->delay, NB_DELAY );
	    PackBytes( pending.rtime, ev->rtime, NB_RTIME );
	    PackBytes( pending.eval, ev->eval, NB_EVAL );
	    if( Fwrite( &pending, Size_File_Pend, fd ) == 0 )
		goto abort;
	  }
      }
    else
      {
	PackBytes( h_end.npend, 0, NB_RTIME );
	if( Fwrite( &h_end, Size_File_Hist, fd ) == 0 )
	    goto abort;
      }

    return( TRUE );

  abort:
    lprintf( stderr, "can't write to file, history dump aborted\n" );
    return( FALSE );
  }


public void DumpHist( fname )
  char  *fname;
  {
    File_Head  fh;
    File_Extra ft;
    int        flags;
    long       mag;

    if( (fd = fopen( fname, "w" )) == NULL )
      {
	lprintf( stderr, "can not open file '%s'\n", fname );
	return;
      }
    bcopy( fh_header, fh.header, NB_HEADER );
    PackBytes( fh.hsize, GetHashSize(), NB_NUMBER );
    PackBytes( fh.nnodes, nnodes, NB_NUMBER );
    PackBytes( fh.cur_delta, cur_delta, NB_TIME );
    mag = ( (cur_delta ^ nnodes) & 0xffff ) | 0x5500000;
    PackBytes( fh.magic, mag, NB_NUMBER );
    PackBytes( fh.version, dumph_version, NB_VERSION );
    if( Fwrite( &fh, Size_File_Head, fd ) == 0 )
      {
	lprintf( stderr, "can't write to file\n" );
	fclose( fd );
	return;
      }
    
    flags = FLAGS_TIME0;
    PackBytes( ft.flags, flags, 2 );
    PackBytes( ft.time0, sim_time0, NB_NUMBER );
    bzero( ft.free, 40 );
    Fwrite( &ft, Size_File_Extra, fd );

    bcopy( marker, h_end.mark, NB_TIME );	/* initialize h_end */
    bzero( h_end.dummy1, NB_DELAY );
    bcopy( "\n", h_end.dummy2, NB_PVAL );

    walk_net_index( DumpNodeHist, FALSE );

    fclose( fd );
  }


private	nptr    ndlist;

private int rd_hist()
  {
    Node_Head  head;
    File_Hist     hist;
    EndHist       *pe;
    int           inp, val, n, pval;
    int           major, minor;
    nptr          nd;
    long          time;
    int           delay;
    int           rtime;
    struct Event  ev;

    pe = (EndHist *) &hist;
    ndlist = NULL;
    while( Fread( &head, Size_Node_Head, fd ) == Size_Node_Head )
      {
	UnpackBytes( head.node.major, major, NB_MAJOR );
	UnpackBytes( head.node.minor, minor, NB_MINOR );
	if( (nd = Index2node( major, minor )) == NULL )
	  {
	    lprintf( stderr, "history read aborted: could not find node\n" );
	    return( FALSE );
	  }

	if( nd->nflags & (POWER_RAIL | ALIAS) )
	  {
	    lprintf( stderr, "warning: %s should not be in history\n",
	      pnode( nd ) );
	  }

	UnpackBytes( head.time, time, NB_TIME );
	UnpackBytes( head.pval, pval, NB_PVAL );
	val = GET_VAL( pval );
	inp = GET_INP( pval );
	nd->head.time = time;
	nd->head.val = val;
	nd->head.inp = inp;
	nd->head.t.r.rtime = nd->head.t.r.delay = 0;
	nd->n.next = ndlist;
	ndlist = nd;

	if( nd->head.next != last_hist )
	    FreeHistList( nd );

	while( TRUE )
	  {
	    if( Fread( &hist, Size_File_Hist, fd ) != Size_File_Hist )
		goto badfile;
	    if( bcmp( pe->mark, marker, NB_TIME ) == 0 )
		break;
	    if( nd->nflags & (POWER_RAIL | ALIAS) )
		continue;
	    UnpackBytes( hist.time, time, NB_TIME );
	    UnpackBytes( hist.delay, delay, NB_DELAY );
	    UnpackBytes( hist.rtime, rtime, NB_RTIME );
	    UnpackBytes( hist.pval, pval, NB_PVAL );
	    val = GET_VAL( pval );
	    inp = GET_INP( pval );

	    ASSERT( delay < 60000 and delay > 0 )
	      {
		lprintf( stdout, "Error: Corrupted history entry:\n" );
		lprintf( stdout, "\t%s time=%.1f delay=%.1f value=%c\n",
		  pnode( nd ), d2ns( time ), d2ns( delay ), vchars[val] );
	      }
	    if( IS_PUNT( pval ) )
	      {
		if( Fread( hist.ptime, Size_Ptime, fd ) != Size_Ptime )
		    goto badfile;
		ev.eval = val;
		ev.ntime = time;
		ev.delay = delay;
		ev.rtime = rtime;
		UnpackBytes( hist.ptime, delay, NB_DELAY );
		cur_delta = time - delay;
	      	AddPunted( nd, &ev, cur_delta );
	      }
	    else
		AddHist( nd, val, inp, time, (long) delay, (long) rtime );
	  }

		/* set INPUT flag and add to corresponding list */
	if( nd->curr->inp and not( nd->nflags & POWER_RAIL) )
	  {
	    switch( val )
	      {
		case HIGH :
		    nd->nflags |= (OLD_INPUT | H_INPUT);
		    iinsert( nd, &o_hinputs );
		    break;

		case LOW :
		    nd->nflags |= (OLD_INPUT | L_INPUT );
		    iinsert( nd, &o_linputs );
		    break;

		case X :
		    nd->nflags |= (OLD_INPUT | U_INPUT);
		    iinsert( nd, &o_uinputs );
		    break;
	      }
	  }

	while( nd->events != NULL )	/* get rid of any pending events */
	    free_event( nd->events );

	UnpackBytes( pe->npend, n, NB_RTIME );
	while( n != 0 )
	  {
	    File_Pend  pend;

	    if( Fread( &pend, Size_File_Pend, fd ) != Size_File_Pend )
		goto badfile;
	    UnpackBytes( pend.cause.major, major, NB_MAJOR );
	    UnpackBytes( pend.cause.minor, minor, NB_MINOR );
	    UnpackBytes( pend.time, time, NB_TIME );
	    UnpackBytes( pend.delay, delay, NB_DELAY );
	    UnpackBytes( pend.rtime, rtime, NB_RTIME );
	    UnpackBytes( pend.eval, val, NB_EVAL );
	    ASSERT( delay < 60000 and delay > 0 )
	      {
		lprintf( stdout, "Error: Corrupted history entry:\n" );
		lprintf( stdout, "\t%s time=%.1f delay=%.1f value=%c [pnd]\n",
		  pnode( nd ), d2ns( time ), d2ns( delay ), vchars[val] );
		n--;
		continue;
	      }

	    cur_node = Index2node( major, minor );
	    cur_delta = time - delay;			/* fake the delay */
	    enqueue_event( nd, val, (long) delay, (long) rtime );
	    n--;
	  }
      }
    return( TRUE );

  badfile:
    lprintf( stderr, "premature eof on history file\n" );
    return( FALSE );
  }


private void fix_transistors( nd )
  register nptr  nd;
  {
    register lptr  l;

    while( nd != NULL )
      {
	nd->npot = nd->curr->val;
	if( nd->curr->inp )
	    nd->nflags |= INPUT;

	for( l = nd->ngate; l != NULL; l = l->next )
	    l->xtor->state = compute_trans_state( l->xtor );
	nd = nd->n.next;
      }
    for( l = VDD_node->ngate; l != NULL; l = l->next )
	l->xtor->state = compute_trans_state( l->xtor );
    for( l = GND_node->ngate; l != NULL; l = l->next )
	l->xtor->state = compute_trans_state( l->xtor );
  }


public void ReadHist( fname )
  char  *fname;
  {
    File_Head  fh;
    File_Extra ft;
    long       newTime, time0, magic;
    int        n_nodes, hsize;
    int        status;

    if( (fd = fopen( fname, "r" )) == NULL )
      {
	lprintf( stderr, "can not open file '%s'\n", fname );
	return;
      }

    if( Fread( &fh, Size_File_Head, fd ) != Size_File_Head )
      {
	lprintf( stderr, "can't read file\n" );
	fclose( fd );
	return;
      }

    if( strncmp( fh_header, fh.header, NB_HEADER ) != 0 )
      {
	lprintf( stderr, "ReadHist: bad format for dump file '%s'\n", fname );
	fclose( fd );
	return;
      }

    UnpackBytes( fh.version, hsize, NB_VERSION );
    if( hsize != dumph_version )
      {
	lprintf( stderr, "ReadHist: Incompatible version: %d (%d)\n",
	  hsize, dumph_version );
	fclose( fd );
	return;
      }
    UnpackBytes( fh.hsize, hsize, NB_NUMBER );
    UnpackBytes( fh.nnodes, n_nodes, NB_NUMBER );
    UnpackBytes( fh.cur_delta, newTime, NB_TIME );
    UnpackBytes( fh.magic, magic, NB_NUMBER );
    if( nnodes != n_nodes or hsize != GetHashSize() or 
      ( magic & 0xffff) != ((newTime ^ n_nodes) & 0xffff) )
      {
	lprintf( stderr, "ReadHist: incompatible or bad history dump\n" );
	fclose( fd );
	return;
      }

    if( (magic & 0xffff0000) == 0x5500000 )
      {
	if( Fread( &ft, Size_File_Extra, fd ) == Size_File_Extra )
	  {
	    int  flags;

	    UnpackBytes( ft.flags, flags, 2 );
	    if( flags & FLAGS_TIME0 )
		UnpackBytes( ft.time0, time0, NB_NUMBER );
	  }
	else
	  {
	    lprintf( stderr, "ReadHist: Premature EOF on hist file\n" );
	    fclose( fd );
	    return;
	  }
      }
    else
     {
	time0 = sim_time0;
     }

    ClearInputs();

    if( rd_hist() == FALSE )
      {
	nptr  n;

	for( n = ndlist; n != NULL; n = n->n.next )
	  {
	    FreeHistList( n );			/* undo any work done */
	    while( n->events != NULL )
		free_event( n->events );
	  }
	fclose( fd );
	return;
      }

    sim_time0 = time0;
    cur_delta = newTime;
    if( cur_delta > 0 )
	NoInit();
    fix_transistors( ndlist );

    fclose( fd );
  }
