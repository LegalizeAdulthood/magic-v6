/*
 * CmdCD.c --
 *
 * Commands with names beginning with the letters C through D.
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
static char rcsid[] = "$Header: CmdCD.c,v 6.0 90/08/28 18:07:08 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "commands.h"
#include "utils.h"
#include "textio.h"
#include "drc.h"
#include "graphics.h"
#include "txcommands.h"
#include "cif.h"
#include "calma.h"
#include "styles.h"
#include "rtrDcmpose.h"
#include "select.h"
#include "malloc.h"
#ifdef SYSV
#include <string.h>
#endif

/* The following structure is used by CmdCorner to keep track of
 * areas to be filled.
 */

struct cmdCornerArea
{
    Rect cca_area;			/* Area to paint. */
    TileType cca_type;			/* Type of material. */
    struct cmdCornerArea *cca_next;	/* Next in list of areas to paint. */
};

/* Forward declarations */
int cmdDumpFunc();
#ifndef NO_CALMA

/*
 * ----------------------------------------------------------------------------
 *
 * CmdCalma --
 *
 * Implement the "calma" command.
 *
 * Usage:
 *	calma option args
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	There are no side effects on the circuit.  Currently, there
 *	is only a single option, "write", to write a CALMA stream
 *	file.
 *
 * ----------------------------------------------------------------------------
 */

#define CALMA_HELP	0
#define	CALMA_FLATTEN	1
#define	CALMA_LABELS	2
#define	CALMA_LOWER	3
#define	CALMA_NOFLATTEN	4
#define	CALMA_NOLABELS	5
#define	CALMA_NOLOWER	6
#define	CALMA_READ	7
#define	CALMA_WRITE	8

	/* ARGSUSED */
CmdCalma(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int option;
    char **msg, *namep;
    CellDef *rootDef;
    FILE *f;

    static char *cmdCalmaOption[] =
    {	
	"help		print this help information",
	"flatten		output arrays as individual subuses (like in CIF)",
	"labels		cause labels to be output when writing GDS-II",
	"lower		allow both upper and lower case in labels",
	"noflatten		output arrays intact; don't flatten",
	"nolabels		labels won't be output with GDS-II",
	"nolower	convert all labels to upper case",
	"read file		read Calma GDS-II format from \"file\"\n\
			into edit cell",
	"write file		output Calma GDS-II format to \"file\"\n\
			for the window's root cell",
	NULL
    };

    if (w == (Window *) NULL)
    {
	TxError("Point to a window first\n");
	return;
    }
    rootDef = ((CellUse *) w->w_surfaceID)->cu_def;

    if (cmd->tx_argc == 1)
    {
	namep = rindex(rootDef->cd_name, '/');
	if (namep == (char *) NULL)
	    namep = rootDef->cd_name;
	goto outputCalma;
    }

    option = Lookup(cmd->tx_argv[1], cmdCalmaOption);
    if (option < 0)
    {
	TxError("\"%s\" isn't a valid calma option.\n", cmd->tx_argv[1]);
	option = CALMA_HELP;
	cmd->tx_argc = 2;
    }

    switch (option)
    {
	case CALMA_HELP:
	    TxPrintf("Calma commands have the form \":calma option\",");
	    TxPrintf(" where option is one of:\n");
	    for (msg = &(cmdCalmaOption[0]); *msg != NULL; msg++)
	    {
		if (**msg == '*') continue;
		TxPrintf("    %s\n", *msg);
	    }
	    TxPrintf("If no option is given, a CALMA GDS-II stream file is\n");
	    TxPrintf("    produced for the root cell.\n");
	    TxPrintf("The current CIF output style (\"cif ostyle\") is used\n");
	    TxPrintf("    to select the mask layers output by :calma write.\n");
	    TxPrintf("The current CIF input style (\"cif istyle\") is used\n");
	    TxPrintf("    to select the mask layers read by :calma read.\n");
	    return;

	case CALMA_LABELS:
	    if (cmd->tx_argc != 2)
	    {
		wrongNumArgs:
		TxError("Wrong number of arguments in \"calma\" command.");
		TxError("  Try \":calma help\" for help.\n");
		return;
	    }
	    CalmaDoLabels = TRUE;
	    TxPrintf("Labels will be output in .strm files\n");
	    return;

	case CALMA_NOLABELS:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    CalmaDoLabels = FALSE;
	    TxPrintf("Labels won't be output in .strm files\n");
	    return;

	case CALMA_FLATTEN:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    CalmaFlattenArrays = TRUE;
	    TxPrintf("Each element of an array will be output individually\n");
	    TxPrintf("in .strm files\n");
	    return;

	case CALMA_NOFLATTEN:
	    if (cmd->tx_argc != 2) goto wrongNumArgs;
	    CalmaFlattenArrays = FALSE;
	    TxPrintf("Arrays will be output intact in .strm files\n");
	    return;

	case CALMA_LOWER:
	    TxPrintf("Labels in .strm file will be output as upper and lower case\n");
	    CalmaDoLower = TRUE;
	    return;

	case CALMA_NOLOWER:
	    TxPrintf("Labels in .strm file will be converted to upper case\n");
	    CalmaDoLower = FALSE;
	    return;

	case CALMA_WRITE:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    namep = cmd->tx_argv[2];
	    goto outputCalma;

	case CALMA_READ:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    f = PaOpen(cmd->tx_argv[2], "r", ".strm", Path,
		    (char *) NULL, (char **) NULL);
	    if (f == (FILE *) NULL)
	    {
		TxError("Cannot open %s.strm to read Calma GDS-II.\n",
			    cmd->tx_argv[2]);
		return;
	    }
	    CalmaReadFile(f);
	    (void) fclose(f);
	    return;
    }

    /*
     * If control gets here, we're going to output GDS-II (stream)
     * into the file given by namep.
     */

outputCalma:
    f = PaOpen(namep, "w", ".strm", ".", (char *) NULL, (char **) NULL);
    if (f == (FILE *) NULL)
    {
	TxError("Cannot open %s.strm to write Calma stream output\n", namep);
	return;
    }

    if (!CalmaWrite(rootDef, f))
    {
	TxError("I/O error in writing file %s.\n", namep);
	TxError("File may be incompletely written.\n");
    }
    (void) fclose(f);
}
#endif
#ifndef NO_ROUTE

