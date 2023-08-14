#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"

/* front end for mos simulator -- Chris Terman (6/84) */
/* sunbstantial changes: Arturo Salz (88) */

#define	LSIZE		2000	/* max size of command line (in chars) */
#define	MAXARGS		100	/* maximum number of command-line arguments */
#define	MAXCOL		80	/* maximum width of print line */
#define MAXITEMS	10	/* maximum number of items in search path */


#define	ITERATOR_START		'{'
#define	ITERATOR_END		'}'

private	Bits     *blist = NULL;

typedef struct Command
  {
    struct Command    *next;		/* linked list of commands in bucket */
    char              *name;		/* name of this command */
    int               (*handler)();	/* handler for this command */
  } Command;

private	Command    *cmdtbl[128];	/* 1 bucket for every 1st character */

typedef struct sequence *sptr;

struct sequence
  {
    sptr    next;			/* next vector in linked list */
    int     which;			/* 0 => node; 1 => vector */
    union
      {
	nptr  n;
	bptr  b;
      } ptr;				/* pointer to node/vector */
    int     vsize;			/* size of each value */
    int     nvalues;			/* number of values specified */
    char    values[1];			/* array of values */
  } *slist = NULL;


private	int      maxsequence = 0;	/* longest sequence we've seen */
private	int      maxclock = 0;		/* longest clock sequence we've seen */

private	sptr     clock = NULL;		/* vectors which make up clock */

public	iptr     wlist = NULL;		/* list of nodes to be displayed */
private	iptr     wvlist = NULL;		/* list of vectors to be displayed */

public	char     *filename;		/* current input file */
private	int      column = 0;		/* current output column */

					
private	char     *cmdpath[ MAXITEMS ];	/* search path for cmd files */
private	int      numitems = 0;		/* # of items in search path */

private	int      lineno = 0;		/* current line number */
private	long     stepsize = 1000;	/* duration of sim-step, in Delta's */
private	int      dcmdfile = 0;		/* display commands read from file */
private	int      ddisplay = 1;		/* if <>0 run "d" at end of step */

private	char     *targv[ MAXARGS ];	/* array of tokens on command line */
private	int      targc;			/* number of args on command line */
private	char     wildCard[MAXARGS];	/* set if corresponding arg has '*' */

public	int      analyzerON = FALSE;	/* set when analyzer is running */
public	long     sim_time0 = 0;		/* starting time (see flush_hist) */
private	nptr     ch_list = NULL;	/* 'changed' nodes after update net */
public	FILE     *logfile = NULL;	/* log file of transactions */


private	int      expand();		/* forward references */
private	void     input(), shift_args();


/* parse input line into tokens, filling up targv and setting targc */
private int parse_line( line, bufsize )
  register char  *line;
  int            bufsize;
  {
    char          *extra;
    register int  i;
    register char ch;
    char          wc;			/* wild card indicator */

	/* extra storage comes out of unused portion of line buffer */
    i = strlen( line ) + 1;
    bufsize -= i;
    extra = &line[i];
    targc = 0;
    while( i = *line++ )
      {
	    /* skip past white space */
	if( i <= ' ' )
	    continue;
	    /* found start of new argument, remember where it begins */
	if( targc >= MAXARGS )
	  {
	    error( filename, lineno, "too many command arguments\n" );
	    break;
	  }
	else
	    targv[targc++] = --line;

	  /* skip past text of argument, terminate with null character.
	   * While scanning argument remember if we see a "{" which marks
	   * the possible beginning of a iteration expression.
	   */

	wc = FALSE;
	i = 0;
	while( (ch = *line) > ' ' )
	  {
	    if( ch == '*' )
		wc = TRUE;
	    else if( ch == ITERATOR_START )
		i = 1;
	    line++;
	  }
	*line++ = '\0';

	  /* if the argument might contain one or more iterators, process
	   * it more carefully...
	   */
	if( targv[0][0] == '|' )
	  {
	    targc = 0;
	    return;
	  }
	if( i == 1 )
	  {
	    if( expand( targv[--targc], &extra, &bufsize, wc ) )
		break;
	  }
	else
	    wildCard[targc - 1] = wc;
      }
  }


/* given a text string, expand any iterators it contains.  For example, the
 * string "out.{1:10}" expands into ten arguments "out.1", ..., "out.10".
 * The string can contain multiple iterators which will be expanded
 * independently, e.g., "out{1:10}{1:20:2}" expands into 100 arguments.
 * Buffer and bufsize describe a byte buffer which can be used for expansion.
 * Return 0 if expansion succeeds, 1 otherwise.
 */
private int expand( string, buffer, bufsize, wc )
  register char  *string;
  char           **buffer;
  int            *bufsize;
  char           wc;
  {
    register char  *p;
    char           prefix[100], index[100];
    int            start, stop, step, length;

	/* copy string until we which beginning of iterator */
    p = prefix;
    length = 0;
    while( *string )
      {
	if( *string == ITERATOR_START )
	  {
	    *p = 0;
	    goto gotit;
	  }
	*p++ = *string++;
      }
    *p = 0;

	/* if we get here, there was no iterator in the string, so save what
	 * we have as another argument.
	 */
    length = strlen( prefix ) + 1;
    if( length > *bufsize )
      {
	error( filename, lineno, "too many command arguments\n" );
	return( 1 );
      }
    strcpy( *buffer, prefix );
    wildCard[targc] = wc;
    targv[targc++] = *buffer;
    *bufsize -= length;
    *buffer += length;
    return( 0 );

	/* gobble down iterator */
  gotit :
    start = 0;
    stop = 0;
    step = 0;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	start = start * 10 + *string - '0';
    if( *string != ':' )
	goto err;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	stop = stop * 10 + *string - '0';
    if( *string == ITERATOR_END )
	goto done;
    if( *string != ':' )
	goto err;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	step = step * 10 + *string - '0';
    if( *string == ITERATOR_END )
	goto done;

  err :
    error( filename, lineno, "syntax error in name iterator" );
    return( 1 );

  done :	/* suffix starts just past '}' which terminates iterator */
    string += 1;

	/* figure out correct step size */
    if( step == 0 )
	step = 1;
    else if( step < 0 )
	step = -step;
    if( start > stop )
	step = -step;

	/* expand the iterator */
    while( (step > 0 and start <= stop) or (step < 0 and start >= stop) )
      {
	sprintf( index, "%s%d%s", prefix, start, string );
	if( expand( index, buffer, bufsize, wc ) )
	    return( 1 );
	start += step;
      }
    return( 0 );
  }


/* apply given function to each argument on the command line.  Arguments are
 * checked first to make sure they are the name of some node in the circuit
 * or the name of a vector; wild-card patterns are allowed as names.  If arg
 * is non-negative it is used as parameter when calling function, otherwise
 * parameter passed to specified function is 1 if node name was preceded by a
 * "-", 0 otherwise.  First argument is called if command line arg is a node
 * name, second arg is called if arg is a vector name.  If second arg is NULL,
 * first arg is called on each node of the vector.
 */
private void apply( fun, vfun, arg )
  int  (*fun)(), (*vfun)();
  {
    register char  *p;
    register bptr  b;
    register int   i, flag, j, found;

    for( i = 1; i < targc; i += 1 )
      {
	p = targv[i];
	if( arg == -1 )
	  {
	    if( *p == '-' )
	      {
		flag = 1;
		p += 1;
	      }
	    else
		flag = 0;
	  }
	else
	    flag = arg;

	found = 0;
	if( wildCard[i] )
	  {
	    for( b = blist; b != NULL; b = b->next )
		if( str_match( p, b->name ) )
		  {
		    if( vfun != NULL )
			(*vfun)(b, flag);
		    else
			for( j = 0; j < b->nbits; j += 1 )
			    (*fun)(b->nodes[j], flag);
		    found = 1;
		  }
	    found += match_net( p, fun, flag );
	  }
	else
	  {
	    nptr  n = find( p );

	    if( n != NULL )
		found += (*fun)( n, flag );
	    else
	      {
		for( b = blist; b != NULL; b = b->next )
		    if( str_eql( p, b->name ) == 0 )
		      {
			if( vfun != NULL )
			    (*vfun)(b, flag);
			else
			    for( j = 0; j < b->nbits; j += 1 )
				(*fun)(b->nodes[j], flag);
			found = 1;
			break;
		      }
	      }
	  }
	if( found == 0 )
	    error( filename, lineno, "cannot find node or vector %s\n", p );
      }
  }


