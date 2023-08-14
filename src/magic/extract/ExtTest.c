/*
 * ExtTest.c --
 *
 * Circuit extraction.
 * Interface for testing.
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
static char rcsid[] = "$Header: ExtTest.c,v 6.0 90/08/28 18:15:32 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "utils.h"
#include "geometry.h"
#include "graphics.h"
#include "styles.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "windows.h"
#include "dbwind.h"
#include "main.h"
#include "commands.h"
#include "textio.h"
#include "txcommands.h"
#include "debug.h"
#include "extract.h"
#include "extractInt.h"

int extDebAreaEnum;
int extDebArray;
int extDebHardWay;
int extDebHierCap;
int extDebHierAreaCap;
int extDebLabel;
int extDebNeighbor;
int extDebNoArray;
int extDebNoFeedback;
int extDebNoHard;
int extDebNoSubcell;
int extDebLength;
int extDebPerim;
int extDebResist;
int extDebVisOnly;
int extDebYank;

/*
 * The following are used for selective redisplay while debugging
 * the circuit extractor.
 */
Rect extScreenClip;
CellDef *extCellDef;
Window *extDebugWindow;

/* The width of an edge in pixels when it is displayed */
int extEdgePixels = 4;

int extShowInter();

/*
 * ----------------------------------------------------------------------------
 *
 * ExtractTest --
 *
 * Command interface for testing circuit extraction.
 * Usage:
 *	*extract
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Extracts the current cell, writing a file named
 *	currentcellname.ext.
 *
 * ----------------------------------------------------------------------------
 */

