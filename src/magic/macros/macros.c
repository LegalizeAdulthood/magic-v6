/*
 * macros.c --
 *
 * Defines and retrieves macros
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
static char rcsid[]="$Header: macros.c,v 6.0 90/08/28 18:47:03 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <signal.h>
#include "magic.h"
#include "utils.h"
#include "macros.h"



/* 
 * Local data structures
 *
 */

static char *(macTab[128]);



/*---------------------------------------------------------
 * MacroDefine:
 *	This procedure defines a macro.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The string passed is copied and considered to be the
 *	macro definition for the character.
 *---------------------------------------------------------
 */

void
MacroDefine(c, str)
char c;			/* The single character name of the macro */
char *str;		/* ...and the string to be attached to it */
{
   c &= 127;
   (void) StrDup(&(macTab[c]), str); 
}


/*---------------------------------------------------------
 * MacroRetrieve:
 *	This procedure retrieves a macro.
 *
 * Results:
 *	A pointer to a new Malloc'ed string is returned.
 *	This structure should be freed when the caller is
 *	done with it.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */

char *
MacroRetrieve(c)
char c;			/* the single character name of the macro */
{
   c &= 127;
   return ( StrDup( (char **) NULL, macTab[c]) ); 
}


/*---------------------------------------------------------
 * MacroDelete:
 *	This procedure deletes a macro.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The string that defines a macro is deleted.  This means
 *	that if anybody still has a pointer to that string
 *	they are in trouble.
 *---------------------------------------------------------
 */

void
MacroDelete(c)
char c;			/* the single character name of the macro */
{
    c &= 127;
    (void) StrDup(&(macTab[c]), (char *) NULL); 
}
