/*
 * magicusage --
 *
 * Produce a listing of all files used in Magic or Caesar design.
 *
 * Usage:
 *	magicusage [-T tech] [-p path] rootcell1 rootcell2 ... 
 *
 * Produces on the standard output a list of all files used in the
 * designs rooted at rootcell1, rootcell2, ...
 *
 * Reads the user's .magic file(s) for a search path; we look first
 * in ~cad/lib/magic/sys, then in ~, then in . for a .magic file that
 * contains a path command, and use the last one.  If the '-p path'
 * flag is specified, we use 'path' instead.
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

#include <stdio.h>
#include "magic.h"
#include "paths.h"
#include "hash.h"
#include "stack.h"
#include "utils.h"
#include "pathvisit.h"
#include "malloc.h"
#include <ctype.h>

#ifndef lint
static char rcsid[]="$Header: magicusage.c,v 6.0 90/08/28 19:07:55 mayo Exp $";
#endif  not lint

#define	INIT_HASH_SIZE	512

char *SearchPath = NULL;
char TechDefault[] = "nmos";
char TechReal[BUFSIZ];
char *TechName = TechDefault;
char LibSearchPath[BUFSIZ];

HashTable KnownCells;
Stack *NameStack;

typedef struct
{
    char	*c_name;	/* Name of cell */
    char	*c_file;	/* File containing cell */
} Cell;

main(argc, argv)
    char *argv[];
{
    HashInit(&KnownCells, INIT_HASH_SIZE, 0);
    NameStack = StackNew(100);
    StackPush((ClientData) NULL, NameStack);

    argc--, argv++;
    if (argc <= 0)
	goto usage;

    while (argc--)
    {
	if (argv[0][0] == '-')
	{
	    switch (argv[0][1])
	    {
		case 'T':
		    if (TechName != TechDefault)
		    {
			printf("At most one -T flag allowed\n");
			goto usage;
		    }
		    if (argv[0][2]) TechName = &argv[0][2];
		    else if (argc-- == 0) goto usage;
		    else TechName = *++argv;
		    argv++;
		    break;

		case 'p':
		    if (SearchPath)
		    {
			printf("At most one -p flag allowed\n");
			goto usage;
		    }
		    if (argv[0][2]) SearchPath = &argv[0][2];
		    else if (argc-- == 0) goto usage;
		    else SearchPath = *++argv;
		    argv++;
		    break;

		default:
		    printf("Unrecognized flag: \"%s\"\n", argv[0]);
		    goto usage;
	    }
	}
	else StackPush((ClientData) StrDup((char **) NULL, *argv++), NameStack);
    }

    (void) sprintf(LibSearchPath, MAGIC_LIB_PATH, TechName);
    if (SearchPath == NULL)
	LoadSearchPath(&SearchPath);
    ReadAll();
    exit (0);

usage:
    printf("Usage: magicusage [-T tech] [-p path] rootcell1 rootcell2 ...\n");
    exit (1);
}

ReadAll()
{
    char *name;

    while (name = (char *) StackPop(NameStack))
	ReadChildren(name);
}

ReadChildren(cellName)
    char *cellName;
{
    HashEntry *hp;
    Cell *cell;
    char *fileName;
    FILE *f;

    hp = HashFind(&KnownCells, cellName);
    cell = (Cell *) HashGetValue(hp);
    if (cell)
	return;

    cell = (Cell *) mallocMagic(sizeof (Cell));
    HashSetValue(hp, (ClientData) cell);
    cell->c_name = cellName;
    f = PaOpen(cellName, "r", ".mag", SearchPath, LibSearchPath, &fileName);
    if (f == NULL)
    {
	cell->c_file = (char *) NULL;
	printf("%s ::: <<not found>>\n", cellName);
	return;
    }
    printf("%s ::: %s\n", cellName, fileName);
    cell->c_file = StrDup((char **) NULL, fileName);
    ReadUses(f);
    fclose(f);
}

ReadUses(f)
    register FILE *f;
{
    register c;
    char line[512], cellname[256], useid[256], tech[256];

    while ((c = getc(f)) != EOF)
    {
	switch (c)
	{
	    default:
		while (c != '\n' && c != EOF)
		    c = getc(f);
		break;
	    case 't':
		line[0] = c;
		if (fgets(&line[1], sizeof line - 2, f) == NULL)
		    break;
		if (sscanf(line, "tech %255s", tech) == 1)
		    if (TechName == TechDefault)
		    {
			strcpy(TechReal, tech);
			TechName = TechReal;
			(void) sprintf(LibSearchPath, MAGIC_LIB_PATH, TechName);
		    }
		break;
	    case 'u':
		line[0] = c;
		if (fgets(&line[1], sizeof line - 2, f) == NULL)
		    break;
		if (sscanf(line, "use %255s %255s", cellname, useid) >= 1)
		    StackPush(StrDup((char **) NULL, cellname), NameStack);
		break;
	}
    }
}

LoadSearchPath(path)
    char **path;	/* *path should be either NULL or point to something
			 * allocated from the heap.
			 */
{
    PaVisit *pv, *PaVisitInit();
    int loadPathFunc();

    *path = NULL;
    pv = PaVisitInit();
    PaVisitAddClient(pv, "path", loadPathFunc, (ClientData) path);
    PaVisitFiles(DOT_MAGIC_PATH, ".magic", pv);
    PaVisitFree(pv);
    if (*path == NULL)
	*path = ".";
}

loadPathFunc(line, ppath)
    char *line;
    char **ppath;
{
    register char *cp, *dp, c;
    char path[BUFSIZ];
    FILE *f;

    /* Skip leading blanks */
    for (cp = &line[4]; *cp && isspace(*cp); cp++)
	/* Nothing */;

    /* Copy the path into 'path' */
    for (dp = path; (c = *cp++) && !isspace(c) && c != '\n'; )
    {
	if (c == '"')
	{
	    while ((c = *cp++) && c != '"') *dp++ = c;
	    if (c == '\0') break;
	}
	else *dp++ = c;
    }
    *dp = '\0';
    (void) StrDup(ppath, path);
}

