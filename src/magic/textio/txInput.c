/*
 * txInput.c --
 *
 * 	Handles 'stdin' and terminal driver settings.
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
static char rcsid[]="$Header: txInput.c,v 6.0 90/08/28 18:58:14 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#ifdef SYSV
#include <sys/termio.h>
#endif
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "txcommands.h"
#include "textioInt.h"
#include "dqueue.h"
#include "graphics.h"


/* Characters for input processing.  Set to -1 if not defined */

char txEraseChar = -1;		/* Erase line (e.g. ^H) */
char txKillChar = -1;		/* Kill line (e.g. ^U or ^X) */
char txWordChar = -1;		/* Erase a word (e.g. ^W) */
char txReprintChar = -1;	/* Reprint the line (e.g. ^R */
char txLitChar = -1;		/* Literal next character (e.g. ^V */
char txBreakChar = -1;		/* Break to a new line (normally -1) */
char TxEOFChar = -1;		/* The current EOF character (e.g. ^D) */
char TxInterruptChar = -1;	/* The current interrupt character (e.g. ^C) */

static char txPromptChar;	/* the current prompt */
bool txHavePrompt = FALSE;	/* is a prompt on the screen? */

char *txReprint1 = NULL;
char *txReprint2 = NULL;

/*
 * ----------------------------------------------------------------------------
 *
 * TxReprint --
 *
 *	Reprint the current input line.  Used for ^R and after ^Z.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
TxReprint()
{
    (void) txFprintfBasic(stdout, "\n");
    if (txReprint1 != NULL) (void) txFprintfBasic(stdout, "%s", txReprint1);
    if (txReprint2 != NULL) (void) txFprintfBasic(stdout, "%s", txReprint2);
    (void) fflush(stdout);
}


/*
 * ----------------------------------------------------------------------------
 * TxSetPrompt --
 *
 *	Set the current prompt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	You can guess.
 * ----------------------------------------------------------------------------
 */

void
TxSetPrompt(ch)
    char ch;
{
    txPromptChar = ch;
}


/*
 * ----------------------------------------------------------------------------
 * TxPrompt --
 *
 *	Put up the prompt which was set by TxSetPrompt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	You can guess.
 * ----------------------------------------------------------------------------
 */

void
TxPrompt()
{
    static char lastPromptChar;
    static char prompts[2];

    if (txHavePrompt && (lastPromptChar == txPromptChar)) return;

    (void) fflush(stderr);
    if (txHavePrompt) TxUnPrompt();
    if (TxInteractive) txFprintfBasic(stdout, "%c", txPromptChar);
    (void) fflush(stdout);
    prompts[0] = txPromptChar;
    prompts[1] = '\0';
    txReprint1 = prompts;
    txHavePrompt = TRUE;
    lastPromptChar = txPromptChar;
}


/*
 * ----------------------------------------------------------------------------
 * TxRestorePrompt --
 *
 *	The prompt was erased for some reason.  Restore it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	You can guess.
 * ----------------------------------------------------------------------------
 */

void
TxRestorePrompt()
{
    if (txHavePrompt)
    {
	txHavePrompt = FALSE;
	TxPrompt();
    }
}


/*
 * ----------------------------------------------------------------------------
 * TxUnPrompt --
 *
 *	Erase the prompt.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	You can guess.
 * ----------------------------------------------------------------------------
 */

void
TxUnPrompt()
{
    if (txHavePrompt)
    {
	(void) fflush(stderr);
	if (TxInteractive) fputs("\b \b", stdout);
	(void) fflush(stdout);
	txReprint1 = NULL;
	txHavePrompt = FALSE;
    }
}
/*
 * ----------------------------------------------------------------------------
 *
 * TxGetChar --
 *
 *	Get a single character from the input stream (terminal or whatever).
 *
 * Results:
 *	One character, or EOF.
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
TxGetChar()
{
    int ch;
    extern DQueue txInputEvents, txFreeEvents;
    extern TxInputEvent txLastEvent;
    TxInputEvent *event;
    while (TRUE) {
	/* Get an input character.  Don't let TxGetInputEvent return
	 * because of a SigWinch or whatever.  It would be nice to process
	 * SigWinches in a middle of text input, but that is difficult
	 * to do since SigWinches are processed in the normal
	 * command-interpretation process.  Thus, we delay the processing
	 * until after the text is collected.
	 */
	if (DQIsEmpty(&txInputEvents)) TxGetInputEvent(TRUE, FALSE);
	event = (TxInputEvent *) DQPopFront(&txInputEvents);
	ASSERT(event != NULL, "TxGetChar");
	txLastEvent = *event;
	if (event->txe_button == TX_EOF) {
	    ch = EOF; 
	    goto gotone;
	}
	if (event->txe_button == TX_CHARACTER) {
	    ch = event->txe_ch;
	    goto gotone;
	}
	DQPushRear(&txFreeEvents, (ClientData) event);
    }

