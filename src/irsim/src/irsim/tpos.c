#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"

#define	HASHSIZE	1021

public	int    txt_coords = TRUE;	/* TRUE if trans. coordinates exist */

private tptr   tpostbl[ HASHSIZE ];


#define	HashPos( N, X, Y )	\
  {				\
    if( (N = X * Y) < 0 )	\
	N = ~(N);		\
    N = N % HASHSIZE;		\
  }				\


public void EnterPos( tran )
  tptr  tran;
  {
    long  n;

    HashPos( n, tran->x, tran->y );

    tran->tlink = tpostbl[n];
    tpostbl[n] = tran;
  }


public tptr FindTxtorPos( x, y )
  register long  x, y;
  {
    register tptr  t;
    long           n;

    HashPos( n, x, y );

    for( t = tpostbl[n]; t != NULL; t = t->tlink )
      {
	if( t->x == x and t->y == y )
	    return( t );
      }
    return( NULL );
  }


public void DeleteTxtorPos( tran )
  tptr  tran;
  {
    register tptr  *t;
    long           n;

    HashPos( n, tran->x, tran->y );

    for( t = &(tpostbl[n]); *t != NULL; t = &((*t)->tlink) )
      {
	if( *t == tran )
	  {
	    *t = tran->tlink;
	    break;
	  }
      }
  }


public void ChangeTxtorPos( tran, x, y )
  tptr  tran;
  long  x, y;
  {
    register tptr  *t;
    long           n1, n2;

    HashPos( n1, tran->x, tran->y );
    HashPos( n2, x, y );
    tran->x = x;
    tran->y = y;
    if( n1 == n2 )
	return;
    for( t = &(tpostbl[n1]); *t != NULL; t = &((*t)->tlink) )
      {
	if( *t == tran )
	  {
	    *t = tran->tlink;
	    break;
	  }
      }
    tran->tlink = tpostbl[n2];
    tpostbl[n2] = tran;
  }


public nptr FindNode_TxtorPos( s )
  char  *s;
  {
    long  x, y;
    tptr  t;

    if( sscanf( &s[3], "%ld,%ld", &x, &y ) != 2 )
	return( NULL );

    if( (t = FindTxtorPos( x, y )) == NULL )
	return( NULL );

    switch( s[2] )
      {
	case 'g': return( t->gate );
	case 'd': return( t->drain );
	case 's': return( t->source );
      }
    return( NULL );
  }
