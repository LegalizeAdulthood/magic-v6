/*
 * prleak -- program to analyze traced malloc's/free's
 *
 * This is as described in ACM SIGPLAN Notices, Vol 17, #5 (May 82)
 * by Barach and Taenzer.
 *
 * Usage:
 *	prleak [-a] [-l] [-d] [binfile [tracefile]]
 *
 * Uses the namelist contained in binfile (or a.out if no binfile is
 * specified) to decode the addresses in tracefile (or malloc.out
 * if no tracefile is given).
 *
 * Copyright (C) 1985 Walter S. Scott
 */

#ifndef lint
static char rcsid[]="$Header: prleak.c,v 4.7 89/04/03 15:59:46 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#include <stab.h>
#include <signal.h>
#include "magic.h"
#include "malloc.h"
#include "hash.h"

/* ---------------- Hash table of active allocations ------------------ */

/*
 * Internal active allocation record representation
 */

struct active {
    int			 a_size;	/* Size of allocation */
    long		 a_seek;	/* Trace file index of backtrace info */
};

/* --------------------- Miscellaneous globals ------------------------ */

static long	 totalSize = 0;
static long	 nAllocs = 0;
static FILE	*traceFile;
static FILE	*dupFile;
static FILE	*nlistFile;
static char	 dupName[] = "/tmp/prldaXXXXX";
static char	 leakName[] = "/tmp/prllaXXXXX";
static char	 allName[] = "/tmp/prlaaXXXXX";

long ftell();
long getw();
FILE *uopen();
int cleanup();

int aflag = 0;	/* -a: print all calls to malloc/free */
int dflag = 0;	/* -d: print duplicate frees */
int lflag = 0;	/* -l: print leaks */
int vflag = 0;	/* -v: verbose */
int Vflag = 0;	/* -V: very verbose */

HashTable hashTable;

main(argc, argv)
char	*argv[];
{
    static char iname[] = "a.out";
    static char tname[] = "malloc.out";
    char *name = iname;
    char *tracename = tname;
    char **av = argv+1, *arg;
    int ac = argc-1;
    long avgSize;

    while (ac--)
    {
	arg = *av++;
	if (arg[0] == '-')
	{
	    while (*++arg)
		switch (*arg)
		{
		    case 'a':	/* -a: print all calls to malloc/free */
			aflag++;
			break;
		    case 'l':	/* -l: print leaks only */
			lflag++;
			break;
		    case 'd':	/* -d: print duplicate frees only */
			dflag++;
			break;
		    case 'v':	/* -v: verbose */
			vflag++;
			break;
		    case 'V':	/* -V: verbose */
			Vflag++;
			break;
		    default:
			printf("Unrecognized switch '-%c'\n", *arg);
			goto usage;
		}
	}
	else
	{
	    if (name != iname)
	    {
		if (tracename != tname)
		    goto usage;
		tracename = arg;
	    }
	    else name = arg;
	}
    }

    if (!aflag && !dflag && !lflag)
    {
	dflag++;
	lflag++;
    }

    signal(SIGQUIT, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGHUP, cleanup);
    nlistFile = uopen(name, "r");
    traceFile = uopen(tracename, "r");
    dupFile = uopen(mktemp(dupName), "w");

    HashInit(&hashTable, 1024, HT_WORDKEYS);
    while (process());
    avgSize = nAllocs ? (totalSize / (long) nAllocs) : 0;
    printf("Average allocation size = %ld bytes\n\n", avgSize);

    /*
     * Dump all remaining active allocations to file.  Then
     * initialize internal copy of namelist and print list
     * of the allocations we dumped to the file.
     */
    prleaks(lflag);
    if (vflag)
	printf("===== Done processing leaks =====\n");

    /* If we are to print all calls to malloc/free, do so here */
    if (aflag)
    {
	prall();
	if (vflag)
	    printf("===== Done processing all calls to malloc/free =====\n");
    }

    /*
     * Now print the list of duplicate frees.  The internal
     * copy of namelist has already been built by prleaks().
     */
    fclose(dupFile);
    if (dflag)
	prlist(dupName,
	    "\n\n--------- ------\nDuplicate frees:\n--------- ------\n\n", 0);
    unlink(dupName);
    exit (0);

usage:
    printf("Usage: %s [-dla] [file]\n", argv[0]);
    exit (1);
}

