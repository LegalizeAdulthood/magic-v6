/*
 * DBcellname.c --
 *
 * CellUse/CellDef creation, deletion, naming.
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
static char rcsid[] = "$Header: DBcellname.c,v 6.0 90/08/28 18:09:34 mayo Exp $";
#endif  not lint

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#endif
#include "magic.h"
#include "hash.h"
#include "utils.h"
#include "geometry.h"
#include "tile.h"
#include "database.h"
#include "databaseInt.h"
#include "signals.h"
#include "undo.h"
#include "malloc.h"
#include "windows.h"
#include "textio.h"
#include "main.h"

/*
 * Hash tables for CellDefs and CellUses
 */

#define	NCELLDEFBUCKETS	64	/* Initial number of buckets for CellDef tbl */
#define	NCELLUSEBUCKETS	128	/* Initial number of buckets for CellUse tbl */

HashTable dbCellDefTable;
HashTable dbUniqueDefTable;
HashTable dbUniqueNameTable;
bool dbWarnUniqueIds;

/*
 * Routines used before defined
 */
CellDef *DBCellDefAlloc();
int dbLinkFunc();

/*
 * Routines from other database modules
 */
extern void dbComputeBbox();

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellInit --
 *
 * Initialize the world of the cell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up the symbol tables for CellDefs and CellUses.
 *
 * ----------------------------------------------------------------------------
 */

