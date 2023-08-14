/*
 * malloc.h --
 *
 * Memory allocator.
 * Magic's version of malloc, called mallocMagic() and freeMagic(),
 * maintains free lists of small objects, and tries to cluster objects
 * of the same size within the same pages for good paging behavior.
 *
 * rcsid "$Header: malloc.h,v 6.1 90/08/31 12:15:07 mayo Exp $"
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
 */

/*
 * The following are debugging flags for the memory allocator.
 * If any are enabled, the MALLOC() and FREE() macros become defined
 * as just procedure calls on mallocMagic() and freeMagic() respectively.  
 *
 * To ensure that the monitoring code in each of these procedures gets
 * executed for all allocations/frees, every module that uses the
 * MALLOC or FREE macros should be recompiled.  Modules that use
 * only the mallocMagic() or freeMagic() procedure calls do not need to be
 * recompiled.
 */

extern char *mallocMagic();

/* #define	FREEDEBUG	/* Enable checking of assertions in free() */
/* #define	MALLOCMEASURE	/* Measure frequence of allocation per size */
/* #define	MALLOCTRACE	/* Produce a malloc.out trace file for prleak */

#if	defined(FREEDEBUG) || defined(debug) || defined(MALLOCMEASURE) || defined(MALLOCTRACE)
#	define	NOMACROS
#endif

/*
 * A different implementation may need to redefine INT, ALIGN, NALIGN
 * where INT is integer type to which a pointer can be cast
 */

#define	INT		int
#define ALIGN		int
#define NALIGN		1
#define	BYPERWD		(sizeof (INT))
#define	LOGBYPERWD	2				    /* LOG2(BYPERWD) */
#define	BYTETOWORD(n)	(((n) + (BYPERWD-1)) >> LOGBYPERWD)

/*
 * To speed the allocation of "small" objects, we maintain a separate
 * free list for each size of object up to BIGOBJECT bytes.  Since we
 * always return chunks aligned up to the nearest INT boundary, the
 * free lists are indexed by BYTETOWORD(nbytes) where nbytes is the
 * size of the object requested.
 */

#define	BIGOBJECT	150	/* # bytes in largest freelist-alloc'd object */

/*
 * When an object is freed, its size must be known so it can be placed
 * on the appropriate free list.  We use a word per object to store
 * this information.
 */

union store {
    union store	*ptr;
    unsigned size;		/* size of this object (words) */
    ALIGN dummy[NALIGN];
};

/*
 * Turn an external object pointer into an internal store pointer
 * and vice versa.
 */
#define	tointernal(op)	( ((union store *) (op)) - 1 )
#define	toexternal(sp)	( (char *) (((union store *) (sp)) + 1) )

/*
 * Test whether an object is new-style malloc'd or old-style.
 * This assumes that the first address ever returned by the
 * old style of malloc is greater than or equal to BIGOBJECT.
 *
 * Both macros take internal store pointers (union store *)
 * as arguments.
 */
#define	isnewstyle(sp)	( (sp)->size < BYTETOWORD(BIGOBJECT) )
#define	isoldstyle(sp)	( (sp)->size >= BYTETOWORD(BIGOBJECT) )

/*
 * Memory for new objects is allocated in pages of size MAG_PAGESIZE.
 * Each page holds objects of only a single size.
 */

#define	MAG_PAGESIZE	4096
#define	MAG_PAGEBYTES	(MAG_PAGESIZE - sizeof (struct mallocPageHdr))
#define	MAG_PAGEMASK	(~(MAG_PAGESIZE-1))

#define	OBJECTTOPAGE(o)	((struct mallocPage *) (((int) o) & MAG_PAGEMASK))
#define	ROUNDUPPAGE(o)	((struct mallocPage *) \
				((((int) o) + MAG_PAGESIZE-1) & MAG_PAGEMASK))

struct mallocPageHdr
{
    struct mallocPage	*mPh_next;	/* Next page of this size */
    struct mallocPage	*mPh_prev;	/* Previous page of this size */
    union store		*mPh_free;	/* List of free objects on this page */
    short		 mPh_threshobjs;/* Min number of free objects */
    short		 mPh_freeobjs;	/* # free objects on this page */
};

struct mallocPage
{
    struct mallocPageHdr	mP_hdr;
    char			mP_data[MAG_PAGEBYTES];
};

#define	mP_next		mP_hdr.mPh_next
#define mP_prev		mP_hdr.mPh_prev
#define	mP_free		mP_hdr.mPh_free
#define	mP_freeobjs	mP_hdr.mPh_freeobjs
#define	mP_threshobjs	mP_hdr.mPh_threshobjs

#define	THRESHOLD(n)	((n) / 10)	/* 10% */

/*
 * Macros for speedy allocation/freeing of objects in the
 * most common cases.
 */

#ifndef	NOMACROS
#	define	MALLOC(type, pt, nbytes) \
    { \
	register unsigned nwX_ = BYTETOWORD(nbytes); \
	register struct mallocPage *ppX_; \
	register union store *spX_; \
 \
	if (nwX_ < BYTETOWORD(BIGOBJECT) \
		&& (ppX_ = mallocPageFirst[nwX_]) \
		&& (spX_ = ppX_->mP_free)) { \
	    ppX_->mP_free = spX_->ptr; \
	    spX_->size = nwX_; \
	    (pt) = (type) toexternal(spX_); \
	} else (pt) = (type) mallocMagic((unsigned) (nbytes)); \
    }

#       define  CALLOC(type, pt, nbytes) \
    { \
	MALLOC(type, pt, nbytes); \
	bzero((char *) (pt), nbytes); \
    }

/*
 * The call to freeMagic below can't be replaced by a call to
 * the ordinary procedure free(), since magic.h defines free
 * to be an error string.  The idea is that only malloc.c
 * can refer to the old malloc/free itself, to avoid any
 * possibility that magic code might accidentally call
 * either procedure directly.
 */
#	define FREE(cp) \
    { \
	register union store *spX_ = tointernal(cp); \
	register struct mallocPage *ppX_; \
 \
	if (isoldstyle(spX_)) freeMagic(cp); \
	else \
	{ \
	    ppX_ = OBJECTTOPAGE(spX_); \
	    if (ppX_->mP_prev \
			&& (++(ppX_->mP_freeobjs) == ppX_->mP_threshobjs)) \
		mallocFreePage(ppX_, spX_); \
	    else \
	        spX_->ptr = ppX_->mP_free, ppX_->mP_free = spX_; \
	} \
    }
#else
#	define	MALLOC(type, pt, n)   (pt) = (type) mallocMagic((unsigned) (n))
#	define	CALLOC(type, pt, n)   (pt) = (type) callocMagic((unsigned) (n))
#	define	FREE(cp)	      freeMagic(cp)
#endif	NOMACROS

extern struct mallocPage *mallocPageFirst[];
