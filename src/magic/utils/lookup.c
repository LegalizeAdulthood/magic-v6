/* lookup.c --
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
 * This file contains a single routine used to look up a string in
 * a table, allowing unique abbreviations.
 */

#ifndef lint
static char rcsid[] = "$Header: lookup.c,v 6.0 90/08/28 19:00:59 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "utils.h"


/*---------------------------------------------------------
 * Lookup --
 *	Searches a table of strings to find one that matches a given
 *	string.  It's useful mostly for command lookup.
 *
 *	Only the portion of a string in the table up to the first
 *	blank character is considered significant for matching.
 *
 * Results:
 *	If str is the same as
 *      or an unambiguous abbreviation for one of the entries
 *	in table, then the index of the matching entry is returned.
 *	If str is not the same as any entry in the table, but 
 *      an abbreviation for more than one entry, 
 *	then -1 is returned.  If str doesn't match any entry, then
 *	-2 is returned.  Case differences are ignored.
 *
 * NOTE:  
 *      Table entries need no longer be in alphabetical order
 *      and they need not be lower case.  The irouter command parsing
 *      depends on these features.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */

int
Lookup(str, table)
char *str;			/* Pointer to a string to be looked up */
char *(table[]);		/* Pointer to an array of string pointers
				 * which are the valid commands.  
				 * The end of
				 * the table is indicated by a NULL string.
				 */
{
    int match = -2;	/* result, initialized to -2 = no match */
    int index;

    /* search for match */
    for(index=0; table[index]!=NULL; index++)
    {
	register char *tabc = table[index];
	register char *strc = &(str[0]);
	while(*strc!='\0' && *tabc!=' ' &&
	    ((*tabc==*strc) ||
	     (isupper(*tabc) && islower(*strc) && (tolower(*tabc)== *strc))||
	     (islower(*tabc) && isupper(*strc) && (toupper(*tabc)== *strc)) ))
	{
	    strc++;
	    tabc++;
	}


	if(*strc=='\0') 
	{
	    /* entry matches */
	    if(*tabc==' ' || *tabc=='\0')
	    {
		/* exact match - record it and terminate search */
		match = index;
		break;
	    }    
	    else if (match == -2)
	    {
		/* inexact match and no previous match - record this one 
		 * and continue search */
		match = index;
	    }	
	    else
	    {
		/* previous match, so string is ambiguous unless exact
		 * match exists.  Mark ambiguous for now, and continue
		 * search.
		 */
		match = -1;
	    }
	}
    }
    return(match);
}


/*---------------------------------------------------------
 *
 * LookupStruct --
 *
 * Searches a table of structures, each of which contains a string
 * pointer as its first element, in a manner similar to that of Lookup()
 * above.  Each structure in the table has the following form:
 *
 *	struct
 *	{
 *		char *string;
 *		... rest of structure
 *	};
 *
 * The 'string' field of each structure is matched against the
 * argument 'str'.  The size of a single structure is given by
 * the argument 'size'.
 *
 * Results:
 *	If str is the same as
 *      or an unambiguous abbreviation for one of the entries
 *	in table, then the index of the matching entry is returned.
 *	If str is not the same as any entry in the table, but 
 *      an abbreviation for more than one entry, 
 *	then -1 is returned.  If str doesn't match any entry, then
 *	-2 is returned.  Case differences are ignored.
 *
 *      NOTE:  Table entries need no longer be in alphabetical order
 *      and they need not be lower case.  The irouter command parsing
 *      depends on these features.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

int
LookupStruct(str, table, size)
    char str[];		/* Pointer to a string to be looked up */
    char **table;	/* Pointer to an array of structs containing string
			 * pointers to valid commands.  
			 * The last table entry should have a NULL
			 * string pointer.
			 */
    int size;		/* The size, in bytes, of each table entry */
{
    int match = -2;	/* result, initialized to -2 = no match */
    char **entry;
    int index;

    /* search for match */
    for(entry = table, index=0; 
	    *entry!=NULL; 
	    entry = (char **)((int)entry + size), index++)
    {
	register char *tabc = *entry;
	register char *strc = &(str[0]);
	while(*strc!='\0' && *tabc!=' ' &&
	    ((*tabc== *strc) ||
	     (isupper(*tabc) && islower(*strc) && (tolower(*tabc)== *strc))||
	     (islower(*tabc) && isupper(*strc) && (toupper(*tabc)== *strc)) ))
	{
	    strc++;
	    tabc++;
	}

	if(*strc=='\0') 
	{
	    /* entry matches */
	    if(*tabc==' ' || *tabc=='\0')
	    {
		/* exact match - record it and terminate search */
		match = index;
		break;
	    }    
	    else if (match == -2)
	    {
		/* in exact match and no previous match - record this one 
		 * and continue search */
		match = index;
	    }	
	    else
	    {
		/* previous match, so string is ambiguous unless exact
		 * match exists.  Mark ambiguous for now, and continue
		 * search.
		 */
		match = -1;
	    }
	}
	
    }
    return(match);
}

