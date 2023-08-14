/* grDStyle.c -
 *
 *	Parse and read in the display style file.
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
static char rcsid[]="$Header: grDStyle.c,v 6.0 90/08/28 18:40:44 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "utils.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "glyphs.h"
#include "windows.h"
#include "graphicsInt.h"


/* imports from other graphics files */
extern int (*grSetSPatternPtr)();
extern Void (*grDefineCursorPtr)();


/* data structures local to the graphics module */

global GR_STYLE_LINE grStyleTable[GR_NUM_STYLES];
global int grStippleTable[GR_NUM_STIPPLES][8];
global bool grIsStipple[GR_NUM_STIPPLES];
global int grNumStipples = 0;
global int grNumBitPlanes = 0;	/* Number of bit-planes we are using. */
global int grBitPlaneMask = 0;	/* Mask of the valid bit-plane bits. */
global GrGlyphs *grCursorGlyphs = NULL;

/* MUST be the same indices as the constants in graphics.h */
char *fillStyles[] = {
	"solid",
	"cross",
	"outline",
	"stipple",
	"grid",
	NULL };
	

/* internal constants for each section of the style file */
#define IGNORE		-1
#define	DISP_STYLES	0
#define	STIPPLES	1

#define	STRLEN	200

int GrStyleNames[128];	/* short names for styles */


/*
 * ----------------------------------------------------------------------------
 *
 * GrResetStyles --
 *
 * Re-send the styles information to the graphics dispaly.
 * This is intended to be called only when re-starting a frozen
 * version of Magic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Re-sends style information to the graphics display.
 *
 * ----------------------------------------------------------------------------
 */

GrResetStyles()
{
    int ord;

    if (grDefineCursorPtr && grCursorGlyphs)
	(*grDefineCursorPtr)(grCursorGlyphs);

    if (grSetSPatternPtr)
	for (ord = 0; ord < grNumStipples; ord++)
	    if (grIsStipple[ord])
		(*grSetSPatternPtr)(ord, grStippleTable[ord]);
}


/*
 * ----------------------------------------------------------------------------
 * styleBuildDisplayStyle:
 *
 *	Take one line of the display_styles section and process it.
 *
 * Results:
 *	True if things worked, false otherwise.
 *
 * Side effects:
 *	none
 * ----------------------------------------------------------------------------
 */

int
styleBuildDisplayStyle(line)
char *line;
{
    int res;
    int ord, mask, color, outline, nfill, stipple;
    char shortName;
    char fill[42];

    res = TRUE;

    if (sscanf(line, "%d %o %o %o %40s %d %c", 
	    &ord, &mask, &color, &outline, fill, &stipple, &shortName) != 7)
    {
	res = FALSE;
    }
    else
    {
	if ( (ord < 0) || (ord >= GR_NUM_STYLES) )
	    res = FALSE;
	else
	{
	    grStyleTable[ord].mask = (mask & grBitPlaneMask);
	    grStyleTable[ord].color = (color & grBitPlaneMask);
	    grStyleTable[ord].outline = outline;
	    nfill = LookupFull(fill, fillStyles);
	    if (nfill < 0)
		res = FALSE;
	    grStyleTable[ord].fill = nfill;
	    if (stipple >= grMaxStipples)
		res = FALSE;
	    grStyleTable[ord].stipple = stipple;
	    GrStyleNames[ shortName & 127 ] = ord;
	}
    }

    return(res);
}


/*
 * ----------------------------------------------------------------------------
 * styleBuildStippleStyle:
 *
 *	Take one line of the stipples section and process it.
 *
 * Results:
 *	True if things worked, false otherwise.
 *
 * Side effects:
 *	none
 * ----------------------------------------------------------------------------
 */

int
styleBuildStipplesStyle(line)
char *line;
{
    int res;
    int ord;
    int row[8];

    res = TRUE;

    if (sscanf(line, "%d %o %o %o %o %o %o %o %o",
	    &ord, &(row[0]), &(row[1]), &(row[2]), &(row[3]), 
	    &(row[4]), &(row[5]), &(row[6]), &(row[7]) ) != 9) 
    {
	res = FALSE;
    }
    else
    {
	if ( (ord < 0) || (ord >= grMaxStipples) )
	    res = FALSE;
	else
	{
	    int i;

	    grNumStipples = MAX(grNumStipples, ord);
	    grIsStipple[ord] = TRUE;
	    for (i = 0; i < 8; i++)
	    {
		grStippleTable[ord][i] = row[i];
	    }
	    if (grSetSPatternPtr)
		(*grSetSPatternPtr)(ord, grStippleTable[ord]);
	}
    }
    return(res);
}