/*
 * process() -- process the record of allocations
 */

process()
{
    register FILE *fd = traceFile;
    register unsigned addr;
    unsigned char *cp;
    int size, w, c;
    short flag;
    long seekp;

    /* Process the record */
    cp = (unsigned char *) &flag;
    if ((c = getc(fd)) == EOF) return 0;
    *cp++ = c;
    if ((c = getc(fd)) == EOF) return 0;
    *cp = c;

    /* Special skip marker */
    if (flag == 2)
	return 1;

    addr = getw(fd);
    size = getw(fd);
    if (Vflag)
	printf("%s 0x%x/%d\n", flag ? "malloc" : "free", addr, size);

    if (flag) {
	doalloc(addr, size, ftell(fd));
	totalSize += size;
	nAllocs++;
    } else htdelete(addr);

    /* Skip to end of backtrace addresses */
    while (!feof(fd) && (w = getw(fd)) != NULL)
	if (Vflag)
	    printf("    skip 0x%x\n", w);
    return 1;
}

/*
 * doalloc(addr, size, seekp) -- record alloc of addr
 */

doalloc(addr, size, seekp)
    register unsigned addr;
    register int size;
    long seekp;
{
    register struct active *ap;
    register HashEntry *he;

    he = HashFind(&hashTable, (char *) addr);
    if (HashGetValue(he))
    {
	printf("Dup alloc of 0x%x (%d bytes)\n", addr, size);
	return;
    }

    MALLOC(struct active *, ap, sizeof (struct active));
    ap->a_size = size;
    ap->a_seek = seekp;
    HashSetValue(he, (char *) ap);
}

/*
 * htdelete(addr) -- remove record for addr from table
 */

htdelete(addr)
register unsigned addr;
{
    register struct active *ap;
    register HashEntry *he;

    he = HashLookOnly(&hashTable, (char *) addr);
    if (he && (ap = (struct active *) HashGetValue(he)))
    {
	FREE((char *) ap);
	HashSetValue(he, (char *) NULL);
	return;
    }

    putw(addr, dupFile);		/* Address freed */
    putw(0, dupFile);			/* Size */
    putw(ftell(traceFile), dupFile);	/* Offset of backtrace information */
}

/* ------------------- Output of trace information -------------------- */

/*
 * prleaks() -- dump contents of hash table, showing unfreed allocs
 *
 * Frees all entries in the hash table.
 */

prleaks(lflag)
    int lflag;	/* If nonzero, actually print list of leaks */
{
    HashSearch hs;
    register struct active *ap;
    register HashEntry *he;
    register FILE *fd;

    /*
     * Create file of all leaks
     */
    fd = uopen(mktemp(leakName), "w");
    HashStartSearch(&hs);
    while (he = HashNext(&hashTable, &hs))
    {
	if (ap = (struct active *) HashGetValue(he))
	{
	    putw(he->h_key.h_ptr, fd);
	    putw(ap->a_size, fd);
	    putw(ap->a_seek, fd);
	    FREE((char *) ap);
	}
    }
    fclose(fd);
    HashKill(&hashTable);
    if (vflag)
	printf("===== Generated leak file =====\n");

    /* Initialize internal copy of namelist */
    nlinit();

    /* Now print the file */
    if (lflag)
	prlist(leakName, "\n\n------\nLeaks:\n------\n\n", 1);
    unlink(leakName);
}

/*
 * prall --
 *
 * Print all calls to malloc/free
 */

