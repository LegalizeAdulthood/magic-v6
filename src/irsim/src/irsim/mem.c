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
 *   This is a very low overhead storage allocator for managing many
 * "small" objects.
 *
 *   The allocator recognizes 2 different types of objects: (1) small
 * fixed size objects, and (2) large or variable size objects.
 *
 *   Variable size objects are allocated on demand, using a next-fit
 * algorithm.  The same is used for large objects.
 *
 *   When a small fixed size object is requested, the allocator will
 * pre-allocate as many such objects as will fit in one page (4Kbytes in-
 * this implementation).  The unused objects are put in a free list for
 * quick access later on.  A separate free list for every object size is
 * maintained.  When such an object is freed, it is simply put back in
 * its respective free list (small objects don't migrate from one free
 * list to another).
 *
 *   All objects returned by Malloc are word aligned: a word is defined to
 * be the smallest size integer into which an address can be cast.  All
 * requests are rounded to a multiple of words (as defined above).
 *
 *   The allocator does not "remember" the size of fixed-size objects. Keeping
 * this information is left to the user.  It does however, remeber the size of
 * objects allocated using Valloc.
 *
 *   The allocator provides the following interfaces:
 *	1) Falloc( nbytes )
 *	    returns a pointer to nbytes of storage. For fixed size objects.
 *	2) Valloc( nbytes )
 *	    returns a pointer to nbytes of storage. For variable size objects.
 *	3) MallocList( size )
 *	    returns a linked list of 'size' bytes objects.
 *	4) Ffree( ptr, nbytes )
 *	    deallocate the fixed size object of 'nbytes' pointed to by 'ptr'.
 *	4) Vfree( ptr )
 *	    deallocate the variable object of 'nbytes' pointed to by 'ptr'.
 *
 *    The argument to Ffree can be any any object obtained from Falloc, or
 * any one of the elements obtained from MallocList.
 */



#include <sys/types.h>
#ifndef SYS_V
#    include <sys/time.h>
#    include <sys/resource.h>
#endif
#include <stdio.h>
#include "defs.h"


	/* number of bytes in a word */
#define	WORDSIZE		( sizeof( Object ) )

	/* round x to nearest z (ceiling function) */
#define	ROUND( x, z )		( ((x) + (z) - 1) / (z) )

	/* convert size in bytes to nearest number of words */
#define	NWORDS( x )		ROUND( x, WORDSIZE )

	/* number of bytes/words per page */
#define	DATABYTES		4096
#define	DATAWORDS		( DATABYTES / WORDSIZE )

	/* objects larger than this many words are considered large */
#define	BIG_OBJ			40

#define	NBINS			(BIG_OBJ + 1)


typedef union Object *Pointer;

typedef union Object		/* Basic data object (i.e. `machine word') */
  {
    Pointer  ptr;
    int      num;
    int      align[1];
  } Object;



	/* export this definition for the user's sake */
public typedef union MElem
  {
    union MElem  *next;		/* points to next element in linked list */
    int          align[1];	/* dummy used to force word alignment */
  } *MList;


typedef struct
  {
    Pointer  free1;		/* Lists of free objects of this size */
    Pointer  free2;
    Pointer  freed;		/* List of freed objects */
    int      n_freed;		/* number of freed objects */
  } Bucket;


private	Bucket  bucket[NBINS];		/* free list hash table */


	/* External definitions */
extern char      *sbrk();
extern           etext;
extern int       setrlimit(), getrlimit();

#define	SBRK_FAIL		( (char *) -1 )
#define	MAXTRIES		5

#define	PAGE_SIZE		1024			/* for alignment */
#define	PAGE_MASK		( PAGE_SIZE - 1 )


/*
 * Interface to the system's sbrk call.  If sbrk fails, try to determine
 * what's happening and attempt to solve it.  If all fails return NULL.
 */

#ifdef SYS_V

