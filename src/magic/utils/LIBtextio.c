/*
 * LIBtextio.c --
 *
 * File that only goes in libmagicutils.a to define procedures
 * referenced from textio that might not be defined elsewhere.
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

#ifndef	lint
static char rcsid[] = "$Header: LIBtextio.c,v 6.0 90/08/28 19:00:24 mayo Exp $";
#endif	not lint

#include <stdio.h>
#include <varargs.h>

/*
 * ----------------------------------------------------------------------------
 *
 * TxGetLine --
 *
 * Like fgets() from standard input.
 *
 * Results:
 *	Returns a pointer to 'buf' if successful, or NULL on EOF.
 *
 * Side effects:
 *	Fills in 'buf' with the next line from the standard input.
 *
 * ----------------------------------------------------------------------------
 */

char *
TxGetLine(buf, size)
    char *buf;
    int size;
{
    return (fgets(buf, size, stdin));
}

/*
 * ----------------------------------------------------------------------------
 *
 * TxError --
 *
 * Like fprintf(stderr, ...)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the standard error.
 *
 * ----------------------------------------------------------------------------
 */

    /*VARARGS*/
TxError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list ap;
 
    (void) fflush(stdout);
    (void) fflush(stderr);
    va_start(ap);
    _doprnt(fmt, ap, stderr);
    va_end(ap);
    (void) fflush(stderr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * TxPrintf --
 *
 * Like printf(...)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the standard output.
 *
 * ----------------------------------------------------------------------------
 */

    /*VARARGS*/
TxPrintf(fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list ap;
 
    (void) fflush(stderr);
    (void) fflush(stdout);
    va_start(ap);
    _doprnt(fmt, ap, stdout);
    va_end(ap);
    (void) fflush(stdout);
}

/*
 * ----------------------------------------------------------------------------
 *
 * TxFlush --
 *
 * Like fflush(stderr), fflush(stdout).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

TxFlush()
{
    (void) fflush(stderr);
    (void) fflush(stdout);
}
