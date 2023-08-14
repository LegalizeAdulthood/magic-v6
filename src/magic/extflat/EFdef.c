/*
 * EFdef.c -
 *
 * Procedures for managing the database of Defs.
 * There is a single Def for each .ext file in a hierarchically
 * extracted circuit.
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
static char rcsid[] = "$Header: EFdef.c,v 6.0 90/08/28 18:13:29 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "utils.h"
#include "malloc.h"
#include "extflat.h"
#include "EFint.h"

/* Initial size of def hash table */
#define	INITDEFSIZE	128

/* Initial size of node hash table in each def */
#define	INITNODESIZE	32

/* Def hash table itself; maps from def names into pointers to Defs */
HashTable efDefHashTable;

/* Hash table used for checking for malloc leaks */
HashTable efFreeHashTable;

/*
 * ----------------------------------------------------------------------------
 *
 * EFInit --
 *
 * Initialize the hash table of def names and global signal names.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

EFInit()
{
#ifdef	MALLOCTRACE
    mallocTraceInit("malloc.out");
    mallocTraceEnable();
#endif	MALLOCTRACE
    HashInit(&efFreeHashTable, 32, HT_WORDKEYS);
    HashInit(&efDefHashTable, INITDEFSIZE, 0);
    efSymInit();
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFDone --
 *
 * Overall cleanup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Eliminates the def and global name hash tables.
 *	If malloc tracing is enabled, also frees everything else we
 *	allocated with malloc.
 *
 * ----------------------------------------------------------------------------
 */

EFDone()
{
    Connection *conn;
    HashSearch hs;
    HashEntry *he;
    Kill *kill;
    Def *def;
    Use *use;
    Fet *fet;
    int n;

    HashStartSearch(&hs);
    while (he = HashNext(&efDefHashTable, &hs))
    {
	def = (Def *) HashGetValue(he);
	FREE(def->def_name);
	efFreeNodeTable(&def->def_nodes);
	efFreeNodeList(&def->def_firstn);
	HashKill(&def->def_nodes);
	HashKill(&def->def_dists);
	for (use = def->def_uses; use; use = use->use_next)
	{
	    FREE(use->use_id);
	    FREE((char *) use);
	}
	for (conn = def->def_conns; conn; conn = conn->conn_next)
	    efFreeConn(conn);
	for (conn = def->def_caps; conn; conn = conn->conn_next)
	    efFreeConn(conn);
	for (conn = def->def_resistors; conn; conn = conn->conn_next)
	    efFreeConn(conn);
	for (fet = def->def_fets; fet; fet = fet->fet_next)
	{
	    for (n = 0; n < fet->fet_nterm; n++)
		if (fet->fet_terms[n].fterm_attrs)
		    FREE((char *) fet->fet_terms[n].fterm_attrs);
	    FREE((char *) fet);
	}
	for (kill = def->def_kills; kill; kill = kill->kill_next)
	{
	    FREE(kill->kill_name);
	    FREE((char *) kill);
	}
	FREE((char *) def);
    }

    /* Misc cleanup */
    for (n = 0; n < EFFetNumTypes; n++) FREE(EFFetTypes[n]);
    for (n = 0; n < EFLayerNumNames; n++) FREE(EFLayerNames[n]);
    if (EFTech) FREE(EFTech);

    /* Free up all HierNames that were stored in efFreeHashTable */
    HashStartSearch(&hs);
    while (he = HashNext(&efFreeHashTable, &hs))
	FREE(he->h_key.h_ptr);
    HashKill(&efFreeHashTable);

    /* Final cleanup */
    HashKill(&efDefHashTable);
#ifdef	MALLOCTRACE
    mallocTraceDone();
#endif	MALLOCTRACE
}

/*
 * ----------------------------------------------------------------------------
 *
 * efDefLook --
 *
 * Look for a def by the given name in the hash table.
 * If the def doesn't exist, return NULL; otherwise, return
 * a pointer to the def.
 *
 * Results:
 *	Returns a pointer to a def, or NULL if none by that
 *	name exists.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

Def *
efDefLook(name)
    char *name;
{
    HashEntry *he;

    he = HashLookOnly(&efDefHashTable, name);
    if (he == (HashEntry*) NULL)
	return ((Def *) NULL);

    return ((Def *) HashGetValue(he));
}

/*
 * ----------------------------------------------------------------------------
 *
 * efDefNew --
 *
 * Allocate a new def by the given name.
 *
 * Results:
 *	Returns a pointer to a def.
 *
 * Side effects:
 *	Allocates a new def and initializes it.
 *
 * ----------------------------------------------------------------------------
 */

Def *
efDefNew(name)
    char *name;
{
    HashEntry *he;
    Def *newdef;

    he = HashFind(&efDefHashTable, name);
    MALLOC(Def *, newdef, sizeof (Def));
    HashSetValue(he, (char *) newdef);

    newdef->def_name = StrDup((char **) NULL, name);
    newdef->def_flags = 0;
    newdef->def_scale = 1;
    newdef->def_conns = (Connection *) NULL;
    newdef->def_caps = (Connection *) NULL;
    newdef->def_resistors = (Connection *) NULL;
    newdef->def_fets = (Fet *) NULL;
    newdef->def_uses = (Use *) NULL;
    newdef->def_kills = (Kill *) NULL;

    /* Initialize circular list of nodes */
    newdef->def_firstn.efnode_next = (EFNodeHdr *) &newdef->def_firstn;
    newdef->def_firstn.efnode_prev = (EFNodeHdr *) &newdef->def_firstn;

    /* Initialize hash table of node names */
    HashInit(&newdef->def_nodes, INITNODESIZE, HT_STRINGKEYS);

    /* Initialize hash table of distances */
    HashInitClient(&newdef->def_dists, INITNODESIZE, HT_CLIENTKEYS,
	    efHNDistCompare, efHNDistCopy, efHNDistHash, efHNDistKill);

    return (newdef);
}