prall()
{
    int ismalloc;
    register FILE *trace, *all;
    long ftell();

    rewind(traceFile);
    trace = traceFile;
    all = uopen(mktemp(allName), "w");
    while ((ismalloc = getc(trace)) != EOF)
    {
	putc(ismalloc, all);
	putw(getw(trace), all);
	putw(getw(trace), all);
	putw(ftell(trace), all);
	while (!feof(trace) && getw(trace) != NULL);
    }
    fclose(all);

    /* Now print the file */
    printf("\n\n------------\nAllocs/frees:\n------------\n\n");
    all = uopen(allName, "r");
    while ((ismalloc = getc(all)) != EOF)
    {
	unsigned addr = getw(all);
	int size = getw(all);
	long seekp = getw(all);

	printf("\n");
	prcall(addr, size, seekp, 1, ismalloc);
    }
    fclose(all);
    unlink(allName);
}

/*
 * prlist(name, banner, flag) -- print contents of file `name'
 *
 * The argument `banner' is printed before the first item.
 * `Flag' is nonzero if sizes should be printed, 0 otherwise.
 */

prlist(name, banner, flag)
char *name, *banner;
int flag;
{
    register FILE *fd;
    register unsigned addr;
    register int printed = 0;
    int size;
    long seekp;

    fd = uopen(name, "r");

    while (!feof(fd)) {
	addr = getw(fd);
	if (feof(fd))
	    break;
	size = getw(fd);
	seekp = getw(fd);
	if (!printed) {
	    printed = 1;
	    printf(banner);
	}
	putchar('\n');
	prcall(addr, size, seekp, flag, 1);
    }
}

/*
 * prcall(addr, size, seekp, flag, ismalloc) -- print call plus backtrace
 *
 * If `flag' is nonzero, the size of the allocation is printed; otherwise,
 * it is omitted.  If `ismalloc' is nonzero, we print the entry as
 * though it is a call to mallocMagic(); otherwise, we print as though it
 * is a call to freeMagic().
 */

prcall(addr, size, seekp, flag, ismalloc)
    unsigned addr;
    int size;
    long seekp;
    int flag;
    int ismalloc;	/* 1 if malloc, 0 if free */
{
    register FILE *tfd = traceFile;
    register unsigned back;
    char *prefix = "at";

    printf("%s 0x%x", ismalloc ? "mallocMagic" : "freeMagic",  addr);
    if (flag && ismalloc)
	printf("\t[%d bytes]", size);
    putchar('\n');

    /* Print backtrace */
    fseek(tfd, seekp, 0);
    if (feof(tfd))
	return;

    while (!feof(tfd) && (back = getw(tfd)) != NULL)
    {
	printf("\t%s ", prefix);
	praddr(back);
	putchar('\n');
	prefix = "called from";
    }
}

/* --------------------- Name list manipulation ----------------------- */

static int		 noNlist = 1;	/* Nonzero if no name list */
static struct nlist	*nameList, *nameEnd;

/*
 * nlinit() -- initialize namelist
 *
 * Reads the namelist in from file nlistFile, which is assumed to
 * be open to an executable file with a symbol table.
 */