/*
 * ----------------------------------------------------------------------------
 *
 * CmdChannel --
 *
 * Implement the "channel" command.  Generate a cell __CHANNEL__ showing
 * the channel structure for the edit cell, within the area of the box.
 *
 * Useage:
 *      :channel [netlist]
 *      :channel [-]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a __CHANNEL__ def if it does not already exist.  Paints
 *	on the error layer of that def.  Uses the feedback layer to display
 *	the channels that result.
 *
 * ----------------------------------------------------------------------------
 */
    /* ARGSUSED */

CmdChannel(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Rect newBox;
    CellDef *def, *RtrDecomposeName();
    char *name;
    extern int cmdChannelFunc();		/* Forward declaration. */

    if (cmd->tx_argc > 3)
    {
	TxError("Usage: %s [netlist | -]\n", cmd->tx_argv[0]);
	return;
    }

    if (!ToolGetEditBox(&newBox))
	return;

    name = (char *) NULL;
    if (cmd->tx_argc == 2)
	name = cmd->tx_argv[1];

    if (RtrDecomposeName(EditCellUse, &newBox, name) == (CellDef *) NULL)
    {
	TxError("\nRouting area (box) is too small to hold useful channels.\n");
	return;
    }
    TxPrintf("\n");

    /* Display the channels using feedback. */

    def = DBCellLookDef("__CHANNEL__");
    if (def == NULL) return;
    (void) DBSrPaintArea((Tile *) NULL, def->cd_planes[PL_DRC_ERROR],
	&newBox, &DBSpaceBits, cmdChannelFunc,
	(ClientData) NULL);
}

