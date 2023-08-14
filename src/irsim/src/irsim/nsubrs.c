#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"

/* useful subroutines for dealing with network */

public	char  *ttype[NTTYPES] = 
  {
    "n-channel",
    "p-channel",
    "depletion",
    "pullup",
    "resistor"
  };

#define	HASHSIZE		4387

private	nptr  hash[HASHSIZE];


public int GetHashSize()
  {
    return( HASHSIZE );
  }


/* hashing function used in interning symbols */
private	char  lcase[128] = 
  {
    0,		01,	02,	03,	04,	05,	06,	07,
    010,	011,	012,	013,	014,	015,	016,	017,
    020,	021,	022,	023,	024,	025,	026,	027,
    030,	031,	032,	033,	034,	035,	036,	037,
    ' ',	'!',	'"',	'#',	'$',	'%',	'&',	047,
    '(',	')',	'*',	'+',	',',	'-',	'.',	'/',
    '0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
    '8',	'9',	':',	';',	'<',	'=',	'>',	'?',
    '@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
    'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
    'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
    'x',	'y',	'z',	'[',	0134,	']',	'^',	'_',
    '`',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
    'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
    'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
    'x',	'y',	'z',	'{',	'|',	'}',	'~',	0177
  };



private int sym_hash( name )
  register char  *name;
  {
    register int  hashcode = 0;

    do
	hashcode = (hashcode << 1) ^ (*name | 0x20);
    while( *(++name) );
    return( ((hashcode >= 0) ? hashcode : ~hashcode) % HASHSIZE );
  }


/*
 * Compare 2 strings, case doesn't matter.  Return value is an integer greater
 * than, equal to, or less than 0, according as 's1' is lexicographically
 * greater than, equal to, or less than 's2'.
 */
public int str_eql( s1, s2 )
  register char  *s1, *s2;
  {
    register int  cmp;

    while( *s1 )
      {
	if( (cmp = lcase[*s1++] - lcase[*s2++]) != 0 )
	    return( cmp );
      }
    return( 0 - *s2 );
  }


/* compare pattern with string, case doesn't matter.  "*" wildcard accepted */
public int str_match( p, s )
  register char  *p, *s;
  {
    while( 1 )
      {
	if( *p == '*' )
	  {
	    /* skip past multiple wildcards */
	    do
		p++;
	    while( *p == '*' );

	    /* if pattern ends with wild card, automatic match */
	    if( *p == 0 )
		return( 1 );

	    /* *p now points to first non-wildcard character, find matching
	     * character in string, then recursively match remaining pattern.
	     * if recursive match fails, assume current '*' matches more...
	     */
	    while( *s != 0 )
	      {
		while( lcase[*s] != lcase[*p] )
		    if( *s++ == 0 )
			return( 0 );
		if( str_match( p + 1, ++s ) )
		    return( 1 );
	      }

	    /* couldn't find matching character after '*', no match */
	    return( 0 );
	  }
	else if( *p == 0 )
	    return( *s == 0 );
	else if( lcase[*p++] != lcase[*s++] )
	    return( 0 );
      }
  }


/* find node in network */
public nptr find( name )
  register char  *name;
  {
    register nptr  ntemp;
    register int   cmp = 1;

    if( txt_coords and name[0] == '@' and name[1] == '=' )
	if( (ntemp = FindNode_TxtorPos( name )) != NULL )
	    return( ntemp );

    for( ntemp = hash[sym_hash( name )]; ntemp != NULL; ntemp = ntemp->hnext )
      {
	if( (cmp = str_eql( name, ntemp->nname )) >= 0 )
	    break;
      }
    if( cmp == 0 )
      {
	while( ntemp->nflags & ALIAS )
	    ntemp = ntemp->nlink;
	return( ntemp );
      }
    return( NULL );
  }


/*
 * Get node structure.  If not found, create a new one.
 */
public nptr GetNode( name )
  register char  *name;
  {
    register nptr  n, *prev;
    register int   i;

    prev = &hash[ sym_hash( name ) ];
    for( i = 1, n = *prev; n != NULL; n = *(prev = &n->hnext) )
	if( (i = str_eql( name, n->nname )) >= 0 )
	    break;

    if( i == 0 )
      {
	while( n->nflags & ALIAS )
	    n = n->nlink;
	return( n );
      }

	/* allocate new node from free storage */
    if( (n = (nptr) freeNodes) == NULL )
      {
	if( (freeNodes = MallocList( sizeof( struct Node ) )) == NULL )
	  {
	    lprintf( stderr, "Out of memory, can not create new node\n" );
	    exit( 1 );
	  }

	n = (nptr) freeNodes;
      }
    freeNodes = freeNodes->next;

    nnodes++;

    n->hnext = *prev;		/* insert node into hash table, after prev */
    *prev = n;

	/* initialize node entries */
    n->ngate = n->nterm = NULL;
    n->nflags = 0;
    n->ncap = MIN_CAP;
    n->vlow = LOWTHRESH;
    n->vhigh = HIGHTHRESH;
    n->c.time = 0;
    n->tplh = 0;
    n->tphl = 0;
    n->t.cause = NULL;
    n->nlink = NULL;
    n->events = NULL;
    n->npot = X;

    n->head.next = last_hist;
    n->head.time = 0;
    n->head.val = X;
    n->head.inp = 0;
    n->head.punt = 0;
    n->head.t.r.rtime = n->head.t.r.delay = 0;
    n->curr = &(n->head);

	/* store all node-names as strings */
    i = strlen( name ) + 1;
    if( (n->nname = Valloc( i )) == NULL )
      {
	lprintf( stderr, "Out of memory, can not store node name\n" );
	exit( 1 );
      }

    bcopy( name, n->nname, i );

    return( n );
  }