gotone:
    DQPushRear(&txFreeEvents, (ClientData) event);
    return ch;
}

/*
 * ----------------------------------------------------------------------------
 *
 * TxGetLinePrompt --
 *
 *	Just like the following TxGetLine proc, but it prompts first.  It
 *	is an advantage to have this proc put out the prompt, since that
 *	way if the user suspends Magic and then continues it the prompt will
 *	reappear in the proper fashion.
 *
 * Results:
 *	A char pointer or NULL (see TxGetLine).
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

char *
TxGetLinePrompt(dest, maxChars, prompt)
    char *dest;
    int maxChars;
    char *prompt;
{
    char *res;
    if (txHavePrompt) TxUnPrompt();
    if (prompt != NULL) TxPrintf("%s", prompt);
    txReprint1 = prompt;
    res = TxGetLine(dest, maxChars);
    txReprint1 = NULL;
    return res;
}


/*
 * ----------------------------------------------------------------------------
 * TxGetLine:
 *
 * 	Reads a line from the input queue (terminal or whatever).
 *
 * Results:
 *	A char pointer to the string is returned.
 *	If an end-of-file is typed, returns (char *) NULL and 
 *	stores in the string any characters recieved up to that point.
 *
 * Side effects:
 *	The input stream is read, and 'dest' is filled in with up to maxChars-1
 *	characters.  Up to maxChars of the 'dest' may be changed during the
 *	input process, however, since a '\0' is added at the end.  There is no 
 *	newline at the end of 'dest'.
 * ----------------------------------------------------------------------------
 */

char *
TxGetLine(dest, maxChars)
    char *dest;
    int maxChars;
{
    int i;
    char *ret;
    int ch;
    bool literal;

    if (maxChars < 1) return dest;
    if (txHavePrompt) TxUnPrompt();
    ret = dest;
    dest[0] = '\0';
    txReprint2 = dest;

#define ERASE()	( (i>0) ?(i--, fputs("\b \b", stdout), 0) :0)

    (void) fflush(stderr);
    literal = FALSE;
    i = 0;
    while (TRUE) {
	dest[i] = '\0';
	if (i >= maxChars - 1) break;
	(void) fflush(stdout);
	ch = TxGetChar();
	if (ch == EOF || ch == -1 || ch == TxEOFChar) {
	    TxError("\nEOF encountered on input stream.\n");
	    ret = NULL;
	    break;
	} else if (literal) {
	    literal = FALSE;
	    dest[i] = ch;
	    txFprintfBasic(stdout, "%c", ch);
	    i++;
	} else if (ch == '\n' || ch == txBreakChar) {
	    break;
	} else if (ch == '\t') {
	    while (TRUE) {
		dest[i] = ' ';
		txFprintfBasic(stdout, " ");
		i++;
		if (((i + strlen(txReprint1)) % 8) == 0) break;
	    }
	} else if (ch == txEraseChar) {
	    ERASE();
	} else if (ch == txKillChar) {
	    while (i > 0) ERASE();
	} else if (ch == txWordChar) {
	    while (i > 0 && isspace(dest[i-1])) ERASE();
	    while (i > 0 && !isspace(dest[i-1])) ERASE();
	} else if (ch == txReprintChar) {
	    TxReprint();
	} else if (ch == txLitChar) {
	    literal = TRUE;
	} else {
	    dest[i] = ch;
	    txFprintfBasic(stdout, "%c", ch);
	    i++;
	}
    }
    txFprintfBasic(stdout, "\n");
    txHavePrompt = FALSE;
    txReprint2 = NULL;

    return ret;
}


/*
 * ----------------------------------------------------------------------------
 * txGetTermState:
 *
 *	Read the state of the terminal and driver.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	The passed structure is filled in.
 * ----------------------------------------------------------------------------
 */

#ifdef SYSV
void
txGetTermState(buf)
    struct termio *buf;

{
    ioctl( fileno( stdin ), TCGETA, buf);
}

#else

void
txGetTermState(buf)
    txTermState *buf;
{
    ASSERT(TxStdinIsatty, "txGetTermState");
    /* save the current terminal characteristics */
    (void) ioctl(fileno(stdin), TIOCGETP, (char *) &(buf->tx_i_sgtty) );
    (void) ioctl(fileno(stdin), TIOCGETC, (char *) &(buf->tx_i_tchars) );
}
#endif SYSV


