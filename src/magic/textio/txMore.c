/* txMore.c -
 *
 *	Routine to pause until user hits `return'
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
static char rcsid[]  =  "$Header: txMore.c,v 6.0 90/08/28 18:58:17 mayo Exp $";
#endif  not lint

#include "magic.h"


/*
 * ----------------------------------------------------------------------------
 *
 * TxMore --
 *
 * Wait for the user to hit a carriage-return.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints a "<mesg> --more--" prompt.
 *
 * ----------------------------------------------------------------------------
 */
Void
TxMore(mesg)
    char *mesg;
{
    char prompt[512];
    char line[512];

    (void) sprintf(prompt, "%s --more-- (Hit <RETURN> to continue)", mesg);
    (void) TxGetLinePrompt(line, sizeof line, prompt);
}
