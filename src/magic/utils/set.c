/* set.c -
 *
 *	Generic routines for setting (from a string) and printing
 *      parameter values.  Error messages are printed for invalid
 *      input strings.
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
static char rcsid[]  =  "$Header: set.c,v 6.0 90/08/28 19:01:21 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "utils.h"
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "doubleint.h"
#include "list.h"


/*
 * ----------------------------------------------------------------------------
 *
 * SetNoisy<type> --
 *
 * Set parameter and print current value.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	If valueS is a nonnull string, interpret as <type> and set parm
 *      accordingly.
 *
 *      If valueS is null, the parameter value is left unaltered.
 *
 *      If file is nonnull parameter value is written to file.
 *
 *      If file is null, parameter value is written to magic text window via
 *      TxPrintf
 *	
 * ----------------------------------------------------------------------------
 */

/* SetNoisyInt -- */
Void
SetNoisyInt(parm,valueS,file)
    int *parm;
    char *valueS;
    FILE *file;
{

    /* If value not null, set parm */
    if (valueS)
    {
	if(!StrIsInt(valueS))
	{
	    TxError("Noninteger value for integer parameter (\"%.20s\") ignored.\n",
		valueS);
	}
	else
	{
	    *parm = atoi(valueS);
	}
    }

    /* Print parm value */
    if(file)
	fprintf(file,"%8d ", *parm);
    else
	TxPrintf("%8d ", *parm);

    return;
}

/* SetNoisyBool --  */
Void
SetNoisyBool(parm,valueS,file)
    bool *parm;
    char *valueS;
    FILE *file;
{
    int n, which;

    /* Bool string Table */
    static struct
    {
	char	*bS_name;	/* name */
	bool    bS_value;	/* procedure processing this parameter */
    } boolStrings[] = {
	"yes",		TRUE,
	"no",		FALSE,
	"true",		TRUE,
	"false",	FALSE,
	"1",		TRUE,
	"0",		FALSE,
	"on",		TRUE,
	"off",		FALSE,
	0
    };
    
    /* If value not null, set parm */
    if (valueS)
    {
	/* Lookup value string in boolString table */
	which = LookupStruct(
	    valueS, 
	    (LookupTable *) boolStrings, 
	    sizeof boolStrings[0]);

        /* Process result of lookup */
	if (which >= 0)
	{
	    /* string found - set parm */
	    *parm = boolStrings[which].bS_value;
	}
	else if (which == -1)
	{
	    /* ambiguous boolean value - complain */
	    TxError("Ambiguous boolean value: \"%s\"\n", 
		valueS);
	}
	else 
	{
	    TxError("Unrecognized boolean value: \"%s\"\n", valueS);
	    TxError("Valid values are:  ");
	    for (n = 0; boolStrings[n].bS_name; n++)
		TxError(" %s", boolStrings[n].bS_name);
	    TxError("\n");
	}
    }
    
    /* Print parm value */
    if(file)
	fprintf(file,"%8.8s ", *parm ? "YES" : "NO");
    else
	TxPrintf("%8.8s ", *parm ? "YES" : "NO");

    return;
}

/* SetNoisyDI -- */
/* double size non-negative integer */
Void
SetNoisyDI(parm,valueS,file)
    DoubleInt *parm;
    char *valueS;
    FILE *file;
{
    /* If value not null, set parm */
    if (valueS)
    {
	if(!StrIsInt(valueS))
	{
	    TxError("Noninteger value for integer parameter (\"%.20s\") ignored.\n",
		valueS);
	}
	else
	{
	    int i = atoi(valueS);

	    DOUBLE_CREATE(*parm, i);
	}
    }

    
    /* Print parm value */
    {
	if(file)
	{
	    fprintf(file,"%.0f ", DoubleToDFloat(*parm));
	}
	else
	{
	    TxPrintf("%.0f ", DoubleToDFloat(*parm));
	}
    }

    return;
}
