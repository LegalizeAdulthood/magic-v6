/*
 * EFread.c -
 *
 * Procedures to read a .ext file and call the procedures
 * in EFbuild.c to build up a description of each def.
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
static char rcsid[] = "$Header: EFread.c,v 6.0 90/08/28 18:13:40 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "utils.h"
#include "extflat.h"
#include "EFint.h"
#include "paths.h"

/*
 * The following table describes the kinds of lines
 * that may be read in a .ext file.
 */
typedef enum
{
    ADJUST, ATTR, CAP, DIST, EQUIV, FET, KILLNODE, MERGE, NODE,
    RESISTOR, RESISTCLASS, RNODE, SCALE, TECH, TIMESTAMP, USE, VERSION
} Key;

static struct
{
    char	*k_name;	/* Name of first token on line */
    Key 	 k_key;		/* Internal name for token of this type */
    int		 k_mintokens;	/* Min total # of tokens on line of this type */
}
keyTable[] =
{
    "adjust",		ADJUST,		4,
    "attr",		ATTR,		8,
    "cap",		CAP,		4,
    "distance",		DIST,		4,
    "equiv",		EQUIV,		3,
    "fet",		FET,		12,
    "killnode",		KILLNODE,	2,
    "merge",		MERGE,		3,
    "node",		NODE,		7,
    "resist",		RESISTOR,	4,
    "resistclasses",	RESISTCLASS,	1,
    "rnode",		RNODE,		5,
    "scale",		SCALE,		4,
    "tech",		TECH,		2,
    "timestamp",	TIMESTAMP,	2,
    "use",		USE,		9,
    "version",		VERSION,	2,
    0
};

/* Data local to this file */
static char *efReadFileName;	/* Name of file currently being read */
static int efReadLineNum;	/* Current line number in above file */

/*
 * ----------------------------------------------------------------------------
 *
 * EFReadFile --
 *
 * Main procedure to read a .ext file.  If there is no Def by the
 * name of 'name', allocates a new one.  Calls efReadDef to do the
 * work of reading the def itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *	Leaves EFTech set to the technology specified with the -T flag
 *	if there was one.  Leaves EFScale set to 1 if it changed while
 *	reading the .ext files.
 *
 * ----------------------------------------------------------------------------
 */

