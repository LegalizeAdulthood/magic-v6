/*
 * plotMain.c --
 *
 * This is the central file in the plot module.  It contains tables
 * that define the various styles of plotting that are available, and
 * also contains central technology-file reading routines.
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
static char rcsid[]="$Header: plotMain.c,v 6.0 90/08/28 18:51:52 mayo Exp $";
#endif  not lint

#include <stdio.h>
#ifndef SYSV
#include <vfont.h>
#endif
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "tech.h"
#include "malloc.h"
#include "plotInt.h"
#include "textio.h"
#include "utils.h"

/* Magic can generate plots in several different ways, e.g. as a
 * Gremlin file or as a direct raster plot to a Versatec printer.
 * For each style of plot, there is a subsection of the plot section
 * of technology files.  The tables below define the names of those
 * subsections, and the procedures to call to handle lines within
 * those subsections.  To add a new style of plot, extend the tables
 * below and then modify the procedure CmdPlot to actually invoke
 * the top-level plotting routine.
 */

static char *plotStyles[] =		/* Names of tech subsections. */
{
    "gremlin",
    "versatec",
    "colorversatec",
    "pixels",
    NULL
};


static Void (*plotInitProcs[])() =	/* Initialization procedures for
					 * each style.
					 */
{
    PlotGremlinTechInit,
    PlotVersTechInit,
    PlotColorVersTechInit,
    PlotPixTechInit,
    NULL
};

static bool (*plotLineProcs[])() =	/* Proc to call for each line in
					 * relevant subsection of tech file.
					 */
{
    PlotGremlinTechLine,
    PlotVersTechLine,
    PlotColorVersTechLine,
    PlotPixTechLine,
    NULL
};

static Void (*plotFinalProcs[])() =	/* Proc to call at end of reading
					 * tech files.
					 */
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static int plotCurStyle = -1;		/* Current style being processed in
					 * technology file.  -1 means no
					 * "style" line seen yet.  -2 means
					 * skipping to next "style" line.
					 */

bool PlotShowCellNames = TRUE;		/* TRUE if cell names and use-ids
					 * should be printed inside cell
					 * bounding boxes; if this is FALSE,
					 * then only the bounding box is
					 * drawn.
					 */

/*
 * ----------------------------------------------------------------------------
 *	PlotTechInit --
 *
 * 	Called once at beginning of technology file read-in to initialize
 *	data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the initialization procedures (if any) for each of the
 *	various styles of plotting.
 * ----------------------------------------------------------------------------
 */

Void
PlotTechInit()
{
    int i;

    PlotRastInit();

    plotCurStyle = -1;
    for (i = 0; plotStyles[i] != NULL; i++)
    {
	if (plotInitProcs[i] != NULL)
	    (*(plotInitProcs[i]))();
    }
}

/*
 * ----------------------------------------------------------------------------
 *	PlotTechLine --
 *
 * 	This procedure is invoked by the technology module once for
 *	each line in the "plot" section of the technology file.  It
 *	processes "style x" lines directly, to change the current style
 *	of plot information.  For other lines, it just passes the lines
 *	onto the procedure for the current style.
 *
 * Results:
 *	Returns whatever the handler for the current style returns when
 *	we call it.
 *
 * Side effects:
 *	Builds up plot technology information.
 * ----------------------------------------------------------------------------
 */

bool
PlotTechLine(sectionName, argc, argv)
    char *sectionName;		/* Name of this section. */
    int argc;			/* Number of arguments on line. */
    char *argv[];		/* Pointers to fields of line. */
{
    int i;

    if (strcmp(argv[0], "style") == 0)
    {
	if (argc != 2)
	{
	    TechError("\"style\" lines must have exactly two arguments\n");
	    return TRUE;
	}

	/* Change the style of plot for which information is being read. */

	plotCurStyle = -2;
	for (i = 0; plotStyles[i] != NULL; i++)
	{
	    if (strcmp(argv[1], plotStyles[i]) == 0)
	    {
		plotCurStyle = i;
		break;
	    }
	}

	if (plotCurStyle == -2)
	{
	    TechError("Plot style \"%s\" doesn't exist.  Ignoring.\n",
		    argv[1]);
	}
	return TRUE;
    }

    /* Not a new style.  Just farm out this line to the handler for the
     * current style.
     */
    
    if (plotCurStyle == -1)
    {
	TechError("Must declare a plot style before anything else.\n");
	plotCurStyle = -2;
	return TRUE;
    }
    else if (plotCurStyle == -2)
	return TRUE;
    
    if (plotLineProcs[plotCurStyle] == NULL)
	return TRUE;
    return (*(plotLineProcs[plotCurStyle]))(sectionName, argc, argv);
}

/*
 * ----------------------------------------------------------------------------
 *	PlotTechFinal --
 *
 * 	Called once at the end of technology file read-in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the finalization procedures (if any) for each of the
 *	various style of plotting.
 * ----------------------------------------------------------------------------
 */

