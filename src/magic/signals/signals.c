/*
 * signals.c --
 *
 * Handles signals, such as stop, start, interrupt.
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
static char rcsid[]="$Header: signals.c,v 6.0 90/08/28 18:57:09 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <signal.h>
#include <sgtty.h>
#include <fcntl.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "signals.h"
#include "graphics.h"


#ifndef FASYNC
#  define	FASYNC	00100	/* kludge for SUN2s */
#endif

/* macII's support BSD4.2 signals, so turn off the SYSV flag for this module */
#ifdef macII
#undef SYSV
#endif

/* Specially imported */
extern Void DBPanicSave();

/*
 * Global data structures
 *
 */

/* becomes true when we get an interrupt */
global bool SigInterruptPending = FALSE;


/* Becomes true when IO is possible on one of the files passed to SigWatchFile.
 * Spurious signals are sometimes generated -- use select() to make
 * sure that what you want is really there.
 */
global bool SigIOReady = FALSE;


/* If true, we will set SigInterruptPending whenever we set SigIOReady. */
global bool SigInterruptOnSigIO;

/*
 * Set to true when we recieve a SIGWINCH/SIGWINDOW signal 
 * (indicating that a window has changed size or otherwise needs attention).
 */
global bool SigGotSigWinch = FALSE;


/* 
 * Local data structures
 *
 */
static bool sigInterruptRecieved = FALSE;
static int sigNumDisables = 0;


/* The following array is used to hold arguments to sigvec calls. */

static struct sigvec vec = {0, 0, 0};


extern bool mainDebug;


/*---------------------------------------------------------
 * sigOnStop:
 *	This procedure handles stop signals.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The text display is reset, and we stop
 *---------------------------------------------------------
 */

sigRetVal
sigOnStop()
{
    /* fix things up */
    TxResetTerminal();
    GrStop();

    /* restore the default action and resend the signal */

#ifndef SYSV
    vec.sv_handler = SIG_DFL;
    sigvec(SIGTSTP, &vec, 0);		/* stop on stops */
    (void) sigsetmask(0);
    (void) kill(getpid(), SIGTSTP); 
#else
# ifdef SIGTSTP
    signal(SIGTSTP, SIG_DFL);		/* stop on stops */
    (void) kill(getpid(), SIGTSTP); 
# endif
#endif
    
    /* -- we stop here -- */

    /* NOTE:  The following code really belongs in a routine that is
     * called in response to a SIGCONT signal, but it doesn't seem to
     * work that way.  Maybe there is a Unix bug with this???
     */

    GrResume();
    TxSetTerminal();
    TxReprint();

    /* catch future stops now that we have finished resuming */

#ifndef SYSV
    vec.sv_handler = sigOnStop;
    sigvec(SIGTSTP, &vec, 0);
#else
# ifdef SIGTSTP
    signal(SIGTSTP, sigOnStop);
# endif
#endif 
}


/*---------------------------------------------------------
 * sigEnableInterrupts:
 *	This procedure reenables our handling of interrupts.
 *
 * Results:	None.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */

void
SigEnableInterrupts()
{
    sigNumDisables--;
    if (sigNumDisables == 0)
    {
	SigInterruptPending = sigInterruptRecieved;
	sigInterruptRecieved = FALSE;
    }
}


/*---------------------------------------------------------
 * sigDisableInterrupts:
 *	This procedure disables our handling of interrupts.
 *
 * Results:	None.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */

void
SigDisableInterrupts()
{
    if (sigNumDisables == 0)
    {
	sigInterruptRecieved = SigInterruptPending;
	SigInterruptPending = FALSE;
    }
    sigNumDisables++;
}


/*
 * ----------------------------------------------------------------------------
 * SigWatchFile --
 *
 *	Take interrupts on a given IO stream.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	SigIOReady will be set when the IO stream becomes ready.  It is
 *	the responsibility of the client to clear that flag when needed.
 * ----------------------------------------------------------------------------
 */

