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
 * electrical parameters used for deriving capacitance info for charge
 * sharing.  Default values aren't for any particular process, but are
 * self-consistent.
 *	Area capacitances are all in pfarads/sq-micron units.
 *	Perimeter capacitances are all in pfarads/micron units.
 */
public	double  CM2A = .00000;	/* 2nd metal capacitance -- area */
public	double  CM2P = .00000;	/* 2nd metal capacitance -- perimeter */
public	double  CMA = .00003;	/* 1st metal capacitance -- area */
public	double  CMP = .00000;	/* 1st metal capacitance -- perimeter */
public	double  CPA = .00004;	/* poly capacitance -- area */
public	double  CPP = .00000;	/* poly capacitance -- perimeter */
public	double  CDA = .00010;	/* n-diffusion capacitance -- area */
public	double  CDP = .00060;	/* n-diffusion capacitance -- perimeter */
public	double  CPDA = .00010;	/* p-diffusion capacitance -- area */
public	double  CPDP = .00060;	/* p-diffusion capacitance -- perimeter */
public	double  CGA = .00040;	/* gate capacitance -- area */

public	double  LAMBDA = 2.5;	  /* microns/lambda */
public	double  LAMBDA2;	  /* LAMBDA**2 */
public	double  LOWTHRESH = 0.3;  /* low voltage threshold, normalized units */
public	double  HIGHTHRESH = 0.8; /* high voltage threshold,normalized units */
public	double  DIFFEXT = 0;	  /* width of source/drain diffusion */

public	int	config_flags = 0;

/* values of config_flags */
public
#define	DIFFPERIM	0x1	/* set if diffusion perimeter does not 	    */
				/* include sources/drains of transistors.   */
public
#define	CNTPULLUP	0x2	/* set if capacitance from gate of pullup   */
				/* should be included.			    */
public
#define	SUBPAREA	0x4	/* set if poly over xistor doesn't make a   */
				/* capacitor.				    */
public
#define	DIFFEXTF	0x8	/* set if we should add capacitance due to  */
				/* diffusion-extension of source/drain.	    */


typedef struct	/* table translating parameters to its associated names */
  {
    char      *name;
    int       flag;
    double    *dptr;
  } pTable;

private	pTable  parms[] = 
  {
    "capm2a",	    0x0,	&CM2A,
    "capm2p",	    0x0,	&CM2P,
    "capma",	    0x0,	&CMA,
    "capmp",	    0x0,	&CMP,
    "cappa",	    0x0,	&CPA,
    "cappp",	    0x0,	&CPP,
    "capda",	    0x0,	&CDA,
    "capdp",	    0x0,	&CDP,
    "cappda",	    0x0,	&CPDA,
    "cappdp",	    0x0,	&CPDP,
    "capga",	    0x0,	&CGA,
    "lambda",	    0x0,	&LAMBDA,
    "lowthresh",    0x0,	&LOWTHRESH,
    "highthresh",   0x0,	&HIGHTHRESH,
    "diffperim",    DIFFPERIM,	NULL,
    "cntpullup",    CNTPULLUP,	NULL,
    "subparea",	    SUBPAREA,	NULL,
    "diffext",	    DIFFEXTF,	&DIFFEXT,
    NULL,	    0x0,	NULL
  };


#define	LSIZE		500	/* max size of parameter file input line */
#define	MAXARGS		10	/* max number of arguments in line */

private	int     lineno;		/* current line number */
private	char    *currfile;	/* current input file */
private	int     nerrs = 0;	/* errors found in config file */
private	int     maxerr;
private	void    insert();	/* forward reference */


private int ParseLine( line, args )
  register char  *line;
  char           **args;
  {
    register char  c;
    int            ac = 0;

    for( ; ;  )
      {
	while( (c = *line) <= ' ' and c != '\0' )
	    line++;
	if( c == '\0' or c == ';' )
	    break;
	*args++ = line;
	ac++;
	while( (c = *line) > ' ' and c != ';' )
	    line++;
	*line = '\0';
	if( c == '\0' or c == ';' )
	    break;
	line++;
      }
    *line = '\0';
    *args = NULL;
    return( ac );
  }


