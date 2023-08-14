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
 * The routines in this file handle binary network files input/output.
 */

#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"
#include "bin_io.h"
#include "net_macros.h"


private	FILE   *fnet;			/* file for reading/writing network */
private	tptr   tlist;			/* list of transistor just read */
private	int    were_stacked;		/* when net file was written */
private	nptr   ndlist;			/* list of nodes just read */


private	char   netHeader[] = "***inet***";

#define	HEADER_LEN		(sizeof( netHeader ) - 1)
#define	STACK_INDEX		(1 << 0)
#define	COORD_INDEX		(1 << 1)

	/* define number of bytes used to write/read network parameter */
#define	NB_VTHRESH	2
#define	NB_TP		2
#define	NB_NCAP		4
#define	NB_FLAGS	1
#define	NB_STRLEN	2
#define	NB_RESIST	4
#define	NB_TTYPE	1
#define	NB_MAJOR	2
#define	NB_MINOR	2
#define	NB_COORD	4

	/* node flags we are interested in preserving */
#define	SAVE_FLAGS	(POWER_RAIL | ALIAS | MERGED | USERDELAY | ALIASED)

	/* scale factors for floating-point numbers, use "nice" powers of 2 */
#define	RES_SCALE		256.0
#define	VTH_SCALE		256.0
#define	CAP_SCALE		16384.0


typedef struct		/* tuple that identifies a node */
  {
    char  major[ NB_MAJOR ];
    char  minor[ NB_MINOR ];
  } NDid;

#define	Size_NDid	( NB_MAJOR + NB_MINOR )


typedef struct		/* format of node records in net file */
  {
    char  vhigh[ NB_VTHRESH ];
    char  vlow[ NB_VTHRESH ];
    char  tphl[ NB_TP ];
    char  tplh[ NB_TP ];
    union
      {
	char  cap[ NB_NCAP ];
	NDid  alias;
      } n;
    char  flags[ NB_FLAGS ];
    char  slen[ NB_STRLEN ];
  } File_Node;

#define	Size_File_Node 	( NB_VTHRESH + NB_VTHRESH + NB_TP + NB_TP + \
		Size_NDid + NB_FLAGS + NB_STRLEN )


typedef struct		/* format of transistor records in net file */
  {
    char  staticr[ NB_RESIST ];
    char  rdynhigh[ NB_RESIST ];
    char  rdynlow[ NB_RESIST ];
    char  ttype[ NB_TTYPE ];
    NDid  src;
    NDid  drn;
    NDid  gate;
    char  x[ NB_COORD ];
    char  y[ NB_COORD ];
  } File_Trans;

#define	Size_File_Trans	( NB_RESIST + NB_RESIST + NB_RESIST + NB_TTYPE + \
		Size_NDid + Size_NDid + Size_NDid + NB_COORD + NB_COORD )


private tptr wr_nodes()
  {
    register nptr  n;
    register tptr  txlist;
    File_Node      node;
    int            slen;

    txlist = NULL;
    for( n = GetNodeList(); n != NULL; n = n->n.next )
      {
	slen = strlen( pnode( n ) ) + 1;
	PackBytes( node.vhigh, n->vhigh * VTH_SCALE, NB_VTHRESH );
	PackBytes( node.vlow, n->vlow * VTH_SCALE, NB_VTHRESH );
	PackBytes( node.tphl, n->tphl, NB_TP );
	PackBytes( node.tplh, n->tplh, NB_TP );
	if( n->nflags & ALIAS )
	  {
	    int   major, minor;

	    Node2index( n->nlink, &major, &minor );
	    PackBytes( node.n.alias.major, major, NB_MAJOR );
	    PackBytes( node.n.alias.minor, minor, NB_MINOR );
	  }
	else
	    PackBytes( node.n.cap, n->ncap * CAP_SCALE, NB_NCAP );
	PackBytes( node.flags, n->nflags & SAVE_FLAGS, NB_FLAGS );
	PackBytes( node.slen, slen, NB_STRLEN );
	Fwrite( &node, Size_File_Node, fnet );
	Fwrite( pnode( n ), slen, fnet );

	  {
	    register lptr  l;
	    register tptr  t;

	    for( l = n->nterm; l != NULL; l = l->next )
	      {
		t = l->xtor;
		if( not (t->tflags & ACTIVE_T) )
		  {
		    t->scache.t = txlist;
		    txlist = t;
		    t->tflags |= ACTIVE_T;
		  }
	      }
	  }
      }
    return( txlist );
  }