void
SigWatchFile(filenum, filename)
    int filenum;		/* A file descriptor number */
    char *filename;		/* Used to recognize special files that
				 * don't support a full range of fcntl
				 * calls (such as windows: /dev/winXX).
				 */
{
    int flags;
    bool iswindow;

    iswindow = (filename && (strncmp(filename, "/dev/win", 8) == 0));

    flags = fcntl(filenum, F_GETFL, 0);
    if (flags == -1)
    {
	perror("(Magic) SigWatchFile1");
	return;
    }

    if (!mainDebug)
    {
	/* turn on FASYNC */
#ifndef SYSV
#ifdef F_SETOWN
	if (!iswindow)
	{
	    if (fcntl(filenum, F_SETOWN, -getpid()) == -1)
		perror("(Magic) SigWatchFile2"); 
	}
#endif
#endif SYSV
#ifdef FASYNC
	if (fcntl(filenum, F_SETFL, flags | FASYNC) == -1) 
	    perror("(Magic) SigWatchFile3");
#else
# ifdef FIOASYNC
	flags = 1;
	if (ioctl(filenum, FIOASYNC, &flags) == -1) 
	    perror("(Magic) SigWatchFile3a");
# endif
#endif
    }
    else
    {
#ifdef FASYNC
	/* turn off FASYNC */
	if (fcntl(filenum, F_SETFL, flags & (~FASYNC)) == -1) 
	    perror("(Magic) SigWatchFile4");
#else
# ifdef FIOASYNC
	flags = 0;
	if (ioctl(filenum, FIOASYNC, &flags) == -1) 
	    perror("(Magic) SigWatchFile3b");
# endif
#endif
    }
}


/*
 * ----------------------------------------------------------------------------
 * SigUnWatchFile --
 *
 *	Do not take interrupts on a given IO stream.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	SigIOReady will be not set when the IO stream becomes ready.  
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

void
SigUnWatchFile(filenum, filename)
    int filenum;		/* A file descriptor number */
    char *filename;		/* Used to recognize special files that
				 * don't support a full range of fcntl
				 * calls (such as windows: /dev/winXX).
				 */
{
    int flags;

    flags = fcntl(filenum, F_GETFL, 0);
    if (flags == -1)
    {
	perror("(Magic) SigUnWatchFile1");
	return;
    }

#ifdef FASYNC
    /* turn off FASYNC */
    if (fcntl(filenum, F_SETFL, flags & (~FASYNC)) == -1) 
	perror("(Magic) SigUnWatchFile4");
#else
# ifdef FIOASYNC
    flags = 0;
    if (ioctl(filenum, FIOASYNC, &flags) == -1) 
	perror("(Magic) SigWatchFile3");
# endif
#endif
}


/*---------------------------------------------------------
 * sigOnInterrupt:
 *	This procedure handles interupt signals.
 *
 * Results:	None.
 *
 * Side Effects:
 *    A global flag is set
 *---------------------------------------------------------
 */

sigRetVal
sigOnInterrupt()
{
    if (sigNumDisables != 0)
	sigInterruptRecieved = TRUE;
    else
	SigInterruptPending = TRUE;
#ifdef SYSV
    signal(SIGINT, sigOnInterrupt);
#endif
}


/*
 * ----------------------------------------------------------------------------
 * sigOnTerm:
 *
 *	Catch the terminate (SIGTERM) signal.
 *	Force all modified cells to be written to disk (in new files,
 *	of course).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes cells out to disk (by calling DBPanicSave()).
 *	Exits.
 * ----------------------------------------------------------------------------
 */

sigRetVal
sigOnTerm()
{
    DBPanicSave();
    exit (1);
}



/*
 * ----------------------------------------------------------------------------
 * sigOnWinch --
 *
 *	A window has changed size or otherwise needs attention.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets a global flag.
 * ----------------------------------------------------------------------------
 */

sigRetVal
sigOnWinch()
{
    SigGotSigWinch = TRUE;
}

/*
 * ----------------------------------------------------------------------------
 * sigIO --
 *
 *	Some IO device is ready (probably the keyboard or the mouse).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets a global flag.
 * ----------------------------------------------------------------------------
 */