private Pointer GetMoreCore( npages )
  int  npages;
  {
    char  *ret;
    int   cursize, newsize, nbytes;
    int   tries, inc;
    long  lim;

    cursize = (int) sbrk( 0 );			/* align on 1K boundary */
    inc = PAGE_SIZE - (cursize & PAGE_MASK) & PAGE_MASK;

    nbytes = npages * DATABYTES + inc;

    ret = sbrk( nbytes );
    if( ret != SBRK_FAIL )
	return( (Pointer) ret );

    newsize = cursize + nbytes;
    lim = ulimit( 3, 0 );

    ret = SBRK_FAIL;

    if( newsize <= lim )
      {
	for( tries = 0; tries < MAXTRIES and ret == SBRK_FAIL; tries++ )
	  {
	    if( tries == 0 )
	      {
		fprintf( stderr, "*** MEMORY WARNING ***\n" );
		fprintf( stderr, "Current data size: %d (%dK)\n", cursize,
		  ROUND( cursize, 1024 ));
		fprintf( stderr, "New data size = %d (%dK)\n", newsize,
		  ROUND( newsize, 1024 ) );
		fprintf( stderr, "Hard limit = %d (%dK)\n", lim,
		  ROUND( lim, 1024 ));
	      }
	    fprintf( stderr, "I seem to be short on swap space\n" );
	    fprintf( stderr, "Will sleep for 15 seconds and try again\n");
	    sleep( 15 );
	    ret = sbrk( nbytes );
	  }
      }

    return( ( ret == SBRK_FAIL ) ? (Pointer) NULL : (Pointer) ret );
  }

#else		/* BSD */

#ifdef host_mips			/* kludge for mips based systems */
#    define	heap_start	(char *) 0x10000000
#else
#    define	heap_start	(char *) &etext
#endif

private Pointer GetMoreCore( npages )
  int  npages;
  {
    char           *ret;
    int            cursize, newsize, nbytes;
    int            tries, inc;
    struct rlimit  lim;
    
    cursize = (int) sbrk( 0 );			/* align on 1K boundary */
    inc = PAGE_SIZE - (cursize & PAGE_MASK) & PAGE_MASK;

    nbytes = npages * DATABYTES + inc;

    ret = sbrk( nbytes );
    if( ret != SBRK_FAIL )
	return( (Pointer) ret );

    cursize -= (int) heap_start;
    newsize = cursize + nbytes;
    (void) getrlimit( RLIMIT_DATA, &lim );

    if( newsize > lim.rlim_max )
      {
	fprintf( stderr, "Memory Error: Hard limit exceeded %d\n",
	  ROUND( lim.rlim_max, 1024 ) );
	return( (Pointer) NULL );
      }

    ret = SBRK_FAIL;
    for( tries = 0; tries < MAXTRIES and ret == SBRK_FAIL; tries++ )
      {
	if( newsize < lim.rlim_cur )
	  {
	    if( tries == 0 )
	      {
		fprintf( stderr, "*** MEMORY WARNING ***\n" );
		fprintf( stderr, "Current data size: %d (%dK)\n", cursize,
		  ROUND( cursize, 1024 ));
		fprintf( stderr, "New data size = %d (%dK)\n", newsize,
		  ROUND( newsize, 1024 ) );
		fprintf( stderr, "Soft limit = %d (%dK)\n", lim.rlim_cur,
		  ROUND( lim.rlim_cur, 1024 ));
		fprintf( stderr, "Hard limit = %d (%dK)\n", lim.rlim_max,
		  ROUND( lim.rlim_max, 1024 ));
	      }
	    fprintf( stderr, "I seem to be short on swap space\n" );
	    fprintf( stderr, "Will sleep for 15 seconds and try again\n");
	    sleep( 15 );
	  }
	else if( newsize < lim.rlim_max )
	  {
	    int  softlim = lim.rlim_cur;

	    fprintf( stderr, "MEMORY WARNING: Soft limit exceeded\n" );
	    lim.rlim_cur = lim.rlim_max;
	    if( setrlimit( RLIMIT_DATA, &lim ) == 0 )
	      {
		fprintf( stderr, 
		  " => Soft limit increased from %d (%dK) to %d (%d)\n",
		  softlim, ROUND( softlim, 1024 ),
		  lim.rlim_max, ROUND( lim.rlim_max, 1024 ) );
	      }
	    else
	      {
		fprintf( stderr,
		  " => Can NOT increase Soft limit [%d (%dK)] to %d (%d)\n",
		  softlim, ROUND( softlim, 1024 ),
		  lim.rlim_max, ROUND( lim.rlim_max, 1024 ) );
		fprintf( stderr, "I Will try again in 15 seconds\n" );
		sleep( 15 );
	      }
	  }
	ret = sbrk( nbytes );
      }

    return( ( ret == SBRK_FAIL ) ? (Pointer) NULL : (Pointer) ret );
  }

