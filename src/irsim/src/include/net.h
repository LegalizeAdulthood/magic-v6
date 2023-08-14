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


/* header for event driven mosfet simulator.  Chris Terman (6/84) */

typedef struct Event	*evptr;
typedef struct Node	*nptr;
typedef struct Trans	*tptr;
typedef struct Input	*iptr;
typedef struct Tlist	*lptr;
typedef struct HistEnt	*hptr;
typedef struct Bits     *bptr;
typedef struct thevenin *Thev;


struct Tlist
  {
    lptr    next;		/* next list element */
    tptr    xtor;		/* txtor connected to this node */
  };


struct Event
  {
    evptr    flink, blink;	/* doubly-linked event list */
    evptr    nlink;		/* link for list of events for this node */
    nptr     enode;		/* node this event is all about */
    union
      {
	nptr  cause;		/* node which caused this event to happen */
	hptr  hist;		/* ptr. to history entry that queued this */
	long  oldt;		/* original time for delayed events */
      } p;
    long     ntime;		/* time, in DELTAs, of this event */
    long     rtime;		/* rise/fall time, in DELTAs */
    short    delay;		/* delay associated with this event */
    char     eval;		/* new value */
    char     type;		/* type of event (for incremental only) */
  };

typedef unsigned long  Ulong;
typedef unsigned int   Uint;
typedef int (*ifun)();

typedef struct
  {
    short    delay;		      /* delay from input */
    short    rtime;		      /* rise/fall time */
  } RegTimes;

typedef struct
  {
    Uint    delay : 12;		      /* delay from input */
    Uint    rtime : 10;		      /* rise/fall time */
    Uint    ptime : 10;		      /* punt time */
  } PuntTimes;

typedef struct HistEnt
  {
    hptr     next;			      /* next transition in history */
    Ulong    time  : sizeof(Ulong) * 8 - 4;   /* time of transition */
    Uint     inp   : 1;			      /* 1 if node became an input */
    Uint     punt  : 1;			      /* 1 if this event was punted */
    Uint     val   : 2;			      /* value: HIGH, LOW, or X */
    union
      {
	RegTimes    r;
	PuntTimes   p;
      } t;
  } HistEnt;


#define	MAX_TIME	( (~((Ulong) 0)) >> 4 )		/* a huge time */

extern	Ulong  max_time;		/* contains MAX_TIME (compiler bug) */


struct Node
  {
    nptr     nlink;	/* sundries list */
    evptr    events;	/* charge sharing event */
    lptr     ngate;	/* list of xtors w/ gates connected to this node */
    lptr     nterm;	/* list of xtors w/ src/drn connected to this node */
    nptr     hnext;	/* link in hash bucket */
    float    ncap;	/* capacitance of node in pf */
    float    vlow;	/* low logic threshold for node, normalized units */
    float    vhigh;	/* high logic threshold for node, normalized units */
    short    tplh;	/* low to high transition time in DELTA's */
    short    tphl;	/* high to low transition time in DELTA's */
    union
      {
	Ulong  time;	/* time, in DELTAs, of last transistion */
	float  cap;	/* incremental capacitance during net-changes */
	evptr  event;	/* non threaded events in incremental simulation */
      } c;
    union
     {
	nptr  cause;	/* node which caused last transition of this node */
	hptr  punts;	/* punted events during incremental simulation */
	tptr  tran;	/* transistor into which stacked xtors were merged */
     } t;
    int      npot;	/* current potential */
    long     nflags;	/* flag word (see defs below) */
    char     *nname;	/* ascii name of node */
    union
      {
	Thev  thev;	/* used to temporarily store the thevenin structure */
	nptr  next;	/* used to build node lists during net changes */
	tptr  tran;	/* used to mark parallel transistors */
      } n;
    HistEnt  head;	/* first entry in transition history */
    hptr     curr;	/* ptr. to current history entry */
#ifdef FAULT_SIM
    HistEnt  hchange;	/* special entry to avoid changing the history */ 
#endif
  };


typedef struct		/* same as Res_1 but indexed dynamic resists */
  {
    float  dynres[ 2 ];		/* dynamic resistances [R_LOW - R_MAX] */
    float  rstatic;		/* static resistance of transistor */
  } Resists;

#define	R_LOW		0		/* dynamic low resiatance index */
#define	R_HIGH		1		/* dynamic high resiatance index */

#define	dynlow		dynres[ R_LOW ]	/* abbrevations for above */
#define	dynhigh		dynres[ R_HIGH ]


typedef union
  {
    Thev  r;
    tptr  t;
    int   i;
 } TCache;


struct Trans
  {
    nptr     gate, source, drain;    /* nodes to which trans is connected */
    TCache   scache, dcache;	     /* caches to remember src/drn values */
    char     ttype;		     /* type of transistor */
    char     state;		     /* cache to remember current state */
    char     tflags; 		     /* transistor flags */
    char     n_par;		     /* index into parallel list */
    Resists  r;			     /* transistor resistances */
    long     x, y;		     /* position in the layout (optional) */
    tptr     tlink;		     /* next txtor in position hash table */
  };


typedef struct Bits
  {
    bptr    next;		/* next bit vector in chain */
    char    *name;		/* name of this vector of bits */
    int     traced;		/* <>0 if this vector is being traced */
    int     nbits;		/* number of bits in this vector */
    nptr    nodes[1];		/* pointers to the bits (nodes) */
  } Bits;


	/* linked list of inputs */
struct Input
  {
    iptr    next;		/* next element of list */
    nptr    inode;		/* pointer to this input node */
  };


	/* transistor types (ttype) */