/* open an input stream, and process commands from it.  Returns 0 if
 * file couldn't be opened, 1 otherwise.
 */
private int finput( name )
  char  *name;
  {
    FILE  *next;
    int   olineno;
    char  *ofname;
    char  longname[256];
    int   i;

    for( next = NULL, i = 0; next == NULL and i < numitems; i++ )
      {
	if( *name == '/' )				/* absolute path */
	    strcpy( longname, name );
	else
	    sprintf( longname, "%s/%s", cmdpath[i], name );
	next = fopen( longname, "r" );
      }
    if( next == NULL )
	return( 0 );

    ofname = filename;
    olineno = lineno;
    filename = longname;
    lineno = 0;
    input( next );
    fclose( next );
    filename = ofname;
    lineno = olineno;
    return( 1 );
  }


private int exec_cmd()
  {
    register Command  *cmd;
    char               cmdfile[100];

	/* search command table, dispatch to handler, if any */
    for( cmd = cmdtbl[targv[0][0]]; cmd != NULL; cmd = cmd->next )
      {
	if( strcmp( targv[0], cmd->name ) == 0 )
	    return( (*cmd->handler)() );
      }

	/* no built-in command found, try for a command file */
    if( targc == 1 )
      {
	strcpy( cmdfile, targv[0] );
	strcat( cmdfile, ".cmd" );
	if( not finput( cmdfile ) )
	    error( filename, lineno, "unrecognized command: %s\n", targv[0] );
      }
    else
	error( filename, lineno, "unrecognized command: %s\n", targv[0] );
    return( 0 );
  }


/* process commands read from input stream */
private void input( in )
  FILE  *in;
  {
    char  line[LSIZE];char *a;

    while( 1 )
      {
	  /* output prompt and flush output */
	if( in == stdin )
	    fputs( "irsim> ", stdout );
	fflush( stdout );

	    /* read command, convert into tokens */
	if( fgetline( line, LSIZE, in ) == NULL )
	  {
	    if( (in == stdin) and isatty( fileno( stdin ) )  )
	      {
		if( freopen( "/dev/tty", "r", stdin ) )
		  {
		    fputc( '\n', stdout );
		    continue;
		  }
		else
		    return;
	      }
	    else
		return;
	  }

	if( in == stdin )
	  {
	    if( logfile != NULL )
		fputs( line, logfile );
	  }
	else if( dcmdfile )
	    fprintf( stdout, "%s> %s", filename, line );

	lineno += 1;
	parse_line( line, LSIZE );
	if( targc != 0 )
	    if( exec_cmd() )
		return;

	if( int_received )
	  {
	    if( in == stdin )
		int_received = 0;
	    else
	      {
		fprintf( stderr, "interrupt <%s @ %d>...", filename, lineno );
		return;
	      }
	    fputc( '\n', stderr );
	  }
      }
  }


/* read and process a command file */
private int cmdfile()
  {
    if( targc != 2 )
	error( filename, lineno, "wrong number of args to '@' command\n" );
    else if( not finput( targv[1] ) )
	error( filename, lineno, "cannot open %s for input\n", targv[1] );

    return( 0 );
  }


private void InitCmdPath()
  {
    cmdpath[ 0 ] = Valloc( 2 );
    strcpy( cmdpath[ 0 ], "." );
    numitems = 1;
  }


/* set the search path for command files */
private int docmdpath()
  {
    int  i;

    if( targc == 1 )
      {
    	/* echo current cmdpath */
	for( i = 0; i < numitems; i++ )
	  {
	    lprintf( stdout, "%s ", cmdpath[i] );
	  }
	lprintf( stdout, "\n" );
      }
    else
      {
	/* set new cmdpath */
	if( (numitems = targc - 1) > MAXITEMS )
	  {
	    lprintf( stdout, "Only %d path entries available\n", MAXITEMS );
	    numitems = MAXITEMS;
	  }
	for( i = 0; i < numitems; i++ )
	  {
	    if( cmdpath[i] != NULL )
		Vfree( cmdpath[i] );
	    cmdpath[i] = Valloc( strlen( targv[i + 1] ) );
	    strcpy( cmdpath[i], targv[i + 1] );
	  }
      }
    return( 0 );
  }



/* set value of an input, or let node go altogether */
private int setvalue()
  {
    apply( setin, NULL, targv[0][0] );
    return( 0 );
  }


/* add/delete node to/from display list */
private int xwatch( n, flag )
  nptr  n;
  {
    if( n->nflags & MERGED )
	return( 1 );
    idelete( n, &wlist );
    if( not flag )
	iinsert( n, &wlist );
    return( 1 );
  }


/* add/delete vector to/from display list */
private int vwatch( b, flag )
  bptr  b;
  {
    idelete( (nptr) b, &wvlist );
    if( not flag )
	iinsert( (nptr) b, &wvlist );
  }


/* manipulate display list */
private int display()
  {
    apply( xwatch, vwatch, -1 );
    return( 0 );
  }


/* display bit vector. */
private int dvec( b )
  register bptr  b;
  {
    register int  i;

    i = strlen( b->name ) + 2 + b->nbits;
    if( column + i >= MAXCOL )
      {
	lprintf( stdout, "\n" );
	column = 0;
      }
    column += i;
    lprintf( stdout, "%s=", b->name );

    for( i = 0; i < b->nbits; i += 1 )
	lprintf( stdout, "%c", "0XX1"[b->nodes[i]->npot] );

    lprintf( stdout, " " );
  }


/* print value of specific node */
private void dnode( n )
  register nptr  n;
  {
    register char  *name = pnode( n );
    register int   i;

    i = strlen( name ) + 3;
    if( column + i >= MAXCOL )
      {
	lprintf( stdout, "\n" );
	column = 0;
      }
    column += i;

    while( n->nflags & ALIAS )
	n = n->nlink;
    if( n->nflags & MERGED )
	lprintf( stdout, "%s => node is inside a transistor stack\n", name );
    else
	lprintf( stdout, "%s=%c ", name, "0XX1"[n->npot] );
  }


/* display node values, either those specified or from display list */
private int pnlist()
  {
    column = 0;
    if( targc == 1 )
      {
	register iptr  w;

	    /* print value of each watched bit vector. */
	for( w = wvlist; w != NULL; w = w->next )
	    dvec( (bptr) w->inode );

	    /* print value of each watched node making sure we fit on page */
	for( w = wlist; w != NULL; w = w->next )
	    dnode( (nptr) w->inode );
      }
    else
      {
	register int   i;
	register bptr  b;

	    /* first print any bit vectors the user has specified */
	for( i = 1; i < targc; i += 1 )
	  {
	    if( wildCard[i] )
		for( b = blist; b != NULL; b = b->next )
		  {
		    if( str_match( targv[i], b->name ) )
			dvec( b );
		  }
	    else
		for( b = blist; b != NULL; b = b->next )
		  {
		    if( str_eql( targv[i], b->name ) == 0 )
		      {
			dvec( b );
			break;
		      }
		  }
	  }

	    /* then print individual nodes */
	for( i = 1; i < targc; i += 1 )
	    if( wildCard[i] )
		match_net( targv[i], dnode, 0 );
	    else
	      {
		nptr n = find( targv[i] );
		if( n )
		    dnode( n );
	      }
      }

	/* some concluding info indicating current simulated time and the
	 * state of the event list.
	 */
    if( column != 0 )
	lprintf( stdout, "\n" );
    lprintf( stdout, "time = %2.1fns", d2ns( cur_delta ) );
    if( npending )
	lprintf( stdout, "; there are pending events (%d)",npending );
    lprintf( stdout, "\n" );
    return( 0 );
  }


