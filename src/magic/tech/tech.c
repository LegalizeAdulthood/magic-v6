/*
 * tech.c --
 *
 * Read in a technology file.
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
static char rcsid[] = "$Header: tech.c,v 6.3 90/09/04 15:32:29 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "utils.h"
#include "tech.h"
#include "textio.h"
#include "graphics.h"
#include "malloc.h"

#ifndef NO_VARARGS
#include <varargs.h>
#endif  NO_VARARGS

global char *TechDefault = NULL;

int techLineNumber;
char *techFileName = NULL;

#define	iseol(c)	((c) == EOF || (c) == '\n')

/*
 * Each client of the technology module must make itself known by
 * a call to TechAddClient().  These calls provide both the names
 * of the sections of the technology file, as well as the procedures
 * to be invoked with lines in these sections.
 *
 * The following table is used to record clients of the technology
 * module.
 */

    typedef struct tC
    {
	bool		(*tc_proc)();	/* Procedure to be called for each
					 * line in section.
					 */
	Void		(*tc_init)();	/* Procedure to be called before any
					 * lines in a section are processed.
					 */
	Void		(*tc_final)();	/* Procedure to be called after all
					 * lines in section have been processed.
					 */
	struct tC	*tc_next;	/* Next client in section */
    } techClient;

    typedef struct
    {
	char		*ts_name;	/* Name of section */
	techClient	*ts_clients;	/* Pointer to list of clients */
	bool		 ts_read;	/* Flag: TRUE if section was read */
	SectionID	 ts_thisSect;	/* SectionID of this section */
	SectionID	 ts_prevSects;	/* Mask of sections that must be
					 * read in before this one.  The
					 * mask is constructed from the
					 * section identifiers set by
					 * TechAddClient().
					 */
    } techSection;

#define	MAXSECTIONS	(8 * sizeof (int))	/* Not easily changeable */
#define	MAXARGS		30
#define MAXLINESIZE	1024

#define	SectionToMaskBit(s)		(1 << (s))
#define SectionMaskHasSection(m, s)	(m & SectionToMaskBit(s))

int techSectionNum;			/* ID of next new section */
SectionID techSectionMask;		/* Mask of sections already read */

techSection techSectionTable[MAXSECTIONS];
techSection *techSectionFree;		/* Pointer to next free section */
techSection *techCurrentSection;	/* Pointer to current section */

techSection *techFindSection();

/*
 * ----------------------------------------------------------------------------
 *
 * TechInit --
 *
 * Initialize the technology module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the technology read-in module.
 *	This function must be called before any other functions in
 *	this module are called.  It is called exactly once at the start
 *	of a magic session.
 *
 * ----------------------------------------------------------------------------
 */

