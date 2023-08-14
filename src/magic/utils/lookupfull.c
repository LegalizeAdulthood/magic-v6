/*
 * lookupfull.c --
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
 * Full rights reserved.
 *
 * This file contains a single routine used to look up a string in
 * a table with no abbreviations allowed.
 */

#ifndef lint
static char rcsid[] = "$Header: lookupfull.c,v 6.0 90/08/28 19:01:02 mayo Exp $";
#endif  not lint

/*
 * ----------------------------------------------------------------------------
 * LookupFull --
 *
 * Look up a string in a table of pointers to strings.  The last
 * entry in the string table must be a NULL pointer.
 * This is much simpler than Lookup() in that it does not
 * allow abbreviations.
 *
 * Results:
 *	Index of the name supplied in the table, or -1 if the name
 *	is not found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

LookupFull(name, table)
    char *name;
    char **table;
{
    char **tp;

    for (tp = table; *tp; tp++)
	if (strcmp(name, *tp) == 0)
	    return (tp - table);

    return (-1);
}