private void wr_trans( t, tsize )
  register tptr  t;
  int            tsize;
  {
    File_Trans  trans;
    int         major, minor;

    Node2index( t->gate, &major, &minor );
    PackBytes( trans.gate.major, major, NB_MAJOR );
    PackBytes( trans.gate.minor, minor, NB_MINOR );

    Node2index( t->source, &major, &minor );
    PackBytes( trans.src.major, major, NB_MAJOR );
    PackBytes( trans.src.minor, minor, NB_MINOR );

    Node2index( t->drain, &major, &minor );
    PackBytes( trans.drn.major, major, NB_MAJOR );
    PackBytes( trans.drn.minor, minor, NB_MINOR );

    PackBytes( trans.staticr, t->r.rstatic * RES_SCALE, NB_RESIST );
    PackBytes( trans.rdynhigh, t->r.dynhigh * RES_SCALE, NB_RESIST );
    PackBytes( trans.rdynlow, t->r.dynlow * RES_SCALE, NB_RESIST );

    PackBytes( trans.ttype, t->ttype, NB_TTYPE );

    PackBytes( trans.x, t->x, NB_COORD );
    PackBytes( trans.y, t->y, NB_COORD );

    Fwrite( &trans, tsize, fnet );
  }


private void wr_txtors( t )
  register tptr  t;
  {
    register tptr  next;
    int            tsize;

    tsize = txt_coords ? Size_File_Trans : Size_File_Trans - (2 * NB_COORD);

    do
      {
	next = t->scache.t;

	t->tflags &= ~ACTIVE_T;
	t->scache.t = NULL;

	if( t->ttype & GATELIST )
	  {
	    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
		wr_trans( t, tsize );
	  }
	else
	    wr_trans( t, tsize );

      }
    while( (t = next) != NULL );

    for( t = tcap_list; t != NULL; t = t->scache.t )
	wr_trans( t, tsize );

    for( t = ored_list; t != NULL; t = t->scache.t )
	wr_trans( t, tsize );
  }


public void wr_netfile( fname )
  char  *fname;
  {
    tptr  txlist;
    char  s[6];

    if( (fnet = fopen( fname, "w" )) == NULL )
      {
	fprintf( stderr, "can't open file '%s'\n", fname );
	return;
      }
    s[0] = (stack_txtors) ? '1' : '0';
    s[1] = (txt_coords) ? '1' : '0';
    s[2] = '\0';
    fprintf( fnet, "%s%s\n", netHeader, s );
    fprintf( fnet, "%d %d\n", GetHashSize(), nnodes + naliases );
    WriteParallel( fnet );
    txlist = wr_nodes();
    wr_txtors( txlist );
  }


private void PrematureEof()
  {
    fprintf( stderr, "premature eof in inet file\n" );
    exit( 1 );
  }


private void rd_nodes( nname, n_nodes )
  char  *nname;
  int   n_nodes;
  {
    File_Node  node;
    long       tmp;
    nptr       n;
    nptr       aliases;

    ndlist = NULL;
    aliases = NULL;
    while( n_nodes-- != 0 )
      {
	if( Fread( &node, Size_File_Node, fnet ) != Size_File_Node )
	    PrematureEof();

	UnpackBytes( node.slen, tmp, NB_STRLEN );
	if( Fread( nname, tmp, fnet ) != tmp )
	    PrematureEof();

	n = GetNode( nname );
	UnpackBytes( node.vhigh, tmp, NB_VTHRESH );
	n->vhigh = tmp * (1/VTH_SCALE);
	UnpackBytes( node.vlow, tmp, NB_VTHRESH );
	n->vlow = tmp * (1/VTH_SCALE);
	UnpackBytes( node.tphl, n->tphl, NB_TP );
	UnpackBytes( node.tplh, n->tplh, NB_TP );
	UnpackBytes( node.flags, tmp, NB_FLAGS );
	n->nflags = tmp;

	if( tmp & ALIAS )
	  {
	    int   major, minor;

	    UnpackBytes( node.n.alias.major, major, NB_MAJOR );
	    UnpackBytes( node.n.alias.minor, minor, NB_MINOR );
	    n->ngate = (lptr) major;
	    n->nterm = (lptr) minor;
	    n->t.cause = aliases;
	    aliases = n;
	  }
	else
	  {
	    UnpackBytes( node.n.cap, tmp, NB_NCAP );
	    n->ncap = tmp * (1/CAP_SCALE);
	    if( n->ncap < MIN_CAP )
		n->ncap = MIN_CAP;
	  }

	n->n.next = ndlist;
	ndlist = n;
      }
    VDD_node->nflags |= INPUT;		/* these bits get thrashed above */
    GND_node->nflags |= INPUT;

    while( aliases != NULL )
      {
	n = aliases;
	aliases = n->t.cause;
	n->nlink = Index2node( (int) n->ngate, (int) n->nterm );
	n->ngate = n->nterm = NULL;
	n->t.cause = NULL;
	nnodes--;
	naliases++;
      }
  }