#endif SYS_V


/*
 * Get nPages contiguous pages.  Make sure new pages are aligned on system
 * page boundaries.
 * If 'size' is <> 0, the elements in the page(s) are linked into a list.
 */
private Pointer GetPage( nPages, size )
  int  nPages;
  int  size;
  {
    Pointer     pg;
    int         inc;


    pg = GetMoreCore( nPages );

    if( pg != NULL and size != 0 )		/* Initialize the new page */
      {
	register Pointer  p, page;
	register int      n, np, nwords;

	inc = DATAWORDS / size;
	nwords = size;
	page = pg;
	np = nPages;
	while( np-- > 0 )
	  {
	    n = inc;
	    for( p = page; --n > 0; p->ptr = p + nwords, p += nwords );
	    p->ptr = (np == 0) ? NULL : (page += DATAWORDS);
	  }
      }
    return( pg );
  }


		/* forward references */
private	MList    MallocBigList();
	char     *Valloc();
	void     Ffree(), Vfree();

public char *Falloc( nbytes )
  int  nbytes;
  {
    register Bucket   *bin;
    register Pointer  p;
    register int      nwords;

    if( nbytes <= 0 )			/* ubiquitous check */
	return( NULL );

    nwords = NWORDS( nbytes );
    if( nwords >= NBINS )
	return( Valloc( nbytes ) );

    bin = &(bucket[nwords]);
    if( (p = bin->freed) != NULL )
      {
	bin->freed = p->ptr;
	bin->n_freed -= 1;
      }
    else if( (p = bin->free1) != NULL )
      {
	if( (bin->free1 = p->ptr) == NULL )
	  {
	    bin->free1 = bin->free2;
	    bin->free2 = NULL;
	  }
      }
    else
      {
	int  n;

	p = GetPage( 1, nwords );
	if( p == NULL )			/* Out of memory */
	    return( NULL );

	n = nwords * ((DATAWORDS / nwords) / 2);
	bin->free1 = p + 1;
	bin->free2 = p + n;
	p[n - nwords].ptr = NULL;
      }
    return( (char *) p );
  }


public void Ffree( p, nbytes )
  Pointer  p;
  int      nbytes;
  {
    register int   nwords;

    if( p == NULL or nbytes <= 0 )	/* sanity ? */
	return;

    nwords = NWORDS( nbytes );
    if( nwords >= NBINS  )		/* big block */
	Vfree( p );
    else				/* return to its corresponding bin */
      {
	p->ptr = bucket[nwords].freed;
	bucket[nwords].freed = p;
	bucket[nwords].n_freed += 1;
      }
  }

/*
 * Return a linked list of elements, each 'nbytes' long in size.
 */
public MList MallocList( nbytes )
  int  nbytes;
  {
    register Pointer  p;
    register int      nwords, n;
    register Bucket   *bin;

    if( nbytes <= 0 )
	return( NULL );

    nwords = NWORDS( nbytes );
    if( nwords >= NBINS )
	return( MallocBigList( nbytes ) );

    bin = &(bucket[nwords]);
    n = (DATAWORDS / nwords) / 2;
    if( bin->n_freed >= n / 2 )		/* quarter page => return that */
      {
	p = bin->freed;
	bin->freed = NULL;
	bin->n_freed = 0;
      }
    else if( (p = bin->free1) != NULL )
      {
	bin->free1 = bin->free2;
	bin->free2 = NULL;
      }
    else
      {
	p = GetPage( 1, nwords );
	if( p == NULL )			/* Out of memory */
	    return( NULL );

	n *= nwords;
	bin->free1 = p + n;		/* save 2nd half of the page */
	bin->free2 = NULL;
	p[n - nwords].ptr = NULL;
      }

    return( (MList) p );
  }


/*
 * Handle the large (infrequent) case.
 */
