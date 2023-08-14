/*
 * main.h --
 *
 * Header file containing global variables for all MAGIC modules and a 
 * couple of global procedures.
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
 *
 *
 * rcsid="$Header: main.h,v 6.0 90/08/28 18:47:22 mayo Exp $"
 */

#define _MAIN

#ifndef	_DATABASE
int err0 = Need_to_include_database_header;
#endif	_DATABASE
#ifndef	_WINDOWS
int err1 = Need_to_include_window_header;
#endif	_WINDOWS

/* global data structures */

extern char	*Path;			/* Search path */
extern char	 CellLibPath[];		/* Library search path for cells */
extern char	 SysLibPath[];		/* Library search path for tech files,
					 * etc.
					 */
extern char	*MainMouseFile;		/* The filename of the mouse */
extern char	*MainGraphicsFile;	/* The filename of the display */
extern char	*MainDisplayType;
extern char	*MainMonType;

extern FILE	*mouseStream;		/* the mouse file */
extern FILE	*graphicsStream;	/* the graphics file */

extern bool	mainDebug;		/* just the '-D' flag */

/*
 * The following information is kept about the Edit cell:
 *
 * EditCellUse		pointer to the CellUse from which the edit
 *			cell was selected.
 * EditRootDef		pointer to root def of window in which edit cell
 *			was selected.
 * EditToRootTransform	transform from coordinates of the Def of edit cell
 *			to those of EditRootDef.
 * RootToEditTransform	transform from coordinates EditRootDef to those
 *			of the Def of the edit cell.
 */

extern CellUse	*EditCellUse;
extern CellDef	*EditRootDef;
extern Transform EditToRootTransform;
extern Transform RootToEditTransform;

/* global procedures */

extern MainExit();	/* a way of exiting that cleans up after itself */
extern bool MainLoadStyles(), MainLoadCursors();  /* Used during init & reset */


/* global constants */
