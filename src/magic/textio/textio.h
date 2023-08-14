/*
 * textio.h --
 *
 * Routines in the textio module
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
 * Needs:
 *	stdio.h
 *	sgtty.h   <--- NOTE!
 *	magic.h
 *
 * rcsid $Header: textio.h,v 6.0 90/08/28 18:58:07 mayo Exp $
 */

#define _TEXTIO

#ifndef _MAGIC
	err = Need_to_include_magic_header
#endif

/* printing procedures */
extern void TxPrintf();
extern bool TxPrintOn();  	/* enables TxPrintf output */
extern bool TxPrintOff();	/* disables TxPrintf output */
extern void TxError();
extern void TxFlush();
extern void TxVisChar();
extern void TxUseMore();
extern void TxStopMore();

/* input procedures */
extern char *TxGetLinePrompt();
extern char *TxGetLine();
extern int TxGetChar();

/* prompting procedures */
extern void TxSetPrompt();
extern void TxPrompt();
extern void TxPromptOnNewLine();
extern void TxUnPrompt();
extern void TxRestorePrompt();
extern void TxReprint();

/* terminal-state procedures */
extern void TxSetTerminal();
extern void TxResetTerminal();
extern char TxEOFChar;			/* The current EOF character */
extern char TxInterruptChar;		/* The current interrupt character */

/* command procedures */
extern void TxDispatch();

/* variables that tell if stdin and stdout are to a terminal */
extern bool TxStdinIsatty;
extern bool TxStdoutIsatty;
#define TxInteractive	(TxStdinIsatty && TxStdoutIsatty)

/* Misc procs */
void TxInit();
