/*
 * CmdINTERP.c --
 *
 * 	This file contains the dispatch tables for layout commands.
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
static char rcsid[] = "$Header: CmdINTERP.c,v 6.0 90/08/28 18:07:16 mayo Exp $";
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
#include "textio.h"
#include "txcommands.h"

/*
 * User commands
 */

extern Void CmdAddPath(), CmdArray();
extern Void CmdBox(), CmdCalma(), CmdCif();
extern Void CmdChannel(), CmdClockwise(), CmdCopy(), CmdCorner();
extern Void CmdDelete(), CmdDrc(), CmdDump();
extern Void CmdEdit(), CmdErase(), CmdExpand(), CmdExtract();
extern Void CmdFeedback(), CmdFill(), CmdFindBox(), CmdFlush();
extern Void CmdGaRoute(), CmdGetcell(), CmdGetnode();
extern Void CmdGrid(), CmdIdentify(), CmdIRoute();
extern Void CmdLabel(), CmdLayers(), CmdLoad();
extern Void CmdMove(), CmdPaint();
extern Void CmdPath(), CmdPlow();
extern Void CmdPlot(), CmdRoute(), CmdRsim(), CmdSave(), CmdSee();
extern Void CmdSelect(), CmdSidewalk(), CmdSideways(), CmdSimCmd();
extern Void CmdSnap(), CmdStartRsim();
extern Void CmdStretch(), CmdStraighten();
extern Void CmdTool(), CmdUnexpand();
extern Void CmdUpsidedown(), CmdWhat(), CmdWire(), CmdWriteall();

/*
 * Wizard commands
 */

extern Void CmdCoord();
extern Void CmdExtractTest();
extern Void CmdExtResis();
extern Void CmdIRouterTest();
extern Void CmdGARouterTest();
extern Void CmdGRouterTest();
extern Void CmdMZRouterTest();
extern Void CmdPsearch();
extern Void CmdPlowTest();
extern Void CmdExtResis();
extern Void CmdSeeFlags();
extern Void CmdShowtech();
extern Void CmdTilestats();
extern Void CmdTsearch();
extern Void CmdWatch();

/*
 * Commands only in the Lawrence Livermore Version 
 */
#ifdef	LLNL
extern Void CmdArt();
extern Void CmdMakeSW();
extern Void CmdSgraph();
extern Void CmdYacr();
extern Void CmdArtTest();
#endif	LLNL

global char *CmdLongCommands[] =
{
#ifdef	LLNL
    "*art [cmd [args]]		debug the area router",
#endif	LLNL
    "*coord			show coordinates of various things",
#ifndef NO_EXT
    "*extract [args]	debug the circuit extractor",
#endif
#ifndef NO_ROUTE
    "*garoute [cmd [args]]	debug the gate-array router",
    "*groute [cmd [args]]	debug the global router",
    "*iroute [cmd [args]]	debug the interactive router",
    "*mzroute [cmd [args]]	debug the maze router",
#endif
#ifndef NO_PLOW
    "*plow cmd [args]	debug plowing",
#endif
    "*psearch plane count	invoke point search over box area",
#ifndef NO_ROUTE
    "*seeflags [flag]	display channel flags over channel",
#endif
    "*showtech [file]	print internal technology tables",
    "*tilestats [file]	print statistics on tile utilization",
    "*tsearch plane count	invoke area search over box area",
    "*watch [plane]		enable verbose tile plane redisplay",
    "addpath [path]		append to current search path",
#ifdef	LLNL
    "art			routing using area router",
#endif	LLNL
    "array xsize ysize OR array xlo xhi ylo yhi\n\
			array everything in selection",
    "box [dir [amount]]	move box dist units in direction or (with\n\
			no arguments) show box size",
#ifndef NO_CALMA
    "calma option		Calma GDS-II stream file processor; type\n\
			\":calma help\" for information on options",
#endif
#ifndef NO_ROUTE
    "channels		see channels (feedback) without doing routing",
#endif
#ifndef NO_CIF
    "cif option		CIF processor; type \":cif help\"\n\
			for information on options",
#endif
    "clockwise [deg]		rotate selection and box clockwise",
    "copy [dir [amount]]	copy selection:  what used to be at box\n\
     to x y		lower-left will be copied at cursor location (or,\n\
			copy will appear amount units in dir from original);\n\
			second form will copy to location x y",
    "corner d1 d2 [layers]	make L-shaped wires inside box, filling\n\
			first in direction d1, then in d2",
    "delete			delete everything in selection",
    "drc option		design rule checker; type \":drc help\"\n\
			for information on options",
    "dump cell [child refPointC] [parent refPointP]\n\
			copy contents of cell into edit cell, so that\n\
			refPointC (or lower left) of cell is at refPointP\n\
			(or box lower-left); refPoints are either labels\n\
			or a pair of coordinates (e.g, 100 200)",
    "edit			use selected cell as new edit cell",
    "erase [layers]		erase mask information",
    "expand [toggle]		expand everything under box, or toggle\n\
			expanded/unexpanded cells in selection",
#ifndef NO_EXT
    "ext option		or",
    "extract option		circuit extractor; type \":extract help\"\n\
			for information on options",
#endif
#ifndef NO_RESIS
    "extresist [args]	patch .ext file with resistance info",
#endif
    "feedback option		find out about problems; type \":feedback help\"\n\
			for information on options",
    "fill dir [layers]	fill layers from one side of box to other",
    "findbox [zoom]		center the window on the box and optionally zoom in",
    "flush [cellname]	flush changes to cellname (edit cell default)",
#ifndef NO_ROUTE
    "garoute [cmd [args]]	gate-array router",
#endif
    "get	or",
    "getcell cell [child refPointC] [parent refPointP]\n\
			get cell as a subcell of the edit cell, so that\n\
			refPointC (or lower left) of cell is at refPointP\n\
			(or box lower-left); refPoints are either labels\n\
			or a pair of coordinates (e.g, 100 200)",
#ifndef NO_SIM
    "getnode option	get node names of all selected paint",
#endif
    "grid [xSpacing [ySpacing [xOrigin yOrigin]]]\n\
			toggle grid on/off (and set parameters)",
    "identify use_id		set the use identifier of the selected cell",
#ifndef NO_ROUTE
    "iroute [cmd [args]]	do interactive point to point route",
#endif
    "label str [pos [layer]]	place a label",
    "layers			print names of layers for this technology",
    "load [cellname]		load a cell into a window",
#ifdef	LLNL
    "makesw options		generate scan window for LP apparatus",
#endif	LLNL
    "move [dir [amount]]	move box and selection, either by amount\n\
      to x y		in dir, or pick up by box lower-left and put\n\
			down at cursor position; second form will\n\
			put box at location x y",
    "paint layers		paint mask information",
    "path [searchpath]       set or print cell search path",
#ifndef NO_PLOT
    "plot type [args]	hardcopy plotting; type \"plot help\"\n\
			for information on types and args",
#endif
#ifndef NO_PLOW
    "plow option [args]	stretching and compaction; type \":plow help\"\n\
			for information on options",
#endif
#ifndef NO_ROUTE
    "route			route the current cell",
#endif
#ifndef NO_SIM
    "rsim [options] filename		run Rsim under Magic",
#endif
    "save [filename]		save edit cell on disk",
    "see [no] layers|allSame	change what's displayed in the window",
    "select [option]		change selection; type \":select help\"\n\
			for information on options",
    "sideways		flip selection and box sideways",
#ifndef NO_SIM
    "simcmd cmd	        send a command to Rsim, applying it to selected paint",
#endif
    "snap [on|off]	cause box to snap to window's grid when moved by cursor",
#ifndef NO_PLOW
    "straighten direction	straighten jogs by pulling in direction",
#endif
#ifdef	LLNL
    "sgraph [options]	manipulate a cell's stretch graphs",
#endif	LLNL
#ifndef NO_SIM
    "startrsim [options] file    start Rsim and return to Magic",
#endif
    "stretch [dir [amount]]	stretch box and selection",
    "tool [name|info]	change layout tool or print info about what\n\
			buttons mean for current tool",
    "unexpand		unexpand subcells under box",
    "upsidedown		flip selection and box upside down",
    "what			print out information about what's selected",
    "wire option [args]	wiring-style user interface; type\n\
			\":wire help\" for information on options",
    "writeall [force]	write out all modified cells to disk",
#ifdef	LLNL
    "yacr		Channel routing using YACR2",
#endif	LLNL
   0
};

