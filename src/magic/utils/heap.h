/*
 * heap.h --
 *
 * Routines to create and maintain heaps.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 *
 * rcsid "$Header: heap.h,v 6.0 90/08/28 19:00:53 mayo Exp $";
 *
 * This heap supports the creation and manipulation of heaps.  Features are:
 *	- Ascending and descending heaps.
 *	- Heaps are managed as arrays.  Left and right children are accessed
 *	  via address computation.
 *	- Heap areas are automatically resized when full.
 *	- The heap is sorted just prior to the first removal from the heap.
 *	- After the first removal, additions to the heap are merged into
 *	  existing heap one at a time.
 *	- A variety of types of key are supported:
 *		long
 *		float
 *		double
 *		DoubleInt
 *
 *	#include "heap.h"
 *	#include <stdio.h>
 *	main()
 *	{
 *	    Heap * h;
 *	    HeapEntry * e;
 *	    int key;
 *	
 *	    HeapInit(&h, 1024, 0, FALSE);
 *	    HeapAdd(&h, 0, 0);
 *	    HeapDump(&h);
 *	    e=HeapRemoveTop(&h);
 *	    HeapDump(&h);
 *	    while(1)
 *	    {
 *		fprintf(stderr, "Key:  ");
 *		scanf("%d", &key);
 *		if(key==0) break;
 *		HeapAdd(&h, key, 0);
 *		HeapDump(&h);
 *	    }
 *	    while((e=HeapRemoveTop(&h))!=(HeapEntry *) NULL)
 *	    {
 *		printf("Got heap key %d\n", e->he_key);
 *		HeapDump(&h);
 *	    }
 *	    HeapKill(&h, NULL);
 *	}
 */

#define _HEAP

#ifndef _DOUBLEINT
int err0 = Need_to_include_doubleint_header;
#endif

/* DATA STRUCTURES */

/*
 * A heap entry has some key value on which the entry is sorted, and some
 * identifying data associated with the key.
 */
typedef struct
{
    char	*he_id;		/* One word value to identify the entry */

    /*
     * Key value.
     * The element of the following union that is active depends on the
     * way the Heap was initialized in the call to HeapInit() or
     * HeapInitType().
     */
    union heUnion
    {
	int		 hu_int;
	DoubleInt	 hu_dint;
	float		 hu_float;
	double		 hu_double;
    }		 he_union;
} HeapEntry;

#define	he_int		he_union.hu_int
#define	he_dint		he_union.hu_dint
#define	he_float	he_union.hu_float
#define	he_double	he_union.hu_double

#define	HESIZE(n)	(sizeof (char *) + (n))

/* These define the kind of key used in the heap */
#define	HE_INT		1
#define	HE_DINT		2
#define	HE_FLOAT	3
#define	HE_DOUBLE	4

/* A heap is an array with size set to some power of 2.  Links are implicit:
 * the left child of a node n is 2n, and the right child is 2n+1.  'build'
 * keeps track of the last child participating in a rebuild operation.  If
 * entries have been added since the last rebuild, then process those children. 
 * The he_stringId flag enables automatically copies character string id's
 * during calls to HeapAdd.  If the id is actually a pointer to a struct, you
 * must pass a pointer to a copy and set he_stringId to FALSE.
 */
typedef struct
{
    HeapEntry	*he_list;	/* Pointer to array of entries	*/
    int		 he_size;	/* Size of the array of entries	*/
    int		 he_used;	/* Number of locations used	*/
    int		 he_built;	/* Size when heap was last built*/
    int		 he_stringId;	/* TRUE=>HeapEntry id is string */
    int		 he_big;	/* TRUE if largest at the root	*/
    int		 he_keyType;	/* See type of Heap above */
} Heap;

/* PROCEDURES */
extern void HeapInit(), HeapInitType(), HeapKill(), HeapFreeIdFunc();
extern void HeapAdd(), HeapDump();
extern HeapEntry *HeapRemoveTop();
extern HeapEntry *HeapLookAtTop();
#define HEAP_EMPTY(h)   ((h)->he_used == 0)
