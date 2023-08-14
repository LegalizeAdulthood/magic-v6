/*
 * wireTech.c --
 *
 * This file contains procedures that parse the wiring sections of
 * technology files.
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
static char rcsid[]="$Header: wireTech.c,v 6.0 90/08/28 19:02:54 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "tech.h"
#include "wiring.h"
#include "malloc.h"

/* Table to store contact information collected by this module: */

#define MAXCONTACTS 10
Contact *WireContacts[MAXCONTACTS];
int WireNumContacts = 0;

/*
 * ----------------------------------------------------------------------------
 *	WireTechInit --
 *
 * 	Called once at beginning of technology file read-in to initialize
 *	data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears out the contact table.
 * ----------------------------------------------------------------------------
 */

Void
WireTechInit()
{
    int i;

    for (i = 0; i < MAXCONTACTS; i++)
    {
	if (WireContacts[i] != 0)
	{
	    freeMagic((char *) WireContacts[i]);
	    WireContacts[i] = NULL;
	}
    }

    WireNumContacts = 0;
}

/*
 * ----------------------------------------------------------------------------
 *	WireTechLine --
 *
 * 	This procedure is invoked by the technology module once for
 *	each line in the "wiring" section of the technology file.
 *
 * Results:
 *	Always returns TRUE (otherwise the technology module would
 *	abort Magic with a fatal error).
 *
 * Side effects:
 *	Builds up the contact table, prints error messages if necessary.
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
bool
WireTechLine(sectionName, argc, argv)
    char *sectionName;		/* Name of this section (unused). */
    int argc;			/* Number of arguments on line. */
    char *argv[];		/* Pointers to fields of line. */
{
    register Contact *new;

    if (strcmp(argv[0], "contact") != 0)
    {
	TechError("Unknown wiring keyword: %s.  Line ignored.\n", argv[0]);
	return TRUE;
    }
    if (argc != 7)
    {
	TechError("\"contact\" lines must have exactly 7 arguments.\n");
	return TRUE;
    }
    if (WireNumContacts >= MAXCONTACTS)
    {
	TechError("Sorry, can't have more than %d contacts.\n", MAXCONTACTS);
	return TRUE;
    }

    new = (Contact *) mallocMagic(sizeof(Contact));
    new->con_type = DBTechNoisyNameType(argv[1]);
    new->con_layer1 = DBTechNoisyNameType(argv[3]);
    new->con_layer2 = DBTechNoisyNameType(argv[5]);
    if ((new->con_type < 0) || (new->con_layer1 < 0) || (new->con_layer2 < 0))
    {
	errorReturn:
	freeMagic((char *) new);
	return TRUE;
    }

    if (!StrIsInt(argv[2]))
    {
	TechError("3rd field must be an integer.\n");
	goto errorReturn;
    }
    else new->con_size = atoi(argv[2]);
    if (!StrIsInt(argv[4]))
    {
	TechError("5th field must be an integer.\n");
	goto errorReturn;
    }
    else new->con_surround1 = atoi(argv[4]);
    if (!StrIsInt(argv[6]))
    {
	TechError("6th field must be an integer.\n");
	goto errorReturn;
    }
    else new->con_surround2 = atoi(argv[6]);

    WireContacts[WireNumContacts] = new;
    WireNumContacts += 1;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *	WireTechFinal --
 *
 * 	This procedure is called by the technology module after all the
 *	lines of the tech file have been read.  It doesn't do anything
 *	right now.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

Void
WireTechFinal()
{
    /* Debugging code to print info about layers:

    int i;
    register Contact *con;

    for (i = 0; i < WireNumContacts; i += 1)
    {
	con = WireContacts[i];
	TxPrintf("Contact type \"%s\", size %d connects\n",
	    DBTypeLongName(con->con_type), con->con_size);
	TxPrintf("    \"%s\" (overlap %d) and\n",
	    DBTypeLongName(con->con_layer1), con->con_surround1);
	TxPrintf("    \"%s\" (overlap %d)\n",
	    DBTypeLongName(con->con_layer2), con->con_surround2);
    }
    */
}