private MList MallocBigList( nbytes )
  int  nbytes;
  {
    int      nelem;
    Pointer  head, tail;

    nelem = ( nbytes < 2 * DATABYTES ) ? DATABYTES / nbytes : 2;
    head = tail = (Pointer) Valloc( nbytes );
    if( head == NULL )
	return( NULL );

    while( --nelem > 0 )
      {
	tail->ptr = (Pointer) Valloc( nbytes );
	if( tail->ptr == NULL )
	  {
	    while( head != NULL )
	      {				/* free everything already Malloc`d */
		tail = head->ptr;
		Vfree( head );
		head = tail;
	      }
	    return( NULL );
	  }
	tail = tail->ptr;
      }
    tail->ptr = NULL;
    return( (MList) head );
  }



/*
 * Next-fit storage allocation for variable (infrequent) size objects.
 * Algorithm and variable names from Knuth V1, p 437.  See Exercise 2.5.6.
 */
private	Object    avail[2];		/* dummy header that points to first
					 * free element.  Free list is kept
					 * in address order */
private	Pointer   rover = avail;	/* pointer into the free list */


#define	NEXT( blk )	( (blk)->ptr )		/* next block in free list */
#define	SIZE( blk )	( blk[1].num )		/* words available in block */


/*
 * Add the block pointed to by 'ptr' to the free list.
 */
public void Vfree( ptr )
  Pointer  ptr;
  {
    register Pointer  p, q, r;
    register int      nwords;

    if( ptr == NULL )
	return;

    ptr--;
    nwords = ptr->num;
    if( nwords <= 0 )
	return;

    q = avail;
    r = ptr;
    p = NEXT( q );

    while( (p != NULL) and (p < r) )	/* search for the right place */
      {
	q = p;
	p = NEXT( p );
      }
	/* this is where it should go */
	/* note: since NULL = 0, if p = NULL, p != r + nwords */

    if( p == r + nwords )		/* new block abuts p, consolidate */
      {
	nwords += SIZE( p );
	NEXT( r ) = NEXT( p );
      }
    else				/* does not abut, just connect */
      {
	NEXT( r ) = p;
      }

    if( r == q + SIZE( q ) )		/* new block abuts q, consolidate */
      {
	SIZE( q ) += nwords;
	NEXT( q ) = NEXT( r );
      }
    else				/* does not abut, just connect */
      {
	NEXT( q ) = r;
	SIZE( r ) = nwords;
      }
    rover = q;			/* start searching here next time */
  }


/*
 * Return a pointer to (at least) n bytes of storage.
 */
public char *Valloc( nbytes )
  int  nbytes;
  {
    register Pointer  p, q;
    register int      nwords;
    int               firstTime;

    if( nbytes <= 0 )			/* ubiquitous check */
	return( NULL );

    nwords = (NWORDS( nbytes ) + 2) & ~1;

  Start :
    if( (q = rover) == NULL )
      {
	q = rover = avail;
	firstTime = 0;
      }
    else
	firstTime = 1;

  again:
    p = NEXT( q );
    while( p != NULL )			/* search for a block large enough */
      {
	if( SIZE( p ) >= nwords )
	    goto found;
	q = p;
	p = NEXT( p );
      }

    if( firstTime )
      {
	q = avail;			/* first time, one more chance */
	firstTime = 0;
	goto again;
      }

      {			/* fall through: out of memory, get some more */
	int      nPages;
	Pointer  pg;

	nPages = 2 * (ROUND( nwords, DATAWORDS ));
	pg = GetPage( nPages, 0 );
	if( pg == NULL )
	    return( NULL );

	pg->num =  nPages * DATAWORDS;
	Vfree( pg + 1 );
	goto Start;		/* try again */
      }

  found :
    if( SIZE( p ) == nwords )		/* exact match. remove block */
      {
	NEXT( q ) = NEXT( p );
      }
    else	/* SIZE( p ) > nwords => too large. take part of it */
      {
	register Pointer  r;

	r = p + nwords;		     /* remaining free area */
	NEXT( q ) = r;
	NEXT( r ) = NEXT( p );
	SIZE( r ) = SIZE( p ) - nwords;
      }
    rover = q;			/* start looking here next time */

    p->num = nwords;
    p++;
    return( (char *) p );
  }