#define	NCHAN		0	/* n-channel enhancement */
#define	PCHAN		1	/* p-channel enhancement */
#define	DEP		2	/* depletion */
#define	PULLUP		3	/* pullup => depletion with source == gate */
#define	RESIST		4	/* simple two-terminal resistor */

#define	ALWAYSON	0x06	/* transistors not affected by gate logic */
#define	GATELIST	0x08	/* set if gate of xistor is a node list */
#define	DEP_RES		0x10	/* set if DEP was changed to RESIST */
#define	TCAP		0x20	/* transistor capacitor (source == drain) */
#define	STACKED		0x40	/* transistor was stacked into gate list */
#define	ORED		0x80	/* result of or'ing parallel transistors */

#define	NTTYPES		5	/* number of transistor types defined */

#define	NOT_CONN		( TCAP | STACKED )	/* not connected */
#define	BASETYPE( T )		( (T) & 0x07 )

	/* transistor states (state)*/
#define	OFF		0	/* non-conducting */
#define	ON		1	/* conducting */
#define	UNKNOWN		2	/* unknown */
#define	WEAK		3	/* weak */

	/* transistor temporary flags (tflags) */
#define	CROSSED		0x01	/* Mark for crossing a transistor */
#define BROKEN		0x02	/* Mark a broken transistor to avoid loop */
#define	PBROKEN		0x04	/* Mark as broken a parallel transistor */
#define	PARALLEL	0x08	/* Mark as being a parallel transistor */
#define	ACTIVE_T	0x10	/* incremental status of transistor */

	/* figure what's on the *other* terminal node of a transistor */
#define	other_node( T, N )	((T)->source == (N) ? (T)->drain : (T)->source)

	/* node potentials */
#define	LOW		0	/* low low */
#define	X		1	/* unknown, intermediate, ... value */
#define	HIGH		3	/* logic high */
#define	N_POTS		4	/* number of potentials [LOW-HIGH] */

#define	DECAY		4	/* waiting to decay to X (only in events) */

	/* possible values for nflags */
#define	OLD_VAL		0x000003
#define	WATCHED		0x000004
#define	INPUT		0x000008
#define	POWER_RAIL	0x000010
#define	ALIAS		0x000020
#define	MERGED		0x000040
#define	STOPONCHANGE	0x000080
#define	USERDELAY	0x000100
#define	VISITED		0x000200

#define	ALIASED		0x000400	/* another node is aliased to this */
#define	DELETED		0x000800	/* unused now */

#define	H_INPUT		0x001000	/* node is in high input list */
#define	L_INPUT		0x002000	/* node is in low input list */
#define	U_INPUT		0x003000	/* node is in U input list */
#define	X_INPUT		0x004000	/* node is in X input list */
#define	OLD_INPUT	X_INPUT		/* input has been processed already */

#define	INPUT_MASK		( H_INPUT | L_INPUT | X_INPUT | U_INPUT )
#define	IsInList( flg )		( (flg) & INPUT_MASK )
#define	INPUT_NUM( flg )	( ((flg) & INPUT_MASK) >> 12 )

#define	CHANGED		0x080000	/* node is affected by a net change */
#define	DEVIATED	0x100000	/* node's state differs from hist */
#define	STIM		0x200000	/* node is used as stimuli */
#define	ACTIVE_CL	0x400000	/* node is in an active cluster */
#define	WAS_ACTIVE	0x800000	/* set if node was ever active */

#define	DEV_BIT		20		/* DEVIATED bit position */

	/* resistance types */
#define	STATIC		0	/* static resistance */
#define	DYNHIGH 	1	/* dynamic-high resistance */
#define	DYNLOW  	2	/* dynamic-low resistance */
#define	POWER		3	/* resist. for power calculation (unused) */
#define	R_TYPES		3	/* number of resistance contexts */

	/* Define TRUE and FALSE values */
#define TRUE  1
#define FALSE 0

	/* Possible simulator status */
#define	NORM_SIM		0		/* normal mode */
#define	INCR_SIM		01		/* incremental mode */
#define	OUT_OF_MEM		02		/* out of memory flag */

	/* Event Types (for incremental simulation only) */

#define	IS_INPUT		0x1		/* event makes node input */
#define	IS_XINPUT		0x2		/* event terminates input */

#define	REVAL			0x0		/* result of re-evaluation */
#define	DECAY_EV		0x1		/* node is decaying to X */
#define	PUNTED			0x3		/* previously punted event */

	/* events > THREAD are NOT threaded into node structure */
#define	THREAD			0x3

#define	PENDING			0x4		/* pending from last run */ 
#define	STIMULI			0x8		/* self-schedulled stimuli */
#define	STIM_INP		( STIMULI | IS_INPUT )
#define	STIM_XINP		( STIMULI | IS_XINPUT )

#define	CHECK_PNT		0x10		/* next change in history */
#define	INP_EV			( CHECK_PNT | IS_INPUT )
#define	XINP_EV			( CHECK_PNT | IS_XINPUT )
#define	DELAY_CHK		0x20		/* delayed CHECK_PNT */
#define	DELAY_EV		0x40		/* last REVAL was delayed */

#define	CHNG_MODEL		0x80		/* change evaluation model */


	/* Conversion macros between various time units */
#define	d2ns( D )		( (D) * 0.1 )		/* deltas to ns */
#define	d2ps( D )		( (D) * 100.0 )		/* deltas to ps */
#define	ns2d( N )		( (N) * 10.0 )		/* ns to deltas */
#define	ps2d( P )		( (P) * 0.01 )		/* ps to deltas */
#define	ps2ns( P )		( (P) * 0.001 )		/* ps to ns */