/*
 * ----------------------------------------------------------------------------
 * GrLoadCursors --
 *
 *	Loads the graphics cursors from a given file.  There must not be
 *	any window locks set, as this routine may need to write to the
 *	display.
 *
 * Results:
 *	True if things worked, false otherwise.
 *
 * Side effects:
 *	Cursor patterns are loaded, which may involve writing to the
 *	display.  The file from which the cursors are read is determined
 *	by combining grCursorType (a driver-dependent string) with a
 *	".glyphs" extension.
 * ----------------------------------------------------------------------------
 */

bool
GrLoadCursors(path, libPath)
char *path;
char *libPath;
{
    if (grCursorGlyphs != (GrGlyphs *) NULL)
    {
	GrFreeGlyphs(grCursorGlyphs);
	grCursorGlyphs = (GrGlyphs *) NULL;
    }

    if (!GrReadGlyphs(grCursorType, path, libPath, &grCursorGlyphs))
    {
	return FALSE;
    }

    if (grDefineCursorPtr == NULL)
	TxError("Display does not have a programmable cursor.\n");
    else 
	(*grDefineCursorPtr)(grCursorGlyphs);

    return TRUE;  
}


/*
 * ----------------------------------------------------------------------------
 * GrLoadStyles:
 *
 *	Reads in a display style file.  This has the effect of setting the
 *	box styles and stipple patterns.
 *
 * Results:
 *	-1 if the file contained a format error.
 *	-2 if the file was not found.
 *	0 if everything went OK.
 *
 * Side effects:
 *	global variables are changed.
 * ----------------------------------------------------------------------------
 */

int
GrLoadStyles(techType, path, libPath)
char *techType;			/* Type of styles wanted by the technology
				 * file (usually "std").  We tack two things
				 * onto this name:  the type of styles
				 * wanted by the display, and a version
				 * suffix.  This three-part name is used
				 * to look up the actual display styles
				 * file.
				 */
char *path;
char *libPath;
{
    FILE *inp;
    int res = 0;
    int i;
    char fullName[256];

    for (i = 0; i < 128; i++) GrStyleNames[i] = 0;

    (void) sprintf(fullName, "%.100s.%.100s.dstyle5", techType, grDStyleType);
    inp = PaOpen(fullName, "r", (char *) NULL, path, libPath, (char **) NULL);
    if (inp == NULL)
    {
	TxError("Couldn't open display styles file \"%s\"\n", fullName);
        return(-2);
    }
    else
    {
	char line[STRLEN], sectionName[STRLEN];
	char *sres;
	bool newSection = FALSE;
        int section;

	while (TRUE)
	{
	    sres = fgets(line, STRLEN, inp);
	    if (sres == NULL) break;
	    if (StrIsWhite(line, FALSE)) 
		newSection = TRUE;
	    else if (line[0] == '#')
	    {
		/* comment line */
	    }
	    else if (newSection)
	    {
		if (sscanf(line, "%s", sectionName) != 1)
		{
		    TxError("File contained format error: %s\n", 
			    "unable to read section name.");
		    res = -1;
		}
		if (strcmp(sectionName, "display_styles") == 0)
		{
		    if (sscanf(line, "%s %d", sectionName, &grNumBitPlanes) 
		      != 2) 
		    {
			/* Use the default. */
			grNumBitPlanes = 8;
		    }
		    if (grNumBitPlanes < 1 || grNumBitPlanes > 8)
		    {
			TxError("Bad number of bit planes in .dstyle file: %d\n",
			    grNumBitPlanes);
			MainExit(1);
		    }
		    grBitPlaneMask = ((1 << grNumBitPlanes) - 1);
		    section = DISP_STYLES;
		}
		else if (strcmp(sectionName, "stipples") == 0)
		    section = STIPPLES;
		else
		{
		    TxError("Bad section name \"%s\" in .dstyle file.\n",
			sectionName);
		    section = IGNORE;
		}
		newSection = FALSE;
	    }
	    else
	    {
		int newres = TRUE;

		switch (section)
		{
		    case DISP_STYLES:
			newres = styleBuildDisplayStyle(line);
			break;
		    case STIPPLES:
			newres = styleBuildStipplesStyle(line);
			break;
		    case IGNORE:
			break;
		    default:
			TxError("Internal error in GrStyle\n");
			break;
		}
		if (!newres)
		{
		    TxError("Style line contained format error: %s", line);
		    res = -1;
		}
	    }
	}
    }
    if (fclose(inp) == EOF)
	TxError("Could not close styles file.\n");
    return(res);
}
