/*
 * malloc.c --
 *
 * Memory allocator.
 * This version of malloc maintains free lists of small objects,
 * and tries to cluster objects of the same size within the same
 * pages for good paging behavior.
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

#ifndef lint
static char rcsid[] = "$Header: malloc.c,v 6.1 90/08/31 12:14:44 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include "magic.h"
#include "malloc.h"

/* These were defined to be error strings in malloc.h */
#undef	malloc
#undef	free

/* Imports */
extern void TxError();
extern char *TxGetLine();
extern char *sbrk();
extern char *malloc();

/* Forward declarations */
char *mallocSbrk();

/*
 * Some versions of malloc are incompatible with the standard version.
 * As a result, a strategy of looking at the header word of an object
 * to determine whether it was allocated using the old malloc or our own
 * version will not work.  In order to insure that we can always tell
 * which version of malloc was used, we use an extra header
 * word (the first word of the object returned by the old malloc) to store
 * a fake header value that unambiguously identifies this object as having
 * been allocated using the old malloc:
 *
 *	+--------+
 *	|	 | <--- header word of old malloc
 *	+--------+
 *	|	 | <--- extra header word identifying this as "old-style malloc"
 *	+--------+
 *	|	 | <--- we return a pointer to this word to the caller
 *	|	 |
 *	| (body) |
 *	|	 |
 *	|	 |
 *	+--------+
 *
 */

/*
 * We have to encapsulate
 * everything we allocate using it so it is clearly identifiable as
 * belonging to the old malloc.  See the comments above.
 */
#	define	HEADERWORD	((union store *) (BIGOBJECT + 4))

/* Number of sizes of objects we allocate ourselves */
#define	NSIZES		BYTETOWORD(BIGOBJECT)

#ifdef	FREEDEBUG
#	define	MASSERT(p)	if (!(p)) botch("p"); else
static	int	 allocSmall = 0;	/* Words in smallest object malloc'd */
static	int	 allocLarge = 0;	/* Words in largest object malloc'd */
#else
#	define	MASSERT(p)
#endif	FREEDEBUG

#ifdef	MALLOCMEASURE
/*
 * Statistics gathered for each word size
 * of object up to BIGOBJECT, for objects
 * allocated using the new malloc.
 */
static	unsigned allocCumulativeCount[NSIZES];	/* Total malloc's */
static	unsigned allocCurrentCount[NSIZES];	/* Current malloc's */
static	unsigned allocCumulativeSize;
static	unsigned allocCurrentSize;
#endif	MALLOCMEASURE

/*
 * When tracing is compiled into the malloc code, these
 * variables control the state of tracing.  If mallocTraceEnabled
 * is TRUE, each call to mallocMagic() or freeMagic() will be traced by
 * writing records to the file mallocTraceFile.  If mallocTraceProgram
 * is TRUE, mallocTraceFile is open to a program and we should use
 * pclose() in mallocTraceDone() instead of fclose().
 */
int mallocTraceDisableCount = 0;
bool mallocTraceEnabled = FALSE;
bool mallocTraceProgram = FALSE;
FILE *mallocTraceFile = NULL;

#define MALLOCTRACEONLYMAX	32
char *mallocTraceOnlyPages[MALLOCTRACEONLYMAX];
int mallocTraceOnlyCnt = 0;
bool mallocTraceOnly;

char stdTraceName[] = "malloc.out";	/* For dbx */

/* ------ end of malloc tracing variables ------ */

/*
 * Lists of free pages.
 * For each word size of object, mallocPageFirst[w] points to the
 * first page in a free list of pages containing objects of that size.
 * The last page in this list is pointed to by mallocPageLast[w].
 * The above lists are doubly-linked.
 * 
 * We also maintain a singly-linked list of pages that contain no allocated
 * data.  When the last object on a page (except for pages at the head of
 * their allocation lists) is freed, the page is removed from its allocation
 * list and added to the list whose listhead is mallocPageEmpty.
 */
struct mallocPage *mallocPageFirst[NSIZES];
struct mallocPage *mallocPageLast[NSIZES];

struct mallocPage *mallocPageEmpty;

/*
 * Usually there are either very few objects of a given size, or
 * very many.  To cut down on the overhead of allocating a full
 * page to hold only one or two objects of a given size, we don't
 * start up a list of free pages for a given object size until
 * we have already processed enough allocation requests to exceed
 * a threshold for that object size.
 */