Void (*CmdFuncs[])() =
{
#ifdef	LLNL
    CmdArtTest,
#endif	LLNL
    CmdCoord,
#ifndef NO_EXT
    CmdExtractTest,
#endif
#ifndef NO_ROUTE
    CmdGARouterTest,
    CmdGRouterTest,
    CmdIRouterTest,
    CmdMZRouterTest,
#endif
#ifndef NO_PLOW
    CmdPlowTest,
#endif
    CmdPsearch,
#ifndef NO_ROUTE
    CmdSeeFlags,
#endif
    CmdShowtech,
    CmdTilestats,
    CmdTsearch,
    CmdWatch,
    CmdAddPath,
#ifdef	LLNL
    CmdArt,
#endif	LLNL
    CmdArray,
    CmdBox,
#ifndef NO_CALMA
    CmdCalma,
#endif
#ifndef NO_ROUTE
    CmdChannel,
#endif
#ifndef NO_CIF
    CmdCif,
#endif
    CmdClockwise,
    CmdCopy,
    CmdCorner,
    CmdDelete,
    CmdDrc,
    CmdDump,
    CmdEdit,
    CmdErase,
    CmdExpand,
#ifndef NO_EXT
    CmdExtract,		/* For "ext" abbreviation */
    CmdExtract,		/* For "extract" */
#endif
#ifndef NO_RESIS
    CmdExtResis,
#endif
    CmdFeedback,
    CmdFill,
    CmdFindBox,
    CmdFlush,
#ifndef NO_ROUTE
    CmdGaRoute,
#endif
    CmdGetcell,
    CmdGetcell,
#ifndef NO_SIM
    CmdGetnode,
#endif
    CmdGrid,
    CmdIdentify,
#ifndef NO_ROUTE
    CmdIRoute,
#endif
    CmdLabel,
    CmdLayers,
    CmdLoad,
#ifdef	LLNL
    CmdMakeSW,
#endif	LLNL
    CmdMove,
    CmdPaint,
    CmdPath,
#ifndef NO_PLOT
    CmdPlot,
#endif
#ifndef NO_PLOW
    CmdPlow,
#endif
#ifndef NO_ROUTE
    CmdRoute,
#endif
#ifndef NO_SIM
    CmdRsim,
#endif
    CmdSave,
    CmdSee,
    CmdSelect,
    CmdSideways,
#ifndef NO_SIM
    CmdSimCmd,
#endif
    CmdSnap,
#ifndef NO_PLOW
    CmdStraighten,
#endif
#ifdef	LLNL
    CmdSgraph,
#endif	LLNL
#ifndef NO_SIM
    CmdStartRsim,
#endif
    CmdStretch,
    CmdTool,
    CmdUnexpand,
    CmdUpsidedown,
    CmdWhat,
    CmdWire,
    CmdWriteall,
#ifdef	LLNL
    CmdYacr,
#endif	LLNL
};
