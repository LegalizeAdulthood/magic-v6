/*
 * commands.h --
 *
 * Definitions for the commands module.
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
 * Needs to include: tiles.h, database.h
 *
 * rcsid $Header: commands.h,v 6.0 90/08/28 18:07:33 mayo Exp $
 */

#define	_COMMANDS

#ifndef	_WINDOWS
int err0 = Need_to_include_window_header;
#endif	_WINDOWS

#ifndef	_DATABASE
int err2 = Need_to_include_database_header;
#endif	_DATABASE

/*
 * Name of default yank buffer
 */

#define YANKBUFFERNAME	"y"

/*
 * The table of commands that we will accept
 *
 */

extern char *CmdLongCommands[];
extern Void (*CmdFuncs[])();

/*
 * Manipulation of user-supplied "layer" masks.
 * These may include both layers specifiable in a TileTypeMask,
 * and pseudo-layers such as "subcells" and "labels".
 *
 * These are treated just like other TileTypes, except they
 * reside in the uppermost TT_RESERVEDTYPES tile type numbers.
 */

#define	L_CELL	(TT_MAXTYPES-1)	/* Subcell layer */
#define	L_LABEL	(TT_MAXTYPES-2)	/* Label "layer" */

extern TileTypeBitMask CmdYMCell;
extern TileTypeBitMask CmdYMLabel;
extern TileTypeBitMask CmdYMAllButSpace;

/* --------------------- Global procedure headers --------------------- */

extern Window *CmdGetRootBox();
extern Window *CmdGetEditPoint();
extern Window *CmdGetRootPoint();
extern int CmdWarnWrite();
extern bool CmdParseLayers();
extern void CmdAddSlop();
extern void CmdLabelProc();
extern void CmdSetWindCaption();
extern CellUse *CmdGetSelectedCell();
extern bool CmdIllegalChars();