public void config( cname )
  char  *cname;
  {
    register pTable  *p;
    FILE             *cfile;
    char             prm_file[256];
    char             line[LSIZE];
    char             *targv[MAXARGS];
    int              targc;

    if( *cname != '/' )		/* not full path specified */
      {
	Fstat  *stat;
	
	stat = FileStatus( cname );
	if( not stat->read )
	  {
	    sprintf( prm_file, "%s/%s", cad_lib, cname );
	    stat = FileStatus( prm_file );
	    if( stat->read )
	        cname = prm_file;
	    else
	      {
		strcat( prm_file, ".prm" );
		stat = FileStatus( prm_file );
		if( stat->read )
		    cname = prm_file;
	      }
	  }
      }
    currfile = cname;

    lineno = 0;
    if( (cfile = fopen( cname, "r" )) == NULL )
      {
	fprintf( stderr,"can't open electrical parameters file <%s>\n", cname);
	exit( 1 );
      }

    *line = '\0';
    (void) fgetline( line, LSIZE, cfile );
    if( strncmp( line, "; configuration file", 20 ) )
      {
	rewind( cfile );
	maxerr = 1;
      }
    else
	maxerr = 15;

    while( fgetline( line, LSIZE, cfile ) != NULL )
      {
	lineno++;
	targc = ParseLine( line, targv );
	if( targc == 0 )
	    continue;
	if( str_eql( "resistance", targv[0] ) == 0 )
	  {
	    if( targc >= 6 )
		insert( targv[1], targv[2], targv[3], targv[4], targv[5] );
	    else
	      {
		error( currfile, lineno, "syntax error in resistance spec\n" );
		nerrs++;
	      }
	    continue;
	  }
	else
	  {
	    for( p = parms; p->name != NULL; p++ )
	      {
		if( str_eql( p->name, targv[0] ) == 0 )
		  {
		    if( p->dptr != NULL )
			*(p->dptr) = atof( targv[1] );
		    if( p->flag != 0 and atoi( targv[1] ) != 0 )
			config_flags |= p->flag;
		    break;
		  }
	      }
	    if( p->name == NULL )
	      {
		error( currfile, lineno,
		  "unknown electrical parameter: (%s)\n", targv[0] );
		nerrs++;
	      }
	  }
	if( nerrs >= maxerr )
	  {
	    if( maxerr == 1 )
		fprintf( stderr,
		  "I think %s is not an electrical parameters file\n", cname );
	    else
		fprintf( stderr, "Too many errors in '%s'\n", cname );
	    exit( 1 );
	  }
      }
    LAMBDA2 = LAMBDA * LAMBDA;
    fclose( cfile );
  }


/*
 * info on resistance vs. width and length are stored first sorted by
 * width, then by length.
 */
struct length
  {
    struct length    *next;	/* next element with same width */
    double           l;		/* length of this channel */
    double           r;		/* equivalent resistance/square */
  };

struct width
  {
    struct width     *next;		/* next width */
    double           w;			/* width of this channel */
    struct length    *list;		/* list of length structures */
  } *resistances[4][2 * NTTYPES] = { NULL };


/* linear interpolation, assume that x1 < x <= x2 */
#define	interp( x, x1, y1, x2, y2 )  (((x - x1) / (x2 - x1)) * (y2 - y1) + y1)


/*
 * given a list of length structures, sorted by incresing length return
 * resistance of given channel.  If no exact match, return result of
 * linear interpolation using two closest channels.
 */
private double lresist( list, l, size )
  register struct length  *list;
  double                  l, size;
  {
    register struct length  *p, *q;

    for( p = list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->l == l or( p->l > l and q == NULL ) )
	    return( p->r *size );
	if( p->l > l )
	    return( size *interp( l, q->l, q->r, p->l, p->r ) );
      }
    if( q != NULL )
	return( q->r *size );
    return( 1E4 * size );
  }