void
DBCellInit()
{
    HashInit(&dbCellDefTable, NCELLDEFBUCKETS, 0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellLookDef --
 *
 * Find the definition of the cell with the given name.
 *
 * Results:
 *	Returns a pointer to the CellDef with the given name if it
 *	exists.  Otherwise, returns (CellDef *) NULL.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

CellDef *
DBCellLookDef(cellName)
    char *cellName;
{
    HashEntry *entry;

    entry = HashFind(&dbCellDefTable, cellName);
    return ((CellDef *) HashGetValue(entry));
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellNewDef --
 *
 * Create a new cell definition with the given name.  There must not
 * be any cells already known with the same name.
 *
 * Results:
 *	Returns a pointer to the newly created CellDef.  The CellDef
 *	is completely initialized, showing no uses and having all
 *	tile planes initialized via TiNewPlane() to contain a single
 *	space tile.  The filename associated with the cell is set to
 *	the name supplied, but no attempt is made to open it or create
 *	it.
 *
 *	If the cellName supplied is NULL, the cell is entered into
 *	the symbol table with a name of UNNAMED.
 *
 *	Returns NULL if a cell by the given name already exists.
 *
 * Side effects:
 *	The name of the CellDef is entered into the symbol table
 *	of known cells.
 *
 * ----------------------------------------------------------------------------
 */

CellDef *
DBCellNewDef(cellName, cellFileName)
    char *cellName;		/* Name by which the cell is known */
    char *cellFileName;		/* Name of disk file in which the cell 
				 * should be kept when written out.
				 */
{
    CellDef *cellDef;
    HashEntry *entry;

    if (cellName == (char *) NULL)
	cellName = UNNAMED;

    entry = HashFind(&dbCellDefTable, cellName);
    if (HashGetValue(entry) != (ClientData) NULL)
	return ((CellDef *) NULL);

    cellDef = DBCellDefAlloc();
    HashSetValue(entry, (ClientData) cellDef);
    cellDef->cd_name = StrDup((char **) NULL, cellName);
    if (cellFileName == (char *) NULL)
	cellDef->cd_file = cellFileName;
    else
	cellDef->cd_file = StrDup((char **) NULL, cellFileName);
    return (cellDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellDefAlloc --
 *
 * Create a new cell definition structure.  The new def is not added
 * to any symbol tables.
 *
 * Results:
 *	Returns a pointer to the newly created CellDef.  The CellDef
 *	is completely initialized, showing no uses and having all
 *	tile planes initialized via TiNewPlane() to contain a single
 *	space tile.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

CellDef *
DBCellDefAlloc()
{
    CellDef *cellDef;
    int pNum;

    MALLOC(CellDef *, cellDef, sizeof (CellDef));
    cellDef->cd_flags = 0;
    cellDef->cd_bbox.r_xbot = 0;
    cellDef->cd_bbox.r_ybot = 0;
    cellDef->cd_bbox.r_xtop = 1;
    cellDef->cd_bbox.r_ytop = 1;
    cellDef->cd_name = (char *) NULL;
    cellDef->cd_file = (char *) NULL;
    cellDef->cd_parents = (CellUse *) NULL;
    cellDef->cd_labels = (Label *) NULL;
    cellDef->cd_lastLabel = (Label *) NULL;
    cellDef->cd_client = (ClientData) 0;
    cellDef->cd_props = (ClientData) NULL;
    HashInit(&cellDef->cd_idHash, 16, HT_STRINGKEYS);

    cellDef->cd_planes[PL_CELL] = DBNewPlane((ClientData) NULL);
    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
	cellDef->cd_planes[pNum] = DBNewPlane((ClientData) TT_SPACE);

    return (cellDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellNewUse --
 *
 * Create a new cell use of the supplied CellDef.
 *
 * Results:
 *	Returns a pointer to the new CellUse.  The CellUse is initialized
 *	to reflect that cellDef is its definition.  The transform is
 *	initialized to the identity, and the parent pointer initialized
 *	to NULL.
 *
 * Side effects:
 *	Updates the use list for cellDef.
 *
 * ----------------------------------------------------------------------------
 */

CellUse *
DBCellNewUse(cellDef, useName)
    CellDef *cellDef;	/* Pointer to definition of the cell */
    char *useName;	/* Pointer to use identifier for the cell.  This may
			 * be NULL, in which case a unique use identifier is
			 * generated automatically when the cell use is linked
			 * into a parent def.
			 */
{
    CellUse *cellUse;

    MALLOC(CellUse *, cellUse, sizeof (CellUse));
    cellUse->cu_id = StrDup((char **) NULL, useName);
    cellUse->cu_expandMask = 0;
    cellUse->cu_transform = GeoIdentityTransform;
    cellUse->cu_def = cellDef;
    cellUse->cu_parent = (CellDef *) NULL;
    cellUse->cu_xlo = 0;
    cellUse->cu_ylo = 0;
    cellUse->cu_xhi = 0;
    cellUse->cu_yhi = 0;
    cellUse->cu_xsep = 0;
    cellUse->cu_ysep = 0;
    cellUse->cu_delta = 0;		/* For plowing */
    cellUse->cu_nextuse = cellDef->cd_parents;

    /* Initial client field */
    /* (commands can use this field for whatever
     * they like, but should restore its value to MINFINITY before exiting.) 
     */
    cellUse->cu_client = (ClientData) MINFINITY;

    cellDef->cd_parents = cellUse;
    dbComputeBbox(cellUse);
    return (cellUse);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellRenameDef --
 *
 * Renames the indicated CellDef.
 *
 * Results:
 *	TRUE if successful, FALSE if the new name was not unique.
 *
 * Side effects:
 *	The name of the CellDef is entered into the symbol table
 *	of known cells.  The CDMODIFIED bit is set in the flags
 *	of each of the parents of the CellDef to force them to
 *	be written out using the new name.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellRenameDef(cellDef, newName)
    CellDef *cellDef;		/* Pointer to CellDef being renamed */
    char *newName;		/* Pointer to new name */
{
    HashEntry *oldEntry, *newEntry;
    CellUse *parent;

    oldEntry = HashFind(&dbCellDefTable, cellDef->cd_name);
    ASSERT(HashGetValue(oldEntry) == (ClientData) cellDef, "DBCellRenameDef");

    newEntry = HashFind(&dbCellDefTable, newName);
    if (HashGetValue(newEntry) != (ClientData) NULL)
	return (FALSE);

    HashSetValue(oldEntry, (ClientData) NULL);
    HashSetValue(newEntry, (ClientData) cellDef);
    (void) StrDup(&cellDef->cd_name, newName);

    for (parent = cellDef->cd_parents; parent; parent = parent->cu_nextuse)
	if (parent->cu_parent)
	    parent->cu_parent->cd_flags |= CDMODIFIED|CDGETNEWSTAMP;

    return (TRUE);
}
#ifdef	notused

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellDeleteDef --
 *
 * Removes the CellDef from the symbol table of known CellDefs and
 * frees the storage allocated to the CellDef.  The CellDef must have
 * no CellUses.
 *
 * Results:
 *	TRUE if successful, FALSE if there were any outstanding
 *	CellUses found.
 *
 * Side effects:
 *	The CellDef is removed from the table of known CellDefs.
 *	All storage for the CellDef and its tile planes is freed.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellDeleteDef(cellDef)
    CellDef *cellDef;		/* Pointer to CellDef to be deleted */
{
    HashEntry *entry;

    if (cellDef->cd_parents != (CellUse *) NULL)
	return (FALSE);

    entry = HashFind(&dbCellDefTable, cellDef->cd_name);
    ASSERT(HashGetValue(entry) == (ClientData) cellDef, "DBCellDeleteDef");
    HashSetValue(entry, (ClientData) NULL);
    if (cellDef->cd_props)
	DBPropClearAll(cellDef);
    DBCellDefFree(cellDef);
    return TRUE;
}
#endif	notused

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellDefFree --
 *
 * 	Does all the dirty work of freeing up stuff inside a celldef.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All memory associated with the cellDef is freed.  This may
 *	cause lower-level cellUses and defs to be freed up.  This
 *	procedure is separated from DBDeleteCellDef so that it can
 *	be used for cells that aren't in the hash table (e.g. cells
 *	used by the window manager).
 *
 * ----------------------------------------------------------------------------
 */

void
DBCellDefFree(cellDef)
    CellDef *cellDef;

{
    int pNum;
    register Label *lab;

    if (cellDef->cd_file != (char *) NULL)
	FREE(cellDef->cd_file);
    if (cellDef->cd_name != (char *) NULL)
	FREE(cellDef->cd_name);

    /*
     * We want the following searching to be non-interruptible
     * to guarantee that all storage gets freed.
     */

    SigDisableInterrupts();
    DBFreeCellPlane(cellDef->cd_planes[PL_CELL]);
    TiFreePlane(cellDef->cd_planes[PL_CELL]);

    for (pNum = PL_PAINTBASE; pNum < DBNumPlanes; pNum++)
    {
	DBFreePaintPlane(cellDef->cd_planes[pNum]);
	TiFreePlane(cellDef->cd_planes[pNum]);
	cellDef->cd_planes[pNum] = (Plane *) NULL;
    }

    for (lab = cellDef->cd_labels; lab; lab = lab->lab_next)
	FREE((char *) lab);
    SigEnableInterrupts();
    HashKill(&cellDef->cd_idHash);

    FREE((char *) cellDef);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellDeleteUse --
 *
 * Frees the storage allocated to the CellUse.
 *
 * It is required that the CellUse has been removed from any CellTileBodies
 * in the subcell plane of its parent.  The parent pointer for this
 * CellUse must therefore be NULL.
 *
 * Results:
 *	TRUE if the CellUse was successfully removed, FALSE if
 *	the parent pointer were not NULL.
 *
 * Side effects:
 *	All storage for the CellUse is freed.
 *	The list of all CellUses associated with a given CellDef is
 *	updated to reflect the absence of the deleted CellUse.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellDeleteUse(cellUse)
    CellUse *cellUse;		/* Pointer to CellUse to be deleted */
{
    CellDef *cellDef;
    CellUse *useptr;

    if (cellUse->cu_parent != (CellDef *) NULL)
	return (FALSE);

    cellDef = cellUse->cu_def;
    if (cellUse->cu_id != (char *) NULL)
	FREE(cellUse->cu_id);
    cellUse->cu_id = (char *) NULL;
    cellUse->cu_def = (CellDef *) NULL;

    ASSERT(cellDef->cd_parents != (CellUse *) NULL, "DBCellDeleteUse");

    if (cellDef->cd_parents == cellUse)
	cellDef->cd_parents = cellUse->cu_nextuse;
    else for (useptr = cellDef->cd_parents;  useptr != NULL;
	useptr = useptr->cu_nextuse)
    {
	if (useptr->cu_nextuse == cellUse)
	{
	    useptr->cu_nextuse = cellUse->cu_nextuse;
	    break;
	}
    }

    FREE((char *) cellUse);
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellSrDefs --
 *
 * Search for all cell definitions matching a given pattern.
 * For each cell definition whose flag word contains any of the
 * bits in pattern, the supplied procedure is invoked.
 *
 * The procedure should be of the following form:
 *	int
 *	func(cellDef, cdata)
 *	    CellDef *cellDef;
 *	    ClientData cdata;
 *	{
 *	}
 * Func should normally return 0.  If it returns 1 then the
 * search is aborted.
 *
 * Results:
 *	Returns 1 if the search completed normally, 1 if it aborted.
 *
 * Side effects:
 *	Whatever the user-supplied procedure does.
 *
 * ----------------------------------------------------------------------------
 */

int
DBCellSrDefs(pattern, func, cdata)
    int pattern;	/* Used for selecting cell definitions.  If any
			 * of the bits in the pattern are in a def->cd_flags,
			 * or if pattern is 0, the user-supplied function
			 * is invoked.  
			 */
    int (*func)();	/* Function to be applied to each matching CellDef */
    ClientData cdata;	/* Client data also passed to function */
{
    HashSearch hs;
    HashEntry *he;
    CellDef *cellDef;

    HashStartSearch(&hs);
    while ((he = HashNext(&dbCellDefTable, &hs)) != (HashEntry *) NULL)
    {
	cellDef = (CellDef *) HashGetValue(he);
	if (cellDef == (CellDef *) NULL)
	    continue;
	if ((pattern != 0) && !(cellDef->cd_flags & pattern))
	    continue;
	if ((*func)(cellDef, cdata)) return 1;
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBLinkCell --
 *
 * Set the cu_id for the supplied CellUse appropriately for linking into
 * the parent CellDef.  If the cu_id is NULL, a cu_id unique within the
 * CellDef is automatically generated and stored in cu_id; otherwise, the
 * one supplied in cu_id is used.
 *
 *			*** WARNING ***
 *
 * This operation is not recorded on the undo list, as it always accompanies
 * the creation of a new cell use.
 *
 * Results:
 *	TRUE if the CellUse is unique within the parent CellDef, FALSE
 *	if there would be a name conflict.  If the cu_id of the CellUse
 *	is NULL, TRUE is always returned; FALSE is only returned if
 *	there is an existing name which would conflict with names already
 *	present in the CellUse.
 *
 * Side effects:
 *	Will set cu_id to an automatically generated instance id if
 *	it was originally NULL.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBLinkCell(use, parentDef)
    CellUse *use;
    CellDef *parentDef;
{
    char useId[100], *lastName;
    HashEntry *he;
    int n;

    if (use->cu_id)
    {
	if (DBFindUse(use->cu_id, parentDef))
	    return FALSE;
	DBSetUseIdHash(use, parentDef);
	return TRUE;
    }

    HashInit(&dbUniqueNameTable, 32, 0);	/* Indexed by use-id */

    /*
     * Uses can't contain slashes (otherwise they'd interfere with
     * terminal naming conventions).  If the cellName has a slash,
     * just use the part of it after the last slash.
     */
    lastName = rindex(use->cu_def->cd_name, '/');
    if (lastName == NULL) lastName = use->cu_def->cd_name;
    else lastName++;
    
    /* This search must not be interrupted */
    SigDisableInterrupts();
    (void) DBCellEnum(parentDef, dbLinkFunc, (ClientData) lastName);
    SigEnableInterrupts();

    /* This loop terminates only on interrupt or if an empty useid is found */
    for (n = 0; !SigInterruptPending; n++)
    {
	(void) sprintf(useId, "%s_%d", lastName, n);
	he = HashLookOnly(&dbUniqueNameTable, useId);
	if (he == (HashEntry *) NULL)
	{
	    HashKill(&dbUniqueNameTable);
	    use->cu_id = StrDup((char **) NULL, useId);
	    DBSetUseIdHash(use, parentDef);
	    return (TRUE);
	}
    }

    HashKill(&dbUniqueNameTable);
    return (FALSE);
}

/*
 * dbLinkFunc --
 *
 * Filter function called via DBCellEnum by DBLinkCell above.
 * Creates an entry in the hash table dbUniqueNameTable to
 * indicate that the name "defname_#" is not available.
 */

int
dbLinkFunc(cellUse, defname)
    CellUse *cellUse;
    register char *defname;
{
    register char *usep = cellUse->cu_id;

    /* Skip in the unlikely event that this cell has no use-id */
    if (usep == (char *) NULL)
	return (0);

    /*
     * Only add names whose initial part matches 'defname',
     * and which are of the form 'defname_something'.
     */
    while (*defname)
	if (*defname++ != *usep++)
	    return 0;
    if (*usep++ != '_') return 0;
    if (*usep == '\0') return 0;

    /* Remember this name as being in use */
    (void) HashFind(&dbUniqueNameTable, cellUse->cu_id);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBReLinkCell --
 *
 * Change the instance id of the supplied CellUse.
 * If the instance id is non-NULL, and the new id is the same
 * as the old one, we do nothing.
 *
 * Results:
 *	Returns TRUE if successful, FALSE if the new name was not
 *	unique within the parent def.
 *
 * Side effects:
 *	May modify the cu_id of the supplied CellUse.
 *	Marks the parent of the cell use as having been modified.
 * ----------------------------------------------------------------------------
 */

bool
DBReLinkCell(cellUse, newName)
    CellUse *cellUse;
    char *newName;
{
    if (cellUse->cu_id && strcmp(cellUse->cu_id, newName) == 0)
	return (TRUE);

    if (DBFindUse(newName, cellUse->cu_parent))
	return (FALSE);

    if (cellUse->cu_parent)
	cellUse->cu_parent->cd_flags |= CDMODIFIED;

    /* Old id (may be NULL) */
    if (cellUse->cu_id)
	DBUnLinkCell(cellUse, cellUse->cu_parent);
    if (UndoIsEnabled()) DBUndoCellUse(cellUse, UNDO_CELL_CLRID);

    /* New id */
    (void) StrDup(&cellUse->cu_id, newName);
    DBSetUseIdHash(cellUse, cellUse->cu_parent);
    if (UndoIsEnabled()) DBUndoCellUse(cellUse, UNDO_CELL_SETID);
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBFindUse --
 *
 * Find a CellUse with the given name in the supplied parent CellDef.
 *
 * Results:
 *	Returns a pointer to the found CellUse, or NULL if it was not
 *	found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

CellUse *
DBFindUse(id, parentDef)
    char *id;
    CellDef *parentDef;
{
    HashEntry *he;

    he = HashLookOnly(&parentDef->cd_idHash, id);
    if (he == NULL)
	return (CellUse *) NULL;

    return (CellUse *) HashGetValue(he);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBGenerateUniqueIds --
 *
 * Make certain that the use-id of each CellUse under 'def' is unique
 * if it exists.  If duplicates are detected, all but one is freed and
 * set to NULL.
 *
 * The second pass consists of giving each CellUse beneath 'def' with
 * a NULL use-id a uniquely generated one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May modify the use-id's of the cells in the cell plane of 'def'.
 *	Prints error messages if use-ids had to be reassigned.
 *
 * ----------------------------------------------------------------------------
 */

Void
DBGenerateUniqueIds(def, warn)
    CellDef *def;
    bool warn;		/* If TRUE, warn user when we assign new ids */
{
    int dbFindNamesFunc();
    int dbGenerateUniqueIdsFunc();

    dbWarnUniqueIds = warn;
    HashInit(&dbUniqueDefTable, 32, 1);		/* Indexed by (CellDef *) */
    HashInit(&dbUniqueNameTable, 32, 0);	/* Indexed by use-id */

    /* Build up tables of names, complaining about duplicates */
    (void) DBCellEnum(def, dbFindNamesFunc, (ClientData) def);

    /* Assign unique use-ids to all cells */
    (void) DBCellEnum(def, dbGenerateUniqueIdsFunc, (ClientData) def);

    HashKill(&dbUniqueDefTable);
    HashKill(&dbUniqueNameTable);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbFindNamesFunc --
 *
 * Called via DBCellEnum() on behalf of DBGenerateUniqueIds() above,
 * for each subcell of the def just processed.  If any cell has a
 * use-id, we add it to our table of in-use names (dbUniqueNameTable).
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	If the name already is in the table, free this cell's use-id
 *	and set it to NULL.  In any event, add the name to the table.
 *
 * ----------------------------------------------------------------------------
 */

int
dbFindNamesFunc(use, parentDef)
    register CellUse *use;
    CellDef *parentDef;
{
    register HashEntry *he;

    if (use->cu_id)
    {
	he = HashFind(&dbUniqueNameTable, use->cu_id);
	if (HashGetValue(he))
	{
	    TxError("Duplicate instance-id for cell %s (%s) will be renamed\n",
			use->cu_def->cd_name, use->cu_id);
	    DBUnLinkCell(use, parentDef);
	    FREE(use->cu_id);
	    use->cu_id = (char *) NULL;
	}
	HashSetValue(he, use);
    }
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbGenerateUniqueIdsFunc --
 *
 * Called via DBCellEnum() on behalf of DBGenerateUniqueIds() above,
 * for each subcell of the def just processed.  If the cell has no
 * use-id, we generate one automatically.  In any event, install the
 * use identifier for each cell processed in the HashTable
 * parentDef->cd_idHash.
 *
 * Algorithm:
 *	We generate unique use-id's of the form def_# where #
 *	is an integer such that no other use-id in this cell has
 *	the same name.  The HashTable dbUniqueDefTable is indexed
 *	by CellDef and contains the highest sequence number (#
 *	above) processed for that CellDef.  Each time we process
 *	a cell, we start with this sequence number and continue
 *	to increment it until we find a name that is not in
 *	calmaNamesTable, then generate the use-id, and store
 *	the next sequence number in the HashEntry for the CellDef.
 *
 * Results:
 *	Returns 0 always.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

int
dbGenerateUniqueIdsFunc(use, parentDef)
    register CellUse *use;
    CellDef *parentDef;
{
    register HashEntry *hedef, *hename;
    register int suffix;
    char name[1024];

    if (use->cu_id)
	goto setHash;

    hedef = HashFind(&dbUniqueDefTable, (char *) use->cu_def);
    for (suffix = (int) HashGetValue(hedef); ; suffix++)
    {
	(void) sprintf(name, "%s_%d", use->cu_def->cd_name, suffix);
	hename = HashLookOnly(&dbUniqueNameTable, name);
	if (hename == NULL)
	    break;
    }

    if (dbWarnUniqueIds)
	TxPrintf("Setting instance-id of cell %s to %s\n",
		use->cu_def->cd_name, name);
    use->cu_id = StrDup((char **) NULL, name);
    HashSetValue(hedef, suffix+1);

setHash:
    DBSetUseIdHash(use, parentDef);
    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBSetUseIdHash --
 *
 * Update the use-id hash table in parentDef to reflect the fact
 * that 'use' now has instance-id use->cu_id.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

DBSetUseIdHash(use, parentDef)
    CellUse *use;
    CellDef *parentDef;
{
    HashEntry *he;

    he = HashFind(&parentDef->cd_idHash, use->cu_id);
    HashSetValue(he, (ClientData) use);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBUnLinkCell --
 *
 * Update the use-id hash table in parentDef to reflect the fact
 * that 'use' no longer is known by instance-id use->cu_id.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

DBUnLinkCell(use, parentDef)
    CellUse *use;
    CellDef *parentDef;
{
    HashEntry *he;

    if (he = HashLookOnly(&parentDef->cd_idHash, use->cu_id))
	HashSetValue(he, (ClientData) NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBNewYank --
 *
 * Create a new yank buffer with name 'yname'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in *pydef with a newly created CellDef by that name, and
 *	*pyuse with a newly created CellUse pointing to the new def.
 *	The CellDef pointed to by *pydef has the CD_INTERNAL flag
 *	set, and is marked as being available.
 *
 * ----------------------------------------------------------------------------
 */

DBNewYank(yname, pyuse, pydef)
    char *yname;	/* Name of yank buffer */
    CellUse **pyuse;	/* Pointer to new cell use is stored in *pyuse */
    CellDef **pydef;	/* Similarly for def */
{
    *pydef = DBCellLookDef(yname);
    if (*pydef == (CellDef *) NULL)
    {
	*pydef = DBCellNewDef (yname,(char *) NULL);
	ASSERT(*pydef != (CellDef *) NULL, "DBNewYank");
	DBCellSetAvail(*pydef);
	(*pydef)->cd_flags |= CDINTERNAL;
    }
    *pyuse = DBCellNewUse(*pydef, (char *) NULL);
    DBSetTrans(*pyuse, &GeoIdentityTransform);
    (*pyuse)->cu_expandMask = (~0);	/* This is always expanded. */
}
