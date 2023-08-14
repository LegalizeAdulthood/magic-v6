/*
 * textioInt.h --
 *
 * INTERNAL definitions for the textio module
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
 * rcsid $Header: textioInt.h,v 6.0 90/08/28 18:58:08 mayo Exp $
 */

#define _TEXTIOINT

#ifndef _MAGIC
	err = Need_to_include_magic_header
#endif
#ifndef _TEXTIO
	err = Need_to_include_textio_header
#endif
#ifndef _TXCOMMANDS
	err = Need_to_include_txcommands_header
#endif

extern bool txHavePrompt;

typedef struct {
    int tx_fdmask;		/* A mask of the file descriptors for this 
				 * device.
				 */
    Void (*tx_inputProc)(); 	/* A procedure that fetches events and stores
				 * them in the input queue via TxAddEvent().
				 */
    ClientData tx_cdata;	/* Data to be passed back to caller. */
} txInputDevRec;

/* Maximum number of open files per process.  Must be 32 or less. */
#define	TX_MAX_OPEN_FILES	20

#define TX_PROMPT	'>'
#define TX_LONG_CMD	':'	/* Way of invoking a long command. */
#define TX_LONG_CMD2	';'	/* Alternate way of invoking a long command. */

#ifndef SYSV
#include <sgtty.h>
#endif

/* all of the state associated with a tty terminal */
#ifndef SYSV
typedef struct {
    struct sgttyb tx_i_sgtty;
    struct tchars tx_i_tchars;
} txTermState;
#endif SYSV

extern void TxGetInputEvent();
extern void txFprintfBasic();