sigRetVal
sigIO()
{
    SigIOReady = TRUE;
    if (SigInterruptOnSigIO) sigOnInterrupt();
}

/*
 * ----------------------------------------------------------------------------
 *
 * sigCrash --
 *
 *	Something when wrong, reset the terminal and die.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	We die.
 *
 * ----------------------------------------------------------------------------
 */

sigRetVal
sigCrash(signum)
    int signum;
{
    static int magicNumber = 1239987;
    char *msg;
    extern bool AbortFatal;

    if (magicNumber == 1239987) {
	/* Things aren't screwed up that badly, try to reset the terminal */
	magicNumber = 0;
	switch (signum) {
	    case SIGILL: {msg = "Illegal Instruction"; break;};
	    case SIGTRAP: {msg = "Instruction Trap"; break;};
	    case SIGIOT: {msg = "IO Trap"; break;};
	    case SIGEMT: {msg = "EMT Trap"; break;};
	    case SIGFPE: {msg = "Floating Point Exception"; break;};
	    case SIGBUS: {msg = "Bus Error"; break;};
	    case SIGSEGV: {msg = "Segmentation Violation"; break;};
	    case SIGSYS: {msg = "Bad System Call"; break;};
	    default: {msg = "Unknown signal"; break;};
	};
	strcpy(AbortMessage, msg);
	AbortFatal = TRUE;
	niceabort();
	TxResetTerminal();
    }

    /* Crash & burn */
    magicNumber = 0;
    exit(12);
}


/*
 * ----------------------------------------------------------------------------
 * SigInit:
 *
 *	Set up signal handling for all signals.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signal handling is set up.
 * ----------------------------------------------------------------------------
 */

void
SigInit()
{
    /* catch signals */

#ifndef SYSV
    vec.sv_handler = sigOnStop;
    sigvec(SIGTSTP, &vec, 0);
    vec.sv_handler = sigOnInterrupt;
    sigvec(SIGINT, &vec, 0);
    vec.sv_handler = sigOnTerm;
    sigvec(SIGTERM, &vec, 0);
# ifdef SIGWINCH
    vec.sv_handler = sigOnWinch;
    sigvec(SIGWINCH, &vec, 0);
# endif 
# ifdef SIGWINDOW
    vec.sv_handler = sigOnWinch;
    sigvec(SIGWINDOW, &vec, 0);
# endif sun

#else
# ifdef SIGTSTP
    signal(SIGTSTP, sigOnStop);
# endif
    signal(SIGINT, sigOnInterrupt);
    signal(SIGTERM, sigOnTerm);
# ifdef SIGWINCH
    signal(SIGWINCH, sigOnWinch);
# endif
# ifdef SIGWINDOW
    signal(SIGWINDOW, sigOnWinch);
# endif
#endif SYSV

    if (!mainDebug)
    {
#ifndef SYSV
	vec.sv_handler = sigIO;
	sigvec(SIGIO, &vec, 0);
#else
	signal(SIGIO, sigIO);
#endif SYSV
    }

    /* disastrous conditions */
#ifdef	FANCY_ABORT
    if (!mainDebug)
    {
	vec.sv_handler = sigCrash;
	sigvec(SIGILL, &vec, 0);
	sigvec(SIGTRAP, &vec, 0);
	sigvec(SIGIOT, &vec, 0);
	sigvec(SIGEMT, &vec, 0);
	sigvec(SIGFPE, &vec, 0);
	sigvec(SIGBUS, &vec, 0);
	sigvec(SIGSEGV, &vec, 0);
	sigvec(SIGSYS, &vec, 0);
    };
#endif FANCY_ABORT

    /* signals to ignore */
    if (!mainDebug)
    {
	vec.sv_handler = SIG_IGN;
	signal(SIGALRM,SIG_IGN);
#ifndef SYSV
	sigvec(SIGPIPE, &vec, 0);
#else
	signal(SIGPIPE,SIG_IGN);
#endif SYSV
    }

    sigsetmask(0);
}
