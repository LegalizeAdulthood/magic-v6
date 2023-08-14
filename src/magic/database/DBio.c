/*
 * DBio.c --
 *
 * Reading and writing of cells
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
static char rcsid[] = "$Header: DBio.c,v 6.0 90/08/28 18:09:53 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "utils.h"
#include "hash.h"
#include "database.h"
#include "databaseInt.h"
#include "tech.h"
#include "textio.h"
#include "drc.h"
#include "undo.h"
#include "malloc.h"
#include "signals.h"

extern int errno;
extern char *Path;

/* Suffix for all Magic files */
char *DBSuffix = ".mag";

/* If set to FALSE, don't print warning messages. */
bool DBVerbose = TRUE;

/* Forward declarations */
char *dbFgets();
FILE *dbReadOpen();
int DBFileOffset;

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellRead --
 *
 * If the cell is already marked as available (CDAVAILABLE), do nothing.
 *
 * Otherwise, read in the paint for a cell from its associated disk file.
 * If a filename for the cell is specified, we try to open it
 * somewhere in the search path.  Otherwise, we try the filename
 * already associated with the cell, or the name of the cell itself
 * as the name of the file containing the definition of the cell.
 *
 * Mark the cell definition as "read in" (CDAVAILABLE), and
 * recompute the bounding box.
 *
 *				WARNING:
 *
 * It is the responsibility of the caller to call DBReComputeBbox(cellDef)
 * at a convenient place, as we do not set the bounding box of the cell def.
 * If we were to update the bounding box here, this CellDef would first have
 * to be ripped out of each parent subcell tile plane in which it appears, and
 * then relinked in after its bounding box had changed.  Since DBCellRead() may
 * be called from in the middle of a database search, however, the database
 * modification resulting from this could ruin the search context and crash
 * the system.
 *
 * Results:
 *	TRUE if the cell could be read successfully, FALSE
 *	otherwise.  If the cell is already read in, TRUE is
 *	also returned.
 *
 * Side effects:
 *	Updates the tile planes for the cell definition.
 *	In the event of an error while reading in the cell,
 *	the external integer errno is set to the UNIX error
 *	encountered.
 *
 *	The cell definition is marked as available.
 *	The cell's MODIFIED bit is cleared by this routine.  If
 *	newTechOk is TRUE and the cell's technology is different
 *	from the current one, the current technology is changed.
 *
 * Errors:
 *	If incomplete specs are given either for a rectangle or for
 *	a cell use, then we immediately stop reading the file.
 *
 * File Format:
 *
 *	1. The first line of the file contains the string "magic".
 *
 *	2. Next comes an optional technology line, with the format
 *	"tech <tech>".  <tech> is the technology of the cell.
 *
 *	3. Next comes an optional line giving the cell's timestamp
 *	(the last time it or any of its children changed, as far as
 *	we know).  The syntax is "timestamp <value>", where <value>
 *	is an integer as returned by the library function time().
 *
 *	4. Next come groups of lines describing rectangles of the
 *	Magic tile types.  Each group is headed with a line of the
 *	form "<< layer >>".	The layer name is matched against the
 *	current technology.  Each line after the header has the
 *	format "rect <xbot> <ybot> <xtop> <ytop>".
 *	
 *	6. Zero or more groups of lines describing cell uses.  Each group
 *	is of the form
 *		use <filename> <id>
 *		array <xlo> <xhi> <xsep> <ylo> <yhi> <ysep>
 *		timestamp <int>
 *		transform <a> <b> <c> <d> <e> <f>
 *		box <xbot> <ybot> <xtop> <ytop>
 *	Each group may be preceded by one or more separator lines.  Note
 *	that <id> is optional and is omitted if there is to be no
 *	instance id for the cell use.  If it is omitted, an instance
 *	identifier is generated internally.  The "array" line may be
 *	omitted	if the cell use is not an array.  The "timestamp" line is
 *	optional;  if present, it gives the last time the parent
 *	was aware that the child changed.
 *
 *	7. If the cell contains labels, then the labels are preceded
 *	by the line "<< labels >>".  Each label is one line of the form
 *		rlabel <layer> <xbot> <ybot> <xtop> <ytop> <position> <text>
 *		or
 *		label <layer> <x> <y> <position> <text>
 *	(the second form is obsolete and is for point labels only).
 *
 *	8. The file is terminated by the line "<< end >>".
 *
 *	Note: be careful about any changes to this format:  there are
 *	several previous file formats still in use in old files, and
 *	the current one is upward compatible with all of them.
 *
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellRead(cellDef, name, setFileName)
    CellDef *cellDef;	/* Pointer to definition of cell to be read in */
    char *name;		/* Name of file from which to read definition.
			 * If NULL, then use cellDef->cd_file; if that
			 * is NULL try the name of the cell.
			 */
    bool setFileName;	/* If TRUE then cellDef->cd_file should be updated
			 * to point to the name of the file from which the
			 * cell was loaded.
			 */
{
    int cellStamp = 0, rectCount = 0, rectReport = 500;
    char line[2048], tech[50], layername[50];
    PaintResultType *ptable;
    bool result = TRUE;
    register FILE *f;
    register Rect *rp;
    register c;
    TileType type;
    Plane *plane;
    Rect r;

    if (cellDef->cd_flags & CDAVAILABLE)
	return (TRUE);

    if ((f = dbReadOpen(cellDef, name, setFileName)) == NULL)
	return (FALSE);

    /*
     * It's very important to disable interrupts during the body of
     * this routine.  Otherwise, if the user types the interrupt key
     * only part of the file will be read in, and if he then writes
     * the cell out, the disk copy will get trashed.
     */
    SigDisableInterrupts();

    /*
     * Process the header of the Magic file:
     * make sure that this is a Magic-format file and that it
     * has the right technology.  Magic files have a first line
     * of "magic".
     */
    if (dbFgets(line, sizeof line, f) == NULL)
	goto badfile;

    if (strncmp(line, "magic", 5) != 0)
    {
	TxError("First line in file must be \"magic\"; instead saw: %s", line);
	goto badfile;
    }
    if (dbFgets(line, sizeof line, f) == NULL)
	goto badfile;

    if ((line[0] != '<') && (line[0] != '\0'))
    {
	if (sscanf(line, "tech %49s", tech) != 1)
	{
	    TxError("Malformed \"tech\" line: %s", line);
	    goto badfile;
	}
	if (strcmp(DBTechName, tech) != 0)
	{
	    TxError("Cell %s has wrong technology %s\n", cellDef->cd_name,
			tech);
	    (void) fclose(f);
	    SigEnableInterrupts();
	    return (FALSE);
	}
	if (dbFgets(line, sizeof line, f) == NULL)
	    goto badfile;
	
	/* For backward compatibility, accept (and throw away) lines
	 * whose first word is "maxlabscale".
	 */
	if (line[0] == 'm')
	{
	    int throwAway;
	    if (sscanf(line, "maxlabscale %d", &throwAway) != 1)
		TxError("Expected maxlabscale but got: %s", line);
	    if (dbFgets(line, sizeof line, f) == NULL)
		goto badfile;
	}
	if (line[0] == 't')
	{
	    if (sscanf(line, "timestamp %d", &cellStamp) != 1)
		TxError("Expected timestamp but got: %s", line);
	    if (dbFgets(line, sizeof line, f) == NULL)
		goto badfile;
	}
    }


    /*
     * Next, get the paint, subcells, and labels for this cell.
     * While we are generating paints to the database, we want
     * to disable the undo package.
     */
    rp = &r;
    UndoDisable();
    while (TRUE)
    {
	/*
	 * Read the header line to get the layer name, then read as
	 * many rectangles as are specified on consecutive lines.
	 * If not a layer header line, then it should be a cell
	 * use header line.
	 */
	if (sscanf(line, "<< %s >>", layername) != 1)
	{
	    if (!dbReadUse(cellDef, line, sizeof line, f))
		goto badfile;
	    continue;
	}

	type = DBTechNameType(layername);
	if (type < 0)
	{
	    /*
	     * Look for special layer names:
	     *		labels	-- begins a list of labels
	     *		end	-- marks the end of this file
	     */
	    if (strcmp(layername, "labels") == 0)
	    {
		if (!dbReadLabels(cellDef, line, sizeof line, f)) goto badfile;
		continue;
	    }
	    if (strcmp(layername, "end") == 0) goto done;
	    TxError("Unknown layer %s ignored in %s\n", layername,
			cellDef->cd_name);
	}

	/* Encapsulates painting information for DBPaintPlane */
	if ((type > 0) && (DBPlane(type) > 0))
	{
	    ptable = DBStdPaintTbl(type, DBPlane(type));
	    plane = cellDef->cd_planes[DBPlane(type)];

	    /*
	     * Record presence of material in cell.
	     */
	    
	    TTMaskSetType(&cellDef->cd_types, type);
	}
	else plane = NULL;

	/*
	 * The following loop is executed once for each line
	 * in the file beginning with 'r'.
	 */
nextrect:
	while ((c = getc(f)) == 'r')
	{
	    /*
	     * GetRect actually reads the rest of the line up to
	     * a trailing newline or EOF.
	     */
	    if (!GetRect(f, 4, rp)) goto badfile;
	    if ((++rectCount % rectReport == 0) && DBVerbose)
	    {
		TxPrintf("%s: %d rects\n", cellDef->cd_name, rectCount);
		fflush(stdout);
	    }

	    /*
	     * Only add a new rectangle if it is non-null, and if the
	     * layer is reasonable.
	     */
	    if (!GEO_RECTNULL(rp) && (plane != NULL))
	    {
		/*
		 * We use DBPaintPlane, so inter-plane effects of painting
		 * tiles do not occur during read-in.  For files generated
		 * by Magic itself, this is not a problem; other clients
		 * should beware.
		 */
		DBPaintPlane(plane, rp, ptable, (PaintUndoInfo *) NULL);

		/* If a contact, paint it on all connected planes */
		if (DBConnPlanes[type])
		{
		    register pNum;

		    for (pNum = PL_TECHDEPBASE; pNum < DBNumPlanes; pNum++)
			if (PlaneMaskHasPlane(DBConnPlanes[type], pNum))
			    DBPaintPlane(cellDef->cd_planes[pNum], rp,
				    DBStdPaintTbl(type, pNum),
				    (PaintUndoInfo *) NULL);
		}
	    }
	}

	/*
	 * Ignore comments.
	 * Note we use fgets() since we only want to discard this line.
	 */
	if (c == '#')
	{
	    (void) fgets(line, sizeof line, f);
	    goto nextrect;
	}

	/*
	 * We reach here if the first character on a line is not
	 * 'r', meaning that we have reached the end of this
	 * section of rectangles.
	 */
	if (c == EOF) goto badfile;
	line[0] = c;
	if (dbFgets(&line[1], sizeof line - 1, f) == NULL) goto badfile;
    }

done:
    (void) fclose(f);
    cellDef->cd_flags &= ~(CDMODIFIED|CDBOXESCHANGED|CDGETNEWSTAMP);

    /*
     * Assign instance-ids to cell uses that didn't contain
     * explicit use identifiers.  Warn about duplicate instance
     * ids as well, changing these to unique ones.  We do this
     * here instead of on-the-fly during cell read-in, to avoid
     * an N**2 algorithm that blows up for large #s of subcells.
     */
    DBGenerateUniqueIds(cellDef, TRUE);

    /*
     * If the timestamp in the cell didn't match expectations,
     * notify the timestamp manager.  Note:  it's possible that
     * this cell is used only as the root of windows.  If that
     * is the case, then don't trigger a timestamp mismatch (we
     * can tell this by whether or not there are any parent uses
     * with non-null parent defs.  If the cell on disk had a zero
     * timestamp, then force the cell to be written out with a
     * correct timestamp.
     */
    if ((cellDef->cd_timestamp != cellStamp) || (cellStamp == 0))
    {
	CellUse *cu;
	for (cu = cellDef->cd_parents; cu != NULL; cu = cu->cu_nextuse)
	{
	    if (cu->cu_parent != NULL)
	    {
		DBStampMismatch(cellDef, &cellDef->cd_bbox);
		break;
	    }
	}
    }
    cellDef->cd_timestamp = cellStamp;
    if (cellStamp == 0)
    {
	TxError("\"%s\" has a zero timestamp; it should be written out\n",
	    cellDef->cd_name);
	TxError("    to establish a correct timestamp.\n");
	cellDef->cd_flags |= CDSTAMPSCHANGED|CDGETNEWSTAMP;
    }

    UndoEnable();
    DRCCheckThis(cellDef, TT_CHECKPAINT, (Rect *) NULL);
    SigEnableInterrupts();
    return (result);

badfile:
    TxError("File %s contained format error\n", cellDef->cd_name);
    DRCCheckThis(cellDef, TT_CHECKPAINT, (Rect *) NULL);
    result = FALSE;
    goto done;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbReadOpen --
 *
 * Open the file containing the cell we are going to read.
 * If a filename for the cell is specified ('name' is non-NULL),
 * we try to open it somewhere in the search path.  Otherwise,
 * we try the filename already associated with the cell, or the
 * name of the cell itself as the name of the file containing
 * the definition of the cell.
 *
 * If 'setFileName' is TRUE, then cellDef->cd_file will be updated
 * to point to the name of the file from which the cell was loaded.
 *
 * Results:
 *	Returns an open FILE * if successful, or NULL on error.
 *
 * Side effects:
 *	Opens a FILE.  Leaves cellDef->cd_flags marked as
 *	CDAVAILABLE, with the CDNOTFOUND bit clear, if we
 *	were successful.
 *
 * ----------------------------------------------------------------------------
 */

FILE *
dbReadOpen(cellDef, name, setFileName)
    CellDef *cellDef;	/* Def being read */
    char *name;		/* Name if specified, or NULL */
    bool setFileName;	/* If TRUE then cellDef->cd_file should be updated
			 * to point to the name of the file from which the
			 * cell was loaded.
			 */
{
    FILE *f;
    char *filename;

    if (name != (char *) NULL)
    {
	f = PaOpen(name, "r", DBSuffix, Path, CellLibPath, &filename);
    }
    else if (cellDef->cd_file != (char *) NULL)
    {
	f = PaOpen(cellDef->cd_file, "r", "", ".", (char *) NULL, &filename);
    }
    else
    {
	f = PaOpen(cellDef->cd_name, "r", DBSuffix,
			Path, CellLibPath, &filename);
    }

    if (f == NULL)
    {
	/* Don't print another message if we've already tried to read it */
	if (cellDef->cd_flags & CDNOTFOUND)
	    return ((FILE *) NULL);

	if (name != (char *) NULL)
	    TxError("File %s%s couldn't be found\n", name, DBSuffix);
	else if (cellDef->cd_file != (char *) NULL)
	    TxError("File %s couldn't be found\n", cellDef->cd_file);
	else
	    TxError("Cell %s couldn't be found\n", cellDef->cd_name);
	cellDef->cd_flags |= CDNOTFOUND;
	return ((FILE *) NULL);
    }

    if ((access(filename, 02) < 0) && DBVerbose)
	TxPrintf("Warning -- cell %s not writable\n", cellDef->cd_name);

    cellDef->cd_flags &= ~CDNOTFOUND;
    if (setFileName)
	(void) StrDup(&cellDef->cd_file, filename);
    cellDef->cd_flags |= CDAVAILABLE;
    return (f);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbReadUse --
 *
 * Read a single cell use specification.  Create a new cell
 * use that is a child of cellDef.  Create the def for this
 * child use if it doesn't already exist.
 *
 * On input, 'line' contains the "use" line; on exit, 'line'
 * contains the next line in the input after the "use".
 *
 * Results:
 *	Returns TRUE normally, or FALSE on error or EOF.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

bool
dbReadUse(cellDef, line, len, f)
    CellDef *cellDef;	/* Cell whose cells are being read */
    char *line;		/* Line containing "use ..." */
    int len;		/* Size of buffer pointed to by line */
    FILE *f;		/* Input file */
{
    int xlo, xhi, ylo, yhi, xsep, ysep, childStamp;
    int absa, absb, absd, abse;
    char cellname[1024], useid[1024];
    CellUse *subCellUse;
    CellDef *subCellDef;
    Transform t;
    Rect r;

    if (strncmp(line, "use", 3) != 0)
    {
	TxError("Expected \"use\" line but saw: %s", line);
	return (FALSE);
    }

    useid[0] = '\0';
    if (sscanf(line, "use %1023s %1023s", cellname, useid) < 1)
    {
	TxError("Malformed \"use\" line: %s", line);
	return (FALSE);
    }

    if (dbFgets(line, len, f) == NULL)
	return (FALSE);

    if (strncmp(line, "array", 5) == 0)
    {
	if (sscanf(line, "array %d %d %d %d %d %d",
		&xlo, &xhi, &xsep, &ylo, &yhi, &ysep) != 6)
	{
	    TxError("Malformed \"array\" line: %s", line);
	    return (FALSE);
	}
	if (dbFgets(line, len, f) == NULL)
	    return (FALSE);
    }
    else
    {
	xlo = ylo = 0;
	xhi = yhi = 0;
	xsep = ysep = 0;
    }

    if (strncmp(line, "timestamp", 9) == 0)
    {
	if (sscanf(line, "timestamp %d", &childStamp) != 1)
	{
	    TxError("Malformed \"timestamp\" line: %s", line);
	    return (FALSE);
	}
	if (dbFgets(line, len, f) == NULL)
	    return (FALSE);
    } else childStamp = 0;

    if (sscanf(line, "transform %d %d %d %d %d %d",
	    &t.t_a, &t.t_b, &t.t_c, &t.t_d, &t.t_e, &t.t_f) != 6)
    {
badTransform:
	TxError("Malformed or illegal \"transform\" line: %s", line);
	return (FALSE);
    }

    /*
     * Sanity check for transform.
     * Either a == e == 0 and both abs(b) == abs(d) == 1,
     *     or b == d == 0 and both abs(a) == abs(e) == 1.
     */
    if (t.t_a == 0)
    {
	absb = t.t_b > 0 ? t.t_b : -t.t_b;
	absd = t.t_d > 0 ? t.t_d : -t.t_d;
	if (t.t_e != 0 || absb != 1 || absd != 1)
	    goto badTransform;
    }
    else
    {
	absa = t.t_a > 0 ? t.t_a : -t.t_a;
	abse = t.t_e > 0 ? t.t_e : -t.t_e;
	if (t.t_b != 0 || t.t_d != 0 || absa != 1 || abse != 1)
	    goto badTransform;
    }

    if (dbFgets(line, len, f) == NULL)
	return (FALSE);

    if (sscanf(line, "box %d %d %d %d",
	    &r.r_xbot, &r.r_ybot, &r.r_xtop, &r.r_ytop) != 4)
    {
	TxError("Malformed \"box\" line: %s", line);
	return (FALSE);
    }

    /*
     * Set up cell use.
     * If the definition for this use has not been read in,
     * make a dummy one that's marked not available.  For now,
     * don't change the bounding box if the cell already exists
     * (we'll fix it below when handling timestamp problems).
     */
    subCellDef = DBCellLookDef(cellname);
    if (subCellDef == (CellDef *) NULL)
    {
	subCellDef = DBCellNewDef(cellname, (char *) NULL);
	subCellDef->cd_timestamp = childStamp;

	/* Make sure rectangle is non-degenerate */
	if (GEO_RECTNULL(&r))
	{
	    TxPrintf("Subcell has degenerate bounding box: %d %d %d %d\n",
		    r.r_xbot, r.r_ybot, r.r_xtop, r.r_ytop);
	    TxPrintf("Adjusting bounding box of subcell %s of %s",
		    cellname, cellDef->cd_name);
	    if (r.r_xtop <= r.r_xbot) r.r_xtop = r.r_xbot + 1;
	    if (r.r_ytop <= r.r_ybot) r.r_ytop = r.r_ybot + 1;
	    TxPrintf(" to %d %d %d %d\n",
		    r.r_xbot, r.r_ybot, r.r_xtop, r.r_ytop);
	}
	subCellDef->cd_bbox = r;
    }
    else if (DBIsAncestor(subCellDef, cellDef))
    {
	/*
	 * Watchout for attempts to create circular structures.
	 * If this happens, disregard the subcell.
	 */
	TxPrintf("Subcells are used circularly!\n");
	TxPrintf("Ignoring subcell %s of %s.\n", cellname,
	    cellDef->cd_name);
	goto nextLine;
    }

    subCellUse = DBCellNewUse(subCellDef, useid[0] ? useid : (char *) NULL);

    /*
     * Instead of calling DBLinkCell for each cell, DBGenerateUniqueIds()
     * gets called for the entire cell at the end.
     */

    DBMakeArray(subCellUse, &GeoIdentityTransform,
			    xlo, ylo, xhi, yhi, xsep, ysep);
    DBSetTrans(subCellUse, &t);

    /*
     * Link the subcell into the parent.
     * This should be the only place where a cell use
     * gets created as part of the database, and not recorded
     * on the undo list (because undo is disabled while the
     * cell is being read in).
     */
    DBPlaceCell(subCellUse, cellDef);

    /*
     * Things get real tricky if the our guess about the
     * timestamp doesn't match the existing timestamp in
     * subCellDef.  This can be because (1) subCellDef has
     * been read in, so we're just a confused parent, or
     * (2) the cell hasn't been read in yet, so two parents
     * disagree, and it's not clear which is correct.  In
     * either event, call the timestamp manager with our guess
     * area, since it seems to be wrong, and in case (2) also
     * call the timestamp manager with the existing area, since
     * that parent is probably confused too.  If the cell isn't
     * available, set the timestamp to zero to force mismatches
     * forever until the cell gets read from disk.
     */
    if ((childStamp != subCellDef->cd_timestamp) || (childStamp == 0))
    {
	DBStampMismatch(subCellDef, &r);
	if (!(subCellDef->cd_flags & CDAVAILABLE))
	    subCellDef->cd_timestamp = 0;
	else DBStampMismatch(subCellDef, &subCellDef->cd_bbox);
    }

nextLine:
    return (dbFgets(line, len, f) != NULL);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbReadLabels --
 *
 * Starting with the line << labels >>, read labels for 'cellDef'
 * up until the end of a label section.  On exit, 'line' contains
 * the line that terminated the label section, which will be either
 * a line of the form "<< something >>" or one beginning with a
 * character other than 'r' or 'l'.
 *
 * Results:
 *	Returns TRUE normally, or FALSE on error or EOF.
 *
 * Side effects:
 *	See above.
 *
 * ----------------------------------------------------------------------------
 */

bool
dbReadLabels(cellDef, line, len, f)
    CellDef *cellDef;	/* Cell whose labels are being read */
    char *line;		/* Line containing << labels >> */
    int len;		/* Size of buffer pointed to by line */
    FILE *f;		/* Input file */
{
    char layername[50], text[1024];
    TileType type;
    int orient;
    Rect r;

    /* Get first label line */
    if (dbFgets(line, len, f) == NULL) return (FALSE);

    while (TRUE)
    {
	/* Skip blank lines */
	while (line[0] == '\0')
	    if (dbFgets(line, len, f) == NULL)
		return (TRUE);

	/* Stop when at end of labels section (either paint or cell use) */
	if (line[0] != 'r' && line[0] != 'l') break;

	/*
	 * Labels may be either point labels or rectangular ones.
	 * Since each label is associated with a particular
	 * tile, the type of tile is also stored.
	 */
	if (line[0] == 'r')
	{
	    if (sscanf(line, "rlabel %1023s %d %d %d %d %d %99[^\n]",
		    layername, &r.r_xbot, &r.r_ybot, &r.r_xtop, &r.r_ytop,
			       &orient, text) != 7)
	    {
		TxError("Skipping bad \"rlabel\" line: %s", line);
		goto nextlabel;
	    }
	}
	else
	{
	    if (sscanf(line, "label %1023s %d %d %d %99[^\n]",
		    layername, &r.r_xbot, &r.r_ybot, &orient, text) != 5)
	    {
		TxError("Skipping bad \"label\" line: %s", line);
		goto nextlabel;
	    }
	    r.r_xtop = r.r_xbot;
	    r.r_ytop = r.r_ybot;
	}
	type = DBTechNameType(layername);
	if (type < 0)
	{
	    TxError("\
Warning: label \"%s\" attached to unknown type \"%s\"\n", text, layername);
	    type = TT_SPACE;
	}

	(void) DBPutLabel(cellDef, &r, orient, text, type);

nextlabel:
	if (dbFgets(line, len, f) == NULL)
	    break;
    }

    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbFgets --
 *
 * Like fgets(), but ignore lines beginning with a pound-sign.
 *
 * Results:
 *	Returns a pointer to 'line', or NULL on EOF.
 *
 * Side effects:
 *	Stores characters into 'line', terminating it with a
 *	NULL byte.
 *
 * ----------------------------------------------------------------------------
 */

char *
dbFgets(line, len, f)
    char *line;
    int len;
    register FILE *f;
{
    register char *cs;
    register int l;
    register c;

    do
    {
	cs = line, l = len;
	while (--l > 0 && (c = getc(f)) != EOF)
	{
	    *cs++ = c;
	    if (c == '\n')
		break;
	}

	if (c == EOF && cs == line)
	    return (NULL);

	*cs = '\0';
    } while (line[0] == '#');

    return (line);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellWriteFile --
 *
 * NOTE: this routine is usually not want you want.  Use DBCellWrite().
 *
 * Write out the paint for a cell to the specified file.
 * Mark the cell as having been written out.  Before calling this
 * procedure, the caller should make sure that timestamps have been
 * updated where appropriate.
 *
 * Results:
 *	TRUE if the cell could be written successfully, FALSE otherwise.
 *
 * Side effects:
 *	Writes a file to disk.
 * 	Does NOT close the file 'f', but does fflush(f) before
 * 	returning.
 *
 *	If successful, clears the CDMODIFIED, CDBOXESCHANGED,
 *	and CDSTAMPSCHANGED bits in cellDef->cd_flags.
 *
 *	In the event of an error while writing out the cell,
 *	the external integer errno is set to the UNIX error
 *	encountered, and the above bits are not cleared in
 *	cellDef->cd_flags.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellWriteFile(cellDef, f)
    CellDef *cellDef;	/* Pointer to definition of cell to be written out */
    FILE *f;		/* The FILE to write to */
{
    int dbWritePaintFunc(), dbWriteCellFunc();
    register Label *lab;
    struct writeArg arg;
    int pNum;
    TileType type;
    TileTypeBitMask typeMask;

#define FPRINTF(f,s)\
{\
     if (fprintf(f,s) == EOF) goto ioerror;\
     DBFileOffset += strlen(s);\
}
#define FPRINTR(f,s)\
{\
     if (fprintf(f,s) == EOF) return 1;\
     DBFileOffset += strlen(s);\
}

    if (f == NULL) return FALSE;

    /* If interrupts are left enabled, a partial file could get written.
     * This is not good.
     */

    SigDisableInterrupts();
    DBFileOffset = 0;

    if (cellDef->cd_flags & CDGETNEWSTAMP)
	TxPrintf("Magic error: writing out-of-date timestamp for %s.\n",
	    cellDef->cd_name);

    {
    	 char headerstring[256];
	 sprintf(headerstring,"magic\ntech %s\ntimestamp %d\n",
	 				DBTechName,cellDef->cd_timestamp);
	 FPRINTF(f,headerstring);
    }

    /*
     * Output the paint of the cell.
     * Note that we only output up to the last layer appearing
     * in the technology file (DBNumUserLayers-1), not any of the
     * automatically generated contact layers (DBNumUserLayers
     * through DBNumTypes-1).
     */
    arg.wa_file = f;
    for (type = TT_PAINTBASE; type < DBNumUserLayers; type++)
    {
	if ((pNum = DBPlane(type)) < 0)
	    continue;
	arg.wa_found = FALSE;
	arg.wa_type = type;
	TTMaskSetOnlyType(&typeMask, type);
	if (DBSrPaintArea((Tile *) NULL, cellDef->cd_planes[pNum],
		&TiPlaneRect, &typeMask, dbWritePaintFunc, (ClientData) &arg))
	    goto ioerror;
    }

    /* Now the cell uses */
    if (DBCellEnum(cellDef, dbWriteCellFunc, (ClientData) f))
	goto ioerror;

    /* Now labels */
    if (cellDef->cd_labels)
    {
	char lstring[256];

	FPRINTF(f,"<< labels >>\n");
	for (lab = cellDef->cd_labels; lab; lab = lab->lab_next)
	{
	       sprintf(lstring, "rlabel %s %d %d %d %d %d %s\n",
		    DBTypeLongName(lab->lab_type),
		    lab->lab_rect.r_xbot, lab->lab_rect.r_ybot,
		    lab->lab_rect.r_xtop, lab->lab_rect.r_ytop,
		    lab->lab_pos, lab->lab_text);
	        FPRINTF(f,lstring);
	}
    }
    FPRINTF(f, "<< end >>\n");

    if (fflush(f) == EOF || ferror(f))
    {
ioerror:
	TxError("Warning: I/O error in writing file\n");
	SigEnableInterrupts();
	return (FALSE);
    }
    cellDef->cd_flags &= ~(CDMODIFIED|CDBOXESCHANGED|CDSTAMPSCHANGED);
    SigEnableInterrupts();
    return (TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBCellWrite --
 *
 * Write out the paint for a cell to its associated disk file.
 * Mark the cell as having been written out.  Before calling this
 * procedure, the caller should make sure that timestamps have been
 * updated where appropriate.
 * 
 * This code is fairly tricky to ensure that we never destroy the
 * original contents of a cell in the event of an I/O error.  We
 * try the following approaches in order.
 *
 *  1.	If we can create a temporary file in the same directory as the
 *	target cell, do so.  Then write to the temporary file and rename
 *	it to the target cell name.
 *
 *  2.	If we can't create the above temporary file, open the target
 *	cell for APPENDING, then write the new contents to the END of
 *	the file.  If successful, rewind the now-expanded file and
 *	overwrite the beginning of the file, then truncate it.
 *
 *
 * Results:
 *	TRUE if the cell could be written successfully, FALSE otherwise.
 *
 * Side effects:
 *	Writes a file to disk.
 *	If successful, clears the CDMODIFIED, CDBOXESCHANGED,
 *	and CDSTAMPSCHANGED bits in cellDef->cd_flags.
 *
 *	In the event of an error while writing out the cell,
 *	the external integer errno is set to the UNIX error
 *	encountered, and the above bits are not cleared in
 *	cellDef->cd_flags.
 *
 * ----------------------------------------------------------------------------
 */

bool
DBCellWrite(cellDef, fileName)
    CellDef *cellDef;	/* Pointer to definition of cell to be written out */
    char *fileName;	/* If not NULL, name of file to write.  If NULL,
			 * the name associated with the CellDef is used
			 */
{
#define NAME_SIZE	1000
    char *template = ".XXXXXXX";
    char *realname, *tmpname, *expandname;
    char *cp1, *cp2;
    char expandbuf[NAME_SIZE];
    FILE *realf, *tmpf;
    struct stat statb;
    bool result;

    result = FALSE;

    /*
     * Figure out the name of the file we will eventually write.
     */
    if (fileName)
    {
	MALLOC(char *, realname, strlen(fileName)+strlen(DBSuffix)+1);
	(void) sprintf(realname, "%s%s", fileName, DBSuffix);
    }
    else if (cellDef->cd_file)
    {
	realname = StrDup((char **) NULL, cellDef->cd_file);
    }
    else if (cellDef->cd_name)
    {
	MALLOC(char *, realname, strlen(cellDef->cd_name)+strlen(DBSuffix)+1);
	(void) sprintf(realname, "%s%s", cellDef->cd_name, DBSuffix);
    }
    else return (FALSE);

    /*
     * Expand the filename, removing the leading ~, if any.
     */
    expandname = expandbuf;
    cp1 = realname;
    cp2 = expandname;
    if (PaConvertTilde(&cp1, &cp2, NAME_SIZE) == -1)
	expandname = realname;

    /*
     * Figure name of the temp file to write.
     */
    MALLOC(char *, tmpname, strlen(expandname) + strlen(template) + 1);
    (void) sprintf(tmpname, "%s%s", expandname, template);
    mktemp(tmpname);

    /* Critical: disable interrupts while we do our work */
    SigDisableInterrupts();

    /* Don't allow a write if the file isn't writable! */
    if (access(expandname, F_OK) == 0 && access(expandname, W_OK) < 0)
    {
	perror(expandname);
	goto cleanup;
    }

    /*
     * See if we can create a temporary file in this directory.
     * If so, write to the temp file and then rename it after
     * we're done.
     */
    if (tmpf = fopen(tmpname, "w"))
    {
	result = DBCellWriteFile(cellDef, tmpf);
	(void) fclose(tmpf);
	if (!result)
	{
	    /*
	     * Total loss -- just can't write the file.
	     * The error message is printed elsewhere.
	     */
	    (void) unlink(tmpname);
	    goto cleanup;
	}

	/*
	 * The temp file is in good shape -- rename it to the real name,
	 * thereby completing the write.  The error below should NEVER
	 * normally happen.
	 */
	if (rename(tmpname, expandname) < 0)
	{
	    result = FALSE;
	    perror("rename");
	    TxError(
"\
ATTENTION: Magic was unable to rename file %s to %s.\n\
If the file %s exists, it is the old copy of the cell %s.\n\
The new copy is in the file %s.  Please copy this file\n\
to a safe place before executing any more Magic commands.\n",
		tmpname, expandname, expandname, cellDef->cd_name, tmpname);
	    goto cleanup;
	}
    }
    else
    {
	/*
	 * Couldn't create a temp file in this directory.  Instead, open
	 * the original file for APPENDING, write this cell (just to make
	 * sure the file is big enough), then rewind and write this cell
	 * again, and finally truncate the file.  The idea here is that
	 * by appending to realf, we don't trash the existing data, but
	 * do guarantee that there's enough space left to rewrite the
	 * file (in effect, we're pre-reserving space for it).
	 */
	realf = fopen(expandname, "a");
	if (realf == (FILE *) NULL)
	{
	    perror(expandname);
	    result = FALSE;
	    goto cleanup;
	}

	/* Remember the original length of the file for later truncation */
	(void) fstat(fileno(realf), &statb);

	/* Try to write by appending to the end of realf */
	if (!(result = DBCellWriteFile(cellDef, realf)))
	{
	    /* Total loss -- just can't write the file */
	    (void) fclose(realf);
	    (void) truncate(expandname, (long) statb.st_size);
	    goto cleanup;
	}

	/*
	 * Only try rewriting if the file wasn't zero-size to begin with.
	 * (If the file were zero-size, we're already done).
	 */
	if (statb.st_size > 0)
	{
	    rewind(realf);
	    result = DBCellWriteFile(cellDef, realf);
	    if (!result)
	    {
		/* Should NEVER happen */
		if (errno) perror(expandname);
		TxError("Something went wrong and the file %s was truncated\n",
			expandname);
		TxError("Try saving it in another file that is on a \n");
		TxError("filesystem where there is enough space!\n");
		(void) fclose(realf);
		goto cleanup;
	    }

	    /* Successful writing the second time around */
	    statb.st_size = ftell(realf);
	    (void) fclose(realf);
	    (void) truncate(expandname, (long) statb.st_size);
	}
    }

    /* Everything worked. */
    (void) StrDup(&cellDef->cd_file, expandname);
    result = TRUE;
    {
    	 struct stat thestat;
	 realf = fopen(expandname,"r");
	 fstat(fileno(realf),&thestat);
	 if (thestat.st_size != DBFileOffset)
	 {
              cellDef->cd_flags |= CDMODIFIED;
	      TxError("Warning: I/O error in writing file\n");
	 }
	 fclose(realf);
    }


cleanup:
    SigEnableInterrupts();
    FREE(realname);
    FREE(tmpname);
    return result;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbWritePaintFunc --
 *
 * Filter function used to write out a single paint tile.
 * Only writes out tiles of type arg->wa_type.
 * If the tile is the first encountered of its type, the header
 *	<< typename >>
 * is output.
 *
 * Results:
 *	Normally returns 0; returns 1 on I/O error.
 *
 * Side effects:
 *	Writes to the disk file.
 *
 * ----------------------------------------------------------------------------
 */

int
dbWritePaintFunc(tile, cdarg)
    Tile *tile;
    ClientData cdarg;
{
    char pstring[256];
    struct writeArg *arg = (struct writeArg *) cdarg;
    TileType type = TiGetType(tile);

    if (type != arg->wa_type)
	return 0;

    if (!arg->wa_found)
    {
	sprintf(pstring, "<< %s >>\n", DBTypeLongName(type));
	FPRINTR(arg->wa_file,pstring);
	arg->wa_found = TRUE;
    }

    sprintf(pstring, "rect %d %d %d %d\n",
	    LEFT(tile), BOTTOM(tile), RIGHT(tile), TOP(tile));
    FPRINTR(arg->wa_file,pstring);
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbWriteCellFunc --
 *
 * Filter function used to write out a single cell use in the
 * subcell tile plane for a cell.
 *
 * Results:
 *	Normally returns 0; return 1 on I/O error
 *
 * Side effects:
 *	Writes to the disk file.
 *
 * ----------------------------------------------------------------------------
 */

int
dbWriteCellFunc(cellUse, f)
    CellUse *cellUse;	/* Cell use whose "call" is to be written to a file */
    FILE *f;		/* File to which use is to be written */
{
    register Transform *t;
    register Rect *b;
    char     cstring[256];

    t = &(cellUse->cu_transform);
    b = &(cellUse->cu_def->cd_bbox);
    sprintf(cstring, "use %s %s\n", cellUse->cu_def->cd_name,cellUse->cu_id);
    FPRINTR(f,cstring);

    if ((cellUse->cu_xlo != cellUse->cu_xhi)
	    || (cellUse->cu_ylo != cellUse->cu_yhi))
    {
	sprintf(cstring, "array %d %d %d %d %d %d\n",
		cellUse->cu_xlo, cellUse->cu_xhi, cellUse->cu_xsep,
		cellUse->cu_ylo, cellUse->cu_yhi, cellUse->cu_ysep);
	FPRINTR(f,cstring);
    }

    sprintf(cstring, "timestamp %d\n", cellUse->cu_def->cd_timestamp);
    FPRINTR(f,cstring)
    sprintf(cstring, "transform %d %d %d %d %d %d\n",
	    t->t_a, t->t_b, t->t_c, t->t_d, t->t_e, t->t_f);
    FPRINTR(f,cstring)
    sprintf(cstring, "box %d %d %d %d\n",
	    b->r_xbot, b->r_ybot, b->r_xtop, b->r_ytop);
    FPRINTR(f,cstring)
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBGetTech --
 *
 * 	Reads the first few lines of a file to find out what technology
 *	it is.
 *
 * Results:
 *	The return value is a pointer to a string containing the name
 *	of the technology of the file containing cell cellName.  NULL
 *	is returned if the file couldn't be read or isn't in Magic
 *	format.  The string is stored locally to this procedure and
 *	will be overwritten on the next call to this procedure.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

char *
DBGetTech(cellName)
    char *cellName;			/* Name of cell whose technology
					 * is desired.
					 */
{
    FILE *f;
    static char line[512];
    char *p;

    f = PaOpen(cellName, "r", DBSuffix, Path, CellLibPath, (char **) NULL);
    if (f == NULL) return NULL;

    p = (char *) NULL;
    if (dbFgets(line, sizeof line - 1, f) == NULL) goto ret;
    if (strcmp(line, "magic\n") != 0) goto ret;
    if (dbFgets(line, sizeof line - 1, f) == NULL) goto ret;
    if (strncmp(line, "tech ", 5) != 0) goto ret;
    for (p = &line[5]; (*p != '\n') && (*p != 0); p++)
	/* Find the newline */;
    *p = 0;
    for (p = &line[5]; isspace(*p); p++)
	/* Find the tech name */;

ret:
    (void) fclose(f);
    return (p);
}

/*
 * ----------------------------------------------------------------------------
 *
 * DBPanicSave --
 *
 * Save all modified cells to disk, in files "cellname.save".
 * This is intended as an emergency measure for cases when magic
 * has to die (eg, upon receiving a SIGTERM signal).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes cells to disk.
 *	Does NOT clear the modified bits.
 *
 * ----------------------------------------------------------------------------
 */

void
DBPanicSave()
{
    int dbPanicFunc();
    int flags = CDMODIFIED | CDBOXESCHANGED | CDSTAMPSCHANGED;

    DBUpdateStamps();
    (void) DBCellSrDefs(flags, dbPanicFunc, (ClientData) NULL);
}

/*
 * Filter function used by DBPanicSave() above.
 * This function is called for each known CellDef whose modified bit
 * is set.
 */

    /*ARGSUSED*/
int
dbPanicFunc(def)
    CellDef *def;	/* Pointer to CellDef to be saved */
{
    static char suffix[] = ".save";
    char savename[512], *name = def->cd_file;

    if (def->cd_flags & CDINTERNAL) return 0;
    if (name == NULL) name = def->cd_name;
    strncpy(savename, name, sizeof savename - sizeof suffix);
    savename[sizeof savename - sizeof suffix] = '\0';
    strcat(savename, ".save");
    (void) DBCellWrite(def, savename);
    return 0;
}