static int mallocInitialCount[NSIZES];
static bool mallocCountsInitialized = FALSE;

/*
 * ----------------------------------------------------------------------------
 *
 * mallocMagic --
 *
 * Allocate a chunk of memory of 'nbytes' bytes.
 *
 * Results:
 *	Returns a pointer to a chunk of memory of the given
 *	size.
 *
 * Side effects:
 *	May cause more memory to be requested for this process.
 *
 * ----------------------------------------------------------------------------
 */

char *
mallocMagic(nbytes)
    unsigned nbytes;
{
    unsigned nwords = BYTETOWORD(nbytes);
    register struct mallocPage *pp;
    register union store *sp;
    int ntries;
    char *ret;

    MASSERT(nbytes != 0);

#ifdef	FREEDEBUG
    if (allocSmall == 0 || nwords < allocSmall) allocSmall = nwords;
    if (nwords > allocLarge) allocLarge = nwords;
#endif	FREEDEBUG

    if (nwords >= BYTETOWORD(BIGOBJECT)) goto oldmalloc;

    while (pp = mallocPageFirst[nwords])
    {
	MASSERT(pp == ROUNDUPPAGE(pp));
	MASSERT(pp->mP_next || pp == mallocPageLast[nwords]);
	MASSERT(pp->mP_next == ROUNDUPPAGE(pp->mP_next) || (pp->mP_next == NULL && pp == mallocPageLast[nwords]));
	MASSERT(pp->mP_prev != pp);
	if (pp->mP_free) goto foundpage;

	/*
	 * Set up page to accumulate free objects while not on allocation list.
	 * Then remove from allocation list.
	 */
	pp->mP_freeobjs = 0;
	pp->mP_threshobjs = THRESHOLD(pp->mP_threshobjs);
	pp->mP_prev = pp;
	if ((mallocPageFirst[nwords] = pp->mP_next) != NULL)
	    pp->mP_next->mP_prev = (struct mallocPage *) NULL;
    }
    mallocPageLast[nwords] = (struct mallocPage *) NULL;

    /*
     * Initialize the threshold for page allocation if
     * the first time around.
     */
    if (!mallocCountsInitialized)
    {
	register n;

	for (n = 1; n < NSIZES; n++)
	    mallocInitialCount[n] = MAG_PAGEBYTES / (BYPERWD * (n+1));
	mallocCountsInitialized = TRUE;
    }

    /*
     * If we haven't allocated sufficiently many objects of
     * this size to fill up a single page, we allocate using
     * the old malloc.  This should cut down significantly on
     * wasted space for object sizes with very small populations.
     */
    if (mallocInitialCount[nwords])
    {
	mallocInitialCount[nwords]--;
	goto oldmalloc;
    }

    /*
     * Before allocating memory for this page, see if any empty pages are
     * available.
     */

    pp = mallocPageEmpty;
    if (pp != (struct mallocPage *) NULL) 
    {
	mallocPageEmpty = pp->mP_next;
	goto initpage;
    }

    /*
     * We must allocate a new page.
     * We set up a completely empty free list on this page.
     * If the current memory break is not page-aligned,
     * we make it so.
     */
    pp = (struct mallocPage *) mallocSbrk(0);
    if (pp != ROUNDUPPAGE(pp))
    {
	int incr, misalign;

	misalign = ((unsigned) pp) & (sizeof (ALIGN) - 1);
	if (misalign)
		(void) mallocSbrk(sizeof (ALIGN) - misalign);

	incr = (INT *) ROUNDUPPAGE(pp) - (INT *) pp;
	(void) mallocSbrk(incr * sizeof (union store));
    }

    pp = (struct mallocPage *) mallocSbrk(MAG_PAGESIZE);
    if ((int) pp == -1)
	return ((char *) NULL);		/* Out of memory */

initpage:
    mallocInitPage(pp, nwords);

    /*
     * Found a free page, or allocated a new one.
     * In either event, pp is guaranteed to point to the first
     * page in the list pointed to by mallocPageFirst[nwords].
     */
foundpage:
    sp = pp->mP_free;
    MASSERT(pp == OBJECTTOPAGE(sp));
    pp->mP_free = sp->ptr;
    MASSERT(sp->ptr == (union store *) NULL || pp == OBJECTTOPAGE(sp->ptr));
    sp->size = nwords;
    ret = toexternal(sp);
#ifdef	MALLOCMEASURE
    allocCumulativeCount[nwords]++;
    allocCurrentCount[nwords]++;
    allocCumulativeSize += nwords;
    allocCurrentSize += nwords;
#endif	MALLOCMEASURE
    goto out;

oldmalloc:
    /*
     * Allocate using the old version of malloc.
     * We must encapsulate the object to make sure we can distinguish it from 
     * something we allocated using our new scheme.
     */
    for (ntries = 0; 
	(ret = malloc(nbytes + sizeof (union store))) == (char *) NULL;
	ntries++)
    {
	/* Infer the cause of our running out of memory and try to fix it */
	if (ntries > 5)
	{
	    mallocSbrkFatal();
	    /*NOTREACHED*/
	}
	mallocSbrkError((int) nbytes + sizeof (union store));
    }
    sp = (union store *) ret;
    sp->ptr = HEADERWORD;
    ret += sizeof (union store);

    /* Common exit */
out:
#ifdef	MALLOCTRACE
    if (mallocTraceEnabled)
	mallocTraceWrite(ret, nbytes);
#endif	MALLOCTRACE
    return (ret);
}