TechInit()
{
    techCurrentSection = (techSection *) NULL;
    techSectionFree = techSectionTable;
    techSectionNum = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * TechAddClient --
 *
 * Add a client to the technology module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Identifies "sectionName" as a valid name for a section of a .tech
 *	file, and specifies that init() is the procedure to be called when
 *	a new technology is loaded, proc() as the procedure to be called
 *	for each line in the given section, and final() as the procedure to
 *      be called after the last line in the given section.
 *
 *	The init() procedure takes no arguments.
 *	The proc() procedure should be of the following form:
 *		bool
 *		proc(sectionName, argc, argv)
 *			char *sectionName;
 *			int argc;
 *			char *argv[];
 *		{
 *		}
 *	The final() procedure takes no arguments.
 *
 *	The argument prevSections should be a mask of the SectionID's
 *	of all sections that must be read in before this one.
 *
 *	If the argument 'pSectionID' is non-NULL, it should point to
 *	an int that will be set to the sectionID of this section.
 *
 *	It is legal for several procedures to be associated with a given
 *	sectionName; this is accomplished through successive calls to
 *	TechAddClient with the same sectionName.  The procedures will
 *	be invoked in the order in which they were handed to TechAddClient().
 *
 *	If the procedure given is NULL for init(), proc(), or final(), no
 *	procedure is invoked.
 *
 * ----------------------------------------------------------------------------
 */

bool
TechAddClient(sectionName, init, proc, final, prevSections, pSectionID)
    char *sectionName;
    Void (*init)();
    bool (*proc)();
    Void (*final)();
    SectionID prevSections;
    SectionID *pSectionID;
{
    techSection *tsp;
    techClient *tcp, *tcl;

    tsp = techFindSection(sectionName);
    if (tsp == (techSection *) NULL)
    {
	tsp = techSectionFree++;
	ASSERT(tsp < &techSectionTable[MAXSECTIONS], "TechAddClient");
	tsp->ts_name = StrDup((char **) NULL, sectionName);
	tsp->ts_clients = (techClient *) NULL;
	tsp->ts_thisSect = SectionToMaskBit(techSectionNum);
	tsp->ts_prevSects = (SectionID) 0;
	techSectionNum++;
    }

    tsp->ts_prevSects |= prevSections;
    if (pSectionID)
	*pSectionID = tsp->ts_thisSect;

    tcp = (techClient *) mallocMagic(sizeof (techClient));
    ASSERT(tcp != (techClient *) NULL, "TechAddClient");
    tcp->tc_init = init;
    tcp->tc_proc = proc;
    tcp->tc_final = final;
    tcp->tc_next = (techClient *) NULL;

    if (tsp->ts_clients == (techClient *) NULL)
	tsp->ts_clients = tcp;
    else
    {
	for (tcl = tsp->ts_clients; tcl->tc_next; tcl = tcl->tc_next)
	    /* Nothing */;
	tcl->tc_next = tcp;
    }
}

/*
 * ----------------------------------------------------------------------------
 * TechLoad --
 *
 * Initialize technology description information from a file.
 *
 * Results:
 *	TRUE if technology is successfully initialized (all required
 *	sections present and error free); FALSE otherwise.  Unrecognized
 *	sections cause an error message to be printed, but do not otherwise
 *	affect the result returned by TechLoad().
 *
 * Side effects:
 *	Calls technology initialization routines of other modules
 *	to initialize technology-specific information.
 *
 * ----------------------------------------------------------------------------
 */

bool
TechLoad(filename)
    char *filename;
{
    register FILE *tf;
    register techSection *tsp;
    register techClient *tcp;
    char suffix[20], line[MAXLINESIZE], *realname;
    char *argv[MAXARGS];
    SectionID mask, badMask;
    int argc, s;
    bool retval, skip;

    techLineNumber = 0;
    badMask = (SectionID) 0;
    (void) sprintf(suffix, ".tech%d", VERSION);
    tf = PaOpen(filename, "r", suffix, ".", SysLibPath, &realname);
    if (tf == (FILE *) NULL) {
	TxError("Could not find file '%s%s' in any of these directories:\n         %s\n",
	    filename, suffix, SysLibPath);
	return (FALSE);
    }
    (void) StrDup(&techFileName, realname);

    /*
     * Mark all sections as being unread.
     */
    techSectionMask = 0;
    for (tsp = techSectionTable; tsp < techSectionFree; tsp++)
    {
	tsp->ts_read = FALSE;
    }

    /*
     * Sections in a technology file begin with a single line containing
     * the keyword identifying the section, and end with a single line
     * containing the keyword "end".
     */

    retval = TRUE;
    skip = FALSE;
    while ((argc = techGetTokens(line, sizeof line, tf, argv)) >= 0)
    {
	if (!skip && techCurrentSection == NULL)
	{
	    if (argc != 1)
	    {
		TechError("Bad section header line\n");
		goto skipsection;
	    }

	    tsp = techFindSection(argv[0]);
	    if (tsp == (techSection *) NULL)
	    {
		TechError("Unrecognized section name: %s\n", argv[0]);
		goto skipsection;
	    }
	    if (mask = (tsp->ts_prevSects & ~techSectionMask))
	    {
		register techSection *sp;

		TechError("Section %s appears too early.\n", argv[0]);
		TxError("\tMissing prerequisite sections:\n");
		for (sp = techSectionTable; sp < techSectionFree; sp++)
		    if (mask & sp->ts_thisSect)
			TxError("\t\t%s\n", sp->ts_name);
		goto skipsection;
	    }
	    techCurrentSection = tsp;

	    /* Invoke initialization routines for all clients that
	     * provided them.
	     */

	    for (tcp = techCurrentSection->ts_clients;
		    tcp != NULL;
		    tcp = tcp->tc_next)
	    {
		if (tcp->tc_init)
		    (void) (*tcp->tc_init)();
	    }
	    continue;
	}

	/* At the end of the section, invoke the finalization routine
	 * of the client's, if there is one.
	 */

	if (argc == 1 && strcmp(argv[0], "end") == 0)
	{
	    if (!skip)
	    {
		techSectionMask |= techCurrentSection->ts_thisSect;
		techCurrentSection->ts_read = TRUE;
		for (tcp = techCurrentSection->ts_clients;
			tcp != NULL;
			tcp = tcp->tc_next)
		{
		    if (tcp->tc_final)
			(*tcp->tc_final)();
		}
	    }
	    techCurrentSection = (techSection *) NULL;
	    skip = FALSE;
	    continue;
	}

	if (!skip)
	    for (tcp = techCurrentSection->ts_clients;
			tcp != NULL;
			tcp = tcp->tc_next)
		if (tcp->tc_proc)
		{
		    if (!(*tcp->tc_proc)(techCurrentSection->ts_name,argc,argv))
		    {
			retval = FALSE;
			badMask |= techCurrentSection->ts_thisSect;
		    }
		}
	continue;

skipsection:
	TxError("[Skipping to \"end\"]\n");
	skip = TRUE;
    }

    if (badMask)
    {
	TxError("The following sections of %s contained errors:\n", realname);
	for (s = 0; s < techSectionNum; s++)
	    if (SectionMaskHasSection(badMask, s))
		TxError("    %s\n", techSectionTable[s].ts_name);
    }

    for (tsp = techSectionTable; tsp < techSectionFree; tsp++)
	if (tsp->ts_read == FALSE)
	{
	    TxError("Section \"%s\" was missing from %s.\n",
		tsp->ts_name, realname);
	    retval = FALSE;
	}

    fclose(tf);
    return (retval);
}

/*
 * ----------------------------------------------------------------------------
 *
 * TechError --
 *
 * Print an error message referring to a given line number in the
 * technology module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 * ----------------------------------------------------------------------------
 */

void
TechPrintLine()
{
    char *section;

    if (techCurrentSection)
	section = techCurrentSection->ts_name;
    else
	section = "(none)";

    TxError("%s: line %d: section %s:\n\t",
		techFileName, techLineNumber, section);
}

#ifndef	NO_VARARGS

 /*VARARGS0*/

void
TechError(va_alist)
va_dcl
{
    va_list args;
    char *fmt;

    TechPrintLine();
    va_start(args);
    fmt = va_arg(args, char *);
    (void) GrVfprintf(stderr, fmt, args);
    va_end(args);
}
#else	NO_VARARGS

 /*VARARGS0*/

void
TechError(f, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
    char *format;
    FILE *f;
{
    TechPrintLine();
    (void) GrFprintf(stderr, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, 
		     a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
}
#endif	NO_VARARGS

/* ================== Functions local to this module ================== */

/*
 * ----------------------------------------------------------------------------
 *
 * techFindSection --
 *
 * Return a pointer to the entry in techSectionTable for the section
 * of the given name.
 *
 * Results:
 *	A pointer to the new entry, or NULL if none could be found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

techSection *
techFindSection(sectionName)
    char *sectionName;
{
    register techSection *tsp;

    for (tsp = techSectionTable; tsp < techSectionFree; tsp++)
	if (strcmp(tsp->ts_name, sectionName) == 0)
	    return (tsp);

    return ((techSection *) NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * techGetTokens --
 *
 * Read a line from the technology file and split it up into tokens.
 * Blank lines are ignored.  Lines ending in backslash are joined
 * to their successor lines.
 * We assume that all macro definition and comment elimination has
 * been done by the C preprocessor.
 *
 * Results:
 *	Returns the number of tokens into which the line was split, or
 *	-1 on end of file.  Never returns 0.
 *
 * Side effects:
 *	Copies the line just read into 'line'.  The trailing newline
 *	is turned into a '\0'.  The line is broken into tokens which
 *	are then placed into argv.
 *
 * ----------------------------------------------------------------------------
 */

techGetTokens(line, size, file, argv)
    char *line;			/* Character array into which line is read */
    int size;			/* Size of character array */
    register FILE *file;	/* Open technology file */
    char *argv[];		/* Vector of tokens built by techGetTokens() */
{
    register char *get, *put;
    bool inquote;
    int argc = 0;

    /* Read one line into the buffer, joining lines when they end
     * in backslashes.
     */

start:
     get = line;
     while (size > 0)
     {
	techLineNumber += 1;
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
    if (size == 0) TechError("long line truncated\n");

    get = put = line;

    if (*line == '#') goto start;	/* Ignore comments */

    while (*get != '\0')
    {
	/* Skip leading blanks */

	while (isspace(*get)) get++;

	/* Beginning of the token is here. */

	argv[argc] = put = get;
	if (*get == '"')
	{
	    get++;
	    inquote = TRUE;
	} else inquote = FALSE;

	/*
	 * Grab up characters to the end of the token.  Any character
	 * preceded by a backslash is taken literally.
	 */
	
	while (*get != '\0')
	{
	    if (inquote)
	    {
		if (*get == '"') break;
	    }
	    else if (isspace(*get)) break;

	    if (*get == '\\')	/* Process quoted characters literally */
	    {
		get += 1;
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