/*
 * ----------------------------------------------------------------------------
 * txSetTermState:
 *
 *	Set the state of the terminal and driver.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	The terminal driver is set up.
 * ----------------------------------------------------------------------------
 */

void
txSetTermState(buf)
#ifdef SYSV
    struct termio *buf;
#else
    txTermState *buf;
#endif SYSV
{
#ifdef SYSV
    ioctl( fileno(stdin), TCSETAF, buf );
#else
    /* set the current terminal characteristics */
    (void) ioctl(fileno(stdin), TIOCSETN, (char *) &(buf->tx_i_sgtty) );
    (void) ioctl(fileno(stdin), TIOCSETC, (char *) &(buf->tx_i_tchars) );
#endif SYSV
}



/*
 * ----------------------------------------------------------------------------
 * txInitTermRec:
 *
 * 	Sets the terminal record that it has echo turned off,
 *	cbreak on, and no EOFs the way we like it.
 *
 * Results:
 *	The passed terminal record is modified.
 *
 * Side effects:
 *	No terminal modes are actually changed.
 * ----------------------------------------------------------------------------
 */

void
txInitTermRec(buf)
#ifdef SYSV
    struct termio *buf;
#else
    txTermState *buf;
#endif SYSV
{
#ifdef SYSV
    buf->c_lflag = ISIG;    /* raw: no echo and no processing, allow signals */
    buf->c_cc[ VMIN ] = 1;
    buf->c_cc[ VTIME ] = 0;
#else
    /* set things up for us, turn off echo, turn on cbreak, no EOF */
    buf->tx_i_sgtty.sg_flags |= CBREAK;
    buf->tx_i_sgtty.sg_flags &= ~ECHO;
    buf->tx_i_tchars.t_eofc = -1;

    /* Give the graphics module a chance to override this. */
    if (GrTermStatePtr) 
	(*GrTermStatePtr)(&buf->tx_i_sgtty, &buf->tx_i_tchars);
#endif SYSV
}



#ifdef SYSV
struct termio closeTermState;
#else
static txTermState closeTermState;
#endif SYSV

static bool haveCloseState = FALSE;

/*
 * ----------------------------------------------------------------------------
 * txSaveTerm:
 *
 * 	Save the terminal characteristics so they can be restored when
 *	magic leaves.
 *	
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	a global variable is set.
 * ----------------------------------------------------------------------------
 */

void
txSaveTerm()
{
#ifdef SYSV
    ioctl( fileno( stdin ), TCGETA, &closeTermState);
    txEraseChar = closeTermState.c_cc[VERASE];
    txKillChar =  closeTermState.c_cc[VKILL];
    TxEOFChar = closeTermState.c_cc[VEOF];
    TxInterruptChar = closeTermState.c_cc[VINTR];
    haveCloseState = TRUE;
#else
    struct ltchars lt;
    txGetTermState(&closeTermState);
    (void) ioctl(fileno(stdin), TIOCGLTC, (char *) &lt);
    txEraseChar = closeTermState.tx_i_sgtty.sg_erase;
    txKillChar = closeTermState.tx_i_sgtty.sg_kill;
    txWordChar = lt.t_werasc;
    txReprintChar = lt.t_rprntc;
    txLitChar = lt.t_lnextc;
    txBreakChar = closeTermState.tx_i_tchars.t_brkc;
    TxEOFChar = closeTermState.tx_i_tchars.t_eofc;
    TxInterruptChar = closeTermState.tx_i_tchars.t_intrc;
    haveCloseState = TRUE;
#endif SYSV
}


/*
 * ----------------------------------------------------------------------------
 * TxSetTerminal:
 *
 * 	Sets the terminal up the way we like it.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	The terminal modes are changed.
 * ----------------------------------------------------------------------------
 */

void
TxSetTerminal()
{
#ifdef SYSV
    struct termio buf;
#else
    txTermState buf;
#endif SYSV

    if (TxStdinIsatty)
    {
	if (!haveCloseState) txSaveTerm();
	buf = closeTermState;
	txInitTermRec(&buf);
	txSetTermState(&buf);
    }
}


/*
 * ----------------------------------------------------------------------------
 * TxResetTerminal:
 *
 * 	Returns the terminal to the way it was when Magic started up.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	The terminal modes are changed.
 * ----------------------------------------------------------------------------
 */

void
TxResetTerminal()
{
    if (TxStdinIsatty && haveCloseState) txSetTermState(&closeTermState);
}
