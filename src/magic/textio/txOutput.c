/*
 * txOutput.c --
 *
 * 	Handles 'stdout' and 'stderr' output.
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
static char rcsid[] = "$Header: txOutput.c,v 6.0 90/08/28 18:58:18 mayo Exp $";
#endif  not lint

#include <stdio.h>
#ifndef	SYSV
#include <sys/wait.h>
#include <strings.h>
#else
#include <string.h>
#endif SYSV
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "txcommands.h"
#include "textioInt.h"
#include "graphics.h"
#include "paths.h"

#ifndef	NO_VARARGS
#include <varargs.h>
#endif	NO_VARARGS

/* When a pipe has been opened to "more", the following variables
 * keep track of the file and process.  The "TxMoreFile" variable is
 * public so that routines like GrVfprintf() can check it to see if it
 * is NULL or not.  It is guaranteed to be NULL if we don't want to send
 * stuff through more.
 */

FILE * TxMoreFile = NULL;
static int txMorePid;
static bool txPrintFlag = TRUE;


/*
 * ----------------------------------------------------------------------------
 * txFprintfBasic:
 *
 *	Textio's own version of printf.  Not to be used outside of this module.
 *
 * Tricks:
 *	Called with a variable number of arguments -- may not be portable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	text appears on stdout on the text terminal
 *
 * Note: 
 *	Many thanks to Paul Chow at Stanford for getting this to run on
 *	a Pyramid machine.
 * ----------------------------------------------------------------------------
 */ 
#ifndef	NO_VARARGS

 /*VARARGS0*/

void
txFprintfBasic(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    FILE *f;

    va_start(args);
    f = va_arg(args, FILE *);
    fmt = va_arg(args, char *);
    (void) GrVfprintf(f, fmt, args);
    va_end(args);
}
#else	NO_VARARGS

 /*VARARGS0*/