ExtractTest(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    extern int extSubtreeTotalArea;
    extern int extSubtreeInteractionArea;
    extern int extSubtreeClippedArea;
    static Plane *interPlane = (Plane *) NULL;
    static int areaTotal = 0, areaInteraction = 0, areaClipped = 0;
    int a1, a2, n, halo, bloat;
    CellUse *selectedCell;
    Rect editArea;
    char *addr, *name;
    FILE *f;
    typedef enum {  CLRDEBUG, CLRLENGTH, DRIVER, INTERACTIONS,
		    INTERCOUNT, PARENTS, RECEIVER, SETDEBUG, SHOWDEBUG,
		    SHOWPARENTS, SHOWTECH, STATS, STEP, TIME } cmdType;
    static struct
    {
	char	*cmd_name;
	cmdType	 cmd_val;
    } cmds[] = {
	"clrdebug",		CLRDEBUG,
	"clrlength",		CLRLENGTH,
	"driver",		DRIVER,
	"interactions",		INTERACTIONS,
	"intercount",		INTERCOUNT,
	"parents",		PARENTS,
	"receiver",		RECEIVER,
	"setdebug",		SETDEBUG,
	"showdebug",		SHOWDEBUG,
	"showparents",		SHOWPARENTS,
	"showtech",		SHOWTECH,
	"stats",		STATS,
	"step",			STEP,
	"times",		TIME,
	0
    };

    if (cmd->tx_argc == 1)
    {
	selectedCell = CmdGetSelectedCell((Transform *) NULL);
	if (selectedCell == NULL)
	{
	    TxError("No cell selected\n");
	    return;
	}

	extDispInit(selectedCell->cu_def, w);
	ExtCell(selectedCell->cu_def, selectedCell->cu_def->cd_name, FALSE);
	return;
    }

    n = LookupStruct(cmd->tx_argv[1], (LookupTable *) cmds, sizeof cmds[0]);
    if (n < 0)
    {
	TxError("Unrecognized subcommand: %s\n", cmd->tx_argv[1]);
	TxError("Valid subcommands:");
	for (n = 0; cmds[n].cmd_name; n++)
	    TxError(" %s", cmds[n].cmd_name);
	TxError("\n");
	return;
    }

    switch (cmds[n].cmd_val)
    {
	case STATS:
	    areaTotal += extSubtreeTotalArea;
	    areaInteraction += extSubtreeInteractionArea;
	    areaClipped += extSubtreeClippedArea;
	    TxPrintf("Extraction statistics (recent/total):\n");
	    TxPrintf("Total area of all cells = %d / %d\n",
			extSubtreeTotalArea, areaTotal);
	    a1 = extSubtreeTotalArea;
	    a2 = areaTotal;
	    if (a1 == 0) a1 = 1;
	    if (a2 == 0) a2 = 1;
	    TxPrintf(
	    "Total interaction area processed = %d (%.2f%%) / %d (%.2f%%)\n",
		extSubtreeInteractionArea,
		((double) extSubtreeInteractionArea) / ((double) a1) * 100.0,
		((double) areaInteraction) / ((double) a2) * 100.0);
	    TxPrintf(
	    "Clipped interaction area= %d (%.2f%%) / %d (%.2f%%)\n",
		extSubtreeClippedArea,
		((double) extSubtreeClippedArea) / ((double) a1) * 100.0,
		((double) areaClipped) / ((double) a2) * 100.0);
	    extSubtreeTotalArea = 0;
	    extSubtreeInteractionArea = 0;
	    extSubtreeClippedArea = 0;
	    break;
	case INTERACTIONS:
	    if (interPlane == NULL)
		interPlane = DBNewPlane((ClientData) TT_SPACE);
	    halo = 1, bloat = 0;
	    if (cmd->tx_argc > 2) halo = atoi(cmd->tx_argv[2]) + 1;
	    if (cmd->tx_argc > 3) bloat = atoi(cmd->tx_argv[3]);
	    ExtFindInteractions(EditCellUse->cu_def, halo, bloat, interPlane);
	    (void) DBSrPaintArea((Tile *) NULL, interPlane, &TiPlaneRect,
			&DBAllButSpaceBits, extShowInter, (ClientData) NULL);
	    DBClearPaintPlane(interPlane);
	    break;
	case INTERCOUNT:
	    f = stdout;
	    halo = 1;
	    if (cmd->tx_argc > 2)
		halo = atoi(cmd->tx_argv[2]);
	    if (cmd->tx_argc > 3)
	    {
		f = fopen(cmd->tx_argv[3], "w");
		if (f == NULL)
		{
		    perror(cmd->tx_argv[3]);
		    break;
		}
	    }
	    ExtInterCount((CellUse *) w->w_surfaceID, halo, f);
	    if (f != stdout)
		(void) fclose(f);
	    break;
	case TIME:
	    f = stdout;
	    if (cmd->tx_argc > 2)
	    {
		f = fopen(cmd->tx_argv[2], "w");
		if (f == NULL)
		{
		    perror(cmd->tx_argv[2]);
		    break;
		}
	    }
	    ExtTimes((CellUse *) w->w_surfaceID, f);
	    if (f != stdout)
		(void) fclose(f);
	    break;
	case PARENTS:
	    if (ToolGetEditBox(&editArea))
		ExtParentArea(EditCellUse, &editArea, TRUE);
	    break;

	case DRIVER:
	    if (cmd->tx_argc != 3)
	    {
		TxError("Usage: *extract driver terminalname\n");
		break;
	    }
	    ExtSetDriver(cmd->tx_argv[2]);
	    break;
	case RECEIVER:
	    if (cmd->tx_argc != 3)
	    {
		TxError("Usage: *extract receiver terminalname\n");
		break;
	    }
	    ExtSetReceiver(cmd->tx_argv[2]);
	    break;
	case CLRLENGTH:
	    TxPrintf("Clearing driver/receiver length list\n");
	    ExtLengthClear();
	    break;

	case SHOWPARENTS:
	    if (ToolGetEditBox(&editArea))
		ExtParentArea(EditCellUse, &editArea, FALSE);
	    break;
	case SETDEBUG:
	    DebugSet(extDebugID, cmd->tx_argc - 2, &cmd->tx_argv[2], TRUE);
	    break;
	case CLRDEBUG:
	    DebugSet(extDebugID, cmd->tx_argc - 2, &cmd->tx_argv[2], FALSE);
	    break;

	case SHOWDEBUG:
	    DebugShow(extDebugID);
	    break;
	case SHOWTECH:
	    extShowTech(cmd->tx_argc > 2 ? cmd->tx_argv[2] : "-");
	    break;
	case STEP:
	    TxPrintf("Current interaction step size is %d\n",
		    ExtCurStyle->exts_stepSize);
	    if (cmd->tx_argc > 2)
	    {
		ExtCurStyle->exts_stepSize = atoi(cmd->tx_argv[2]);
		TxPrintf("New interaction step size is %d\n",
			ExtCurStyle->exts_stepSize);
	    }
	    break;
    }
}