nlinit()
{
    int compare();
    FILE *fi = nlistFile;
    struct exec ahdr;
    struct stat stb;
    struct nlist sym;
    int n, t, strsiz, nsyms, j;
    char *strp;
    off_t symoff;

    nameList = (struct nlist *) NULL;
    fstat(fileno(fi), &stb);
    fread((char *) &ahdr, sizeof ahdr, 1, fi);
    if (N_BADMAG(ahdr)) {
	printf("Bad format detected in object file\n");
	return;
    }

    symoff = N_SYMOFF(ahdr) - sizeof (struct exec);
    fseek(fi, symoff, 1);
    t = n = ahdr.a_syms / sizeof(struct nlist);
    if (n == 0)
    {
	printf("No name list\n");
	return;
    }

    if (N_STROFF(ahdr) + sizeof (off_t) > stb.st_size)
    {
	printf("old format .o (no string table) or truncated file\n");
	return;
    }

    if (vflag)
	printf("Total of %d symbols\n", t);

    nsyms = 0;
    strp = (char *) NULL;
    nameList = (struct nlist *) mallocMagic((t+1) * sizeof (struct nlist));
    if (nameList == NULL)
    {
	printf("out of memory\n");
	return;
    }

    while (--n >= 0)
    {
	fread((char *)&sym, 1, sizeof(sym), fi);
	if (vflag && ((t - n) % 200 == 0))
	    printf("%d/%d\n", nsyms, t-n);
	if ((sym.n_type&N_EXT)==0)
	    continue;
	if (sym.n_type&N_STAB)
	    continue;
	if ((sym.n_type&N_TYPE) != N_TEXT)
	    continue;
	nameList[nsyms++] = sym;
    }

    if (fread((char *)&strsiz, sizeof(strsiz), 1, fi) != 1)
    {
	printf("No string table (old format .o?)");
	return;
    }

    if (vflag)
	printf("Allocating %d bytes for string table\n", strsiz);

    strp = (char *) mallocMagic(strsiz);
    if (strp == NULL)
    {
	printf("Ran out of memory\n");
	return;
    }

    if (fread(strp + sizeof(strsiz), strsiz - sizeof(strsiz), 1, fi) != 1)
    {
	printf("Error reading string table\n");
	return;
    }

    for (j = 0; j < nsyms; j++)
	if (nameList[j].n_un.n_strx)
	    nameList[j].n_un.n_name = nameList[j].n_un.n_strx + strp;
	else
	    nameList[j].n_un.n_name = "";

    if (vflag)
	printf("Sorting %d symbols\n", nsyms);

    qsort(nameList, nsyms, sizeof(struct nlist), compare);
    nameEnd = &nameList[nsyms];
    noNlist = 0;
}

/*
 * compare() -- compare addresses
 */

compare(n1, n2)
struct nlist *n1, *n2;
{
    register unsigned n1v = n1->n_value;
    register unsigned n2v = n2->n_value;

    if (n1v > n2v)
	return (1);
    else if (n1v < n2v)
	return (-1);
    else
	return (0);
}

/*
 * praddr(addr) -- print address in nice format
 */

praddr(addr)
register unsigned addr;
{
    register res;
    register struct nlist *srchp;
    struct nlist *startp, *endp;

    if (noNlist) {
	printf("0x%x", addr);
	return;
    }

    /*
     * Binary search for address.  We wish to find the greatest address
     * in the symbol table that is still less than `addr'.
     */

    startp = nameList;
    endp = nameEnd;
    while (startp <= endp) {
	srchp = startp + ((int) (endp - startp) / 2);
	res = addr - srchp->n_value;
	if (res == 0)
	    goto found;
	if (res < 0)
	    endp = srchp - 1;
	else
	    startp = srchp + 1;
    }

    /*
     * If the address we are looking for is beyond the symbol's
     * value, back up to the previous symbol, making sure that
     * we don't back up over the beginning of the table.
     */

    res = addr - srchp->n_value;
    if (res < 0) {
	if (srchp <= nameList) {
	    printf("0x%x", addr);
	    return;
	}
	res = addr - (--srchp)->n_value;
    }

found:
    printf("%s", (srchp)->n_un.n_name);
    if (res)
	printf("+0x%x", res);
}

/* ---------------------- Miscellaneous I/O --------------------------- */

/*
 * uopen(name, mode) -- fopen that complains if error
 */

FILE *
uopen(name, mode)
char *name, *mode;
{
    register FILE *ret = fopen(name, mode);

    if (ret == NULL) {
	perror(name);
	exit (1);
    }

    return (ret);
}

cleanup()
{
    unlink(dupName);
    unlink(leakName);
    exit (1);
}

