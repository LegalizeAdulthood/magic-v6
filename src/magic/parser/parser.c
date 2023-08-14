/*
 * parser.c --
 *
 * Handles textual parsing.
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
static char rcsid[] = "$Header: parser.c,v 6.0 90/08/28 18:51:17 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "textio.h"

#define ISEND(ch)	((ch == '\0') || (ch == ';'))


/*
 * ----------------------------------------------------------------------------
 * ParsSplit:
 *
 *	Take a string and split it into argc, argv format.
 *	The result of this split is stored back into the original string,
 *	and an array of pointers is set up to index into it.  Note that
 *	the contents of the original string are lost.
 *
 * Results:
 *	TRUE if everything went OK.
 *	FALSE otherwise.
 *
 * Side effects:
 *	argc is filled in, str is changed, argv[] is changed to point into str
 * ----------------------------------------------------------------------------
 */
int
ParsSplit(str, maxArgc, argc, argv, remainder)
    char *str;
    int maxArgc;
    int *argc;
    char **argv;
    char **remainder;
{
    int res = TRUE;

    char **largv;
    char *newstrp;
    char *strp;
    char terminator;

    *argc = 0;
    largv = argv;
    newstrp = str;
    strp = str;

    while (isspace(*strp) && (!ISEND(*strp)) ) strp++;

    terminator = *strp;
    *largv = strp;
    while (!ISEND(*strp))
    {
	if (*strp == '\\')
	{
	    strp++;
	    *newstrp++ = *strp++;
	}
	else if ( (*strp == '\'') || (*strp == '"') )
	{
	    char compare;

	    compare = *strp++;
	    while ( (*strp != compare) && (*strp != '\0'))
	    {
		if (*strp == '\\')
		    strp++;
		*newstrp++ = *strp++;
	    }
	    if (*strp == compare)
		strp++;
	    else
		TxError("Unmatched %c in string, %s.\n", compare,
			"I'll pretend that there is one at the end");
	}
	else
	    *newstrp++ = *strp++;
	if (isspace(*strp) || (ISEND(*strp)))
	{
	    while (isspace(*strp) && (!ISEND(*strp))) strp++;
	    terminator = *strp;
	    *newstrp++ = '\0';
	    (*argc)++;
	    if (*argc < maxArgc)
	    {
		*++largv = newstrp;
	    }
	    else 
	    {
		TxError("Too many arguments.\n");
		*remainder = NULL;
		return FALSE;
	    }
	}
    }
    
    ASSERT(remainder != (char **) NULL, "ParsSplit");

    if (terminator != '\0')
    {
	/* save other commands (those after the ';') for later parsing */
	strp++;
	while (isspace(*strp) && (!ISEND(*strp))) strp++;
	*remainder = strp;
    }
    else
	*remainder = NULL;

    return(res);
}