/*
 * ----------------------------------------------------------------------------
 *
 * callocMagic --
 *
 * Allocate and clear a chunk of memory of 'nbytes' bytes.
 *
 * Results:
 *	Returns a pointer to a chunk of memory of the given
 *	size.
 *
 * Side effects:
 *	May cause more memory to be requested for this process.
 *
 * ----------------------------------------------------------------------------
 */

char *
callocMagic(nbytes)
    unsigned nbytes;
{
    register char *cp;

    cp = mallocMagic(nbytes);
    bzero(cp, (int) nbytes);

    return (cp);
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocInitPage --
 *
 * Set up a newly allocated page.  This routine will only be called when
 * the free page list for the required size objects is empty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the header of the page 'pp', and builds
 *	an initial free list of objects each of size 'nwords'.
 *
 * ----------------------------------------------------------------------------
 */

mallocInitPage(pp, nwords)
    register struct mallocPage *pp;
    unsigned nwords;
{
    register union store *freep;
    register INT *ip;
    int n, realnwords = nwords + 1;

    MASSERT(mallocPageFirst[nwords] == (struct mallocPage *) NULL);
    MASSERT(mallocPageLast[nwords] == (struct mallocPage *) NULL);

    pp->mP_threshobjs = pp->mP_freeobjs = MAG_PAGEBYTES / (realnwords * BYPERWD);
    freep = (union store *) NULL;
    for (n = pp->mP_freeobjs, ip = (INT *) pp->mP_data; n--; ip += realnwords)
    {
	((union store *) ip)->ptr = freep;
	freep = (union store *) ip;
    }
    pp->mP_free = freep;

    pp->mP_next = pp->mP_prev = (struct mallocPage *) NULL;
    mallocPageFirst[nwords] = pp;
    mallocPageLast[nwords] = pp;
}

/*
 * ----------------------------------------------------------------------------
 *
 * freeMagic --
 *
 * Free a chunk of memory which had been allocated by mallocMagic()
 * above.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the object pointed to by 'cp'.  The caller should
 *	make no further references to this object after the next
 *	call to mallocMagic().
 *
 * ----------------------------------------------------------------------------
 */

freeMagic(cp)
    char *cp;
{
    register union store *sp = tointernal(cp);
    register struct mallocPage *pp;
#ifdef MALLOCMEASURE
    register unsigned nwords;
#endif

#ifdef	MALLOCTRACE
    if (mallocTraceEnabled)
	mallocTraceWrite(cp, 0);
#endif	MALLOCTRACE

    if (isoldstyle(sp))
    {
	/*
	 * Remember: we encapsulated this block with an extra word
	 * so we would be certain to recognize it as something allocated
	 * using the old malloc instead of from our own free lists.
	 */
	free(cp - sizeof (union store));
	return;
    }

#ifdef	FREEDEBUG
    MASSERT(sp->size >= allocSmall && sp->size <= allocLarge);
#endif	FREEDEBUG

    /*
     * If this page is not on the allocation list, and if
     * the number of free objects crosses the threshold,
     * call mallocFreePage to process the page.
     *
     * Since we don't decrement the mP_freeobjs count on the first page
     * (mallocPageFirst[nwords]) when we allocate new objects, we don't
     * increment it here.
     */

    pp = OBJECTTOPAGE(sp);
    if ((pp->mP_prev != (struct mallocPage *) NULL) &&
	    (++(pp->mP_freeobjs) == pp->mP_threshobjs))
    {
	mallocFreePage(pp, sp);
    }
    else
    {
#ifdef MALLOCMEASURE
	nwords = sp->size;	/* Must come before setting sp->ptr */
	allocCurrentCount[nwords]--;
	allocCurrentSize -= nwords;
#endif
	sp->ptr = pp->mP_free;
	pp->mP_free = sp;
    }

}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocFreePage --
 *
 * Process a page where the number of free objects has crossed the
 * threshold.  Note that this procedure will never be called with the
 * first page on an allocation list.
 *
 * 'sp' points to an object to be freed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	There are two cases.  If we have just created a totally empty
 *	page, remove the page from its allocation list and add it to
 *	the empty page list (mallocPageEmpty).
 *
 *	Otherwise, add the page to its allocation list and set the next
 *	threshold to be when all objects on the page have been freed.
 *
 *	Can modify mallocPageLast[nwords] and mallocPageFirst[nwords],
 *	as well as the mP_next and mP_prev pointers of various pages.
 *	May also modify mallocPageEmpty.
 *
 * ----------------------------------------------------------------------------
 */

mallocFreePage(pp, sp)
    register struct mallocPage *pp;
    register union store *sp;
{
    register struct mallocPage *op;
    register unsigned nwords = sp->size;

    MASSERT(pp != mallocPageFirst[nwords]);
    MASSERT(pp->mP_prev != (struct mallocPage *) NULL);

    if (pp->mP_freeobjs == MAG_PAGEBYTES / ((nwords + 1) * BYPERWD))
    {
	/*
	 * Remove this page from the allocation list for 'nwords' and
	 * add it to the empty list.  We need not add 'sp' to the object
	 * list for this page because the list will be reinitialized before
	 * the page is used.
	 */

	MASSERT(pp->mP_next || pp == mallocPageLast[nwords]);
	MASSERT(pp->mP_prev != pp);

	if (pp->mP_next == (struct mallocPage *) NULL)
	    mallocPageLast[nwords] = pp->mP_prev;
	else
	    pp->mP_next->mP_prev = pp->mP_prev;

	pp->mP_prev->mP_next = pp->mP_next;

	pp->mP_next = mallocPageEmpty;
	mallocPageEmpty = pp;
    }
    else
    {
	/*
	 * Add this page to the allocation list for 'nwords' and set new
	 * threshold value.  But first add 'sp' to the free object list of
	 * this page.
	 */

	sp->ptr = pp->mP_free;
	pp->mP_free = sp;

#ifdef	MALLOCMEASURE
	allocCurrentCount[nwords]--;
	allocCurrentSize -= nwords;
#endif	MALLOCMEASURE

	if (op = mallocPageLast[nwords])
	    op->mP_next = pp;
	else
	    mallocPageFirst[nwords] = pp;

	pp->mP_next = (struct mallocPage *) NULL;
	pp->mP_prev = mallocPageLast[nwords];
	mallocPageLast[nwords] = pp;

	pp->mP_threshobjs = MAG_PAGEBYTES / ((nwords + 1) * BYPERWD);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocSbrk --
 *
 * Interface to the system "sbrk" routine that tries to discern the
 * reason why there is no more memory available.
 *
 * Results:
 *	Returns a pointer to a chunk of memory of the given size.
 *
 * Side effects:
 *	May cause more memory to be requested for this process.
 *
 * ----------------------------------------------------------------------------
 */

char *
mallocSbrk(size)
    int size;		/* Number of bytes requested */
{
    char *block;
    int ntries;

    for (ntries = 0; (block = sbrk(size)) == (char *) -1; ntries++)
    {
	if (ntries > 5)
	{
	    mallocSbrkFatal();	/* Exits */
	    /*NOTREACHED*/
	}
	mallocSbrkError(size);
    }

    return (block);
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocSbrkError --
 *
 * Try to discern the reason why there is no more memory available,
 * and fix it if possible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Either increases the resource limits for this process and
 *	returns, or exits.
 *
 * ----------------------------------------------------------------------------
 */

mallocSbrkError(size)
    int size;		/* Number of bytes requested */
{
    char *lastaddr;
    struct rlimit limit;
    int cursize;
    extern etext;
    extern int errno;
    extern char *sys_errlist[];

    /*
     * Hmmmm.  Somebody's not giving us enough memory.
     */
    printf("\n\n****** WARNING: NO MORE MEMORY ******\n\n");
#ifndef SYSV
    if (errno != ENOMEM)
	printf("Strange: sbrk(%d) returned -1 but with error %s\n",
		size, sys_errlist[errno]);

    /*
     * Check to see whether we've exceeded our resource
     * limit for the data segment.
     */
    lastaddr = sbrk(0);
    if (lastaddr == (char *) -1)
    {
	printf("Strange: sbrk(0) returned -1\n");
	goto keep_trying;
    }
#ifdef ibm032
    cursize = lastaddr - (char *) 0x10000000 ;
#else
    cursize = lastaddr - (char *) &etext;
#endif
    getrlimit(RLIMIT_DATA, &limit);
    printf("Requesting %d new bytes\n", size);
    printf("Current data segment size is %d bytes (%dK)\n",
		cursize, (cursize + 1023) / 1024);
    printf("New size would be %d bytes (%dK)\n",
		cursize + size, (cursize + size + 1023) / 1024);
    printf("Current soft limit is %d bytes (%dK)\n",
		limit.rlim_cur, (limit.rlim_cur + 1023) / 1024);
    printf("Current hard limit is %d bytes (%dK)\n",
		limit.rlim_max, (limit.rlim_max + 1023) / 1024);

    if (cursize + size < limit.rlim_cur)
    {
	printf("Looks like we're out of swap space!\n");
keep_trying:
	printf("Will sleep for 15 seconds and try again.\n");
	sleep(15);
	return;
    }
    else if (cursize + size < limit.rlim_max)
    {
	limit.rlim_cur = limit.rlim_max;
	printf("Will try to increase our current limit.\n");
	if (setrlimit(RLIMIT_DATA, &limit) == 0)
	{
	    printf("Limit increased.  Will try allocation again.\n");
	    return;
	}
	printf("Couldn't increase current limit.\n");
    }
    else printf("Hard limit exceeded!\n");

#endif SYSV
    mallocSbrkFatal();
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocSbrkFatal --
 *
 * Can't allocate any more memory.  Give the user a chance to send
 * magic a TERM signal so we can save all our cells; otherwise,
 * wait for the user to type "yes" or end-of-file and then exit.
 *
 * Don't try prompting the user if the standard input isn't a
 * terminal.
 *
 * Results:
 *	Never returns.
 *
 * Side effects:
 *	Exits.
 *
 * ----------------------------------------------------------------------------
 */

mallocSbrkFatal()
{
    char line[100];

    if (!isatty(fileno(stdin)))
    {
	printf("You can't get any more memory.\n");
	printf("The standard input is not a tty, so I'm just aborting.\n\n");
	kill(getpid(), SIGTERM);
	goto nuke;
    }

    printf("You can't get any more memory, so you'll have to abort.\n\n");
    printf("Some programs (such as Magic) allow you to save your work\n");
    printf("by typing CTRL-Z and then:\n");
    printf("        kill -TERM %d\n", getpid());
    printf("\n");
    printf("If you are running such a program and wish to try saving\n");
    printf("your work, you may do so now.\n");

    do
    {
	printf("Type \"yes\" when you are ready to abort: ");
	fflush(stdout);
	if (TxGetLine(line, sizeof line) == NULL)
	    break;
    } while (strncmp(line, "yes", 3) != 0);

nuke:
    printf("Going bye-bye....\n");
    MainExit(1);
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocPrintArena --
 *
 * ***DEBUGGING PROCEDURE***
 *
 * Print out a snapshot of the arena used by "slow" malloc to the
 * indicated file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
mallocPrintArena(initialp, f)
    union store *initialp;
    FILE *f;
{
    TxError("This procedure won't work for the new version of malloc.\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocSizeStats --
 *
 * ***DEBUGGING PROCEDURE***
 *
 * Fill in two user-supplied arrays with statistics about the number
 * of objects allocated.  Both are indexed by the number of words in
 * an object, from 0 up to BYTETOWORD(BIGOBJECT).  The zero entry
 * is for all objects of size BYTETOWORD(BIGOBJECT) words or greater;
 * the other entries are for objects of that number of words.
 *
 * The first array, 'nCumulative' contains for each size object the total
 * number of calls to malloc for that size object.  The second array,
 * 'nCurrent', contains the current population of each size of object.
 * Both arrays must contain at least BYTETOWORD(BIGOBJECT)+1 elements.
 *
 * In addition, two variables are set.  The first, 'sCumulative', contains
 * the total number of words in all requests to malloc.  The second,
 * 'sCurrent', contains the total number of words of storage currently
 * allocated.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

mallocSizeStats(nCumulative, nCurrent, sCumulative, sCurrent)
    register unsigned nCumulative[NSIZES], nCurrent[NSIZES];
    register unsigned *sCumulative, *sCurrent;
{
    register n;

#ifdef	MALLOCMEASURE
    for (n = 0; n < NSIZES; n++)
    {
	nCumulative[n] = allocCumulativeCount[n];
	nCurrent[n] = allocCurrentCount[n];
    }

    *sCumulative = allocCumulativeSize;
    *sCurrent = allocCurrentSize;
#else
    TxError("This version of MAGIC does not gather malloc stats\n");
    for (n = 0; n < NSIZES; n++)
	nCumulative[n] = nCurrent[n] = 0;

    *sCumulative = *sCurrent = 0;
#endif	MALLOCMEASURE
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocPageStats --
 *
 * ***DEBUGGING PROCEDURE***
 *
 * Print to the supplied (FILE *) a map showing the distribution
 * of objects over pages.
 *
 * For each word size of object from 1 up to BYTETOWORD(BIGOBJECT),
 * prints a list of pages in the order in which they appear on the
 * free list, with the following information listed per page:
 *
 *	- Maximum number of objects on this page
 *	- Number of free objects
 *	- Number of objects in use
 *	- Percentage free, measured as  (max - current) / max
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints statistics to the supplied FILE *.
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
mallocPageStats(f)
    FILE *f;
{
#ifdef	MALLOCMEASURE
    register n;
    register struct mallocPage *pp;
    register union store *sp;
    int nmax, nfree;
    double pfree;

    for (n = 1; n < NSIZES; n++)
    {
	for (pp = mallocPageFirst[n]; pp; pp = pp->mP_next)
	{
	    nmax = MAG_PAGEBYTES / (BYPERWD * (n+1));
	    for (nfree = 0, sp = pp->mP_free; sp; sp = sp->ptr, nfree++)
		/* Nothing */;
	    pfree = (100.0 * (double) nfree) / (double) nmax;
	    fprintf(f, "%d\t#%x\t%d\t%d\t%d\t%.0f\n",
			n, pp, nmax, nfree, nmax-nfree, pfree);
	}
    }
    fflush(f);
#else
    TxError("This version of MAGIC does not gather malloc stats\n");
#endif	MALLOCMEASURE
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocSanity --
 *
 * ***DEBUGGING PROCEDURE***
 *
 * Usually called from dbx or some other debugger.
 *
 * Scan through our free lists and anything else that we have around,
 * checking for unusual conditions and printing out info as we go.
 *
 * This only checks the free lists of 'magicMalloc', not those of the
 * main 'malloc'.
 *
 *
 * Results:
 *	TRUE if the free list looks OK, otherwise prints an error message
 *	and returns FALSE.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
mallocSanity()
{
    int n;
    struct mallocPage *pp;
    union store *sp;
    int numfree, numbytes;

    for (n = 1; n < NSIZES; n++)
    {
	for (pp = mallocPageFirst[n]; pp; pp = pp->mP_next)
	{
	    if (pp != ROUNDUPPAGE(pp))
	    {
		TxError("magicMalloc page pointer (0x%x)", pp);
		TxError(" for size %d is insane!\n", n);
		goto mallocError;
	    }
	    if ((pp->mP_next == NULL) && (pp != mallocPageLast[n]))
	    {
		TxError("magicMalloc next pointer (0x%x)", pp);
		TxError(" for size %d is insane!\n", n);
		goto mallocError;
	    }
	    if ((pp->mP_prev == NULL) && (pp != mallocPageFirst[n]))
	    {
		TxError("magicMalloc back pointer (0x%x)", pp);
		TxError(" for size %d is insane!\n", n);
		goto mallocError;
	    }
	    if ((pp->mP_next != NULL) && (pp->mP_next->mP_prev != pp))
	    {
		TxError("magicMalloc chaining for block 0x%x", pp);
		TxError(" for size %d is incorrect!\n", n);
		goto mallocError;
	    }
	    numfree = 0;
	    for (sp = pp->mP_free; sp; sp = sp->ptr)
	    {
		numfree++;
		if (pp != OBJECTTOPAGE(sp))
		{
		    TxError("mallocMagic object pointer (0x%x)", sp);
		    TxError("for size %d is insane!\n", n);
		    goto mallocError;
		}
	    }
	    numbytes = numfree * (n + 1);
	    if (numbytes > MAG_PAGEBYTES)
	    {
		TxError("magicMalloc has %d free bytes", numbytes);
		TxError(" on a page of size %d!\n", MAG_PAGEBYTES);
		goto mallocError;
	    }
	}
    }

    return TRUE;

mallocError:
    /* Set a breakpoint here to catch errors. */
    return FALSE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * mallocTraceInit --
 * mallocTraceInitProg --
 * mallocTraceEnable --
 * mallocTraceDisable --
 * mallocTraceDone --
 *
 * mallocTraceOnlyWatched --
 * mallocTraceWatch --
 * mallocTraceUnWatch --
 *
 * mallocTraceWrite --
 *
 * ***DEBUGGING PROCEDURES***
 *
 * These procedures are used for tracing all allocations and deallocations.
 * The result will be a file, whose name was supplied in the call to
 * mallocTraceInit(), containing the following information for each
 * allocation or free:
 *	1 short		if 0, this is a free; if 1, this is a malloc
 *			if 2, this word should be ignored
 *	1 long		the address of the datum allocated or freed
 *	1 long		the size in bytes of the datum (zero if this is a free)
 *	N longs		one word for each savec PC in the call stack leading
 *			to this malloc; this list is terminated by a NULL PC.
 *
 * If mallocTraceInitProg() is called instead of mallocTraceInit(), it
 * should be given the name of a file from which to read a namelist (normally
 * the name of the currently running program binary) and the path of a program
 * to which the trace output will be piped.  This feature is provided for
 * use when the total size of the trace file would be unmanageably large.
 *
 * If tracing is to be temporarily disabled, mallocTraceDisable() should be
 * called, and then mallocTraceEnable() to re-enable tracing.  These calls
 * are nestable.
 *
 * If only certain addresses are to be traced, mallocTraceOnlyWatch() should
 * be called, then mallocTraceWatch() used to add them to the table.
 * If no more addresses can be added, mallocTraceWatch returns FALSE;
 * otherwise, it returns TRUE.  mallocTraceUnWatch() may be used to remove
 * addresses added with mallocTraceWatch.
 *
 * The trace file is closed and tracing turned off when mallocTraceDone()
 * gets called.
 *
 * The internal procedure mallocTraceWrite() is used to write the actual
 * stack trace.  Note: the code for this procedure is machine-dependent.
 * It also expects that there are machine-language procedures from
 * machine.s:
 *
 *	whence() that will return the frame pointer of the caller's
 *		 stack frame,
 *	thispc() that will return the PC from a frame pointer,
 *	nextfp() that will return the previous frame pointer from the
 *		 current one.
 *
 * Results:
 *	None of these procedures return anything.
 *
 * Side effects:
 *	Calling mallocTraceInit creates a trace file, and calling
 *	mallocTraceDone closes it.
 *
 *	Calling mallocTraceWrite writes to the file.
 *
 * ----------------------------------------------------------------------------
 */

mallocTraceInit(name)
    char *name;		/* Name of file to hold trace information */
{
    if (mallocTraceEnabled)
    {
	TxError("Memory allocation tracing is already enabled (%s).\n", name);
	return;
    }

    if (mallocTraceFile = fopen(name, "w"))
    {
	/* Force stdio library to do its allocation */
	putc('\0', mallocTraceFile);
	fflush(mallocTraceFile);
	fseek(mallocTraceFile, 0L, 0);

	/* Now we can start tracing */
	mallocTraceEnabled = TRUE;
	mallocTraceDisableCount = 0;
	mallocTraceProgram = FALSE;
    }
    else
    {
	perror(name);
	TxError("Memory allocation tracing not enabled.\n");
    }
}

mallocTraceInitProg(binaryFileName, leakProgName)
    char *binaryFileName;	/* Name of binary for namelist */
    char *leakProgName;		/* Path to prleak program */
{
    static short dummyWord = 2;
    unsigned char *cp;
    char cmd[1024];

    if (mallocTraceEnabled)
    {
	TxError("Memory allocation tracing is already enabled (%s,%s).\n",
		binaryFileName, leakProgName);
	return;
    }

    (void) sprintf(cmd, "%s -l -d %s -", leakProgName, binaryFileName);
    if (mallocTraceFile = popen(cmd, "w"))
    {
	/*
	 * Force stdio library to do its allocation.
	 * However, since this is a pipe, we can't seek back, so
	 * instead of rewinding after output, we make this a dummy
	 * entry.
	 */
	cp = (unsigned char *) &dummyWord;
	putc(*cp++, mallocTraceFile);
	putc(*cp++, mallocTraceFile);
	fflush(mallocTraceFile);

	/* Now we can start tracing */
	mallocTraceEnabled = TRUE;
	mallocTraceDisableCount = 0;
	mallocTraceProgram = TRUE;
    }
    else
    {
	perror(cmd);
	TxError("Memory allocation tracing not enabled.\n");
    }
}

mallocTraceEnable()
{
    if (mallocTraceDisableCount > 0)
	mallocTraceDisableCount--;

    if (mallocTraceDisableCount == 0 && mallocTraceFile)
	mallocTraceEnabled = TRUE;
}

mallocTraceDisable()
{
    mallocTraceDisableCount++;
    mallocTraceEnabled = FALSE;
}

mallocTraceDone()
{
    mallocTraceEnabled = FALSE;
    if (mallocTraceFile)
    {
	if (mallocTraceProgram)
	    (void) pclose(mallocTraceFile);
	else
	    (void) fclose(mallocTraceFile);
	mallocTraceFile = (FILE *) NULL;
    }
}

mallocTraceOnlyWatch(only)
    bool only;	/* If TRUE, only watch addresses in list mallocTraceWatchTbl */
{
    mallocTraceOnly = only;
}

bool
mallocTraceWatch(addr)
    char *addr;
{
    if (mallocTraceOnlyCnt >= MALLOCTRACEONLYMAX)
	return (FALSE);

    mallocTraceOnlyPages[mallocTraceOnlyCnt++] = addr;
    return (TRUE);
}

mallocTraceUnWatch(addr)
    char *addr;
{
    int n;

    for (n = 0; n < mallocTraceOnlyCnt; n++)
	if (mallocTraceOnlyPages[n] == addr)
	{
	    mallocTraceOnlyPages[n] =
		mallocTraceOnlyPages[mallocTraceOnlyCnt - 1];
	    break;
	}
    mallocTraceOnlyCnt--;
}

mallocTraceWrite(ptr, size)
    char *ptr;	/* Object being traced */
    int size;	/* Size in bytes; 0 means this is a free */
{
#ifdef	MALLOCTRACE
#ifndef macII
    int *ret, pc, *whence();
    short flag = (size != 0);
    unsigned char *cp;

    if (mallocTraceOnly)
    {
	register char **cp, **ep;

	for (cp = mallocTraceOnlyPages, ep = &cp[mallocTraceOnlyCnt]; cp < ep; )
	    if (ptr == *cp++)
		goto found;

	return;
    }

found:
    /* Output flag as a short */
    cp = (unsigned char *) &flag;
    putc(*cp++, mallocTraceFile);
    putc(*cp++, mallocTraceFile);

    putw((int) ptr, mallocTraceFile);
    putw(size, mallocTraceFile);

    /* Trace of call addrs will be null-terminated */
    for (pc = 0, ret = whence(); ret; ret = (int *) nextfp(ret))
    {
	pc = thispc(ret);
	putw(pc, mallocTraceFile);
	if (pc < 0x40)
	    break;
    }
    if (pc != 0)
	putw(0, mallocTraceFile);
#endif  macII
#endif	MALLOCTRACE
}

#ifdef	FREEDEBUG
botch(s)
char *s;
{
	printf("assertion botched: %s\n",s);
	fflush(stdout);
	fflush(stderr);
	mallocTraceDisable();
	mallocTraceDone();
	abort();
}
#endif	FREEDEBUG