/* set/clear trace bit in node */
private int xtrace( n, flag )
  nptr  n;
  {
    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & MERGED )
      {
	lprintf( stdout, "can't trace %s\n", pnode( n ) );
	return( 1 );
      }

    if( flag )
      {
	if( n->nflags & WATCHED )
	    lprintf( stdout, "%s was watched; not any more\n", pnode( n ) );
	n->nflags &= ~WATCHED;
      }
    else
	n->nflags |= WATCHED;
    return( 1 );
  }


/* set/clear trace bit in vector */
private int vtrace( b, flag )
  register bptr  b;
  {
    register int   i;
    register nptr  n;

    for( i = 0; i < b->nbits; i += 1 )
      {
	n = b->nodes[i];
	if( flag )
	    n->nflags &= ~STOPONCHANGE;
	else
	    n->nflags |= STOPONCHANGE;
      }
    b->traced = not flag;

	/* just in case node appears in more than one bit vector, run
	 * through all the vectors currently being traced and make sure
	 * the STOPONCHANGE flag is set for each node.
	 */
    if( flag )
	for( b = blist; b != NULL; b = b->next )
	    if( b->traced )
		for( i = 0; i < b->nbits; i += 1 )
		  {
		    n = b->nodes[i];
		    n->nflags |= STOPONCHANGE;
		  }
    return( 0 );
  }


/* add/remove nodes from trace */
private int settrace()
  {
    apply( xtrace, vtrace, -1 );
    return( 0 );
  }


/* define bit vector */
private int dovector()
  {
    register nptr  n;
    register bptr  b, last;
    register int   i;

    if( targc < 3 )
      {
	error( filename, lineno, "wrong number of args to 'vector' command\n");
	return( 0 );
      }

    if( find( targv[1] ) != NULL )
      {
	error( filename, lineno, "'%s' is a node, can't be a vector\n",
	  targv[1] );
	return( 0 );
      }

	/* get rid of any vector with the same name */
    for( b = blist, last = NULL; b != NULL; last = b, b = b->next )
	if( strcmp( b->name, targv[1] ) == 0 )
	  {
	    if( last == NULL )
		blist = b->next;
	    else
		last->next = b->next;		/* remove from display list */
	    idelete( (nptr) b, &wvlist );	/* untrace its nodes */
	    vtrace( b, 1 );
	    if( analyzerON )
		RemoveVector( b );
	    Vfree( b->name );
	    Vfree( b );
	    break;
	  }
    b = (bptr) Valloc( sizeof( Bits ) + (targc - 3) * sizeof( nptr ) );
    if( b == NULL )
	return( 0 );
    b->name = Valloc( strlen( targv[1] ) + 1 );
    if( b->name == NULL )
      {
	Vfree( b );
	return( 0 );
      }
    b->traced = 0;
    b->nbits = 0;
    strcpy( b->name, targv[1] );

    for( i = 2; i < targc; i += 1 )
      {
	if( (n = find( targv[i] )) == NULL )
	    error( filename, lineno, "cannot find node %s\n", targv[i] );
	else if( n->nflags & MERGED )
	    error( filename, lineno, "%s can not be part of a vector\n",
	      pnode( n ) );
	else
	    b->nodes[b->nbits++] = n;
      }

    if( b->nbits == targc - 2 )
      {
	b->next = blist;
	blist = b;
      }
    else
      {
	Vfree( b->name );
	Vfree( b );
      }

    return( 0 );
  }


/* set bit vector */
private int setvector()
  {
    register nptr  n;
    register bptr  b;
    register int   i;

    if( targc != 3 )
      {
	error( filename, lineno, "wrong number of args to 'set' command\n" );
	return( 0 );
      }

	/* find vector */
    for( b = blist; b != NULL; b = b->next )
      {
	if( strcmp( b->name, targv[1] ) == 0 )
	    break;
      }
    if( b == NULL )
      {
	error( filename, lineno, "undefined vector name in 'set' command\n" );
	return( 0 );
      }

	/* set nodes */
    if( strlen( targv[2] ) != b->nbits )
      {
	error( filename, lineno, "wrong number of bits for this vector\n" );
	return( 0 );
      }
    for( i = 0; i < b->nbits; i++ )
      {
	n = b->nodes[i];
	switch( targv[2][i] )
	  {
	    case '1' :
		setin( n, 'h' );
		break;
	    case '0' :
		setin( n, 'l' );
		break;
	    case 'x' :
	    case 'X' :
		setin( n, 'x' );
	    case 'u' :
	    case 'U' :
		setin( n, 'u' );
		break;
	    default :
		error( filename, lineno, "bit value must be 0,1,x or u\n" );
		return( 0 );
	  }
      }

    return( 0 );
  }


private nptr  one_node;
private bptr  one_vec;
private int   one_or_more;

private void SetNode( nd, flag )
  nptr  nd;
  int   flag;
  {
    one_node = nd;
    one_or_more++;
  }

private void SetVector( bp, flag )
  bptr  bp;
  int   flag;
  {
    one_vec = bp;
    one_or_more++;
  }


private void CompareVector( np, name, nbits, mask, value )
  nptr  *np;
  char  *name;
  int   nbits;
  char  *mask;
  char  *value;
  {
    int   i;
    nptr  n;

    if( strlen( value ) != nbits )
      {
	error( filename, lineno, "wrong number of bits for value\n" );
	return;
      }
    if( mask != NULL and strlen( mask ) != nbits )
      {
	error( filename, lineno, "wrong number of bits for mask\n" );
	return;
      }

    for( i = 0; i < nbits; i++ )
      {
	if( mask != NULL and mask[i] != '0' )
	    continue;
	n = np[i];
	switch( value[i] )
	  {
	    case '1' :
		if( n->npot != HIGH )
		    goto fail;
		break;
	    case '0' :
		if( n->npot != LOW )
		    goto fail;
		break;
	    case 'x' :
	    case 'X' :
		if( n->npot != X )
		    goto fail;
	    default :
		error( filename, lineno, "bit value must be [0,1,X]\n" );
		return;
	  }
      }
    return;

  fail :
    lprintf( stdout, "(%s, %d): assertion failed on '%s' ",
     filename, lineno, name );
    for( i = 0; i < nbits; i++ )
      {
	if( mask != NULL and mask[i] != '0' )
	  {
	    lprintf( stdout, "-" );
	    value[i] = '-';
	  }
	else
	    lprintf( stdout, "%c", "0XX1"[np[i]->npot] );
      }
    lprintf( stdout, " (%s)\n", value );
  }


/* assert a bit vector */
private int doAssert()
  {
    char *mask;
    char *value;

    if( targc < 3 or targc > 4 )
      {
	error( filename, lineno, "bad number of args to 'assert' command\n" );
	return( 0 );
      }

    if( targc == 4 )
      {
	mask = targv[2];
	value = targv[3];
      }
    else
      {
	mask = NULL;
	value = targv[2];
      }

	/* find vector or node */
    targc = 2;
    one_or_more = 0;
    one_node = NULL;
    one_vec = NULL;
    apply( SetNode, SetVector, 0 );

    if( one_or_more > 1 )
	error( filename, lineno, "assert: only one signal/vector allowed\n" );
    else if( one_node != NULL )
	CompareVector( &one_node, pnode( one_node ), 1, mask, value );
    else if( one_vec != NULL )
	CompareVector( one_vec->nodes, one_vec->name, one_vec->nbits, mask, value );

    return( 0 );
  }


