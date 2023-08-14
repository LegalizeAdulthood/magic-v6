/*
 * EFargs.c -
 *
 * General command-line argument processing and overall initialization
 * for the .ext file flattener.
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
static char rcsid[] = "$Header: EFargs.c,v 6.0 90/08/28 18:13:23 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#ifdef SYSV
#include <string.h>
#endif
#include "magic.h"
#include "paths.h"
#include "geometry.h"
#include "hash.h"
#include "utils.h"
#include "malloc.h"
#include "pathvisit.h"
#include "extflat.h"
#include "EFint.h"

/* --------------------- Visible outside extflat ---------------------- */

    /* Command-line flags */
int EFCapThreshold = 10;	/* -c/-C: (fF) smallest interesting C */
int EFResistThreshold = 10;	/* -r/-R: (Ohms) smallest interesting R */
int EFTrimFlags = 0;		/* -t: output of nodename trailing #!'s */
char *EFSearchPath = NULL;	/* -p: Search path for .ext files */
char *EFArgTech = NULL;		/* -T: Tech specified on command line */

    /* Misc globals */
int EFScale = 0;		/* Uninitialized scale factor */
char *EFVersion = "5.0";	/* Version number of .ext format we read */
char *EFLibPath = NULL;		/* Library search path for .ext files */
char *EFTech = "";		/* Start with no technology */

/* -------------------- Visible only inside extflat ------------------- */

    /* Command-line flags */
bool efWarn = FALSE;		/* -v: Warn about duplicate node names */
bool efHNStats = FALSE;		/* -z: TRUE if we gather mem usage stats */
bool efWatchNodes = FALSE;	/* -n: TRUE if watching nodes in table below */
HashTable efWatchTable;		/* -n: Names to watch, keyed by HierName */

    /* Misc globals */
int efResists[128];		/* Sheet resistivity for each resist class */
int efNumResistClasses = 0;	/* Number of resist classes */
bool efResistChanged = FALSE;	/* TRUE if .ext resist classes mismatch */
bool efScaleChanged = FALSE;	/* TRUE if .ext scales mismatch */

    /* Forward declarations */
int efLoadPathFunc();

/*
 * ----------------------------------------------------------------------------
 *
 * EFArgs --
 *
 * Process command-line arguments that are relevant to the extractor
 * flattener.  Arguments that are specific to the calling function
 * are processed by the procedure (*argsProc)(), which should
 * have the following form:
 *
 *	(*argsProc)(pargc, pargv, cdata)
 *	    int *pargc;
 *	    char ***pargv;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * If we don't recognize an argument, we call (*argsProc)() with
 * *pargc and *pargv pointing to the position in the argument
 * vector that we didn't recognize.  If (*argsProc)() also doesn't
 * recognize the argument, it exits; otherwise, it updates *pargc
 * and *pargv to point past the argument it gobbled off and returns.
 * If argsProc is NULL, then any arguments we don't recognize are
 * considered errors.
 *
 * Arguments processed are:
 *
 *	-T techname	Specify the name of the technology, leaving
 *			EFArgTech pointing to the technology name.
 *	-p path		Use the colon-separated search path 'path'
 *			for finding .ext files.  Overrides any paths
 *			found in .magic files.
 *	-s sym=value	Set the name 'sym' in the symbol hash table to
 *			have value 'value', where 'value' is an integer.
 *			Certain attributes interpreted during circuit
 *			flattening may have symbolic values; the -s flag
 *			provides a means of associating a numeric value
 *			with a symbol.
 *	-S symfile	Read the file 'symfile', which should consist of
 *			lines of the form sym=value, processing each line
 *			as though it were an argument to -s.
 *
 * The following flags are for debugging purposes only:
 *	-n nodename	For debugging: print all merges involving
 *			the node named 'nodename'.
 *	-N nodefile	For debugging: print all merges involving
 *			any of the nodes whose names appear in the
 *			file 'nodefile' (one node name per line).
 *	-v		Warn about unusual occurrences while flattening
 *			the circuit; mainly for debugging.
 *	-z		Print memory utilized for names.
 *
 * Results:
 *	Returns a pointer to a string containing the base name
 *	of the input .ext file.
 *
 * Side effects:
 *	Can set global variables based on the values of command-line
 *	arguments.
 *
 * ----------------------------------------------------------------------------
 */

