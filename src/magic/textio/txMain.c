/*
 * txMain.c --
 *
 * 	This module handles output to the text terminal as well as
 *	collecting input and sending the commands to the window package.
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
static char rcsid[]="$Header: txMain.c,v 6.0 90/08/28 18:58:16 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "txcommands.h"
#include "textioInt.h"


/* Global variables that indicate if we are reading or writing to a tty.
 */
global bool TxStdinIsatty;
global bool TxStdoutIsatty;



/*
 * ----------------------------------------------------------------------------
 * TxVisChar:
 *
 *	Make a string that is the visable representation of a character.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string is filled in with at most 3 characters.
 * ----------------------------------------------------------------------------
 */ 

void
TxVisChar(str, ch)
char *str;		/* the string to be filled in */
char ch;		/* the character to be printed */
{
    if (ch < ' ')
    {
	str[0] = '^';
	str[1] = ch + '@';
	str[2] = '\0';
    }
    else if (ch == 0x7F)
    {
	str[0] = '<';
	str[1] = 'd';
	str[2] = 'e';
	str[3] = 'l';
	str[4] = '>';
	str[5] = '\0';
    }
    else
    {
	str[0] = ch;
	str[1] = '\0';
    }
}




/*
 * ----------------------------------------------------------------------------
 * TxInit:
 *
 *	Initialize this module.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	misc.
 * ----------------------------------------------------------------------------
 */ 

void
TxInit()
{
    static char sebuf[BUFSIZ];
    setbuf(stderr, sebuf);
    setbuf(stdin, (char *) NULL);  /* required for LPENDIN in textio to work */
    TxStdinIsatty = (isatty(fileno(stdin)));
    TxStdoutIsatty = (isatty(fileno(stdout)));
    txCommandsInit();
}


