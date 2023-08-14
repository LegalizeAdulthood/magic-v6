/*
 * signals.h --
 *
 * Routines to signals, such as handle keyboard interrupts
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
 *
 * rcsid[]="$Header: signals.h,v 6.0 90/08/28 18:57:11 mayo Exp $";
 */

#define _SIGNAL
#ifndef	_MAGIC
	err = Need_to_include_magic_h
#endif

/* data structures */
extern bool SigInterruptPending;
extern bool SigIOReady;
extern bool SigInterruptOnSigIO;
extern bool SigGotSigWinch;

/* procedures */
extern void SigInit();
extern void SigDisableInterrupts();
extern void SigEnableInterrupts();
extern void SigWatchFile();
extern void SigUnWatchFile();

/* Some machines have signal handlers returning an int, while other machines
 * have it returning a void.  If you have a machine that requires ints put 
 * it in the list of machines in misc/magic.h.
 */
#ifdef	SIG_RETURNS_INT
#define	sigRetVal	int
#else
#define	sigRetVal	void
#endif
