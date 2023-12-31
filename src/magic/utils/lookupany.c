/*
 * lookupany.c --
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
 * This file contains a single routine used to find a string in a
 * table which contains the supplied character.
 */

#ifndef lint
static char rcsid[] = "$Header: lookupany.c,v 6.0 90/08/28 19:01:01 mayo Exp $";
#endif  not lint

/*
 * ----------------------------------------------------------------------------
 * LookupAny --
 *
 * Look up a single character in a table of pointers to strings.  The last
 * entry in the string table must be a NULL pointer.
 * The index of the first string in the table containing the indicated
 * character is returned.
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

LookupAny(c, table)
    char c;
    char **table;
{
    char **tp;
    char *index();

    for (tp = table; *tp; tp++)
	if (index(*tp, c) != (char *) 0)
	    return (tp - table);

    return (-1);
}
