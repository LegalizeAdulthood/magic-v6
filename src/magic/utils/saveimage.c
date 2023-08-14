/*
 * saveimage.c --
 *
 * Function to save an executable copy of the current process's
 * image in a file.
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
static char rcsid[] = "$Header: saveimage.c,v 6.0 90/08/28 19:01:19 mayo Exp $";
#endif  not lint

#include "magic.h"
#include <sys/types.h>
#include <sys/stat.h>

/* This code only works on a VAX or a SUN */
#if	(sun | vax)
#include <a.out.h>
extern off_t lseek();	/* For lint */
#endif 	(sun | vax)


#define	BUFSIZE		8192


/*
 * ----------------------------------------------------------------------------
 *
 * SaveImage --
 *
 * Save a load image of the current process.
 * The argument 'origFileName' is the name of the file from
 * which this one was exec'd, and is used to extract the a.out
 * header and symbol table info.  The argument 'saveFileName'
 * is the name of the file that will contain the saved image.
 * It must not already exist.
 *
 * Results:
 *	TRUE if the image were saved successfully, FALSE if
 *	an error were encountered.
 *
 * Side effects:
 *	Saves an image of the current process in the file
 *	named by 'saveFileName'.
 *
 * WARNINGS:
 *	Open file descriptors other than stdin, stdout, and
 *	stderr cannot be preserved across an image save.
 *	Clients should make their own arrangements for re-opening
 *	any files when the image is restarted.
 *
 * ----------------------------------------------------------------------------
 */

bool
SaveImage(origFileName, saveFileName)
    char *origFileName;
    char *saveFileName;
{
#if	(sun | vax)
    int origFd = -1, saveFd = -1;
    char *startData, *endData, *endRound, *sbrk();
    struct exec oldHdr, newHdr;
    struct stat oldStat;
    int n, pageSize;

    if ((origFd = open(origFileName, 0)) < 0)
    {
	perror(origFileName);
	printf("Cannot open a.out file\n");
	goto bad;
    }
    if (fstat(origFd, &oldStat) < 0)
    {
	perror(origFileName);
	printf("Cannot stat a.out file\n");
	goto bad;
    }

    if (access(saveFileName, 0) == 0)
    {
	printf("Save file %s already exists.\n", saveFileName);
	goto bad;
    }

    /*
     * Read the a.out header from the original file.
     * Check to see that it makes sense.
     */
    if (read(origFd, (char *) &oldHdr, sizeof oldHdr) != sizeof oldHdr)
    {
	printf("Can't read a.out header from %s\n", origFileName);
	goto bad;
    }
    if (N_BADMAG(oldHdr))
    {
	printf("File %s has a bad magic number (%o)\n",
			origFileName, oldHdr.a_magic);
	goto bad;
    }
    if (oldHdr.a_magic != ZMAGIC)
    {
	printf("File %s is not demand-paged\n", origFileName);
	goto bad;
    }

    /*
     * Open the output file.
     */
    if ((saveFd = creat(saveFileName, 0777)) < 0)
    {
	perror(saveFileName);
	printf("Cannot create save file.\n");
	goto bad;
    }

    /*
     * Find out how far the data segment extends.
     * We will want to generate a load image that has a data
     * segment covering all of data memory, and no bss segment.
     * Make sure that the data size gets rounded up to a page
     * multiple if needed.
     */
    newHdr = oldHdr;
    endData = sbrk(0);
    pageSize = getpagesize();
    n = ((int) endData) + pageSize - 1;
    n /= pageSize;
    n *= pageSize;
    endRound = (char *) n;
    if (endRound > endData)
	endData = sbrk(endRound - endData);

#ifdef	vax
    startData = (char *) oldHdr.a_text;
#else
#ifdef	sun
    startData = (char *) N_DATADDR(oldHdr);
#else
    startData = You_are_not_on_a_Sun_or_a_VAX;
#endif	sun
#endif	vax
    newHdr.a_data = endData - startData;
    newHdr.a_bss = 0;

    /*
     * Generate the new load image.
     * First, the header plus enough pad to extend up to N_TXTOFF.
     */
    write(saveFd, (char *) &newHdr, sizeof newHdr);
    if (!savePad(saveFd, N_TXTOFF(oldHdr) - sizeof newHdr))
	goto bad;

    /*
     * Next output the text.  This may as well come from the disk
     * file, since text is read-only and we don't want to bother
     * page-faulting our whole text image in.
     *
     * We assume that the text size is a multiple of the page size.
     */
    lseek(origFd, (long) N_TXTOFF(oldHdr), 0);
    if (!saveCopyFile(origFd, saveFd, (long) oldHdr.a_text))
    {
	printf("Error while copying text segment.\n");
	goto bad;
    }

    /*
     * Copy the data segment.
     * This has to come from our own address space.
     */
    if (write(saveFd, startData, (int) newHdr.a_data) != newHdr.a_data)
    {
	perror("Write error");
	printf("Error while copying data segment\n");
	goto bad;
    }

    /*
     * Copy the symbol table and everything else.
     * This takes us to the end of the original file.
     */
    lseek(origFd, (long) N_SYMOFF(oldHdr), 0);
    if (!saveCopyFile(origFd, saveFd,
		(long) (oldStat.st_size - N_SYMOFF(oldHdr))))
    {
	printf("Error while copying symbol table.\n");
	goto bad;
    }
    (void) close(origFd);
    (void) close(saveFd);
    return (TRUE);

bad:
    if (origFd >= 0) (void) close(origFd);
    if (saveFd >= 0) (void) close(saveFd);
#endif sun||vax
    return (FALSE);
}

saveCopyFile(inFd, outFd, nbytes)
    int inFd, outFd;
    long nbytes;
{
    char buf[BUFSIZE];
    int nread, ntoread;

    while (nbytes > 0)
    {
	ntoread = nbytes;
	if (ntoread > sizeof buf) ntoread = sizeof buf;
	if ((nread = read(inFd, buf, ntoread)) != ntoread)
	{
	    perror("Read error");
	    return (FALSE);
	}
	if (write(outFd, buf, nread) != nread)
	{
	    perror("Write error");
	    return (FALSE);
	}
	nbytes -= nread;
    }

    return (TRUE);
}

bool
savePad(outFd, nbytes)
    int outFd;
    int nbytes;
{
    char buf[BUFSIZE];
    int nzero;

    nzero = (nbytes > sizeof buf) ? sizeof buf : nbytes;
    bzero(buf, nzero);
    while (nbytes > 0)
    {
	nzero = (nbytes > sizeof buf) ? sizeof buf : nbytes;
	if (write(outFd, buf, nzero) != nzero)
	{
	    perror("Write error while padding");
	    return (FALSE);
	}
	nbytes -= nzero;
    }

    return (TRUE);
}