EFReadFile(name)
    char *name;		/* Name of def to be read in */
{
    Def *def;

    def = efDefLook(name);
    if (def == NULL)
	def = efDefNew(name);

    efReadDef(def);
    if (EFArgTech) EFTech = StrDup((char **) NULL, EFArgTech);
    if (EFScale == 0) EFScale = 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * efReadDef --
 *
 * Procedure to read in a Def.  Actually does the work of reading
 * the file 'def->def_name'.ext to build up the fields of the new
 * def, then recursively reads all uses of this def that haven't
 * yet been read.
 *
 * Results:
 *	Returns TRUE if successful, FALSE if the file for 'name'
 *	could not be found.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

bool
efReadDef(def)
    Def *def;		/* Def to be read in */
{
    int argc, ac, n, cap;
    char line[1024], *argv[64], *name, *attrs;
    int rscale = 1;	/* Multiply resistances by this */
    int cscale = 1;	/* Multiply capacitances by this */
    int lscale = 1;	/* Multiply lambda by this */
    FILE *inf;
    Use *use;
    Rect r;

    /* Mark def as available */
    def->def_flags |= DEF_AVAILABLE;
    name = def->def_name;
    inf = PaOpen(name, "r", ".ext", EFSearchPath, EFLibPath, &efReadFileName);
    if (inf == NULL)
    {
	perror(name);
	return;
    }

    efReadLineNum = 0;
    while ((argc = efReadLine(line, sizeof line, inf, argv)) >= 0)
    {
	n = LookupStruct(argv[0], (LookupTable *) keyTable, sizeof keyTable[0]);
	if (n < 0)
	{
	    efReadError("Unrecognized token \"%s\" (ignored)\n", argv[0]);
	    continue;
	}
	if (argc < keyTable[n].k_mintokens)
	{
	    efReadError("Not enough tokens for %s line\n", argv[0]);
	    continue;
	}
	switch (keyTable[n].k_key)
	{
	    /* scale rscale cscale lscale */
	    case SCALE:
		rscale = atoi(argv[1]);
		if (rscale == 0)
		{
		    efReadError("Bad resistance scaling = 0; reset to 1.\n");
		    rscale = 1;
		}
		cscale = atoi(argv[2]);
		if (cscale == 0)
		{
		    efReadError("Bad capacitance scaling = 0; reset to 1.\n");
		    cscale = 1;
		}
		lscale = atoi(argv[3]);
		if (lscale == 0)
		{
		    efReadError("Bad linear scaling = 0; reset to 1.\n");
		    lscale = 1;
		}
		def->def_scale = lscale;
		if (EFScale != lscale)
		{
		    if (EFScale != 0) efScaleChanged = TRUE, EFScale = 1;
		    else EFScale = lscale;
		}
		break;

	    /* attr node xlo ylo xhi yhi type text */
	    case ATTR:
		r.r_xbot = atoi(argv[2]);
		r.r_ybot = atoi(argv[3]);
		r.r_xtop = atoi(argv[4]);
		r.r_ytop = atoi(argv[5]),
		efBuildAttr(def, argv[1], &r, argv[6], argv[7]);
		break;

	    /* cap node1 node2 capacitance */
	    case CAP:
		efBuildCap(def, argv[1], argv[2], cscale*atoi(argv[3]));
		break;

	    /* equiv node1 node2 */
	    case EQUIV:
		efBuildEquiv(def, argv[1], argv[2]);
		break;

	    /* fet type xlo ylo xhi yhi area perim substrate GATE T1 T2 ... */
	    case FET:
		if ( ((argc-9) % 3) != 0)
		{
		    efReadError("Incomplete terminal description for fet\n");
		    continue;
		}
		r.r_xbot = atoi(argv[2]);
		r.r_ybot = atoi(argv[3]);
		r.r_xtop = atoi(argv[4]);
		r.r_ytop = atoi(argv[5]),
		efBuildFet(def, argv[1], &r,
		    atoi(argv[6]), atoi(argv[7]), argv[8], argc-9, &argv[9]);
		break;

	    /* merge node1 node2 C a1 p1 a2 p2 ... */
	    case MERGE:
		if (argc > 3 && argc - 4 < 2 * efNumResistClasses)
		{
		    efReadError(
		    "Too few area/perim values: assuming remainder are zero\n");
		}
		cap = argc > 3 ? atoi(argv[3]) * cscale : 0;
		efBuildConnect(def, argv[1], argv[2], cap, &argv[4], argc - 4);
		break;

	    /* node name R C x y layer a1 p1 a2 p2 ... [ attrs ] */
	    case NODE:
		attrs = NULL;
		ac = argc - 7;
		if (ac & 01)
		    attrs = argv[argc-1], ac--;
		if (ac < 2*efNumResistClasses)
		{
		    efReadError(
		    "Too few area/perim values: assuming remainder are zero\n");
		}
		/* Note: resistance is ignored; we use perim/area instead */
		efBuildNode(def, argv[1], atoi(argv[3])*cscale,
			    atoi(argv[4]), atoi(argv[5]), argv[6],
			    &argv[7], ac, attrs);
		break;

	    /*
	     * rnode name C x y layer
	     * These are nodes resulting from resistance extraction and
	     * so have no "intrinsic" resistance per se.
	     */
	    case RNODE:
		efBuildNode(def, argv[1], atoi(argv[3])*cscale,
			    atoi(argv[4]), atoi(argv[5]), argv[6],
			    (char *) NULL, 0, (char *) NULL);
		break;

	    /* resist r1 r2 ... */
	    case RESISTCLASS:
		if (efNumResistClasses == 0)
		{
		    efNumResistClasses = argc-1;
		    for (n = 0; n < efNumResistClasses; n++)
			efResists[n] = atoi(argv[n + 1]);
		}
		else if (efNumResistClasses != argc-1)
		{
		    efReadError(
	    "Number of resistance classes doesn't match:\n");
resistChanged:
		    efReadError(
	    "Re-extract the entire tree with the same technology file\n");
		    efResistChanged = TRUE;
		    break;
		}
		for (n = 0; n < efNumResistClasses; n++)
		    if (efResists[n] != atoi(argv[n + 1]))
		    {
			efReadError("Resistance class values don't match:\n");
			goto resistChanged;
		    }
		break;

	    /* use def use-id T0 .. T5 */
	    case USE:
		efBuildUse(def, argv[1], argv[2],
			atoi(argv[3]), atoi(argv[4]), atoi(argv[5]),
			atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
		break;

	    /* tech techname */
	    case TECH:
		if (EFTech && EFTech[0])
		{
		    if (strcmp(EFTech, argv[1]) != 0)
		    {
			efReadError("Technology doesn't match: %s\n", argv[1]);
			break;
		    }
		}
		else EFTech = StrDup((char **) NULL, argv[1]);
		if (!EFLibPath[0])	/* Put in a path if there wasn't one */
		    (void) sprintf(EFLibPath, EXT_PATH, EFTech);
		break;

	    /* version version-number */
	    case VERSION:
		if (strcmp(argv[1], EFVersion) != 0)
		{
		    efReadError(
	"Cell was extracted using version %s of the extractor.\n", argv[1]);
		    efReadError("   It should be re-extracted.\n");
		}
		break;

	    /* distance driver receiver min max */
	    case DIST:
		efBuildDist(def, argv[1], argv[2],
			lscale*atoi(argv[3]), lscale*atoi(argv[4]));
		break;

	    /* killnode nodename */
	    case KILLNODE:
		efBuildKill(def, argv[1]);
		break;

	    /* resistor node1 node2 resistance */
	    case RESISTOR:
		efBuildResistor(def, argv[1], argv[2], rscale*atoi(argv[3]));
		break;

	    /* Ignore the rest for now */
	    case ADJUST:	/* Unused */
	    case TIMESTAMP:	/* Unused */
	    default:
		break;
	}
    }
    (void) fclose(inf);

    /* Read in each def that has not yet been read in */
    for (use = def->def_uses; use; use = use->use_next)
	if ((use->use_def->def_flags & DEF_AVAILABLE) == 0)
	    efReadDef(use->use_def);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efReadLine --
 *
 * Read a line from a .ext file and split it up into tokens.
 * Blank lines are ignored.  Lines ending in backslash are joined
 * to their successor lines.  Lines beginning with '#' are considered
 * to be comments and are ignored.
 *
 * Results:
 *	Returns the number of tokens into which the line was split, or
 *	-1 on end of file.  Never returns 0.
 *
 * Side effects:
 *	Copies the line just read into 'line'.  The trailing newline
 *	is turned into a '\0'.  The line is broken into tokens which
 *	are then placed into argv.  Updates *plinenum to point to the
 *	current line number in 'file'.
 *
 * ----------------------------------------------------------------------------
 */

efReadLine(line, size, file, argv)
    char *line;			/* Character array into which line is read */
    int size;			/* Size of character array */
    register FILE *file;	/* Open .ext file */
    char *argv[];		/* Vector of tokens built by efReadLine() */
{
    register char *get, *put;
    bool inquote;
    int argc = 0;

    /* Read one line into the buffer, joining lines when they end in '\' */
start:
     get = line;
     while (size > 0)
     {
	efReadLineNum += 1;
	if (fgets(get, size, file) == NULL) return (-1);
	for (put = get; *put != '\n'; put++) size -= 1;
	if ((put != get) && (*(put-1) == '\\'))
	{
	    get = put-1;
	    continue;
	}
	*put= '\0';
	break;
    }
    if (size == 0) efReadError("long line truncated\n");

    get = put = line;

    if (*line == '#') goto start;	/* Ignore comments */

    while (*get != '\0')
    {
	/* Skip leading blanks */
	while (isspace(*get)) get++;

	/* Beginning of the token is here */
	argv[argc] = put = get;
	inquote = FALSE;

	/*
	 * Grab up characters to the end of the token.  Any character
	 * preceded by a backslash is taken literally.
	 */
	while (*get != '\0')
	{
	    if (inquote)
	    {
		if (*get == '"')
		{
		    get++;
		    inquote = FALSE;
		    continue;
		}
	    }
	    else
	    {
		if (isspace(*get))
		    break;
		if (*get == '"')
		{
		    get++;
		    inquote = TRUE;
		    continue;
		}
	    }

	    if (*get == '\\')	/* Process quoted characters literally */
	    {
		get++;
		if (*get == '\0') break;
	    }

	    /* Copy into token receiving area */
	    *put++ = *get++;
	}

	/*
	 * If we got no characters in the token, we must have been at
	 * the end of the line.
	 */
	if (get == argv[argc])
	    break;
	
	/* Terminate the token and advance over the terminating character. */

	if (*get != '\0') get++;	/* Careful!  could be at end of line! */
	*put++ = '\0';
	argc++;
    }

    if (argc == 0)
	goto start;

    return (argc);
}

/*
 * ----------------------------------------------------------------------------
 *
 * efReadError --
 *
 * Complain about an error encountered while reading an .ext file.
 * Called with a variable number of arguments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message to stdout, complete with offending
 *	filename and line number.
 *
 * ----------------------------------------------------------------------------
 */

    /*VARARGS1*/
efReadError(fmt, va_alist)
    va_dcl
    char *fmt;
{
    va_list args;

    (void) printf("%s, line %d: ", efReadFileName, efReadLineNum);
    va_start(args);
    _doprnt(fmt, args, stdout);
    va_end(args);
    (void) fflush(stdout);
}