void
txFprintfBasic(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
    char *format;
    FILE *f;
{
    (void) GrFprintf(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, 
		     a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
}
#endif	NO_VARARGS


/*
 * ----------------------------------------------------------------------------
 * TxPrintf:
 *
 *	Magic's own version of printf
 *
 * Tricks:
 *	Called with a variable number of arguments -- may not be portable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	text appears on stdout on the text terminal
 *
 * Note: 
 *	Many thanks to Paul Chow at Stanford for getting this to run on
 *	a Pyramid machine.
 * ----------------------------------------------------------------------------
 */ 
#ifndef	NO_VARARGS

 /*VARARGS0*/

void
TxPrintf(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    FILE *f;

    if (txPrintFlag)
    {
	if (TxMoreFile != NULL) 
	{
	    f = TxMoreFile;
	}
	else
	{
	    f = stdout;
	}

	if (txHavePrompt)
	{
	    TxUnPrompt();
	    va_start(args);
	    fmt = va_arg(args, char *);
	    (void) GrVfprintf(f, fmt, args);
	    va_end(args);
	    TxPrompt();
	}
	else 
	{
	    va_start(args);
	    fmt = va_arg(args, char *);
	    (void) GrVfprintf(f, fmt, args);
	    va_end(args);
	}

	return;
    }
}
#else	NO_VARARGS

 /*VARARGS1*/

void
TxPrintf(format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
char *format;
{
    FILE *f;

    if (txPrintFlag)
    {
	if (TxMoreFile != NULL) 
	{
	    f = TxMoreFile;
	}
	else
	{
	    f = stdout;
	}

	if (txHavePrompt)
	{
	    TxUnPrompt();
	    (void) GrFprintf(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, 
			     a10, a11, a12, a13, a14, a15, a16, a17, a18, a19,
			     a20);
	    TxPrompt();
	}
	else 
	{
	    (void) GrFprintf(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, 
			     a10, a11, a12, a13, a14, a15, a16, a17, a18, a19,
			     a20);
	}

	return;
    }
}
#endif	NO_VARARGS


/*
 * ----------------------------------------------------------------------------
 * TxPrintOn --
 *
 *	Enables TxPrintf() output.
 *
 * Results:
 *	Previous value of flag.
 *
 * ----------------------------------------------------------------------------
 */ 

bool
TxPrintOn()
{
    bool oldValue = txPrintFlag;

    txPrintFlag = TRUE;
    
    return oldValue;
}


/*
 * ----------------------------------------------------------------------------
 * TxPrintOff --
 *
 *	Disables TxPrintf() output.
 *
 * Results:
 *	Previous value of flag.
 *
 * ----------------------------------------------------------------------------
 */ 

bool
TxPrintOff()
{
    bool oldValue = txPrintFlag;

    txPrintFlag = FALSE;

    return oldValue;
}


/*
 * ----------------------------------------------------------------------------
 * TxFlush --
 *
 *	Flush the standard out and error out.
 *
 * Results:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */ 

void
TxFlush()
{
    (void) fflush(stdout);
    (void) fflush(stderr);
}


/*
 * ----------------------------------------------------------------------------
 * TxError:
 *
 *	Magic's own version of printf, but it goes to stderr
 *
 * Tricks:
 *	Called with a variable number of arguments -- may not be portable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	text appears on stderr on the text terminal
 *
 * Note: 
 *	Many thanks to Paul Chow at Stanford for getting this to run on
 *	a Pyramid machine.
 * ----------------------------------------------------------------------------
 */
 /*VARARGS0*/

#ifndef	NO_VARARGS

void
TxError(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    FILE *f;

    (void) fflush(stdout);
    if (TxMoreFile != NULL) 
	f = TxMoreFile;
    else
	f = stderr;
    if (txHavePrompt)
    {
	TxUnPrompt();
	va_start(args);
	fmt = va_arg(args, char *);
	(void) GrVfprintf(f, fmt, args);
	va_end(args);
	TxPrompt();
    }
    else {
	va_start(args);
	fmt = va_arg(args, char *);
	(void) GrVfprintf(f, fmt, args);
	va_end(args);
    }
    (void) fflush(stderr);
}

#else	NO_VARARGS

 /*VARARGS1*/

void
TxError(format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
char *format;
{
    FILE *f;

    (void) fflush(stdout);
    if (TxMoreFile != NULL) 
	f = TxMoreFile;
    else
	f = stderr;
    if (txHavePrompt)
    {
	TxUnPrompt();
	(void) GrFprintf(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9,
	    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
	TxPrompt();
    }
    else {
	(void) GrFprintf(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, 
	    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    }
    (void) fflush(stderr);
}
#endif	NO_VARARGS


/*
 * ----------------------------------------------------------------------------
 *
 * TxUseMore --
 *
 * 	This procedure forks a "more" process and causes TxError and TxPrintf
 *	to send output through it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A file is opened.  When the caller is finished with output,
 *	it must call TxStopMore to clean up the process.
 *
 * ----------------------------------------------------------------------------
 */

void
TxUseMore()
{
    int pipeEnds[2];
    int moreRunning = TRUE;
    static int moreMsg = FALSE;
    char *pagername;
    extern char *getenv();

    ASSERT(TxMoreFile == NULL, "TxUseMore");
    pipe(pipeEnds);
#ifdef	macII
    txMorePid = fork();
#else
    txMorePid = vfork();
#endif

    /* In the child process, move the pipe input to standard input,
     * delete the output stream, and then run "more".
     */

    if (txMorePid == 0)
    {
	char *argv[100];
	close(pipeEnds[1]);
	dup2(pipeEnds[0], 0);
	if ((pagername = rindex(PAGERDIR, '/')) != (char *) 0) 
	    pagername++;
	else
	    pagername = PAGERDIR;
	execl(PAGERDIR, pagername, 0);

	/* Couldn't find more! Leave a flag around and die. */

	moreRunning = FALSE;
	_exit();
    }

    /* This is the parent process.  If the child process couldn't
     * start up "more", print an error and just use standard output.
     */
    
    if (!moreRunning)
    {
	if (!moreMsg)
	{
	    TxError("Couldn't execute %s to filter output.\n", PAGERDIR);
	    moreMsg = TRUE;
	}
	return;
    }

    /* More's set up OK.  Close the input descriptor and make an
     * official FILE for the output descriptor.
     */
    
    close(pipeEnds[0]);
    TxMoreFile = fdopen(pipeEnds[1], "w");
}

/*
 * ----------------------------------------------------------------------------
 *
 * TxStopMore --
 *
 * 	Close the pipe connecting us to a "more" process and wait for
 *	the "more" process to die.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The connection to more is closed.
 *
 * ----------------------------------------------------------------------------
 */

void
TxStopMore()
{
#ifdef SYSV
    int status;
#else
    union wait status;
#endif SYSV

    /* Close the pipe. */
    ASSERT(TxMoreFile != NULL, "TxStopMore");
    fclose(TxMoreFile);
    TxMoreFile = NULL;

    /* Wait until there are no child processes left.  This is a bit
     * of a kludge, and may screw up if other child processes are
     * created for other purposes at the same time, but I can't see
     * any way around it.
     */
    
    while (wait(&status) != txMorePid) /* try again */;
}

#ifdef	NEED_VFPRINTF

int
vfprintf(iop, fmt, ap)
    FILE *iop;
    char *fmt;
    va_list ap;
{
    int len;
#if defined(MIPSEB) && defined(SYSTYPE_BSD43)
    unsigned char localbuf[BUFSIZ];
#else
    char localbuf[BUFSIZ];
#endif

    if (iop->_flag & _IONBF) {
	iop->_flag &= ~_IONBF;
	iop->_ptr = iop->_base = localbuf;
	len = _doprnt(fmt, ap, iop);
	(void) fflush(iop);
	iop->_flag |= _IONBF;
	iop->_base = NULL;
	iop->_bufsiz = 0;
	iop->_cnt = 0;
    } else
	len = _doprnt(fmt, ap, iop);

    return (ferror(iop) ? EOF : len);
}
#endif  NEED_VFPRINTF