char *
EFArgs(argc, argv, argsProc, cdata)
    int argc;		/* Number of command-line args */
    char *argv[];	/* Vector of command-line args */
    Void (*argsProc)();	/* Called for args we don't recognize */
    ClientData cdata;	/* Passed to (*argsProc)() */
{
    static char libpath[FNSIZE];
    char *realIn, line[1024], *inname = NULL, *name, *cp;
    HierName *hierName;
    FILE *f;

    /* Hash table of nodes we're going to watch if -N given */
    HashInitClient(&efWatchTable, 32, HT_CLIENTKEYS,
	efHNCompare, (char *(*)()) NULL,
	efHNHash, (Void (*)()) NULL);

    /* Process command line options */
    for (argc--, argv++; argc-- > 0; argv++)
    {
	if (argv[0][0] != '-')
	{
	    if (inname)
	    {
		(void) printf("Warning: multiple input files specified; ");
		(void) printf("ignoring %s\n", inname);
	    }
	    inname = argv[0];
	    continue;
	}

	switch (argv[0][1])
	{
	    /*** NORMAL OPTIONS ***/
	    case 'c':
		if ((cp = ArgStr(&argc, &argv, "cap threshold")) == NULL)
		    goto usage;
		EFCapThreshold = atoi(cp);	/* Femtofarads */
		break;
	    case 'p':
		EFSearchPath = ArgStr(&argc, &argv, "search path");
		if (EFSearchPath == NULL)
		    goto usage;
		break;
	    case 'r':
		if ((cp = ArgStr(&argc, &argv, "resist threshold")) == NULL)
		    goto usage;
		EFResistThreshold = atoi(cp);	/* Ohms */
		break;
	    case 's':
		if ((cp = ArgStr(&argc, &argv, "symbolic name")) == NULL)
		    goto usage;
		efSymAdd(cp);
		break;
	    case 't':
		if ((cp = ArgStr(&argc, &argv, "trim characters")) == NULL)
		    goto usage;
		if (index(cp, '!')) EFTrimFlags |= EF_TRIMGLOB;
		if (index(cp, '#')) EFTrimFlags |= EF_TRIMLOCAL;
		break;
	    case 'C':
		EFCapThreshold = INFINITE_THRESHOLD;
		break;
	    case 'R':
		EFResistThreshold = INFINITE_THRESHOLD;
		break;
	    case 'S':
		if ((cp = ArgStr(&argc, &argv, "symbol file")) == NULL)
		    goto usage;
		efSymAddFile(cp);
		break;
	    case 'T':
		if ((EFArgTech = ArgStr(&argc, &argv, "tech name")) == NULL)
		    goto usage;
		break;

	    /*** OPTIONS FOR DEBUGGING ***/
	    case 'n':
		if ((name = ArgStr(&argc, &argv, "nodename")) == NULL)
		    goto usage;
		printf("Watching node '%s'\n", name);
		hierName = EFStrToHN((HierName *) NULL, name);
		(void) HashFind(&efWatchTable, (char *) hierName);
		efWatchNodes = TRUE;
		break;
	    case 'N':
		if ((name = ArgStr(&argc, &argv, "filename")) == NULL)
		    goto usage;

		/* Add everything in the file to the hash table */
		f = fopen(name, "r");
		if (f == NULL)
		{
		    perror(name);
		    break;
		}
		while (fgets(line, sizeof line, f))
		{
		    cp = index(line, '\n');
		    if (cp) *cp = '\0';
		    printf("Watching node '%s'\n", line);
		    hierName = EFStrToHN((HierName *) NULL, line);
		    (void) HashFind(&efWatchTable, (char *) hierName);
		}
		(void) fclose(f);
		efWatchNodes = TRUE;
		break;
	    case 'v':
		efWarn = TRUE;
		break;
	    case 'z':
		efHNStats = TRUE;
		break;

	    /*** Try a caller-supplied argument processing function ***/
	    default:
		if (argsProc == NULL)
		    goto usage;
		(*argsProc)(&argc, &argv, cdata);
		break;
	}
    }

    if (inname == NULL)
	goto usage;

    /* Find the search path if one was not specified */
    if (EFSearchPath == NULL)
	efLoadSearchPath(&EFSearchPath);

    /* Eliminate trailing .ext from input name */
    if ((cp = rindex(inname, '.')) && strcmp(cp, ".ext") == 0)
    {
	MALLOC(char *, realIn, cp - inname + 1);
	(void) strncpy(realIn, inname, cp - inname);
	realIn[cp - inname] = '\0';
	inname = realIn;
    }

    EFLibPath = libpath;
    *EFLibPath = 0; /* start with no path */
    if (EFArgTech) (void) sprintf(EFLibPath, EXT_PATH, EFArgTech);
    return inname;

usage:
    (void) printf("\
Valid argument usage: [-R] [-C] [-T techname] [-r rthresh] [-c cthresh] [-v]\n\
	       [-p searchpath] [-s sym=value] [-S symfile] [-t trimchars]\n\
	       rootfile\n");
    exit (1);
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * efLoadSearchPath --
 *
 * Load the search path string pointed to by 'path'
 * with whatever is specified in the .magic files
 * in ~cad/lib/magic/sys, ~, and ., searched in that
 * order with the last path taking precedence. See paths.h.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Leaves *path pointing to the correct search path,
 *	which may either be static or allocated via StrDup().
 *
 * ----------------------------------------------------------------------------
 */

Void
efLoadSearchPath(path)
    char **path;
{
    PaVisit *pv;

    *path = NULL;
    pv = PaVisitInit();
    PaVisitAddClient(pv, "path", efLoadPathFunc, (ClientData) path);
    (void) PaVisitFiles(DOT_MAGIC_PATH, ".magic", pv);
    PaVisitFree(pv);
    if (*path == NULL)
	*path = ".";
}

efLoadPathFunc(line, ppath)
    char *line;
    char **ppath;
{
    register char *cp, *dp, c;
    char path[BUFSIZ];

    /* Skip leading blanks */
    for (cp = &line[4]; *cp && isspace(*cp); cp++)
	/* Nothing */;

    /* Copy the path into 'path' */
    for (dp = path; (c = *cp++) && !isspace(c) && c != '\n'; )
    {
	if (c == '"')
	{
	    while ((c = *cp++) && c != '"')
		*dp++ = c;
	    if (c == '\0')
		break;
	}
	else *dp++ = c;
    }
    *dp = '\0';
    (void) StrDup(ppath, path);
}