Void
PlotTechFinal()
{
    int i;

    plotCurStyle = -1;
    for (i = 0; plotStyles[i] != NULL; i++)
    {
	if (plotFinalProcs[i] != NULL)
	    (*(plotFinalProcs[i]))();
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlotPrintParams --
 *
 * 	Print out a list of all the plotting parameters and their
 *	current values.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff gets printed.
 *
 * ----------------------------------------------------------------------------
 */

void
PlotPrintParams()
{
    TxPrintf("General plotting parameters are:\n");
    TxPrintf("    showCellNames: %s\n", PlotShowCellNames ? "true" : "false");
    TxPrintf("Versatec plotting parameters are:\n");
    TxPrintf("    cellIdFont:    \"%s\"\n", PlotVersIdFont);
    TxPrintf("    cellNameFont:  \"%s\"\n", PlotVersNameFont);
    TxPrintf("    directory:     \"%s\"\n", PlotTempDirectory);
    TxPrintf("    dotsPerInch:   %d\n", PlotVersDotsPerInch);
    TxPrintf("    labelFont:     \"%s\"\n", PlotVersLabelFont);
    TxPrintf("    printer:       \"%s\"\n", PlotVersPrinter);
    TxPrintf("    spoolCommand:  \"%s\"\n", PlotVersCommand);
    TxPrintf("    swathHeight:   %d\n", PlotVersSwathHeight);
    TxPrintf("    width:         %d\n", PlotVersWidth);
    TxPrintf("    color:         %s\n", PlotVersColor ? "true" : "false");
    TxPrintf("Pixel plotting parameters are:\n");
    TxPrintf("    pixheight:   %d\n", PlotPixHeight);
    TxPrintf("    pixwidth:         %d\n", PlotPixWidth);

}

/*
 * ----------------------------------------------------------------------------
 *
 * PlotSetParam --
 *
 * 	This procedure is called to change the value of one
 *	of the plotting parameters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whichever parameter is named by "name" is set to "value".
 *	The interpretation of "value" depends on the parameter.
 *
 * ----------------------------------------------------------------------------
 */

#define CELLIDFONT	0
#define CELLNAMEFONT	1
#define DIRECTORY	2
#define DOTSPERINCH	3
#define LABELFONT	4
#define PRINTER		5
#define	SHOWCELLNAMES	6
#define SPOOLCOMMAND	7
#define SWATHHEIGHT	8
#define WIDTH		9
#define COLOR          10
#define PIXWIDTH       11
#define PIXHEIGHT      12


void
PlotSetParam(name, value)
    char *name;			/* Name of a parameter. */
    char *value;		/* New value for the parameter. */
{
    int indx, i;
    static char *tfNames[] = { "false", "true", 0 };
    static char *paramNames[] =
    {
	"cellidfont",
	"cellnamefont",
	"directory",
	"dotsperinch",
	"labelfont",
	"printer",
	"showcellnames",
	"spoolcommand",
	"swathheight",
	"width",
	"color",
	"pixwidth",
	"pixheight",
	NULL
    };

    indx = Lookup(name, paramNames);
    if (indx < 0)
    {
	TxError("\"%s\" isn't a valid plot parameter.\n", name);
	PlotPrintParams();
	return;
    }

    i = atoi(value);
    switch (indx)
    {
	case CELLIDFONT:
	    StrDup(&PlotVersIdFont, value);
	    break;
	case CELLNAMEFONT:
	    StrDup(&PlotVersNameFont, value);
	    break;
	case DIRECTORY:
	    StrDup(&PlotTempDirectory, value);
	    break;
	case DOTSPERINCH:
	    if (!StrIsInt(value) || (i <= 0))
	    {
		TxError("DotsPerInch must be an integer greater than zero.]n");
		return;
	    }
	    else PlotVersDotsPerInch = i;
	    break;
	case LABELFONT:
	    StrDup(&PlotVersLabelFont, value);
	    break;
	case PRINTER:
	    StrDup(&PlotVersPrinter, value);
	    break;
	case SHOWCELLNAMES:
	    i = Lookup(value, tfNames);
	    if (i < 0)
	    {
		TxError("ShowCellNames can only be \"true\" or \"false\".\n");
		return;
	    }
	    PlotShowCellNames = i;
	    break;
	case COLOR:
	    i = Lookup(value, tfNames);
	    if (i < 0)
	    {
		TxError("Color can only be \"true\" or \"false\".\n");
		return;
	    }
	    PlotVersColor = i;
	    /* color plotting means our scanlines can only be 6848 (856 bytes) long */
	    PlotVersWidth = i?6848:7040;
	    break;
	case SPOOLCOMMAND:
	    StrDup(&PlotVersCommand, value);
	    break;
	case SWATHHEIGHT:
	    if (!StrIsInt(value) || (i <= 0))
	    {
		TxError("SwathHeight must be an integer greater than zero.\n");
		return;
	    }
	    else PlotVersSwathHeight= i;
	    break;
	case WIDTH:
	    if (!StrIsInt(value) || (i <= 0))
	    {
		TxError("Width must be an integer greater than zero.\n");
		return;
	    }
	    else PlotVersWidth = i;
	    break;
	case PIXWIDTH:
	    if (!StrIsInt(value) || (i <= 0))
	    {
		TxError("PixWidth must be an integer greater than zero.\n");
		return;
	    }
	    else PlotPixWidth = i;
	    break;
	case PIXHEIGHT:
	    if (!StrIsInt(value) || (i <= 0))
	    {
		TxError("PixHeight must be an integer greater than zero.\n");
		return;
	    }
	    else PlotPixHeight = i;
	    break;
    }
}