/* insert node into hash table.  Keep table entry sorted in ascending order */
public void n_insert( nd )
  register nptr  nd;
  {
    register nptr  n, *prev;
    register char  *name;

    name = pnode( nd );
    prev = &hash[ sym_hash( name ) ];
    for( n = *prev; n != NULL; n = *(prev = &n->hnext) )
	if( str_eql( name, n->nname ) >= 0 )
	    break;
    nd->hnext = *prev;
    *prev = nd;
  }


/* delete node from hash table */
public void n_delete( nd )
  register nptr  nd;
  {
    register nptr  n, *prev;

    prev = &hash[ sym_hash( pnode( nd ) ) ];
    for( n = *prev; n != NULL ; n = *(prev = &n->hnext) )
      {
	if( n == nd )
	  {
	    *prev = n->hnext;
	    return;
	  }
      }
  }


/* visit each node in network, calling function passed as arg with current node */
public int walk_net( fun )
  int(*fun)();
  {
    register int   index;
    register nptr  n;
    int            total = 0;

    for( index = 0; index < HASHSIZE; index++ )
	for( n = hash[index]; n; n = n->hnext )
	  if( (n->nflags & MERGED) == 0 )
	    total += (*fun)(n);
    return( total );
  }


/*
 * visit each node in network, calling function passed.  Arguments are current
 * node, index into hash table, and number within hash entry.  If all_nodes is
 * TRUE then even MERGED nodes are passed to the function.
 * If 'fun' returns FALSE then terminate prematurely.
 */
public void walk_net_index( fun, all_nodes )
  int  (*fun)();
  int  all_nodes;
  {
    register int   index;
    register nptr  n;
    int            minor;

    if( all_nodes == TRUE )		/* build mask */
	all_nodes = 0;
    else
	all_nodes = MERGED;

    for( index = 0; index < HASHSIZE; index++ )
	for( n = hash[index], minor = 0; n; n = n->hnext, minor++ )
	    if( (n->nflags & all_nodes) == 0 )
		if( (*fun)( n, index, minor ) == FALSE )
		    return;
  }


/*
 * Return a list of all nodes in the network.
 */
public nptr GetNodeList()
  {
    register int   index;
    register nptr  n, *last;
    struct Node    head;

    last = &(head.n.next);
    for( index = 0; index < HASHSIZE; index++ )
      {
	for( n = hash[index]; n != NULL; n = n->hnext )
	  {
	    *last = n;
	    last = &(n->n.next);
	  }
      }
    *last = NULL;
    return( head.n.next );
  }


/*
 * Return the corresponding node pointer to the major & minor index numbers.
 */
public nptr Index2node( major, minor )
  int  major, minor;
  {
    register nptr  n;

    if( major >= HASHSIZE )
	return( NULL );
    for( n = hash[ major ]; n != NULL and minor != 0; n = n->hnext, minor-- );
    return( n );
  }


/*
 * Set the major & minor index numbers of the corresponding node.
 */
public void Node2index( nd, major, minor )
  nptr  nd;
  int   *major, *minor;
  {
    register nptr  n;
    register int   i;

    if( nd != NULL )
      {
	*major = i = sym_hash( pnode( nd ) );
	for( n = hash[ i ], i = 0; n != NULL; n = n->hnext, i++ )
	  {
	    if( n == nd )
	      {
		*minor = i;
		return;
	      }
	  }
      }
    *major = HASHSIZE;
    *minor = 0;
  }


/* visit each node in network, calling function passed as arg with any node
 * whose name matches pattern
 */
public int match_net( pattern, fun, arg )
  char  *pattern;
  int   (*fun)();
  int   arg;
  {
    register int   index;
    register nptr  n;
    int            total = 0;

    for( index = 0; index < HASHSIZE; index++ )
	for( n = hash[index]; n; n = n->hnext )
	    if( str_match( pattern, pnode( n ) ) )
		total += (*fun)(n, arg);
    return( total );
  }


/* see if asciz string is an integer -- if so, return it; otherwise return 0 */
public int numberp( s )
  register char  *s;
  {
    register unsigned  num = 0;
    register unsigned  temp;

    if( *s == '0' )
	return( 0 );	/* if it starts with zero, forget it */
    while( *s )
      {
	if( *s >= '0' and *s <= '9' )
	  {
	    temp = num * 10 + *s - '0';
	    if( temp < num )
		return( 0 );
	    else
		num = temp;
	  }
	else
	    return( 0 );
	s++;
      }
    return( num );
  }


/* return pointer to asciz name of node */

public
#define	pnode( NODE )	( (NODE)->nname )



/* initialize hash table */
public void init_hash()
  {
    register int  i;

    for( i = 0; i < HASHSIZE; i += 1 )
	hash[i] = NULL;
  }


/*
 * Remove all nodes aliased to node "nd".  Free their space and remove them
 * from the hash table.
 */
public void rm_aliases( nd )
  register nptr  nd;
  {
    register int   index;
    register nptr  n, *list;

    for( index = 0; index < HASHSIZE; index++ )
      {
	list = &(hash[index]);
	while( (n = *list) != NULL )
	  {
	    if( n->nlink == nd )
	      {
		if( n->nflags & ALIASED )	/* an aliased alias: */
		    rm_aliases( n );		/* delete that as well */
		naliases--;
		*list = n->hnext;
		Vfree( n->nname );
		Ffree( n, sizeof( struct Node ) );
	      }
	    else
		list = &(n->hnext);
	  }
      }
  }