extShowInter(tile)
    Tile *tile;
{
    Rect r;

    TiToRect(tile, &r);
    DBWFeedbackAdd(&r, "interaction", EditCellUse->cu_def,
	    1, STYLE_MEDIUMHIGHLIGHTS);

    return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extShowTech --
 *
 * Display the technology-specific tables maintained for circuit
 * extraction in a human-readable format.  Intended mainly for
 * debugging technology files.  If the argument 'name' is "-",
 * the output is to the standard output; otherwise, it is to
 * the file whose name is 'name'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

extShowTech(name)
    char *name;
{
    FILE *out;
    register TileType t, s;
    register int p;
    register EdgeCap *e;

    if (strcmp(name, "-") == 0)
	out = stdout;
    else
    {
	out = fopen(name, "w");
	if (out == NULL)
	{
	    perror(name);
	    return;
	}
    }

    extShowTrans("Transistor", &ExtCurStyle->exts_transMask, out);

    (void) fprintf(out, "\nNode resistance and capacitance:\n");
    (void) fprintf(out, "type     R-ohm/sq  AreaC-ff/l**2\n");
    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
	(void) fprintf(out, "%-8.8s %8d      %9d\n",
		    DBTypeShortName(t),
		    ExtCurStyle->exts_resistByResistClass[
			ExtCurStyle->exts_typeToResistClass[t]],
		    ExtCurStyle->exts_areaCap[t]);

    (void) fprintf(out, "\nTypes contributing to resistive perimeter:\n");
    (void) fprintf(out, "type     R-type boundary types\n");
    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
    {
	(void) fprintf(out, "%-8.8s ", DBTypeShortName(t));
	(void) fprintf(out, "%7d ", ExtCurStyle->exts_typeToResistClass[t]);
	extShowMask(&ExtCurStyle->exts_typesResistChanged[t], out);
	(void) fprintf(out, "\n");
    }

    (void) fprintf(out, "\nSidewall capacitance:\n");
    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
	for (s = 0; s < DBNumTypes; s++)
	    if (ExtCurStyle->exts_perimCap[t][s])
		(void) fprintf(out, "    %-8.8s %-8.8s %8d\n",
			DBTypeShortName(t), DBTypeShortName(s),
			ExtCurStyle->exts_perimCap[t][s]);

    (void) fprintf(out, "\nInternodal overlap capacitance:\n");
    (void) fprintf(out, "\n  (by plane)\n");
    for (p = PL_TECHDEPBASE; p < DBNumPlanes; p++)
    {
	if (PlaneMaskHasPlane(ExtCurStyle->exts_overlapPlanes	, p))
	{
	    (void) fprintf(out, "    %-10.10s: types=", DBPlaneShortName(p));
	    extShowMask(&ExtCurStyle->exts_overlapTypes	[p], out);
	    (void) fprintf(out, "\n");
	}
    }
    (void) fprintf(out, "\n  (by type)\n");
    for (t = 0; t < DBNumTypes; t++)
	if (!TTMaskIsZero(&ExtCurStyle->exts_overlapOtherTypes[t]))
	{
	    (void) fprintf(out, "    %-10.10s: planes=", DBTypeShortName(t));
	    extShowPlanes(ExtCurStyle->exts_overlapOtherPlanes[t], out);
	    (void) fprintf(out, "\n      overlapped types=");
	    extShowMask(&ExtCurStyle->exts_overlapOtherTypes[t], out);
	    (void) fprintf(out, "\n");
	    for (s = 0; s < DBNumTypes; s++)
		if (ExtCurStyle->exts_overlapCap[t][s])
		    (void) fprintf(out, "              %-10.10s: %8d\n",
				DBTypeShortName(s), ExtCurStyle->exts_overlapCap[t][s]);
	}

    (void) fprintf(out, "\nSidewall-coupling/sidewall-overlap capacitance:\n");
    (void) fprintf(out, "\n  (by plane)\n");
    for (p = PL_TECHDEPBASE; p < DBNumPlanes; p++)
    {
	if (PlaneMaskHasPlane(ExtCurStyle->exts_sidePlanes, p))
	{
	    (void) fprintf(out, "    %-10.10s: ", DBPlaneShortName(p));
	    extShowMask(&ExtCurStyle->exts_sideTypes[p], out);
	    (void) fprintf(out, "\n");
	}
    }
    (void) fprintf(out, "\n  (by type)\n");
    for (s = 0; s < DBNumTypes; s++)
	if (!TTMaskIsZero(&ExtCurStyle->exts_sideEdges[s]))
	{
	    (void) fprintf(out, "    %-10.10s: ", DBTypeShortName(s));
	    extShowMask(&ExtCurStyle->exts_sideEdges[s], out);
	    (void) fprintf(out, "\n");
	    for (t = 0; t < DBNumTypes; t++)
	    {
		if (!TTMaskIsZero(&ExtCurStyle->exts_sideCoupleOtherEdges[s][t]))
		{
		    (void) fprintf(out, "                edge mask=");
		    extShowMask(&ExtCurStyle->exts_sideCoupleOtherEdges[s][t], out);
		    (void) fprintf(out, "\n");
		}
		if (!TTMaskIsZero(&ExtCurStyle->exts_sideOverlapOtherTypes[s][t]))
		{
		    (void) fprintf(out, "                overlap mask=");
		    extShowMask(&ExtCurStyle->exts_sideOverlapOtherTypes[s][t],
				out);
		    (void) fprintf(out, "\n");
		}
		if (e = ExtCurStyle->exts_sideCoupleCap[s][t])
		    for ( ; e; e = e->ec_next)
		    {
			(void) fprintf(out, "                COUPLE: ");
			extShowMask(&e->ec_near, out);
			(void) fprintf(out, " || ");
			extShowMask(&e->ec_far, out);
			(void) fprintf(out, ": %d\n", e->ec_cap);
		    }
		if (e = ExtCurStyle->exts_sideOverlapCap[s][t])
		    for ( ; e; e = e->ec_next)
		    {
			(void) fprintf(out, "                OVERLAP: ");
			extShowMask(&e->ec_near, out);
			(void) fprintf(out, ": %d\n", e->ec_cap);
		    }
	    }
	}

    (void) fprintf(out, "\n\nSidewall coupling halo = %d\n", ExtCurStyle->exts_sideCoupleHalo	);

    extShowConnect("\nNode connectivity", ExtCurStyle->exts_nodeConn, out);
    extShowConnect("\nResistive region connectivity", ExtCurStyle->exts_resistConn, out);
    extShowConnect("\nTransistor connectivity", ExtCurStyle->exts_transConn, out);

    if (out != stdout)
	(void) fclose(out);
}

extShowTrans(name, mask, out)
    char *name;
    register TileTypeBitMask *mask;
    FILE *out;
{
    register TileType t;

    (void) fprintf(out, "%s types: ", name);
    extShowMask(mask, out);
    (void) fprintf(out, "\n");

    for (t = 0; t < DBNumTypes; t++)
	if (TTMaskHasType(mask, t))
	{
	    (void) fprintf(out, "    %-8.8s  %d terminals: ",
			DBTypeShortName(t), ExtCurStyle->exts_transSDCount[t]);
	    extShowMask(&ExtCurStyle->exts_transSDTypes[t], out);
	    (void) fprintf(out, "\n\tcap (gate-sd/gate-ch) = %d/%d\n",
			ExtCurStyle->exts_transSDCap[t],
			ExtCurStyle->exts_transGateCap[t]);
	}
}

extShowConnect(hdr, connectsTo, out)
    char *hdr;
    TileTypeBitMask *connectsTo;
    FILE *out;
{
    register TileType t;

    (void) fprintf(out, "%s\n", hdr);
    for (t = TT_TECHDEPBASE; t < DBNumTypes; t++)
	if (!TTMaskEqual(&connectsTo[t], &DBZeroTypeBits))
	{
	    (void) fprintf(out, "    %-8.8s: ", DBTypeShortName(t));
	    extShowMask(&connectsTo[t], out);
	    (void) fprintf(out, "\n");
	}
}

extShowMask(m, out)
    register TileTypeBitMask *m;
    FILE *out;
{
    register TileType t;
    register bool first = TRUE;

    for (t = 0; t < DBNumTypes; t++)
	if (TTMaskHasType(m, t))
	{
	    if (!first)
		(void) fprintf(out, ",");
	    first = FALSE;
	    (void) fprintf(out, "%s", DBTypeShortName(t));
	}
}

extShowPlanes(m, out)
    register int m;
    FILE *out;
{
    register int pNum;
    register bool first = TRUE;

    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
	if (PlaneMaskHasPlane(m, pNum))
	{
	    if (!first)
		(void) fprintf(out, ",");
	    first = FALSE;
	    (void) fprintf(out, "%s", DBPlaneShortName(pNum));
	}
}

/*
 * ----------------------------------------------------------------------------
 *
 * extDispInit --
 *
 * Initialize the screen information to be used during
 * extraction debugging.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes extDebugWindow, extScreenClip, and extCellDef.
 *
 * ----------------------------------------------------------------------------
 */

extDispInit(def, w)
    CellDef *def;
    Window *w;
{
    extDebugWindow = w;
    extCellDef = def;
    extScreenClip = w->w_screenArea;
    GeoClip(&extScreenClip, &GrScreenRect);
}

/*
 * ----------------------------------------------------------------------------
 *
 * extShowEdge --
 *
 * Display the edge described by the Boundary 'bp' on the display,
 * with text string 's' on the text terminal.  Prompt with '--next--'
 * to allow a primitive sort of 'more' processing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the display.
 *
 * ----------------------------------------------------------------------------
 */

extShowEdge(s, bp)
    char *s;
    Boundary *bp;
{
    Rect extScreenRect, edgeRect;
    int style = STYLE_PURPLE1;

    edgeRect = bp->b_segment;
    WindSurfaceToScreen(extDebugWindow, &edgeRect, &extScreenRect);
    if (extScreenRect.r_ybot == extScreenRect.r_ytop)
    {
	extScreenRect.r_ybot -= extEdgePixels/2;
	extScreenRect.r_ytop += extEdgePixels - extEdgePixels/2;
    }
    else /* extScreenRect.r_xtop == extScreenRect.r_xbot */
    {
	extScreenRect.r_xbot -= extEdgePixels/2;
	extScreenRect.r_xtop += extEdgePixels - extEdgePixels/2;
    }

    if (DebugIsSet(extDebugID, extDebVisOnly))
    {
	Rect r;

	r = extScreenRect;
	GeoClip(&r, &extScreenClip);
	if (r.r_xtop <= r.r_xbot || r.r_ytop <= r.r_ybot)
	    return;
    }

    TxPrintf("%s: ", s);
    GrLock(extDebugWindow, TRUE);
    GrClipBox(&extScreenRect, style);
    GrUnlock(extDebugWindow);
    (void) GrFlush();
    extMore();
    GrLock(extDebugWindow, TRUE);
    GrClipBox(&extScreenRect, STYLE_ORANGE1);
    GrUnlock(extDebugWindow);
    (void) GrFlush();
}

/*
 * ----------------------------------------------------------------------------
 *
 * extShowTile --
 *
 * Display the tile 'tp' on the display by highlighting it.  Also show
 * the text string 's' on the terminal.  Prompt with '--next--' to allow
 * a primitive sort of more processing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the display.
 *
 * ----------------------------------------------------------------------------
 */

extShowTile(tile, s, style_index)
    Tile *tile;
    char *s;
    int style_index;
{
    Rect tileRect;
    static int styles[] = { STYLE_PALEHIGHLIGHTS, STYLE_DOTTEDHIGHLIGHTS };

    TiToRect(tile, &tileRect);
    if (!extShowRect(&tileRect, styles[style_index]))
	return;

    TxPrintf("%s: ", s);
    extMore();
    (void) extShowRect(&tileRect, STYLE_ERASEHIGHLIGHTS);
}

bool
extShowRect(r, style)
    Rect *r;
    int style;
{
    Rect extScreenRect;

    WindSurfaceToScreen(extDebugWindow, r, &extScreenRect);
    if (DebugIsSet(extDebugID, extDebVisOnly))
    {
	Rect rclip;

	rclip = extScreenRect;
	GeoClip(&rclip, &extScreenClip);
	if (rclip.r_xtop <= rclip.r_xbot || rclip.r_ytop <= rclip.r_ybot)
	    return (FALSE);
    }

    GrLock(extDebugWindow, TRUE);
    GrClipBox(&extScreenRect, style);
    GrUnlock(extDebugWindow);
    (void) GrFlush();
    return (TRUE);
}

extMore()
{
    char line[100];

    TxPrintf("--next--"); (void) fflush(stdout);
    (void) TxGetLine(line, sizeof line);
}

extNewYank(name, puse, pdef)
    char *name;
    CellUse **puse;
    CellDef **pdef;
{
    DBNewYank(name, puse, pdef);
}
