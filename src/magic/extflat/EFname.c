/*
 * EFhier.c -
 *
 * Procedures for manipulating HierNames.
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
static char rcsid[] = "$Header: EFname.c,v 6.0 90/08/28 18:13:37 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <math.h>
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "hash.h"
#include "malloc.h"
#include "utils.h"
#include "extflat.h"
#include "EFint.h"

/*
 * Hash table containing all flattened node names.
 * The keys in this table are HierNames, processed by the
 * procedures efHNCompare(), efHNHash(), efHierCopy(),
 * and efHierKill().
 */
HashTable efNodeHashTable;

/*
 * Hash table used by efHNFromUse to ensure that it always returns
 * a pointer to the SAME HierName structure each time it is called
 * with the same fields.
 */
HashTable efHNUseHashTable;

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNIsGlob --
 *
 * Determine whether a HierName is of the format of a global name,
 * i.e, it ends in a '!'.
 *
 * Results:
 *	TRUE if the name is a global.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
EFHNIsGlob(hierName)
    register HierName *hierName;
{
    return hierName->hn_name[strlen(hierName->hn_name) - 1] == '!';
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNIsGND --
 *
 * Determine whether a HierName is the same as the global signal GND.
 *
 * Results:
 *	TRUE if the name is GND, false if not.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
EFHNIsGND(hierName)
    register HierName *hierName;
{
    return (hierName->hn_parent == (HierName *) NULL
	    && strcmp(hierName->hn_name, "GND!") == 0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNConcat --
 *
 * Given a HierName prefix and a HierName suffix, make a newly allocated
 * copy of the suffix that points to the prefix.
 *
 * Results:
 *	Pointer to the new HierName as described above.
 *
 * Side effects:
 *	May allocate memory.
 *
 * ----------------------------------------------------------------------------
 */

HierName *
EFHNConcat(prefix, suffix)
    HierName *prefix;		/* Components of name on root side */
    register HierName *suffix;	/* Components of name on leaf side */
{
    register HierName *new, *prev;
    HierName *firstNew;
    unsigned size;

    for (firstNew = prev = (HierName *) NULL;
	    suffix;
	    prev = new, suffix = suffix->hn_parent)
    {
	size = HIERNAMESIZE(strlen(suffix->hn_name));
	MALLOC(HierName *, new, size);
	if (efHNStats) efHNRecord(size, HN_CONCAT);
	new->hn_hash = suffix->hn_hash;
	(void) strcpy(new->hn_name, suffix->hn_name);
	if (prev)
	    prev->hn_parent = new;
	else
	    firstNew = new;
    }
    prev->hn_parent = prefix;

    return firstNew;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFStrToHN --
 *
 * Given a hierarchical prefix (the HierName pointed to by prefix)
 * and a name relative to that prefix (the string 'suffixStr'), return a
 * pointer to the HierName we should use.  Normally, this is just a newly
 * built HierName containing the path components of 'suffixStr' appended to
 * prefix.
 *
 * Results:
 *	Pointer to a name determined as described above.
 *
 * Side effects:
 *	May allocate new HierNames.
 *
 * ----------------------------------------------------------------------------
 */

HierName *
EFStrToHN(prefix, suffixStr)
    HierName *prefix;	/* Components of name on side of root */
    char *suffixStr;	/* Leaf part of name (may have /'s) */
{
    register char *cp;
    register HashEntry *he;
    char *slashPtr;
    HierName *hierName;
    unsigned size;
    int len;

    /* Skip to the end of the relative name */
    slashPtr = NULL;
    for (cp = suffixStr; *cp; cp++)
	if (*cp == '/')
	    slashPtr = cp;

    /*
     * Convert the relative name into a HierName path, with one HierName
     * created for each slash-separated segment of suffixStr.
     */
    cp = slashPtr = suffixStr;
    for (;;)
    {
	if (*cp == '/' || *cp == '\0')
	{
	    size = HIERNAMESIZE(cp - slashPtr);
	    MALLOC(HierName *, hierName, size);
	    if (efHNStats) efHNRecord(size, HN_ALLOC);
	    efHNInit(hierName, slashPtr, cp);
	    hierName->hn_parent = prefix;
	    if (*cp++ == '\0')
		break;
	    slashPtr = cp;
	    prefix = hierName;
	}
	else cp++;
    }

    return hierName;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNToStr --
 *
 * Convert a HierName chain into a printable name.
 * Stores the result in a static buffer.
 *
 * Results:
 *	Returns a pointer to the static buffer containing the
 *	printable name.
 *
 * Side effects:
 *	Overwrites the previous contents of the static buffer.
 *
 * ----------------------------------------------------------------------------
 */

char *
EFHNToStr(hierName)
    HierName *hierName;		/* Name to be converted */
{
    static char namebuf[2048];

    (void) efHNToStrFunc(hierName, namebuf);
    return namebuf;
}

/*
 * efHNToStrFunc --
 *
 * Recursive part of name conversion.
 * Calls itself recursively on hierName->hn_parent and dstp,
 * adding the prefix of the pathname to the string pointed to
 * by dstp.  Then stores hierName->hn_name at the end of the
 * just-stored prefix, and returns a pointer to the end.
 *
 * Results:
 *	Returns a pointer to the null byte at the end of
 *	all the pathname components stored so far in dstp.
 *
 * Side effects:
 *	Stores characters in dstp.
 */

char *
efHNToStrFunc(hierName, dstp)
    HierName *hierName;		/* Name to be converted */
    register char *dstp;	/* Store name here */
{
    register char *srcp;

    if (hierName == NULL)
    {
	*dstp = '\0';
	return dstp;
    }

    if (hierName->hn_parent)
    {
	dstp = efHNToStrFunc(hierName->hn_parent, dstp);
	*dstp++ = '/';
    }

    srcp = hierName->hn_name;
    while (*dstp++ = *srcp++)
	/* Nothing */;

    return --dstp;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNLook --
 *
 * Look for the entry in the efNodeHashTable whose name is formed
 * by concatenating suffixStr to prefix.  If there's not an
 * entry in efNodeHashTable, or the entry has a NULL value, complain
 * and return NULL; otherwise return the HashEntry.
 *
 * The string errorStr should say what we were processing, e.g,
 * "fet", "connect(1)", "connect(2)", etc., for use in printing
 * error messages.  If errorStr is NULL, we don't print any error
 * messages.
 *
 * Results:
 *	See above.
 *
 * Side effects:
 *	Allocates memory temporarily to build the key for HashLookOnly(),
 *	but then frees it before returning.
 *
 * ----------------------------------------------------------------------------
 */

HashEntry *
EFHNLook(prefix, suffixStr, errorStr)
    HierName *prefix;	/* Components of name on root side */
    char *suffixStr;	/* Part of name on leaf side */
    char *errorStr;	/* Explanatory string for errors */
{
    HierName *hierName, *hn;
    bool dontFree = FALSE;
    HashEntry *he;

    if (suffixStr == NULL)
    {
	hierName = prefix;
	dontFree = TRUE;
    }
    else hierName = EFStrToHN(prefix, suffixStr);

    he = HashLookOnly(&efNodeHashTable, (char *) hierName);
    if (he == NULL || HashGetValue(he) == NULL)
    {
	if (errorStr)
	    printf("%s: no such node %s\n", errorStr, EFHNToStr(hierName));
	he = NULL;
    }

    /*
     * Free the portion of the HierName we just allocated for
     * looking in the table, if we allocated one.
     */
    if (!dontFree)
	EFHNFree(hierName, prefix, HN_ALLOC);

    return he;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNConcatLook --
 *
 * Like EFHNLook above, but the argument suffix is itself a HierName.
 * We construct the full name by concatenating the hierarchical prefix
 * and the node name 'suffix', then looking it up in the flat node
 * table for its real name.
 *
 * Results:
 *	See EFHNLook()'s comments.
 *
 * Side effects:
 *	See EFHNLook()'s comments.
 *
 * ----------------------------------------------------------------------------
 */

HashEntry *
EFHNConcatLook(prefix, suffix, errorStr)
    HierName *prefix;	/* Components of name on root side */
    HierName *suffix;	/* Part of name on leaf side */
    char *errorStr;	/* Explanatory string for errors */
{
    HashEntry *he;
    HierName *hn;
    EFNodeName *nn;

    /*
     * Find the last component of the suffix, then temporarily
     * link the HierNames for use as a hash key.  This is safe
     * because HashLookOnly() doesn't ever store anything in the
     * hash table, so we don't have to worry about this temporarily
     * built key somehow being saved without our knowledge.
     */
    hn = suffix;
    while (hn->hn_parent)
	hn = hn->hn_parent;
    hn->hn_parent = prefix;

    he = HashLookOnly(&efNodeHashTable, (char *) suffix);
    if (he == NULL || HashGetValue(he) == NULL)
    {
	printf("%s: no such node %s\n", errorStr, EFHNToStr(suffix));
	he = (HashEntry *) NULL;
    }

    /* Undo the temp link */
    hn->hn_parent = (HierName *) NULL;
    return he;
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNFree --
 *
 * Free a list of HierNames, up to but not including the element pointed
 * to by 'prefix' (or the NULL indicating the end of the HierName list).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 * ----------------------------------------------------------------------------
 */

Void
EFHNFree(hierName, prefix, type)
    HierName *hierName, *prefix;
    int type;	/* HN_ALLOC, HN_CONCAT, etc */
{
    HierName *hn;

    for (hn = hierName; hn; hn = hn->hn_parent)
    {
	if (hn == prefix)
	    break;

	FREE((char *) hn);
	if (efHNStats)
	{
	    int len = strlen(hn->hn_name);
	    efHNRecord(-HIERNAMESIZE(len), type);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * EFHNBest --
 *
 * Determine which of two names is more preferred.  The most preferred
 * name is a global name.  Given two non-global names, the one with the
 * fewest pathname components is the most preferred.  If the two names
 * have equally many pathname components, we choose the shortest.
 * If they both are of the same length, we choose the one that comes
 * later in the alphabet.
 *
 * Results:
 *	TRUE if the first name is preferable to the second, FALSE if not.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

bool
EFHNBest(hierName1, hierName2)
    HierName *hierName1, *hierName2;
{
    int ncomponents1, ncomponents2, len1, len2;
    register HierName *np1, *np2;
    char last1, last2;

    for (ncomponents1 = 0, np1 = hierName1; np1; np1 = np1->hn_parent)
	ncomponents1++;
    for (ncomponents2 = 0, np2 = hierName2; np2; np2 = np2->hn_parent)
	ncomponents2++;

    last1 = hierName1->hn_name[strlen(hierName1->hn_name) - 1];
    last2 = hierName2->hn_name[strlen(hierName2->hn_name) - 1];
    if (last1 != '!' || last2 != '!')
    {
	/* Prefer global over local names */
	if (last1 == '!') return TRUE;
	if (last2 == '!') return FALSE;

	/* Neither name is global, so chose label over generated name */
	if (last1 != '#' && last2 == '#') return TRUE;
	if (last1 == '#' && last2 != '#') return FALSE;
    }

    /*
     * Compare two names the hard way.  Both are of the same class,
     * either both global or both non-global, so compare in order:
     * number of pathname components, length, and lexicographic
     * ordering.
     */
    if (ncomponents1 < ncomponents2) return TRUE;
    if (ncomponents1 > ncomponents2) return FALSE;

    /* Same # of pathname components; check length */
    for (len1 = 0, np1 = hierName1; np1; np1 = np1->hn_parent)
	len1 += strlen(np1->hn_name);
    for (len2 = 0, np2 = hierName2; np2; np2 = np2->hn_parent)
	len2 += strlen(np2->hn_name);
    if (len1 < len2) return TRUE;
    if (len1 > len2) return FALSE;

    return (efHNLexOrder(hierName1, hierName2) > 0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNLexOrder --
 *
 * Procedure to ensure that the canonical ordering used in determining
 * preferred names is the same as would have been used if we were comparing
 * the string version of two HierNames, instead of comparing them as pathnames
 * with the last component first.
 *
 * Results:
 *	Same as strcmp(), i.e,
 *		-1 if hierName1 should precede hierName2 lexicographically,
 *		+1 if hierName1 should follow hierName2, and
 *		 0 is they are identical.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
efHNLexOrder(hierName1, hierName2)
    register HierName *hierName1, *hierName2;
{
    register int i;

    if (hierName1 == hierName2)
	return 0;

    if (hierName1->hn_parent)
	if (i = efHNLexOrder(hierName1->hn_parent, hierName2->hn_parent))
	    return i;

    return strcmp(hierName1->hn_name, hierName2->hn_name);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNFromUse --
 *
 * Construct a HierName for a cell use (for the array element identified
 * by (hc_x, hc_y) if the use is an array).  The parent pointer of this
 * newly allocated HierName will be set to prefix.
 *
 * Results:
 *	Returns a pointer to a newly allocated HierName.
 *
 * Side effects:
 *	See above.
 *	Note: we use a hash table to ensure that we always return
 *	the SAME HierName whenever prefix, the (x, y) use
 *	coordinates, and the use id are the same.
 *
 * ----------------------------------------------------------------------------
 */

HierName *
efHNFromUse(hc, prefix)
    HierContext *hc;	/* Contains use and array information */
    HierName *prefix;	/* Root part of name */
{
    register char *srcp, *dstp;
    char name[2048], *namePtr;
    Use *u = hc->hc_use;
    HierName *hierName;
    bool hasX, hasY;
    HashEntry *he;
    unsigned size;

    hasX = u->use_xlo != u->use_xhi;
    hasY = u->use_ylo != u->use_yhi;
    namePtr = u->use_id;
    if (hasX || hasY)
    {
	namePtr = name;
	srcp = u->use_id;
	dstp = name;
	while (*dstp++ = *srcp++)
	    /* Nothing */;

	/* Array subscript */
	dstp[-1] = '[';

	/* Y comes before X */
	if (hasY)
	{
	    (void) sprintf(dstp, "%d", hc->hc_y);
	    while (*dstp++)
		/* Nothing */;
	    dstp--;		/* Leave pointing to NULL byte */
	}

	if (hasX)
	{
	    if (hasY) *dstp++ = ',';
	    (void) sprintf(dstp, "%d", hc->hc_x);
	    while (*dstp++)
		/* Nothing */;
	    dstp--;		/* Leave pointing to NULL byte */
	}

	*dstp++ = ']';
	*dstp = '\0';
    }

    size = HIERNAMESIZE(strlen(namePtr));
    MALLOC(HierName *, hierName, size);
    if (efHNStats) efHNRecord(size, HN_FROMUSE);
    efHNInit(hierName, namePtr, (char *) NULL);
    hierName->hn_parent = prefix;

    /* See if we already have an entry for this one */
    he = HashFind(&efHNUseHashTable, (char *) hierName);
    if (HashGetValue(he))
    {
	FREE((char *) hierName);
	return (HierName *) HashGetValue(he);
    }
    HashSetValue(he, (ClientData) hierName);

    (void) HashFind(&efFreeHashTable, (char *) hierName);

    return hierName;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNUseCompare --
 *
 *	Compare two HierNames for equality, but using a different sense
 *	of comparison than efHNCompare: two names are considered equal
 *	only if their hn_parent fields are equal and their hn_name strings
 *	are identical.
 *
 * Results: Returns 0 if they are equal, 1 if not.
 *
 * efHNUseHash --
 *
 *	Convert a HierName to a single 32-bit value suitable for being
 *	turned into a hash bucket by the hash module.  Hashes based on
 *	hierName->hn_hash and hierName->hn_parent, rather than summing
 *	the hn_hash values.
 *
 * Results: Returns the 32-bit hash value.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
efHNUseCompare(hierName1, hierName2)
    register HierName *hierName1, *hierName2;
{
    return (hierName1->hn_parent != hierName2->hn_parent
	    || strcmp(hierName1->hn_name, hierName2->hn_name));
}

int
efHNUseHash(hierName)
    register HierName *hierName;
{
    return hierName->hn_hash + (int) hierName->hn_parent;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNInit --
 *
 * Copy the string 'cp' into hierName->hn_name, also initializing
 * the hn_hash fields of hierName.  If 'endp' is NULL, copy all
 * characters in 'cp' up to a trailing NULL byte; otherwise, copy
 * up to 'endp'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

Void
efHNInit(hierName, cp, endp)
    HierName *hierName;		/* Fill in fields of this HierName */
    register char *cp;		/* Start of name to be stored in hn_name */
    register char *endp;	/* End of name if non-NULL; else, see above */
{
    register unsigned hashsum;
    register char *dstp;

    hashsum = 0;
    dstp = hierName->hn_name;
    if (endp)
    {
	while (cp < endp)
	{
	    hashsum = HASHADDVAL(hashsum, *cp);
	    *dstp++ = *cp++;
	}
	*dstp = '\0';
    }
    else
    {
	while (*dstp++ = *cp)
	    hashsum = HASHADDVAL(hashsum, *cp++);
    }

    hierName->hn_hash = hashsum;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNCompare --
 *
 *	Compare two HierNames for equality.  Passed as a client procedure
 *	to the hash module.  The most likely place for a difference in the
 *	two names is in the lowest-level component, which fortunately is
 *	the first in a HierName list.
 *
 * Results:
 *	Returns 0 if they are equal, 1 if not.
 *
 * efHNHash --
 *
 *	Convert a HierName to a single 32-bit value suitable for being
 *	turned into a hash bucket by the hash module.  Passed as a client
 *	procedure to the hash module.
 *
 * Results:
 *	Returns the 32-bit hash value.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
efHNCompare(hierName1, hierName2)
    register HierName *hierName1, *hierName2;
{
    while (hierName1)
    {
	if (hierName1 == hierName2)
	    return 0;

	if (hierName2 == NULL
		|| hierName1->hn_hash != hierName2->hn_hash
		|| strcmp(hierName1->hn_name, hierName2->hn_name) != 0)
	    return 1;
	hierName1 = hierName1->hn_parent;
	hierName2 = hierName2->hn_parent;
    }

    return (hierName2 ? 1 : 0);
}

int
efHNHash(hierName)
    register HierName *hierName;
{
    register int n;

    for (n = 0; hierName; hierName = hierName->hn_parent)
	n += hierName->hn_hash;

    return n;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNDistCompare --
 * efHNDistCopy --
 * efHNDistHash --
 * efHNDistKill --
 *
 * Procedures for managing a HashTable whose keys are pointers
 * to malloc'd Distance structures.  Distances point to a pair of
 * HierNames; the comparison and hashing functions rely directly
 * on those for processing HierNames (efHNCompare() and efHNHash()).
 *
 * Results:
 *	efHNDistCompare returns 0 if the two keys are equal, 1 if not.
 *	efHNDistCopy returns a pointer to a malloc'd copy of its Distance
 *	    argument.
 *	efHNDistHash returns a single 32-bit hash value based on a Distance's
 *	    two HierNames.
 *
 * Side effects:
 *	efHNDistKill frees its Distance argument, and adds the HierNames
 *	pointed to by it to the table of HierNames to free.
 *
 * ----------------------------------------------------------------------------
 */

int
efHNDistCompare(dist1, dist2)
    register Distance *dist1, *dist2;
{
    return efHNCompare(dist1->dist_1, dist2->dist_1)
	|| efHNCompare(dist1->dist_2, dist2->dist_2);
}

char *
efHNDistCopy(dist)
    register Distance *dist;
{
    register Distance *distNew;

    MALLOC(Distance *, distNew, sizeof (Distance));
    *distNew = *dist;
    return (char *) distNew;
}

int
efHNDistHash(dist)
    register Distance *dist;
{
    return efHNHash(dist->dist_1) + efHNHash(dist->dist_2);
}


Void
efHNDistKill(dist)
    register Distance *dist;
{
    register HierName *hn;

    for (hn = dist->dist_1; hn; hn = hn->hn_parent)
	(void) HashFind(&efFreeHashTable, (char *) hn);
    for (hn = dist->dist_2; hn; hn = hn->hn_parent)
	(void) HashFind(&efFreeHashTable, (char *) hn);

    FREE((char *) dist);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNBuildDistKey --
 *
 * Build the key for looking in the Distance hash table for efFlatDists()
 * above.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up *distKey.
 *
 * ----------------------------------------------------------------------------
 */

Void
efHNBuildDistKey(prefix, dist, distKey)
    HierName *prefix;
    Distance *dist;
    Distance *distKey;
{
    HierName *hn1, *hn2;

    hn1 = EFHNConcat(prefix, dist->dist_1);
    hn2 = EFHNConcat(prefix, dist->dist_2);
    if (EFHNBest(hn1, hn2))
    {
	distKey->dist_1 = hn1;
	distKey->dist_2 = hn2;
    }
    else
    {
	distKey->dist_1 = hn2;
	distKey->dist_2 = hn1;
    }

    distKey->dist_min = dist->dist_min;
    distKey->dist_max = dist->dist_max;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efHNDump --
 *
 * Print all the names in the node hash table efNodeHashTable.
 * Used for debugging.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a file "hash.dump" and writes the node names to
 *	it, one per line.
 *
 * ----------------------------------------------------------------------------
 */

efHNDump()
{
    HashSearch hs;
    HashEntry *he;
    FILE *f;

    f = fopen("hash.dump", "w");
    if (f == NULL)
    {
	perror("hash.dump");
	return;
    }

    HashStartSearch(&hs);
    while (he = HashNext(&efNodeHashTable, &hs))
	(void) fprintf(f, "%s\n", EFHNToStr((HierName *) he->h_key.h_ptr));

    (void) fclose(f);
}

int efHNSizes[4];

efHNRecord(size, type)
    int size;
    int type;
{
    efHNSizes[type] += size;
}

efHNPrintSizes(when)
    char *when;
{
    int total, i;

    total = 0;
    for (i = 0; i < 4; i++)
	total += efHNSizes[i];

    printf("Memory used in HierNames %s:\n", when ? when : "");
    printf("%8d bytes for global names\n", efHNSizes[HN_GLOBAL]);
    printf("%8d bytes for concatenated HierNames\n", efHNSizes[HN_CONCAT]);
    printf("%8d bytes for names from cell uses\n", efHNSizes[HN_FROMUSE]);
    printf("%8d bytes for names from strings\n", efHNSizes[HN_ALLOC]);
    printf("--------\n");
    printf("%8d bytes total\n", total);
}
