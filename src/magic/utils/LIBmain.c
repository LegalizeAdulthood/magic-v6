/*
 * LIBmain.c --
 *
 * File that only goes in libmagicutils.a to define procedures
 * referenced from main that might not be defined elsewhere.
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
static char rcsid[] = "$Header: LIBmain.c,v 6.0 90/08/28 19:00:23 mayo Exp $";
#endif	not lint

#include <stdio.h>


/*
 * ----------------------------------------------------------------------------
 *
 * MainExit --
 *
 * Exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Exits.
 *
 * ----------------------------------------------------------------------------
 */

MainExit(code)
    int code;
{
    exit (code);
}

char AbortMessage[500] = "";


/*
 * ----------------------------------------------------------------------------
 *
 * niceabort --
 *
 * Simple version of niceabort, which dumps core and terminates the program.
 * Magic uses the more complex version found in ~cad/src/magic/misc/niceabort.c.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dumps core and exits.
 * ----------------------------------------------------------------------------
 */

niceabort()
{
    (void) fprintf(stderr, "A major internal inconsistency has been detected:\n");
    (void) fprintf(stderr, "        %s\n\n", AbortMessage);
    abort();                    /* core dump! */
}