/*
 * given a pointer to the width structures for a particular type of
 * channel compute the resistance for the specified channel.
 */
private double wresist( list, w, l )
  register struct width  *list;
  double                 w, l;
  {
    register struct width  *p, *q;
    double                 size = l / w;
    double                 temp;

    for( p = list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->w == w or( p->w > w and q == NULL ) )
	    return( lresist( p->list, l, size ) );
	if( p->w > w )
	  {
	    temp = lresist( q->list, l, size );
	    return( interp( w, q->w, temp, p->w, lresist( p->list, l, size ) ) );
	  }
      }
    if( q != NULL )
	return( lresist( q->list, l, size ) );
    return( 1E4 * size );
  }


/*
 * Compute equivalent resistance given width, length and type of transistor.
 * for all contexts (STATIC, DYNHIGH, DYNLOW).  Place the result on the
 * transistor 
 */
public void requiv( width, length, type, rp )
  double   width, length;
  int      type;
  Resists  *rp;
  {
    type = BASETYPE( type );
    rp->rstatic = wresist( resistances[STATIC][type], width, length );
    rp->dynlow = wresist( resistances[DYNLOW][type], width, length );
    rp->dynhigh = wresist( resistances[DYNHIGH][type], width, length );
  }


private void linsert( list, l, resist )
  register struct length  **list;
  double                  l, resist;
  {
    register struct length  *p, *q, *lnew;

    for( p = *list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->l == l )
	  {
	    p->r = resist;
	    return;
	  }
	if( p->l > l )
	    break;
      }
    lnew = (struct length *) Valloc( sizeof( struct length ) );
    lnew->next = p;
    lnew->l = l;
    lnew->r = resist;
    if( q == NULL )
	*list = lnew;
    else
	q->next = lnew;
  }


/* add a new data point to the interpolation array */
private void winsert( list, w, l, resist )
  register struct width  **list;
  double                 w, l, resist;
  {
    register struct width   *p, *q, *wnew;
    register struct length  *lnew;

    for( p = *list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->w == w )
	  {
	    linsert( &p->list, l, resist );
	    return;
	  }
	if( p->w > w )
	    break;
      }
    wnew = (struct width *) Valloc( sizeof( struct width ) );
    lnew = (struct length *) Valloc( sizeof( struct length ) );
    wnew->next = p;
    wnew->list = lnew;
    wnew->w = w;
    if( q == NULL )
	*list = wnew;
    else
	q->next = wnew;
    lnew->next = NULL;
    lnew->l = l;
    lnew->r = resist;
  }


/* interpret resistance specification command */
private void insert( type, context, w, l, r )
  char  *type, *context, *w, *l, *r;
  {
    register int  c, t;
    double        width, length, resist;
    char          temp[100];

    width = atof( w );
    length = atof( l );
    resist = atof( r );
    if( width <= 0 or length <= 0 or resist <= 0 )
      {
	error( currfile, lineno, "bad w, l, or r in config file\n" );
	nerrs++;
	return;
      }

    if( str_eql( context, "static" ) == 0 )
	c = STATIC;
    else if( str_eql( context, "dynamic-high" ) == 0 )
	c = DYNHIGH;
    else if( str_eql( context, "dynamic-low" ) == 0 )
	c = DYNLOW;
    else if( str_eql( context, "power" ) == 0 )
	c = POWER;
    else
      {
	error( currfile, lineno, "bad resistance context in config file\n" );
	nerrs++;
	return;
      }

    for( t = 0; t < NTTYPES; t++ )
      {
	if( str_eql( ttype[t], type ) == 0 )
	  {
	    winsert( &resistances[c][t], width, length, resist*width/length );
	    return;
	  }
	else
	  {
	    strcpy( temp, ttype[t] );
	    strcat( temp, "-with-drop" );
	    if( str_eql( temp, type ) == 0 )
	      {
/*		winsert( &resistances[c][t + NTTYPES], width, length,
		  resist * width / length );	*/
		return;
	      }
	  }
      }
    error( currfile, lineno, "bad resistance transistor type\n" );
    nerrs++;
  }