int
cmdChannelFunc(tile)
    Tile *tile;
{
    Rect area, rootArea;

    TiToRect(tile, &area);
    GeoTransRect(&EditToRootTransform, &area, &rootArea);
    DBWFeedbackAdd(&area, "Channel area", EditRootDef, 1,
	STYLE_OUTLINEHIGHLIGHTS);
    return 0;
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdCif --
 *
 * Implement the "cif" command.
 *
 * Usage:
 *	cif option args
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	There are no side effects on the circuit.  Various options
 *	may produce cif files, read cif, or display cif information
 *	on the screen.
 *
 * ----------------------------------------------------------------------------
 */
#ifndef NO_CIF
#define ARRAY		0
#define HIER		1
#define	AREALABELS	2
#define HELP		3
#define ISTYLE		4
#define OSTYLE		5
#define READ		6
#define SEE		7
#define STATS		8
#define CIF_WRITE	9
#define CIF_WRITE_FLAT 10
	/* ARGSUSED */
CmdCif(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern bool CIFDoAreaLabels;
    int option, yesno;
    char **msg, *namep;
    CellDef *rootDef;
    Rect box;
    FILE *f;
    bool wizardHelp;
    bool flatCif = FALSE;

    static char *cmdCifYesNo[] = { "no", "yes", 0 };
    static char *cmdCifOption[] =
    {	
	"*array layer		display CIF layer under box (array only)",
	"*hier layer		display CIF layer under box (hier only)",
	"arealabels yes|no	enable/disable us of area label extension",
	"help		print this help information",
	"istyle [style]	change style for reading CIF to style",
	"ostyle [style]	change style for writing CIF to style",
	"read file		read CIF from \"file\" into edit cell",
	"see layer		display CIF layer under box",
	"statistics		print out statistics for CIF generator",
	"write file		output CIF for the window's root cell to \"file\"",
	"flat file		output flattened CIF for the window's root cell to \"file\"",
	NULL
    };

    if (w == (Window *) NULL)
    {
	TxError("Point to a window first\n");
	return;
    }
    rootDef = ((CellUse *) w->w_surfaceID)->cu_def;

    if (cmd->tx_argc == 1)
    {
	namep = rindex(rootDef->cd_name, '/');
	if (namep == (char *) NULL)
	    namep = rootDef->cd_name;
	goto outputCIF;
    }

    option = Lookup(cmd->tx_argv[1], cmdCifOption);
    if (option < 0)
    {
	TxError("\"%s\" isn't a valid cif option.\n", cmd->tx_argv[1]);
	option = HELP;
	cmd->tx_argc = 2;
    }

    switch (option)
    {
	case ARRAY:
	    if (cmd->tx_argc != 3)
	    {
		wrongNumArgs:
		TxError("Wrong arguments in \"cif %s\" command:\n",
		    cmd->tx_argv[1]);
		TxError("    :cif %s\n", cmdCifOption[option]);
		TxError("Try \":cif help\" for more help.\n");
		return;
	    }
	    if (!ToolGetBox(&rootDef, &box))
	    {
		TxError("Use the box to select the area in");
		TxError(" which you want to see CIF.\n");
		return;
	    }
	    CIFSeeHierLayer(rootDef, &box, cmd->tx_argv[2], TRUE, FALSE);
	    return;
	
	case HIER:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    if (!ToolGetBox(&rootDef, &box))
	    {
		TxError("Use the box to select the area in");
		TxError(" which you want to see CIF.\n");
		return;
	    }
	    CIFSeeHierLayer(rootDef, &box, cmd->tx_argv[2], FALSE, TRUE);
	    return;

	case AREALABELS:
	    if (cmd->tx_argc > 3) goto wrongNumArgs;
	    if (cmd->tx_argc == 3)
	    {
		yesno = Lookup(cmd->tx_argv[2], cmdCifYesNo);
		if (yesno < 0)
		    goto wrongNumArgs;
		CIFDoAreaLabels = yesno;
	    }
	    TxPrintf("Output of CIF area labels is now %s\n",
		CIFDoAreaLabels ? "enabled" : "disabled");
	    return;

	case HELP:
	    if ((cmd->tx_argc == 3)
		    && (strcmp(cmd->tx_argv[2], "wizard") == 0))
		wizardHelp = TRUE;
	    else wizardHelp = FALSE;
	    TxPrintf("CIF commands have the form \":cif option\",");
	    TxPrintf(" where option is one of:\n");
	    for (msg = &(cmdCifOption[0]); *msg != NULL; msg++)
	    {
		if ((**msg == '*') && !wizardHelp) continue;
		TxPrintf("    %s\n", *msg);
	    }
	    TxPrintf("If no option is given, CIF is output for the");
	    TxPrintf(" root cell.\n");
	    return;
	
	case ISTYLE:
	    if (cmd->tx_argc == 3)
		CIFSetReadStyle(cmd->tx_argv[2]);
	    else if (cmd->tx_argc == 2)
		CIFSetReadStyle((char *) NULL);
	    else goto wrongNumArgs;
	    return;
	
	case OSTYLE:
	    if (cmd->tx_argc == 3)
		CIFSetStyle(cmd->tx_argv[2]);
	    else if (cmd->tx_argc == 2)
		CIFSetStyle((char *) NULL);
	    else goto wrongNumArgs;
	    return;

	case READ:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    f = PaOpen(cmd->tx_argv[2], "r", ".cif", Path,
		    (char *) NULL, (char **) NULL);
	    if (f == (FILE *) NULL)
	    {
		TxError("Cannot open %s.cif to read CIF.\n", cmd->tx_argv[2]);
		return;
	    }
	    CIFReadFile(f);
	    (void) fclose(f);
	    return;
	
	case SEE:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    if (!ToolGetBox(&rootDef, &box))
	    {
		TxError("Use the box to select the area in");
		TxError(" which you want to see CIF.\n");
		return;
	    }
	    CIFSeeLayer(rootDef, &box, cmd->tx_argv[2]);
	    return;
	
	case STATS:
	    CIFPrintStats();
	    return;

	case CIF_WRITE:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    namep = cmd->tx_argv[2];
	    goto outputCIF;

	case CIF_WRITE_FLAT:
	    if (cmd->tx_argc != 3) goto wrongNumArgs;
	    namep = cmd->tx_argv[2];
	    flatCif = TRUE;
	    goto outputCIF;
    }

    /* If control gets here, we're going to output CIF into the
     * file given by namep.
     */

    outputCIF:
    f = PaOpen(namep, "w", ".cif", ".", (char *) NULL, (char **) NULL);
    if (f == (FILE *) NULL)
    {
	TxError("Cannot open %s.cif to write CIF\n", namep);
	return;
    }
    if (flatCif == TRUE)
    {
	if (!CIFWriteFlat(rootDef, f))
	{
	    TxError("I/O error in writing file %s.\n", namep);
	    TxError("File may be incompletely written.\n");
    
	}
    }
    else
    {
	if (!CIFWrite(rootDef, f))
	{
	    TxError("I/O error in writing file %s.\n", namep);
	    TxError("File may be incompletely written.\n");
    
	}
    }
    (void) fclose(f);
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * CmdClockwise --
 *
 * Implement the "clockwise" command.  Rotate the selection and the
 * box clockwise around the point.
 *
 * Usage:
 *	clockwise [degrees]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the edit cell.
 *
 * ----------------------------------------------------------------------------
 */
    /* ARGSUSED */

CmdClockwise(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Transform trans, t2;
    int degrees;
    Rect rootBox,  bbox;
    CellDef *rootDef;

    if (cmd->tx_argc == 1)
	degrees = 90;
    else if (cmd->tx_argc == 2)
    {
	if (!StrIsInt(cmd->tx_argv[1])) goto badusage;
	degrees = atoi(cmd->tx_argv[1]);
    }
    else goto badusage;

    switch (degrees)
    {
	case 90:
	    t2 = Geo90Transform;
	    break;
	case 180:
	    t2 = Geo180Transform;
	    break;
	case 270:
	    t2 = Geo270Transform;
	    break;
	default:
	    TxError("Rotation angle must be 90, 180, or 270 degrees\n");
	    return;
    }


    /* To rotate the selection, first rotate it around the origin
     * then move it so its lower-left corner is at the same place
     * that it used to be.
     */
    
    GeoTransRect(&t2, &SelectDef->cd_bbox, &bbox);
    GeoTranslateTrans(&t2, SelectDef->cd_bbox.r_xbot - bbox.r_xbot,
	SelectDef->cd_bbox.r_ybot - bbox.r_ybot, &trans);

    SelectTransform(&trans);

    /* Rotate the box, if it exists and is in the same window as the
     * selection.
     */
    
    if (ToolGetBox(&rootDef, &rootBox) && (rootDef == SelectRootDef))
    {
	Rect newBox;

	GeoTransRect(&trans, &rootBox, &newBox);
	DBWSetBox(rootDef, &newBox);
    }

    return;

    badusage:
    TxError("Usage: %s [degrees]\n", cmd->tx_argv[0]);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdCopy --
 *
 * Implement the "copy" command.
 *
 * Usage:
 *	copy [direction [amount]]
 *	copy to x y
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is copied.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

CmdCopy(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Transform t;
    Rect rootBox, newBox;
    Point rootPoint, editPoint;
    CellDef *rootDef;

    if (cmd->tx_argc > 4)
    {
	badUsage:
	TxError("Usage: %s [direction [amount]]\n", cmd->tx_argv[0]);
	TxError("   or: %s to x y\n", cmd->tx_argv[0]);
	return;
    }

    if (cmd->tx_argc > 1)
    {
	int indx, amount;
	int xdelta, ydelta;

	if (strcmp(cmd->tx_argv[1], "to") == 0)
	{
	    if (cmd->tx_argc != 4)
		goto badUsage;
	    if (!StrIsInt(cmd->tx_argv[2]) || !StrIsInt(cmd->tx_argv[3]))
		goto badUsage;
	    editPoint.p_x = atoi(cmd->tx_argv[2]);
	    editPoint.p_y = atoi(cmd->tx_argv[3]);
	    GeoTransPoint(&EditToRootTransform, &editPoint, &rootPoint);
	    goto copyToPoint;
	}

	indx = GeoNameToPos(cmd->tx_argv[1], TRUE, TRUE);
	if (indx < 0)
	    return;
	if (cmd->tx_argc == 3)
	{
	    if (!StrIsInt(cmd->tx_argv[2])) goto badUsage;
	    amount = atoi(cmd->tx_argv[2]);
	}
	else amount = 1;

	switch (indx)
	{
	    case GEO_NORTH:
		xdelta = 0;
		ydelta = amount;
		break;
	    case GEO_SOUTH:
		xdelta = 0;
		ydelta = -amount;
		break;
	    case GEO_EAST:
		xdelta = amount;
		ydelta = 0;
		break;
	    case GEO_WEST:
		xdelta = -amount;
		ydelta = 0;
		break;
	    default:
		ASSERT(FALSE, "Bad direction in CmdCopy");
		return;
	}
	GeoTransTranslate(xdelta, ydelta, &GeoIdentityTransform, &t);

	/* Move the box by the same amount as the selection, if the
	 * box exists.
	 */

	if (ToolGetBox(&rootDef, &rootBox) && (rootDef == SelectRootDef))
	{
	    GeoTransRect(&t, &rootBox, &newBox);
	    DBWSetBox(rootDef, &newBox);
	}
    }
    else
    {
	/* Use the displacement between the box lower-left corner and
	 * the point as the transform.
	 */
	
	Window *window;

	window = ToolGetPoint(&rootPoint, (Rect *) NULL);
	if ((window == NULL) ||
	    (EditRootDef != ((CellUse *) window->w_surfaceID)->cu_def))
	{
	    TxError("\"Copy\" uses the point as the place to put down a\n");
	    TxError("    copy of the selection, but the point doesn't\n");
	    TxError("    point to the edit cell.\n");
	    return;
	}

copyToPoint:
	if (!ToolGetBox(&rootDef, &rootBox) || (rootDef != SelectRootDef))
	{
	    TxError("\"Copy\" uses the box lower-left corner as a place\n");
	    TxError("    to pick up the selection for copying, but the box\n");
	    TxError("    isn't in a window containing the selection.\n");
	    return;
	}
	GeoTransTranslate(rootPoint.p_x - rootBox.r_xbot,
	    rootPoint.p_y - rootBox.r_ybot, &GeoIdentityTransform, &t);
	GeoTransRect(&t, &rootBox, &newBox);
	DBWSetBox(rootDef, &newBox);
    }
    
    SelectCopy(&t);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdCorner --
 *
 * Implement the "corner" command.  Find all paint touching one side
 * of the box, and paint it around two edges of the box in an "L"
 * shape.
 *
 * Usage:
 *	corner firstDirection secondDirection [layers]
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The edit cell is modified.
 *
 * ----------------------------------------------------------------------------
 */

/* Data passed between CmdCorner and cmdCornerFunc: */

int cmdCornerDir1;			/* First direction each wire must
					 * be extended.
					 */
int cmdCornerDir2;			/* Second direction each wire must
					 * be extended.
					 */
Rect cmdCornerRootBox;			/* Root coords of box. */
struct cmdCornerArea *cmdCornerList;	/* List of areas to fill. */

	/*ARGSUSED*/
Void
CmdCorner(w, cmd)
    Window *w;		/* Window in which command was invoked. */
    TxCommand *cmd;	/* Describes the command that was invoked. */
{
    TileTypeBitMask maskBits;
    Rect editBox;
    SearchContext scx;
    extern int cmdCornerFunc();
    bool hasErr = FALSE;

    if (cmd->tx_argc < 3 || cmd->tx_argc > 4)
    {
	TxError("Usage: %s direction1 direction2 [layers]\n", cmd->tx_argv[0]);
	return;
    }

    if ( w == (Window *) NULL )
    {
	TxError("Point to a window\n");
	return;
    }

    /* Find and check validity of directions. */

    cmdCornerDir1 = GeoNameToPos(cmd->tx_argv[1], TRUE, TRUE);
    if (cmdCornerDir1 < 0)
	return;
    cmdCornerDir2 = GeoNameToPos(cmd->tx_argv[2], TRUE, TRUE);
    if (cmdCornerDir2 < 0)
	return;
    if ((cmdCornerDir1 == GEO_NORTH) || (cmdCornerDir1 == GEO_SOUTH))
    {
	if ((cmdCornerDir2 == GEO_NORTH) || (cmdCornerDir2 == GEO_SOUTH))
	{
	    TxPrintf("Can't corner-fill %s and then %s.\n",
		    cmd->tx_argv[1], cmd->tx_argv[2]);
	    return;
	}
    }
    else
    {
	if ((cmdCornerDir2 == GEO_EAST) || (cmdCornerDir2 == GEO_WEST))
	{
	    TxPrintf("Can't corner-fill %s and then %s.\n",
		    cmd->tx_argv[1], cmd->tx_argv[2]);
	    return;
	}
    }

    /* Figure out which layers to fill. */

    if (cmd->tx_argc < 4)
	maskBits = DBAllButSpaceAndDRCBits;
    else
    {
	if (!CmdParseLayers(cmd->tx_argv[3], &maskBits))
	    return;
    }

    /* Figure out which material to search for and invoke a search
     * procedure to find it.
     */

    if (!ToolGetEditBox(&editBox)) return;
    GeoTransRect(&EditToRootTransform, &editBox, &cmdCornerRootBox);
    scx.scx_area = cmdCornerRootBox;
    switch (cmdCornerDir1)
    {
	case GEO_NORTH:
	    scx.scx_area.r_ytop = scx.scx_area.r_ybot + 1;
	    scx.scx_area.r_ybot -= 1;
	    break;
	case GEO_SOUTH:
	    scx.scx_area.r_ybot = scx.scx_area.r_ytop - 1;
	    scx.scx_area.r_ytop += 1;
	    break;
	case GEO_EAST:
	    scx.scx_area.r_xtop = scx.scx_area.r_xbot + 1;
	    scx.scx_area.r_xbot -= 1;
	    break;
	case GEO_WEST:
	    scx.scx_area.r_xbot = scx.scx_area.r_xtop - 1;
	    scx.scx_area.r_xtop += 1;
	    break;
    }
    scx.scx_use = (CellUse *) w->w_surfaceID;
    scx.scx_trans = GeoIdentityTransform;
    cmdCornerList = (struct cmdCornerArea *) NULL;

    (void) DBTreeSrTiles(&scx, &maskBits,
	    ((DBWclientRec *) w->w_clientData)->dbw_bitmask,
	    cmdCornerFunc, (ClientData) &hasErr);
    if (hasErr)
    {
	TxError("There's not enough room in the box for all the wires.\n");
    }

    /* Now that we've got all the material, scan over the list
     * painting the material and freeing up the entries on the list.
     */
    while (cmdCornerList != NULL)
    {
	DBPaint(EditCellUse->cu_def, &cmdCornerList->cca_area,
		cmdCornerList->cca_type);
	freeMagic((char *) cmdCornerList);
	cmdCornerList = cmdCornerList->cca_next;
    }

    SelectClear();
    DBAdjustLabels(EditCellUse->cu_def, &editBox);
    DRCCheckThis(EditCellUse->cu_def, TT_CHECKPAINT, &editBox);
    DBWAreaChanged(EditCellUse->cu_def, &editBox, DBW_ALLWINDOWS, &maskBits);
    DBReComputeBbox(EditCellUse->cu_def);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cmdCornerFunc --
 *
 * 	Search procedure called by DBTreeSrTiles from CmdCorner.  Called once
 *	for each tile that crosses the appropriate boundary of the box.
 *	Makes an L-shaped 90 degree turn to extend a wire out of an
 *	adjacent side.
 *
 * Results:
 *	Returns 0 to keep the search alive.
 *
 * Side effects:
 *	Adds paint tiles to the display list.  If there are tiles found
 *	that can't be cornered correctly, the clientData value is set
 *	to TRUE.
 *
 * ----------------------------------------------------------------------------
 */
int
cmdCornerFunc(tile, cxp)
    Tile *tile;			/* Tile to fill with. */
    TreeContext *cxp;		/* Describes state of search. */
{
    Rect r1, r2, r3;
    struct cmdCornerArea *cca;
    bool *errPtr = (bool *) cxp->tc_filter->tf_arg;

    /* Get the tile dimensions in root coordinates.  Clip to the box.
     */
    TiToRect(tile, &r1);
    GeoTransRect(&cxp->tc_scx->scx_trans, &r1, &r2);
    GeoClip(&r2, &cmdCornerRootBox);

    /* Generate r2 and r3, the first and second legs of the L-shaped
     * geometry to be painted for this tile.
     */

    r3 = r2;
    switch (cmdCornerDir1)
    {
	case GEO_NORTH:
	    if (cmdCornerDir2 == GEO_EAST)
	    {
		r2.r_ytop = r3.r_ytop = cmdCornerRootBox.r_ytop
			- (r2.r_xbot - cmdCornerRootBox.r_xbot);
		r3.r_xtop = cmdCornerRootBox.r_xtop;
	    }
	    else
	    {
		r2.r_ytop = r3.r_ytop = cmdCornerRootBox.r_ytop
			- (cmdCornerRootBox.r_xtop - r2.r_xtop);
		r3.r_xbot = cmdCornerRootBox.r_xbot;
	    }
	    r3.r_ybot = r3.r_ytop - (r2.r_xtop - r2.r_xbot);
	    if (r3.r_ybot < cmdCornerRootBox.r_ybot)
		*errPtr = TRUE;
	    break;

	case GEO_SOUTH:
	    if (cmdCornerDir2 == GEO_EAST)
	    {
		r2.r_ybot = r3.r_ybot = cmdCornerRootBox.r_ybot
			+ (r2.r_xbot - cmdCornerRootBox.r_xbot);
		r3.r_xtop = cmdCornerRootBox.r_xtop;
	    }
	    else
	    {
		r2.r_ybot = r3.r_ybot = cmdCornerRootBox.r_ybot
			+ (cmdCornerRootBox.r_xtop - r2.r_xtop);
		r3.r_xbot = cmdCornerRootBox.r_xbot;
	    }
	    r3.r_ytop = r3.r_ybot + (r2.r_xtop - r2.r_xbot);
	    if (r3.r_ytop > cmdCornerRootBox.r_ytop)
		*errPtr = TRUE;
	    break;

	case GEO_EAST:
	    if (cmdCornerDir2 == GEO_NORTH)
	    {
		r2.r_xtop = r3.r_xtop = cmdCornerRootBox.r_xtop
			- (r2.r_ybot - cmdCornerRootBox.r_ybot);
		r3.r_ytop = cmdCornerRootBox.r_ytop;
	    }
	    else
	    {
		r2.r_xtop = r3.r_xtop = cmdCornerRootBox.r_xtop
			- (cmdCornerRootBox.r_ytop - r2.r_ytop);
		r3.r_ybot = cmdCornerRootBox.r_ybot;
	    }
	    r3.r_xbot = r3.r_xtop - (r2.r_ytop - r2.r_ybot);
	    if (r3.r_xbot < cmdCornerRootBox.r_xbot)
		*errPtr = TRUE;
	    break;

	case GEO_WEST:
	    if (cmdCornerDir2 == GEO_NORTH)
	    {
		r2.r_xbot = r3.r_xbot = cmdCornerRootBox.r_xbot
			+ (r2.r_ybot - cmdCornerRootBox.r_ybot);
		r3.r_ytop = cmdCornerRootBox.r_ytop;
	    }
	    else
	    {
		r2.r_xbot = r3.r_xbot = cmdCornerRootBox.r_xbot
			+ (cmdCornerRootBox.r_ytop - r2.r_ytop);
		r3.r_ybot = cmdCornerRootBox.r_ybot;
	    }
	    r3.r_xtop = r2.r_xbot + (r2.r_ytop - r2.r_ybot);
	    if (r3.r_xtop > cmdCornerRootBox.r_xtop)
		*errPtr = TRUE;
	    break;
    }

    /* Clip the resulting geometry to the box, translate to edit cell
     * coords, and add to the paint list if non-NULL.
     */

    GeoClip(&r2, &cmdCornerRootBox);
    GeoTransRect(&RootToEditTransform, &r2, &r1);
    if (!GEO_RECTNULL(&r1))
    {
	/* Add this rectangle to the list. */

	cca = (struct cmdCornerArea *)
		mallocMagic(sizeof(struct cmdCornerArea));
	cca->cca_area = r1;
	cca->cca_type = TiGetType(tile);
	cca->cca_next = cmdCornerList;
	cmdCornerList = cca;
    }

    GeoClip(&r3, &cmdCornerRootBox);
    GeoTransRect(&RootToEditTransform, &r3, &r1);
    if (!GEO_RECTNULL(&r1))
    {
	cca = (struct cmdCornerArea *)
		mallocMagic(sizeof(struct cmdCornerArea));
	cca->cca_area = r1;
	cca->cca_type = TiGetType(tile);
	cca->cca_next = cmdCornerList;
	cmdCornerList = cca;
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdDelete --
 *
 * Implement the "delete" command.
 *
 * Usage:
 *	delete
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection is deleted.
 *
 * ----------------------------------------------------------------------------
 */

    /* ARGSUSED */

CmdDelete(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1) goto badusage;
    SelectDelete("deleted");
    return;

    badusage:
    TxError("Usage: %s\n", cmd->tx_argv[0]);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdDrc --
 *
 * Implement the "drc" command.
 *
 * Usage:
 *	drc option
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Most options have no side effects.  The only major side
 *	effects are to turn continuous DRC on or off, or recheck an
 *	area of a cell.
 *
 * ----------------------------------------------------------------------------
 */

#define FLATCHECK	0
#define SHOWINT		1
#define CATCHUP		2
#define CHECK		3
#define COUNT		4
#define FIND		5
#undef HELP
#define HELP		6
#define OFF		7
#define ON		8
#define PRINTRULES	9
#define RULESTATS	10
#define STATISTICS	11
#define WHY		12

	/* ARGSUSED */
CmdDrc(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    FILE        * fp;
    int		  option, nth, result, radius;
    Rect	  rootArea, area;
    CellUse	* rootUse, *use;
    CellDef	* rootDef;
    Transform	  trans;
    Window	* window;
    char 	**msg;
    bool	  wizardHelp;

    static char *cmdDrcOption[] =
    {	
	"*flatcheck             check box area by flattening",
	"*showint radius        show interaction area under box",
	"catchup                run checker and wait for it to complete",
	"check                  recheck area under box in all cells",
	"count                  count error tiles in each cell under box",
	"find [nth]             locate next (or nth) error in current cell",
	"help                   print this help information",
	"off                    turn off background checker",
	"on                     reenable background checker",
	"printrules [file]      print out design rules in file or on tty",
	"rulestats              print out stats about design rule database",
	"statistics             print out statistics gathered by checker",
	"why                    print out reasons for errors under box",
	NULL
    };

    if (cmd->tx_argc < 2)
    {
	TxError("No option given in \":drc\" command.\n");
	option = HELP;
    }
    else
    {
	option = Lookup(cmd->tx_argv[1], cmdDrcOption);
	if (option < 0)
	{
	    TxError("%s isn't a valid drc option.\n", cmd->tx_argv[1]);
	    option = HELP;
	    cmd->tx_argc = 2;
	}
	if ((cmd->tx_argc > 2) && (option != PRINTRULES) && (option != FIND)
	    && (option != SHOWINT) && (option != HELP))
	{
	    badusage:
	    TxError("Wrong arguments in \"drc %s\" command:\n",
		cmd->tx_argv[1]);
	    TxError("    :drc %s\n", cmdDrcOption[option]);
	    TxError("Try \":drc help\" for more help.\n");
	    return;
	}
    }
    switch (option)
    {
	case FLATCHECK:
	    window = ToolGetBoxWindow(&rootArea, (int *) NULL);
	    if (window == NULL) return;
	    rootUse = (CellUse *) window->w_surfaceID;
	    DRCFlatCheck(rootUse, &rootArea);
	    break;
	
	case SHOWINT:
	    if (cmd->tx_argc != 3) goto badusage;
	    radius = atoi(cmd->tx_argv[2]);
	    if (radius < 0)
	    {
		TxPrintf("Radius must not be negative\n");
		return;
	    }
	    window = ToolGetBoxWindow(&rootArea, (int *) NULL);
	    if (window == NULL) return;
	    rootUse = (CellUse *) window->w_surfaceID;
	    if (!DRCFindInteractions(rootUse->cu_def, &rootArea,
		radius, &area))
	    {
		TxPrintf("No interactions in this area for that radius.\n");
		return;
	    }
	    ToolMoveBox(TOOL_BL, &area.r_ll, FALSE, rootUse->cu_def);
	    ToolMoveCorner(TOOL_TR, &area.r_ur, FALSE, rootUse->cu_def);
	    break;
	
	case CATCHUP:
	    DRCCatchUp();
	    break;

	case CHECK:
	    window = ToolGetBoxWindow(&rootArea, (int *) NULL);
	    if (window == NULL) return;
	    rootUse = (CellUse *) window->w_surfaceID;
	    DRCCheck(rootUse, &rootArea);
	    break;
	
	case COUNT:
	    window = ToolGetBoxWindow(&rootArea, (int *) NULL);
	    if (window == NULL) return;
	    rootUse = (CellUse *) window->w_surfaceID;
	    DRCCount(rootUse, &rootArea);
	    break;
	
	case FIND:
	    if (cmd->tx_argc > 2)
	    {
		if (cmd->tx_argc > 3) goto badusage;
		nth = atoi(cmd->tx_argv[2]);
	    }
	    else nth = 0;
	    use = CmdGetSelectedCell(&trans);
	    rootDef = SelectRootDef;
	    if (use == NULL)
	    {
		use = EditCellUse;
		rootDef = EditRootDef;
		trans = EditToRootTransform;
	    }
	    result = DRCFind(use->cu_def, &area, nth);
	    if (result != 0)
	    {
		GeoTransRect(&trans, &area, &rootArea);
		ToolMoveBox(TOOL_BL, &rootArea.r_ll, FALSE, rootDef);
		ToolMoveCorner(TOOL_TR, &rootArea.r_ur, FALSE, rootDef);
		TxPrintf("Error area #%d:\n", result);
		DRCWhy(use, &area);
	    }
	    else
	    {
		if (nth > 1) TxPrintf("There aren't that many errors");
		else TxPrintf("There are no errors");
		TxPrintf(" in %s.\n", use->cu_def->cd_name);
	    }
	    break;
	
	case HELP:
	    if ((cmd->tx_argc == 3)
		    && (strcmp(cmd->tx_argv[2], "wizard") == 0))
		wizardHelp = TRUE;
	    else wizardHelp = FALSE;
	    TxPrintf("DRC commands have the form \":drc option\",");
	    TxPrintf(" where option is one of:\n");
	    for (msg = &(cmdDrcOption[0]); *msg != NULL; msg++)
	    {
		if ((**msg == '*') && !wizardHelp) continue;
		TxPrintf("    %s\n", *msg);
	    }
	    break;
	
	case OFF:
	    DRCBackGround = FALSE;
	    break;
	
	case ON:
	    DRCBackGround = TRUE;
	    break;
	
	case PRINTRULES:
	    if (cmd->tx_argc > 3) goto badusage;
	    if (cmd->tx_argc < 3)
		fp = stdout;
	    else if ((fp = fopen (cmd->tx_argv[2],"w")) == (FILE *) NULL)
	    {
		TxError("Cannot write file %s\n", cmd->tx_argv[2]);
		return;
	    }
	    DRCPrintRulesTable (fp);
	    if (fp != stdout)
		(void) fclose(fp);
	    break;
	
	case RULESTATS:
	    DRCTechRuleStats();
	    break;
	
	case STATISTICS:
	    DRCPrintStats();
	    break;

	case WHY:
	    window = ToolGetBoxWindow(&rootArea, (int *) NULL);
	    if (window == NULL) return;
	    rootUse = (CellUse *) window->w_surfaceID;
	    DRCWhy(rootUse, &rootArea);
	    break;
    }
    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CmdDump --
 *
 *	Implement the ":dump" command.
 *
 * Usage:
 *	dump cellName [child refPointChild] [parent refPointParent]
 *
 * where the refPoints are either a label name, e.g., SOCKET_A, or an x-y
 * pair of integers, e.g., 100 200.  The words "child" and "parent" are
 * keywords, and may be abbreviated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Copies the contents of a given cell into the edit cell,
 *	so that refPointChild in the child cell (or the lower-left
 *	corner of its bounding box) ends up at location refPointParent
 *	in the edit cell (or the location of the box tool's lower-left).
 *
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
CmdDump(w, cmd)
    Window *w;			/* Window in which command was invoked. */
    TxCommand *cmd;		/* Describes command arguments. */
{
    SearchContext scx;
    CellUse dummy;

    if (cmdDumpParseArgs("dump", w, cmd, &dummy, &scx))
	SelectDump(&scx);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cmdDumpParseArgs --
 *
 * Do the real work of the "dump" and "getcell" commands, by reading
 * in the child cell to be used and figuring out the transform implied
 * by the reference point arguments to the command.
 *
 * Results:
 *      TRUE on success; FALSE if arguments were missing or
 *	incorrect, or if the cell couldn't be found.
 *
 * Side effects:
 *	Fills in *dummy so that dummy->cu_def points to the cell
 *	specified by name in cmd->tx_argv[] (see CmdDump() for a
 *	description of the syntax of these args).  Also fills in
 *	*scx so scx_use is dummy, scx_trans is the desired transform
 *	from dummy->cu_def back to root coordinates, and scx_area
 *	is the bounding box of dummy->cu_def.  (Scx is set up
 *	directly for a call to SelectDump()).
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
bool
cmdDumpParseArgs(cmdName, w, cmd, dummy, scx)
    char *cmdName;	/* Either "dump" or "getcell" */
    Window *w;		/* Window in which command was invoked (UNUSED) */
    TxCommand *cmd;	/* Arguments to command */
    CellUse *dummy;	/* Filled in to point to cell mentioned in command */
    SearchContext *scx;	/* Filled in with the transform from the child cell's
			 * def to ROOT coordinates, the bounding box of the
			 * child cell in child cell coordinates, and with
			 * scx_use = dummy, where dummy->cu_def is the child
			 * cell itself.
			 */
{
    Point childPoint, editPoint, rootPoint;
    CellDef *def, *rootDef, *editDef;
    bool hasChild, hasRoot;
    Rect rootBox;
    char **av;
    int ac;

    if (cmd->tx_argc < 2)
    {
	TxError("Missing cell name in \"%s\" command.\n", cmdName);
	goto usage;
    }

    /* Locate the cell specified by the command */
    if (CmdIllegalChars(cmd->tx_argv[1], "", "Cell name"))
	return (FALSE);
    def = DBCellLookDef(cmd->tx_argv[1]);
    if (def == (CellDef *) NULL)
	def = DBCellNewDef(cmd->tx_argv[1], (char *) NULL);
    editDef = EditCellUse->cu_def;

    /*
     * The following line of code is a bit of a hack.  It's needed to
     * force DBCellRead to print an error message if it can't find the
     * cell.  Otherwise, if the cell wasn't found the last time it was
     * looked for then no new error message will be printed.
     */
    def->cd_flags &= ~CDNOTFOUND;
    if (!DBCellRead(def, (char *) NULL, TRUE))
	return (FALSE);
    DBReComputeBbox(def);
    dummy->cu_def = def;
    dummy->cu_transform = GeoIdentityTransform;
    dummy->cu_expandMask = -1;
    if (DBIsAncestor(def, EditCellUse->cu_def))
    {
	TxError("The edit cell is already a desecendant of \"%s\",\n",
	    cmd->tx_argv[1]);
	TxError("    which means that you're trying to create a circular\n");
	TxError("    structure.  This isn't legal.\n");
	return FALSE;
    }

    /*
     * Parse the remainder of the arguments to find out the reference
     * points in the child cell and the edit cell.  Use the defaults
     * of the lower-left corner of the child cell's bounding box, and
     * the lower-left corner of the box tool, if the respective reference
     * points weren't provided.  (Lower-left of the box tool is interpreted
     * in root coordinates).
     */
    av = &cmd->tx_argv[2];
    ac = cmd->tx_argc - 2;
    hasChild = hasRoot = FALSE;
    while (ac > 0)
    {
	static char *kwdNames[] = { "child", "parent", 0 };
	Label *lab;
	int n;

	n = Lookup(av[0], kwdNames);
	if (n < 0)
	{
	    TxError("Unrecognized parent/child keyword: \"%s\"\n", av[0]);
	    goto usage;
	}
	if (ac < 2)
	{
	    TxError("Keyword must be followed by a reference point\n");
	    goto usage;
	}
	switch (n)
	{
	    case  0:	/* Child */
		if (StrIsInt(av[1]))
		{
		    childPoint.p_x = atoi(av[1]);
		    if (ac < 3 || !StrIsInt(av[2]))
		    {
			TxError("Must provide two coordinates\n");
			goto usage;
		    }
		    childPoint.p_y = atoi(av[2]);
		    av += 3;
		    ac -= 3;
		}
		else
		{
		    childPoint = TiPlaneRect.r_ur;
		    (void) DBSrLabelLoc(dummy, av[1], cmdDumpFunc, &childPoint);
		    if (childPoint.p_x == TiPlaneRect.r_xtop &&
			    childPoint.p_y == TiPlaneRect.r_ytop)
		    {
			TxError("Couldn't find label \"%s\" in cell \"%s\".\n",
				av[1], cmd->tx_argv[1]);
			return FALSE;
		    }
		    av += 2;
		    ac -= 2;
		}
		hasChild = TRUE;
		break;
	    case  1:	/* Parent */
		if (StrIsInt(av[1]))
		{
		    editPoint.p_x = atoi(av[1]);
		    if (ac < 3 || !StrIsInt(av[2]))
		    {
			TxError("Must provide two coordinates\n");
			goto usage;
		    }
		    editPoint.p_y = atoi(av[2]);
		    av += 3;
		    ac -= 3;
		}
		else
		{
		    for (lab = editDef->cd_labels; lab; lab = lab->lab_next)
			if (strcmp(lab->lab_text, av[1]) == 0)
			    break;

		    if (lab == NULL)
		    {
			TxError("Couldn't find label \"%s\" in edit cell.\n",
				av[1]);
			return FALSE;
		    }
		    editPoint = lab->lab_rect.r_ll;
		    av += 2;
		    ac -= 2;
		}
		GeoTransPoint(&EditToRootTransform, &editPoint, &rootPoint);
		hasRoot = TRUE;
		break;
	}
    }

    /*
     * Use the default values if explicit reference points weren't
     * provided.
     */
    if (!hasChild)
	childPoint = def->cd_bbox.r_ll;
    if (!hasRoot)
    {
	if (!ToolGetBox(&rootDef, &rootBox) || (rootDef != EditRootDef))
	{
	    TxError("The box's lower-left corner must point to the place\n");
	    TxError("    in the edit cell where you'd like to put \"%s\".\n",
		cmd->tx_argv[1]);
	    return FALSE;
	}
	rootPoint = rootBox.r_ll;
    }

    scx->scx_use = dummy;
    GeoTransTranslate(rootPoint.p_x - childPoint.p_x,
	    rootPoint.p_y - childPoint.p_y,
	    &GeoIdentityTransform, &scx->scx_trans);
    scx->scx_area = def->cd_bbox;
    return TRUE;

usage:
    TxError(
	"Usage: %s cellName [child refPointChild] [parent refPointParent]\n",
	cmdName);
    TxError("       where the refPoints are either a single label name\n");
    TxError("       or a pair of integer coordinates\n");
    return FALSE;
}

/*
 * cmdDumpFunc --
 *
 * Search function used to locate positioning label.  It just computes
 * the lower-left corner of the label and aborts the search.
 *
 * Results:
 *	Always returns 1.
 *
 * Side effects:
 *	Sets *point to the lower-left corner of the label.
 */

    /* ARGSUSED */
int
cmdDumpFunc(rect, name, label, point)
    Rect *rect;			/* Root coordinates of the label. */
    char *name;			/* Label name (not used). */
    Label *label;		/* Pointer to label (not used). */
    Point *point;		/* Place to store label's lower-left. */
{
    *point = rect->r_ll;
    return 1;
}