private void rd_txtors()
  {
    File_Trans  trans;
    int         major, minor;
    long        tmp;
    tptr        t, *last;
    int         tsize;

    tsize = txt_coords ? Size_File_Trans : Size_File_Trans - (2 * NB_COORD);
    tlist = NULL;
    last = &tlist;
    while( Fread( &trans, tsize, fnet ) == tsize )
      {
	NEW_TRANS( t );

	UnpackBytes( trans.gate.major, major, NB_MAJOR );
	UnpackBytes( trans.gate.minor, minor, NB_MINOR );
	t->gate = Index2node( major, minor );

	UnpackBytes( trans.src.major, major, NB_MAJOR );
	UnpackBytes( trans.src.minor, minor, NB_MINOR );
	t->source = Index2node( major, minor );

	UnpackBytes( trans.drn.major, major, NB_MAJOR );
	UnpackBytes( trans.drn.minor, minor, NB_MINOR );
	t->drain = Index2node( major, minor );

	UnpackBytes( trans.staticr, tmp, NB_RESIST );
	t->r.rstatic = tmp * (1/RES_SCALE);
	UnpackBytes( trans.rdynhigh, tmp, NB_RESIST );
	t->r.dynhigh = tmp * (1/RES_SCALE);
	UnpackBytes( trans.rdynlow, tmp, NB_RESIST );
	t->r.dynlow = tmp * (1/RES_SCALE);
	UnpackBytes( trans.ttype, t->ttype, NB_TTYPE );

	ntrans[ BASETYPE( t->ttype ) ] += 1;

	if( txt_coords )
	  {
	    UnpackBytes( trans.x, t->x, NB_COORD );
	    UnpackBytes( trans.y, t->y, NB_COORD );
	    EnterPos( t );
	  }

	if( (t->ttype & (ORED | TCAP)) == (ORED | TCAP) )
	  {
	    t->scache.t = ored_list;
	    ored_list = t;
	    t->dcache.t = NULL;
	  }
	else
	  {
	    *last = t;
	    last = &(t->scache.t);
	  }
      }
    *last = NULL;
  }


public int rd_netfile( f, line )
  FILE  *f;
  char  *line;
  {
    int   hash_size, n_nodes, i, mask;
    char  *s;

    if( strncmp( line, netHeader, HEADER_LEN ) != 0 )
	return( FALSE );

    for( mask = i = 0, s = line + HEADER_LEN; s[i] != '\0' ; i++ )
      {
	switch( s[i] )
	  {
	    case '1' :
		mask |= (1 << i);
		break;
	    case '\n':
	    case '0' :
		break;
	    default :
		fprintf( stderr, "bad inet file format\n" );
		exit( 1 );
	  }
      }

    were_stacked = ( mask & STACK_INDEX ) ? TRUE : FALSE;
    txt_coords = ( mask & COORD_INDEX ) ? TRUE : FALSE;
		
    if( fgetline( line, 200, f ) == NULL )
	PrematureEof();

    if( sscanf( line, "%d %d", &hash_size, &n_nodes ) != 2 )
      {
	fprintf( stderr, "bad format for net file\n" );
	exit( 1 );
      }
    if( hash_size != GetHashSize() )
      {
	fprintf( stderr, "Incompatible net file version\n" );
	exit( 1 );
      }
    ReadParallel( f );

    fnet = f;
    rd_nodes( line, n_nodes );
    rd_txtors();
    return( TRUE );
  }


/*
 * Traverse the transistor list and add the node connection-list.  We have
 * to be careful with ALIASed nodes.  Note that transistors with source/drain
 * connected to VDD and GND nodes are not linked.
 */
public void bin_connect_txtors()
  {
    register tptr  t, tnext;

    for( t = tlist; t != NULL; t = tnext )
      {
	tnext = t->scache.t;
	t->state = ( t->ttype & ALWAYSON ) ? WEAK : UNKNOWN;
	t->tflags = 0;

	if( t->ttype & TCAP )
	  {
	    LINK_TCAP( t );
	  }
	else if( t->ttype & STACKED )
	  {
	    tnext = ( stack_txtors ) ? AddStack( t ) : UndoStack( t );
	  }
	else
	  {
	    if( (t->ttype & ALWAYSON) == 0 )	/* do not connect ALWAYSON */
	      {					/* since they do not matter */
		CONNECT( t->gate->ngate, t );
	      }
	    if( not (t->source->nflags & POWER_RAIL) )
	      {
		CONNECT( t->source->nterm, t );
	      }
	    if( not (t->drain->nflags & POWER_RAIL) )
	      {
		CONNECT( t->drain->nterm, t );
	      }
	  }
      }

    if( stack_txtors and not were_stacked )
      {
	register nptr  n;

	for( n = ndlist; n != NULL; n = n->n.next )
	    n->nflags |= VISITED;

	make_stacks( ndlist );
      }

    AssignOredTrans();
  }