public void rm_from_vectors( node )
  register nptr  node;
  {
    register bptr  b, *list;
    register int   i;

    list = &blist;
    while( (b = *list) != NULL )
      {
	for( i = b->nbits - 1; i >= 0; i-- )
	  {
	    if( b->nodes[i] == node )
	      {
		b->nbits = 0;
		break;
	      }
	  }
	if( b->nbits == 0 )
	  {
	    *list = b->next;
	    idelete( (nptr) b, &wvlist );	/* remove from trace list */
	    if( analyzerON )
		RemoveVector( b );
	    Vfree( b->name );
	    Vfree( b );
	  }
	else
	    list = &(b->next);
      }
  }


/* display current inputs */
private int inputs()
  {
    register iptr  list;

    lprintf( stdout, "h inputs: " );
    for( list = hinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( list = o_hinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    lprintf( stdout, "\nl inputs: " );
    for( list = linputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( list = o_linputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    lprintf( stdout, "\nu inputs: " );
    for( list = uinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( list = o_uinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    lprintf( stdout, "\n" );
    return( 0 );

  }


/* set stepsize */
private int setstep()
  {
    if( targc == 1 )
	lprintf( stdout, "stepsize = %f\n", d2ns( stepsize ) );
    else if( targc == 2 )
      {
	long  newsize = (long) ns2d( atof( targv[1] ) );

	if( newsize <= 0 )
	    error( filename, lineno, "bad step size: %s\n", targv[1] );
	else
	    stepsize = newsize;
      }
    else
	error( filename,lineno, "wrong number of args to stepsize command\n" );

    return( 0 );
  }


public long GetStepSize()
  {
    return( stepsize );
  }


/* settle network until specified stop time is reached.  Premature returns
 * by the step routine indicate that a node which is part of a traced vector
 * has changed value, so print out the values of all traced vectors.
 */
private void relax( stoptime )
  long  stoptime;
  {
    register nptr  n;
    register bptr  b;
    char           temp[20];

    while( (n = (nptr) step( stoptime )) != NULL )
      {
	sprintf( temp, "time=%2.1fns ", d2ns( cur_delta ) );
	lprintf( stdout, "%s", temp );
	column = strlen( temp );
	for( b = blist; b != NULL; b = b->next )
	    if( b->traced )
		dvec( b );
	lprintf( stdout, "\n" );
      }
  }


/* relax network, optionally set stepsize */
private int dostep()
  {
    long newsize;

    if( targc == 2 )
      {
	newsize = (long) ns2d( atof( targv[1] ) );
	if( newsize <= 0 )
	  {
	    error( filename, lineno, "bad step size: %s\n", targv[1] );
	    return( 0 );
	  }
      }
    else
	newsize = stepsize;

    relax( cur_delta + newsize );
    if( ddisplay )
      {
	targc = 1;
	pnlist();
      }

    return( 0 );
  }


/* display info about a node */
private int quest()
  {
    apply( info, NULL, 0 );
    return( 0 );
  }


private int excl()
  {
    apply( info, NULL, 1 );
    return( 0 );
  }


/* exit this level of command processing */
private int quit()
  {
    return( 1 );
  }


/* return to command level */
private int doexit()
  {
    TerminateAnalyzer();
    if( targc == 1 )
	exit( 0 );
    else
	exit( atoi( targv[1] ) );
  }


/* process command line to yield a sequence structure.  targv[1] is the name
 * of the node/vector for which the sequence is to be define; targv[2] and
 * following are the values.
 */
private void defsequence( list, lmax )
  sptr  *list;
  int   *lmax;
  {
    register sptr  s;
    register bptr  b;
    register sptr  p, t;
    nptr           n = NULL;
    int            which = 0, size = 1;

	/* if no arguments, get rid of all the sequences we have defined */
    if( targc == 1 )
      {
	register sptr this;
	for( this = *list; this != NULL; this = s )
	  {
	    s = this->next;
	    Vfree( this );
	  }
	*list = NULL;
	*lmax = 0;
	return;
      }

	/* see if we can determine if name is for node or vector */
    for( b = blist; b != NULL; b = b->next )
	if( str_eql( b->name, targv[1] ) == 0 )
	  {
	    which = 1;
	    size = b->nbits;
	    goto okay;
	  }

    n = find( targv[1] );
    if( n == NULL )
      {
	error( filename, lineno, "cannot find node or vector %s\n", targv[1] );
	return;
      }
    if( n->nflags & MERGED )
      {
	error( filename, lineno, "%s can't be part of a sequence\n",
	  pnode( n ) );
	return;
      }

  okay :	/* allocate and initialize sequence header */
    if( targc > 2 )
      {
	register int   i;
	register char  *p, *q;

	    /* make sure each value specification is the right length */
	for( i = 2; i < targc; i += 1 )
	    if( strlen( targv[i] ) != size )
	      {
		error( filename, lineno,
		  "value size (%d) is not compatible with size of %s (%d)\n",
		  strlen( targv[i] ), which ? b->name : pnode( n ), size );
		return;
	      }

	s = (sptr) Valloc( sizeof( struct sequence ) + size *(targc - 2)-1 );
	s->which = which;
	if( which )
	    s->ptr.b = b;
	else
	    s->ptr.n = n;
	s->vsize = size;
	s->nvalues = targc - 2;

	  /* process each value specification saving results in sequence */
	for( q = s->values, i = 2; i < targc; i += 1 )
	    for( p = targv[i]; *p != 0; p += 1 )
		switch( *p )
		  {
		    case '0' :
		    case 'l' :
		    case 'L' :
			*q++ = 'l';
			break;
		    case '1' :
		    case 'h' :
		    case 'H' :
			*q++ = 'h';
			break;
		    case 'u' :
		    case 'U' :
			*q++ = 'u';
			break;
		    default :
			error( filename, lineno, "unrecognized value: %c\n",
			  *p );
		    case 'x' :
		    case 'X' :
			*q++ = 'x';
			break;
		  }
      }
    else
	s = NULL;

	/* all done! so insert result onto appropriate list after removing
	 * any old sequences for this node or vector.
	 */
    for( p = NULL, t = *list; t != NULL; p = t, t = t->next )
	if( t->ptr.b == b or t->ptr.n == n )
	  {
	    if( p == NULL )
		*list = t->next;
	    else
		p->next = t->next;
	    Vfree( t );
	    break;
	  }
    if( s != NULL )
      {
	s->next = *list;
	*list = s;
	if( s->nvalues > *lmax )
	    *lmax = s->nvalues;
      }
  }


/* Remove node from any sequence it may be in */
private int rm_from_list( node, list )
  register nptr  node;
  register sptr  *list;
  {
    register sptr  s;
    register int   max;

    max = 0;
    while( (s = *list) != NULL )
      {
	if( s->which == 0 and s->ptr.n == node )
	  {
	    *list = s->next;
	    Vfree( s );
	  }
	else
	  {
	    if( s->nvalues > max )
		max = s->nvalues;
	    list = &(s->next);
	  }
      }
    return( max );
  }


public void rm_from_seq( node )
  nptr node;
  {
    maxsequence = rm_from_list( node, &slist );
  }


public void rm_from_clock( node )
  nptr node;
  {
    maxclock = rm_from_list( node, &clock );
  }


/* set each node/vector in sequence list to its next value */
private void vecvalue( list, index )
  register sptr  list;
  int            index;
  {
    register int   offset, i;
    register bptr  b;

    for( ; list != NULL; list = list->next )
      {
	offset = list->vsize *(index % list->nvalues);
	if( list->which == 0 )
	    setin( list->ptr.n, list->values[offset] );
	else
	  {
	    b = list->ptr.b;
	    for( i = 0; i < b->nbits; i += 1 )
		setin( b->nodes[i], list->values[offset++] );
	  }
      }
  }


/* setup sequence of values for a node */
private int setseq()
  {
	/* process sequence and add to sequence list */
    defsequence( &slist, &maxsequence );
    return( 0 );
  }


/* define clock sequences(s) */
private int setclock()
  {
	/* process sequence and add to clock list */
    defsequence( &clock, &maxclock );
    return( 0 );
  }


/* Step each clock node through one simulation step */
private void step_phase()
  {
    static int  which_phase = 0;

    vecvalue( clock, which_phase );
    relax( cur_delta + stepsize );
    if( ++which_phase == maxclock )
	which_phase = 0;
  }


/* Do one simulation step */
private int dophase()
  {
    step_phase();
    if( ddisplay )
      {
	targc = 1;
	pnlist();
      }
    return( 0 );
  }


/* clock circuit specified number of times */
private void clockit( n )
  register int  n;
  {
    register int  i;

    if( clock == NULL )
	error( filename, lineno, "no clock nodes defined!\n" );
    else
      {
	  /* run 'em by setting each clock node to successive values of its
	   * associated sequence until all phases have been run.
	   */
	while( n-- > 0 )
	    for( i = 0; i < maxclock; i += 1 )
		step_phase();

	    /* finally display results if requested to do so */
	if( ddisplay )
	  {
	    targc = 1;
	    pnlist();
	  }
      }
  }


/* clock circuit through all the input vectors previously set up */
private int runseq()
  {
    register int  i, n;

	/* calculate how many clock cycles to run */
    if( targc == 2 )
      {
	n = atoi( targv[1] );
	if( n <= 0 )
	    n = 1;
      }
    else
	n = 1;

	/* run 'em by setting each input node to successive values of its
	 * associated sequence.
	 */
    if( slist == NULL )
	error( filename, lineno, "no input vectors defined!\n" );
    else
	while( n-- > 0 )
	    for( i = 0; i < maxsequence; i += 1 )
	      {
		vecvalue( slist, i );
		clockit( 1 );
		if( ddisplay )
		  {
		    targc = 1;
		    pnlist();
		  }
	      }

    return( 0 );
  }


/* process "c" command line */
private int doclock()
  {
    register int  i, n;

	/* calculate how many clock cycles to run */
    if( targc == 2 )
      {
	n = atoi( targv[1] );
	if( n <= 0 )
	    n = 1;
      }
    else
	n = 1;

    clockit( n );		/* do the hard work */
    return( 0 );
  }


/* output message to console/log file */
private int domsg()
  {
    register int  n;

    for( n = 1; n < targc; n += 1 )
	lprintf( stdout, "%s ", targv[n] );
    lprintf( stdout, "\n" );
    return( 0 );
  }


	/* various debug flags */
public
#define	DEBUG_EV		0x01		/* event scheduling */
public
#define	DEBUG_DC		0x02		/* final value computation */
public
#define	DEBUG_TAU		0x04		/* tau/delay computation */
public
#define	DEBUG_TAUP		0x08		/* taup computation */
public
#define	DEBUG_SPK		0x10		/* spike analysis */
public
#define	DEBUG_TW		0x20		/* tree walk */

/* set debugging level */
private int setdbg()
  {
    int          i, t;
    static char  *dbg[] = { "ev", "dc", "tau", "taup", "spk", "tw", NULL };

    if( targc == 1 )
      {
	lprintf( stdout, "Debug: " );
	if( debug == 0 )
	    lprintf( stdout, "OFF" );
	else
	  {
	    for( i = 0; dbg[i] != NULL; i++ )
	      {
		if( debug & (1 << i) )
		    lprintf( stdout, " %s", dbg[i] );
	      }
	  }
	lprintf( stdout, "\n" );
      }
    else if( targc == 2 and strcmp( targv[1], "off" ) == 0 )
      {
	debug = 0;
	lprintf( stdout, "Debug is now OFF\n" );
      }
    else if( targc == 2 and strcmp( targv[1], "all" ) == 0 )
      {
	debug = DEBUG_EV | DEBUG_DC | DEBUG_TAU | DEBUG_TAUP | DEBUG_TW |
		DEBUG_SPK;
      }
    else
      {
	for( t = 1, debug = 0; t < targc; t++ )
	  {
	    for( i = 0; dbg[i] != NULL; i++ )
		if( strcmp( dbg[i], targv[t] ) == 0 )
		  {
		    debug |= (1 << i);
		    break;
		  }
	    if( dbg[i] == NULL )
	      {
		error( filename, lineno, "unknown debug level (%s), %s\n",
		  targv[t], "try: [ev dc tau taup tw spk][off][all]\n" );
		break;
	      }
	  }
	targc = 1;
	setdbg();
      }
    return( 0 );
  }


public
#define	LIN_MODEL	0
public
#define	SWT_MODEL	1

/* set which network model to use */
private int setmodel()
  {
    static char  *m_name[] = { "linear", "switch" };

    if( targc == 1 )
	lprintf( stdout, "model = %s\n", m_name[model] );
    else if( targc != 2 )
	error( filename, lineno, "wrong number of args to 'model' command\n" );
    else
      {
	int  i;
	for( i = 0; i < NMODEL; i++ )
	  {
	    if( strcmp( targv[1], m_name[i] ) == 0 )
	      {
		if( i != model )
		  {
		    model = i;
		    NewModel( model );
		  }
		return( 0 );
	      }
	  }
	error( filename, lineno, "unrecognized model: %s\n", targv[1] );
      }
    return( 0 );
  }


/* set up or finish off logfile */
private int setlog()
  {
    if( logfile != NULL )
      {
	fclose( logfile );
	logfile = NULL;
      }

    if( targc == 2 and (logfile = fopen( targv[1], "w" )) == NULL )
	error( filename, lineno, "cannot open log file %s for output\n",
	  targv[1] );
    return( 0 );
  }


/* save/restore state of network */
private int dostate()
  {
    int  ret;

    if( targc != 2 )
	error( filename, lineno, "wrong number of args to '%s' command",
	  targv[0] );
    else
      {
	if( strcmp( targv[0], "<" ) == 0 )
	    ret = rd_state( targv[1], rd_value );
	else if( strcmp( targv[0], "<<" ) == 0 )
	    ret = rd_state( targv[1], rd_restore );
	else
	  {
	    if( wr_state( targv[1] ) )
		error( filename, lineno, "can not write state file: %s\n",
		  targv[1] );
	    return( 0 );
	  }
	switch( ret )
	  {
	    case 0 :
		    break;
	    case 1 :
		error( filename, lineno, "can not read state file: %s\n",
		  targv[1] );
		break;
	    case 2 :
		error( filename, lineno, "bad node count in state file\n" );
		break;
	    case 3 :
		error( filename, lineno, "premature EOF in state file\n" );
		break;
	  }
      }
    return( 0 );
  }


/* set decay parameter */
private int setdecay()
  {
    if( targc == 1 )
	lprintf( stdout, "decay = %.1f\n", d2ns( tdecay ) );
    else if( targc == 2 )
      {
	tdecay = (int) ns2d( atof( targv[1] ) );
	if( tdecay < 0 )
	    tdecay = 0;
      }
    else
	error( filename, lineno, "wrong number of args to 'decay' command\n" );
    return( 0 );
  }


/* set unitdelay parameter */
private int setunit()
  {
    if( targc == 1 )
	lprintf( stdout, "unitdelay = %.1f\n", d2ns( tunitdelay ) );
    else if( targc == 2 )
      {
	tunitdelay = (int) ns2d( atof( targv[1] ) );
	if( tunitdelay < 0 )
	    tunitdelay = 0;
      }
    else
	error( filename, lineno, "wrong number of args for 'unitdelay'\n" );
    return( 0 );
  }


/* set tdebug parameter */
private int setreport()
  {
    if( targc == 1 )
	lprintf( stdout, "report = %d\n", tdebug );
    else if( targc == 2 )
      {
	tdebug = atoi( targv[1] );
	if( tdebug < 0 )
	    tdebug = 0;
      }
    else
	error( filename, lineno, "wrong number of args for 'report'\n" );
    return( 0 );
  }


/* print traceback of node's activity and that of its ancestors */
private long     ptime;		/* used during trace back */

private int cpath( n, level )
  register nptr  n;
  {
    long  temp; /* storing c.time difference between the present node and
			its cause */

    if( level == 0 )
	lprintf( stdout, "critical path for last transition of %s:\n", pnode( n ) );
    while( n->nflags & ALIAS )
	n = n->nlink;

	/* no last transition! */
    if( (n->nflags & MERGED) or n->t.cause == NULL )
	lprintf( stdout, "  there is no previous transition!\n" );

	/* here if we come across node which has changed more recently than
	 * the time we've reached in the backtrace.  We can't continue the
	 * backtrace in any reasonable fashion, so we stop here.
	 */
    else if( n->t.cause == inc_cause )
      {
	if( level == 0 )
	    lprintf( stdout,
	      " previous transition due to incremental update\n" );
	else
	    lprintf( stdout,
	      " transition of %s due to incremental update\n", pnode( n ) );
      }
    else if( level != 0 and n->c.time > ptime )
	lprintf( stdout, "  transition of %s, which has since changed again\n", pnode( n ) );

	/* here if there seems to be a cause for this node's transition.
	 * If the node appears to have 'caused' its own transition (n->t.cause
	 * == n), that means it was input.  Otherwise continue backtrace...
	 */
    else
      {
	if( n->t.cause != n )
	  {
	    if( n->t.cause->nflags & VISITED )
		lprintf( stdout, "  ... loop in traceback\n" );
	    else
	      {
		n->nflags |= VISITED;
		ptime = n->c.time;
		cpath( n->t.cause, level + 1 );
		n->nflags &= ~VISITED;
		temp = n->c.time - ((n->t.cause)->c.time);
		lprintf( stdout, "  %s -> %c @ %2.1fns    (%2.1fns)\n",
		 pnode( n ), "0XX1"[n->npot], d2ns( n->c.time ),d2ns(temp) );
	      }
	  }
	else
	    lprintf( stdout, "  %s -> %c @ %2.1fns , node was an input\n",
	      pnode( n ), "0XX1"[n->npot], d2ns( n->c.time ) );
      }
    return( 1 );
  }


/* discover and print critical path for node's last transistion */
private int dopath()
  {
    apply( cpath, NULL, 0 );
    return( 0 );
  }


/* helper routine for activity command */
#define	NBUCKETS		20	/* number of buckets in histogram */

private	long     abegin, aend, absize;
private	long     acounts[NBUCKETS];

private int adoit( n )
  register nptr  n;
  {
    if( n->nflags & ALIAS )
	return( 0 );

    if( not(n->nflags & MERGED) and n->c.time >= abegin and n->c.time <= aend )
	acounts[(n->c.time - abegin) / absize] += 1;
    return( 0 );
  }


/* print histogram of circuit activity in specified time interval */
private int doactivity()
  {
    register int  i;
    long          total, atol();

    if( targc == 2 )
      {
	abegin = atol( targv[1] );
	aend = cur_delta;
      }
    else if( targc == 3 )
      {
	abegin = atol( targv[1] );
	aend = atol( targv[2] );
      }
    else
      {
	error( filename, lineno, "wrong number of args for 'activity'\n" );
	return( 0 );
      }

	/* collect histogram info by walking the network */
    for( i = 0; i < NBUCKETS; i += 1 )
	acounts[i] = 0;
    if( (absize = (aend - abegin + 1) / NBUCKETS) <= 0 )
	absize = 1;
    walk_net( adoit );

	/* print out what we found */
    for( total = 0, i = 0; i < NBUCKETS; i += 1 )
	total += acounts[i];
    lprintf( stdout, "Histogram of circuit activity, %2.1fns to %2.1fns (bucket size = %2.1fns)\n",
      d2ns( abegin ), d2ns( aend ), d2ns( absize ) );
    for( i = 0; i < NBUCKETS; i += 1 )
	lprintf( stdout, "  [%ld,%ld]\t(%d)\t%s\n",
	  abegin + (i *absize), abegin + ((i + 1) * absize), acounts[i],
	  &"**************************************************"[50-(50 * acounts[i]) / total] );
    return( 0 );
  }


/* print list of nodes which last changed value in specified time interval */
private int cdoit( n )
  register nptr  n;
  {
    char  *nname, *aname = pnode( n );

    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & MERGED != 0 and n->c.time >= abegin and n->c.time <= aend )
      {
	nname = pnode( n );
	if( nname == aname )
	    lprintf( stdout, "  %s\n", nname );
	else
	    lprintf( stdout, "  %s -> %s\n", aname, nname );
      }
    return( 0 );
  }


private int dochanges()
  {
    long  atol();

    if( targc == 2 )
      {
	abegin = atol( targv[1] );
	aend = cur_delta;
      }
    else if( targc == 3 )
      {
	abegin = atol( targv[1] );
	aend = atol( targv[2] );
      }
    else
      {
	error( filename, lineno, "wrong number of args for 'changes'\n" );
	return( 0 );
      }

    lprintf( stdout, "Nodes with last transition in interval %2.1fns to %2.1fns\n",
      d2ns( abegin ), d2ns( aend ) );
    walk_net( cdoit );
    return( 0 );
  }


/* print list of nodes with undefined (X) value */
private int xdoit( n )
  register nptr  n;
  {
    char  *nname, *aname = pnode( n );

    while( n->nflags & ALIAS )
	n = n->nlink;

    if( not (n->nflags & MERGED) and n->npot == X )
      {
	nname = pnode( n );
	if( nname == aname )
	    lprintf( stdout, "  %s\n", nname );
	else
	    lprintf( stdout, "  %s -> %s\n", aname, nname );
      }
    return( 0 );
  }


private int doprintX()
  {
    lprintf( stdout, "Nodes with undefined potential\n" );
    walk_net( xdoit );
    lprintf( stdout, "No more undef nodes found\n" );
    return( 0 );
  }


/* Helper routine which prints all events on the current list */
private int print_list( list, end_of_list )
  evptr  list, end_of_list;
  {
    if( list == NULL )
	return( 0 );
    while( list != end_of_list )
      {
	lprintf( stdout, "Node %s -> %c @ %2.1fns (%2.1fns)\n",
	  pnode( list->enode ), vchars[ list->eval ], d2ns( list->ntime ),
	  d2ns( list->ntime - cur_delta ) );
	list = list->flink;
      }
    lprintf( stdout, "Node %s -> %c @ %2.1fns (%2.1fns)\n",
      pnode( list->enode ), vchars[ list->eval ], d2ns( list->ntime ),
      d2ns( list->ntime - cur_delta ) );
    return( 0 );
  }


/* Print list of pending events */
private int printPending()
  {
    long   delta = 0;
    evptr  list, end_of_list;

    while( (delta = pending_events( delta, &list, &end_of_list )) != 0 )
        print_list( list, end_of_list );
    print_list( list, end_of_list );
  }


/* set/reset various display parameters */
private int dodisplay()
  {
    register int   i, value;
    register char  *p;

    if( targc == 1 )
      {
	lprintf( stdout, "display = %s %s\n",
	  dcmdfile ? "cmdfile" : "-cmdfile",
	  ddisplay ? "automatic" : "-automatic" );
	return( 0 );
      }

    for( i = 1; i < targc; i += 1 )
      {
	p = targv[i];
	if( *p == '-' )
	  {
	    value = 0;
	    p += 1;
	  }
	else
	    value = 1;

	if( str_eql( p, "cmdfile" ) == 0 )
	    dcmdfile = value;
	else if( str_eql( p, "automatic" ) == 0 )
	    ddisplay = value;
	else
	  error( filename, lineno, "unrecognized display parameter: %s\n",
	    targv[i] );
      }

    return( 0 );
  }


private int print_tcap()
  {
    tptr  t;

    if( tcap_list == NULL )
	lprintf( stdout, "there are no shorted transistors\n" );
    else
	lprintf( stdout, "shorted transistors:\n" );
    for( t = tcap_list; t != NULL; t = t->scache.t )
      {
	lprintf( stdout, " %s g=%s s=%s d=%s\n",
	  ttype[BASETYPE( t->ttype )], 
	  pnode( t->gate ), pnode( t->source ), pnode( t->drain ) );
      }
    return( 0 );
  }


private	char     changelog[256] = "rsim.changes.log";

private int update_net()
  {
    if( targc != 2 )
      {
	error( filename, lineno, "syntax = %s <filename>\n", targv[0] );
	return( 0 );
      }

    ch_list = rd_changes( targv[1], (*changelog == 0) ? NULL : changelog );
    return( 0 );
  }


private int set_incres()
  {
    if( targc == 1 )
	lprintf( stdout, "incremental resolution = %.1f\n", d2ns(INC_RES) );
    else if( targc == 2 )
      {
	long  new_res = (long) ns2d( atof( targv[1] ) );

	if( new_res < 0 )
	    error( filename, lineno, "resolution must be positive\n" );
	else
	    INC_RES = new_res;
      }
    else
	error( filename, lineno, "wrong number of args to ires command\n" );

    return( 0 );
  }


private int do_incsim()
  {
    if( sim_time0 != 0 )
      {
	lprintf( stderr, "Warning: part of the history was flushed:\n" );
	lprintf( stderr, "         incremental results may be wrong\n" );
      }
    if( targc > 1 )
	update_net();
    if( ch_list == NULL )
	lprintf( stdout, "no affected nodes: done\n" );
    else
	incsim( ch_list );
    ch_list = NULL;
    return( 0 );
  }


private int setlogchanges()
  {
    Fstat  *stat;

    if( targc == 1 )
      {
	if( *changelog == '\0' )
	    lprintf( stdout, "logfile for changes is turned OFF\n" );
	else
	    lprintf( stdout, "logfile for changes is '%s'\n", changelog );
      }
    else if( targc == 2 )
      {
	if( str_eql( "off", targv[1] ) == 0 )
	    changelog[0] = '\0';
	else
	  {
	    stat = FileStatus( targv[1] );
	    if( stat->write == 0 )
		lprintf( stdout, "can't write to file '%s'\n", targv[1] );
	    else
	      {
		if( stat->exist == 0 )
		    lprintf( stdout, "OK, starting a new log file\n" );
		else
		    lprintf( stdout,"%s already exists, will append to it\n",
		      targv[1] );
		strcpy( changelog, targv[1] );
	      }
	  }
      }
    else
	error( filename, lineno, "wrong syntax: %s [ <filename> | OFF ]\n",
	  targv[0] );

    return( 0 );
  }


private char    x_display[40];

private int xDisplay()
  {
    char  *s, *getenv();

    if( targc == 1 )
      {
	s = x_display;
	if( *s == '\0' )
	    s = getenv( "DISPLAY" );
	if( s == NULL )
	    lprintf( stdout, "DISPLAY is unknown\n" );
	else
	    lprintf( stdout, "DISPLAY = %s\n", s );
      }
    else if( targc == 2 )
      {
	if( analyzerON )
	    lprintf( stdout, "analyzer running, can't change display\n" );
	else
	    strcpy( x_display, targv[1] );
      }
    else
      {
	error( filename, lineno, "Wrong syntax: %s [host:number]\n", *targv );
      }
    return( 0 );
  }


private char    *first_file;

private int analyzer()
  {
    extern int AddNode(), AddVector();

    if( not analyzerON )
      {
	if( not InitDisplay( first_file, (*x_display) ? x_display : NULL ) )
	    return( 0 );
	InitTimes( sim_time0, stepsize, cur_delta );
      }

    if( targc > 1 )
      {
	int  ndigits = 0;

	if( targv[1][0] == '-' and targv[1][2] == '\0' )
	  {
	    switch( targv[1][1] )
	      {
		case 'b' :	ndigits = 1;	shift_args();	break;
		case 'o' :	ndigits = 3;	shift_args();	break;
		case 'h' :	ndigits = 4;	shift_args();	break;
		default : ;
	      }
	  }

	if( targc > 1 )
	    apply( AddNode, AddVector, ndigits );
      }

    DisplayTraces( analyzerON );		/* pass 0 first time */

    analyzerON = TRUE;
    return( 0 );
  }


private int clear_analyzer()
  {
    if( analyzerON )
	ClearTraces();
    else
	lprintf( stdout, "analyzer is off\n" );
    return( 0 );
  }


private int dump_hist()
  {
    if( first_file == NULL or cur_delta == 0 )
      {
	error( filename, lineno, "Nothing to dump\n" );
	return( 0 );
      }

    if( targc == 1 )
      {
	char  fname[ 256 ];

	sprintf( fname, "%s.hist", first_file );
	DumpHist( fname );
      }
    else if( targc == 2 )
	DumpHist( targv[1] );
    else
	error( filename, lineno, "Wrong number of args: %s [filename]\n",
	  targv[0] );
    return( 0 );
  }


private int do_readh()
  {
    if( targc != 2 )
	error( filename, lineno, "Wrong number of args: %s [filename]\n",
	  targv[0] );
    else
      {
	if( analyzerON )
	    StopAnalyzer();
	ReadHist( targv[1] );
	if( analyzerON )
	    RestartAnalyzer( sim_time0, cur_delta, TRUE );
      }
    return( 0 );
  }

private int back_time()
  {
    long  newt;

    if( targc != 2 )
	error( filename, lineno, "Wrong number of args: %s time\n",
	  targv[0] );
    else
      {
	newt = (long) ns2d( atof( targv[1] ) );
	if( newt < sim_time0 or newt >= cur_delta )
	  {
	    error( filename, lineno, "%s => bad time for %s\n",
	      targv[1], targv[0] );
	    return( 0 );
	  }
	if( analyzerON )
	    StopAnalyzer();
	cur_delta = newt;
	ClearInputs();
	back_sim_time( cur_delta, FALSE );
	cur_node = NULL;			/* fudge */
	walk_net( backToTime );
	if( analyzerON )
	    RestartAnalyzer( sim_time0, cur_delta, TRUE );
	targc = 1;
	pnlist();
      }
    return( 0 );
  }


private int wr_net()
  {
    if( first_file == NULL )
      {
	error( filename, lineno, "No network?\n" );
	return( 0 );
      }

    if( targc == 1 )
      {
	char  fname[ 256 ];

	sprintf( fname, "%s.inet", first_file );
	wr_netfile( fname );
      }
    else if( targc == 2 )
	wr_netfile( targv[1] );
    else
	error( filename, lineno, "Wrong number of args : %s [filename]\n", targv[0] );
    return( 0 );
  }


private int do_stats()
  {
    char  n1[10], n2[10];

    lprintf( stdout, "changes = %d\n", num_edges );
    lprintf( stdout, "punts (cns) = %d (%d)\n",
      num_punted, num_cons_punted );
    if( num_punted == 0 )
      {
	strcpy( n1, "0.0" );
	strcpy( n2, n1 );
      }
    else
      {
	sprintf( n1, "%2.2f", 100.0 / ((float) num_edges / num_punted + 1.0) );
	sprintf( n2, "%2.2f", (float) (num_cons_punted * 100.0 /num_punted) );
      }
    lprintf( stdout, "punts = %s%%, cons_punted = %s%%\n", n1, n2 );

    lprintf( stdout, "nevents = %ld; evaluations = %ld\n", nevent, nevals );
    if( i_nevals != 0 )
      {
	lprintf( stdout, "inc. evaluations = %ld; events:\n", i_nevals );
	lprintf( stdout, "reval:      %ld\n", nreval_ev );
	lprintf( stdout, "punted:     %ld\n", npunted_ev );
	lprintf( stdout, "stimuli:    %ld\n", nstimuli_ev );
	lprintf( stdout, "check pnt:  %ld\n", ncheckpt_ev );
	lprintf( stdout, "delay chk:  %ld\n", ndelaychk_ev );
	lprintf( stdout, "delay ev:   %ld\n", ndelay_ev );
      }

    return( 0 );
  }


private void shift_args()
  {
    register int   ac;
    register char  **ap;
    register char  *wp;

    targc--;
    for( ac = 0, ap = targv, wp = wildCard; ac < targc; ac++ )
      {
	ap[ac] = ap[ac + 1];
	wp[ac] = wp[ac + 1];
      }
  }


private int do_time()
  {
    char  usage_str[40];

    shift_args();
    if( targc )
      {
	set_usage();
	exec_cmd();
      }
    print_usage( targc, usage_str );	/* if targc -> print total usage */
    lprintf( stdout, "%s", usage_str );
    return( 0 );
  }


private int flush_hist()
  {
    long ftime;

    if( targc == 1 )
	ftime = cur_delta;
    else if( targc == 2 )
      {
	ftime = ns2d( atof( targv[1] ) );
	if( ftime < 0 or ftime > cur_delta )
	  {
	    error( filename, lineno, "bad flush time specified\n" );
	    return( 0 );
	  }
      }
    else
      {
	error( filename, lineno, "Wrong number of args: %s [time]\n",
	  targv[0] );
	return( 0 );
      }

    if( ftime == 0 )
	return( 0 );

    if( analyzerON )
	StopAnalyzer();

    FlushHist( ftime );
    sim_time0 = ftime;

    if( analyzerON )
	RestartAnalyzer( sim_time0, cur_delta, TRUE );

    return( 0 );
  }


private int HasCoords()
  {
    if( txt_coords )
	lprintf( stdout, "YES\n" );
    return( 0 );
  }


/* Remove path and extension from a filename */
private char *BaseName( fname )
  register char  *fname;
  {
    register char  *s = fname;

    while( *s )
	s++;
    while( s > fname and *s != '/' )
	s--;
    fname = ( *s == '/' ) ? s + 1 : s;

    for( s = fname; *s != '\0' and *s != '.'; s++ );
    *s = '\0';
    return( fname );
  }


/* list of commands and their handlers */
private Command  cmds[] = 
  {
    { NULL, "vector", dovector },
    { NULL, "set", setvector },
    { NULL, "V", setseq },
    { NULL, "display", dodisplay },
    { NULL, "path", dopath },
    { NULL, "activity", doactivity },
    { NULL, "changes", dochanges },
    { NULL, "@", cmdfile },
    { NULL, "logfile", setlog },
    { NULL, "d", pnlist },
    { NULL, "u", setvalue },
    { NULL, "h", setvalue },
    { NULL, "l", setvalue },
    { NULL, "x", setvalue },
    { NULL, "w", display },
    { NULL, "t", settrace },
    { NULL, "inputs", inputs },
    { NULL, "stepsize", setstep },
    { NULL, "s", dostep },
    { NULL, "?", quest },
    { NULL, "!", excl },
    { NULL, "q", quit },
    { NULL, "exit", doexit },
    { NULL, "clock", setclock },
    { NULL, "R", runseq },
    { NULL, "c", doclock },
    { NULL, "p", dophase },
    { NULL, "print", domsg },
    { NULL, "debug", setdbg },
    { NULL, "model", setmodel },
    { NULL, ">", dostate },
    { NULL, "<", dostate },
    { NULL, "<<", dostate },
    { NULL, "decay", setdecay },
    { NULL, "unitdelay", setunit },
    { NULL, "report", setreport },
    { NULL, "printx", doprintX },
    { NULL, "printp", printPending },
    { NULL, "setpath", docmdpath },
    { NULL, "tcap", print_tcap },
    { NULL, "update", update_net },
    { NULL, "setlog", setlogchanges },
    { NULL, "Xdisplay", xDisplay },
    { NULL, "analyzer", analyzer },
    { NULL, "ana", analyzer },			/* for Ted Williams */
    { NULL, "clear", clear_analyzer },
    { NULL, "dumph", dump_hist },
    { NULL, "wnet", wr_net },
    { NULL, "stats", do_stats },
    { NULL, "time", do_time },
    { NULL, "ires", set_incres },
    { NULL, "isim", do_incsim },
    { NULL, "flush", flush_hist },
    { NULL, "readh", do_readh },
    { NULL, "back", back_time },
    { NULL, "assert", doAssert },
    { NULL, "has_coords", HasCoords },
    { NULL, NULL, NULL }
  };


/* VARARGS1 */
private void Usage( msg, s1 )
  char  *msg, *s1;
  {
    fprintf( stderr, msg, s1 );
    fprintf( stderr, "usage:\n irsim " );
    fprintf( stderr, "[-s] prm_file {sim_file ..} [-cmd_file ..] [+h_file]\n" );
    fprintf( stderr, "\t-s\t\tstack series transistors\n" );
    fprintf( stderr, "\tprm_file\telectrical parameters file\n" );
    fprintf( stderr, "\tsim_file\tsim (network) file[s]\n" );
    fprintf( stderr, "\tcmd_file\texecute command file[s]\n" );
    fprintf( stderr, "\th_file\t\tinitialize net from history dump file\n" );
    exit( 1 );
  }


public main( argc, argv )
   char *argv[];
  {
    int  i, arg1;

    InitSignals();
    InitUsage();
    InitThevs();
    InitCAD();
    InitCmdPath();
    init_hist();
    fprintf( stdout, "*** IRSIM %s ***\n", version );
    fflush( stdout );

    filename = "*initialization*";

    for( arg1 = 1; arg1 < argc; arg1++ )
      {
	if( argv[arg1][0] == '-' )
	  {
	    switch( argv[arg1][1] )
	      {
		case 's' :			/* stack series transistors */
		    stack_txtors = TRUE;
		    break;
		default :
		    Usage( "unknown switch: %s\n", argv[arg1] );
	      }
	  }
	else
	  break;
      }

	/* read in the electrical configuration file */
    if( arg1 < argc )
	config( argv[arg1++] );
    else
	Usage( "No electrical parameters file specified. Bye\n" );


	/* Read network files (sim files) */
    for( i = arg1; i < argc; i += 1 )
      {
	if( argv[i][0] != '-' and argv[i][0] != '+' )
	  {
	    rd_network( argv[i] );
	    if( first_file == NULL )
		first_file = BaseName( argv[i] );
	  }
      }

    if( first_file == NULL )
      {
	fprintf( stderr, "No network, no work. Bye...\n" );
	exit( 1 );
      }

    ConnectNetwork();	    /* connect all txtors to corresponding nodes */

	/* set up command table */
      {
	register int      n;
	register Command  *c;

	for( n = 0; n < 128; n++ )
	    cmdtbl[n] = NULL;
	for( c = cmds; c->name != NULL; c += 1 )
	  {
	    c->next = cmdtbl[c->name[0]];
	    cmdtbl[c->name[0]] = c;
	  }
      }

    init_event();

	/* search for +filename for possible history-dump read */
    for( i = arg1; i < argc; i++ )
	if( argv[i][0] == '+' )
	    ReadHist( &argv[i][1] );

	/* search for -filename for command files to process. */
    filename = "command line";
    lineno = 1;
    for( i = arg1; i < argc; i++ )
	if( argv[i][0] == '-' and not finput( &argv[i][1] ) )
	    error( filename, lineno, "cannot open %s for input\n",
	       &argv[i][1] );

	/* finally (assuming we get this far) read commands from user */
    debug = 0;
    filename = "tty";
    lineno = 0;

    input( stdin );
    TerminateAnalyzer();
    exit( 0 );
  }
