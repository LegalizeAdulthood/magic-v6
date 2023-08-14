/*
 * main.c --
 *
 * The topmost module of the Magic VLSI tool.  This module
 * initializes the other modules and then calls the 'textio'
 * module to read and execute commands.
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
static char rcsid[]="$Header: main.c,v 6.3 90/09/12 15:17:15 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <sgtty.h>	
#include <sys/types.h>
#include <sys/times.h>
#include "magic.h"
#include "hash.h"
#include "textio.h"
#include "geometry.h"
#include "txcommands.h"
#include "graphics.h"
#include "tile.h"
#include "tech.h"
#include "database.h"
#include "drc.h"
#include "windows.h"
#include "dbwind.h"
#include "commands.h"
#include "signals.h"
#include "utils.h"
#include "runstats.h"
#include "cif.h"
#include "router.h"
#include "extract.h"
#include "undo.h"
#include "netmenu.h"
#include "plow.h"
#include "paths.h"
#include "wiring.h"
#include "plot.h"
#include "sim.h"
#include "doubleint.h"
#include "list.h"
#include "mzrouter.h"

#ifdef	LLNL
#include "art.h"
#endif	LLNL

/* System library routines: */

extern char *getenv();


/*
 * Global data structures
 *
 */

global char	*Path = NULL;		/* Search path */
global char	CellLibPath[100]	/* Used to find cells. */
    = MAGIC_INIT_PATH;
global char	SysLibPath[100]		/* Used to find color maps, styles, */
    = MAGIC_SYS_PATH;			/* technologies, etc. */

/*
 * Flag that tells if the '-D' (debugging) option is set.
 */

global bool mainDebug = FALSE;


/*
 * See the file main.h for a description of the information kept
 * pertaining to the edit cell.
 */

global CellUse	*EditCellUse = NULL;
global CellDef	*EditRootDef = NULL;
global Transform EditToRootTransform;
global Transform RootToEditTransform;


/*
 * data structures local to main.c
 *
 */

/* the following are used in saving our load image for fast startup */
static bool MainDoFreeze = FALSE;	/* TRUE if we should save our image */
static bool MainWasThawed = FALSE;	/* TRUE if this a restarted image */
static char *MainOrigFile;		/* File from which we were exec'd */
static char *MainSaveFile;		/* File to which image will be saved */
static char MainSaveDate[40];		/* When was image saved */

/* the filename specified on the command line */
static char *MainFileName = NULL;	

/* the filename for the graphics and mouse ports */
global char *MainGraphicsFile = NULL;
global char *MainMouseFile = NULL;

/* information about the color display. */
global char *MainDisplayType = NULL;
global char *MainMonType = NULL;


/* Copyright notice for the binary file. */
global char *MainCopyright = "\n--- MAGIC: Copyright (C) 1985, 1990 Regents of the University of California.  ---\n";


/* If TRUE, then Magic is as silent as possible.  This is only set to
 * TRUE by batch programs such as Mpack and Mocha Chip.
 */
global bool MainSilent = FALSE;

/* Forward declarations */
char *mainArg();

/*
 * ----------------------------------------------------------------------------
 * MainExit:
 *
 *	Magic's own exit procedure
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We exit.
 * ----------------------------------------------------------------------------
 */

global
MainExit(errNum)
int errNum;
{
#ifdef	MOCHA
    MochaExit(errNum);
#endif
    if (GrClosePtr != NULL) /* We are not guarenteed that everthing will
			     * be initialized already!
			     */
	GrClose();
    TxResetTerminal();
    (void) fflush(stderr);
    (void) fflush(stdout);
    exit(errNum);
}


/*
 * ----------------------------------------------------------------------------
 * mainCheckCad:
 *
 *	See if ~cad exists.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

void
mainCheckCad()
{
    char res[100];
    char *cad = "~cad";
    char *src, *dst;

    src = cad;
    dst = res;
    if (PaConvertTilde(&src, &dst, 95) == -1) {
	/* If ~cad does not exist (and/or has not been aliased to something
	 * that does exist via the CADHOME environment variable), then Magic 
	 * won't be able to find any of its system startup files.
	 * So complain and exit.
	 */
	TxError("Could not find ~cad - Magic unable to find its system startup files.\n");
	MainExit(1);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * mainDoArgs:
 *
 *	Process command line arguments
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Global variables are modified
 *
 * Notes:
 *	In order to work properly with the -F flag, we need to
 *	use StrDup() to make copies of any arguments we want
 *	to be visible when we restart a frozen file.
 *
 * ----------------------------------------------------------------------------
 */

void
mainDoArgs(argc, argv)
int argc;
char **argv;
{
    bool haveDashI = FALSE;
    char *cp;

    argc--;
    while (argc-- > 0)
    {
	argv++;
	if (**argv == '-')
	{
	    switch (argv[0][1])
	    {
		case 'g':
		    if ((cp = mainArg(&argc, &argv, "tty name")) == NULL)
			MainExit(1);
		    MainGraphicsFile = StrDup((char **) NULL, cp);
		    if (!haveDashI)
			MainMouseFile = MainGraphicsFile;
		    break;

		case 'i':
		    haveDashI = TRUE;
		    if ((cp = mainArg(&argc, &argv, "tty name")) == NULL)
			MainExit(1);
		    MainMouseFile = StrDup((char **) NULL, cp);
		    break;

		case 'd':
		    if ((cp = mainArg(&argc, &argv, "display type")) ==NULL)
			MainExit(1);
		    MainDisplayType = StrDup((char **) NULL, cp);
		    break;

		case 'm':
		    if ((cp = mainArg(&argc, &argv, "monitor type")) ==NULL)
			MainExit(1);
		    MainMonType = StrDup((char **) NULL, cp);
		    break;

		/*
		 * Indicate that we should create a "freeze"
		 * file after doing all initialization.
		 */
		case 'F':
		    if ((cp = mainArg(&argc, &argv, "executable")) == NULL)
			goto badfopt;
		    MainOrigFile = StrDup((char **) NULL, cp);
		    argc--, argv++;
		    if (argc == 0 || argv[0][0] == '-')
			goto badfopt;
		    MainSaveFile = StrDup((char **) NULL, argv[0]);
		    MainDoFreeze = TRUE;
		    break;

		/*
		 * Change the technology.
		 */
		case 'T':
		    if ((cp = mainArg(&argc, &argv, "technology")) == NULL)
			MainExit(1);
		    TechDefault = StrDup((char **) NULL, cp);
		    break;

		/*
		 * Re-enable profiling until specifically disabled.
		 * This is now handled in mainInitProfile(), so we
		 * ignore it here.
		 */
		case 'P':
		    break;

		/* 
		 * We are being debugged.
		 */
		case 'D':
		    mainDebug = TRUE;
		    break;

		default:
		    TxError("Unknown option: '%s'\n", *argv);
		    TxError("Usage:  magic [-g gPort] [-d devType] [-m monType] [-i tabletPort] [-D] [-F objFile saveFile] [-T technology] [file]\n");
		    MainExit(1);
		    
	    }
	}
	else MainFileName = StrDup((char **) NULL, *argv);
    }

    return;

badfopt:
    TxError("-F requires load and save file names, e.g.,");
    TxError(" \"-F magic magicsave\"\n");
    MainExit(1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * mainArg --
 *
 * Pull off an argument from the (argc, argv) pair and also check
 * to make sure it's not another flag (i.e, it doesn't begin with
 * a '-').
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See the comments in ArgStr() in the utils module -- they
 *	apply here.
 *
 * ----------------------------------------------------------------------------
 */

char *
mainArg(pargc, pargv, mesg)
    int *pargc;
    char ***pargv;
    char *mesg;
{
    char option, *cp;

    option = (*pargv)[0][1];
    cp = ArgStr(pargc, pargv, mesg);
    if (cp == NULL)
	return (char *) NULL;

    if (*cp == '-')
    {
	TxError("Bad name after '-%c' option: '%s'\n", option, cp);
	return (char *) NULL;
    }

    return cp;
}

/* Variables used in freezing and thawing Magic. */
static char *savedDisplayType, *savedMonType, *savedTechName;

/*
 * ----------------------------------------------------------------------------
 * mainInitBeforeArgs:
 *
 *	Initializes things before argument processing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All sorts of initialization.  Most initialization, however, is done
 *	in 'mainInitAfterArgs'.
 * ----------------------------------------------------------------------------
 */

void
mainInitBeforeArgs(argc, argv)
    int argc;
    char *argv[];
{
    if (Path == NULL)
	Path = StrDup((char **) NULL, ".");

    mainInitProfile(argc, argv);

    /* initialize text display */
    TxInit();
    TxSetTerminal();

    /* Initialize double-precision package. */
    DoubleInit();

    /* Identify version first thing */
    TxPrintf("\n%s", MagicVersion);
    if (MainWasThawed)
        TxPrintf("Image saved %s", MainSaveDate);
    TxPrintf("\n");

    /* Make sure ~cad, (or its alias) exists */
    mainCheckCad();


    if (MainWasThawed)
    {
	savedDisplayType = MainDisplayType;
	savedMonType = MainMonType;
	savedTechName = TechDefault;
    }

    /*
     * Get preliminary info on the graphics display.
     * This may be overriden later.
     */
    GrGuessDisplayType(&MainGraphicsFile, &MainMouseFile, 
	&MainDisplayType, &MainMonType);
    FindDisplay((char *)NULL, "displays", CAD_LIB_PATH, &MainGraphicsFile,
	&MainMouseFile, &MainDisplayType, &MainMonType);
}


/*
 * ----------------------------------------------------------------------------
 * mainInitAfterArgs:
 *
 *	Initializes things after argument processing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All sorts of initialization.
 * ----------------------------------------------------------------------------
 */

void
mainInitAfterArgs()
{
    char *home;
    char startupFileName[100];
    FILE *f;
    Void (*nullProc)() = 0;

    /* If no technology has been specified yet, try to read one from
     * the initial cell, or else assign a default.
     */

    if ((TechDefault == NULL) && (MainFileName != NULL))
	(void) StrDup(&TechDefault, DBGetTech(MainFileName));
    if (TechDefault == NULL)
	TechDefault = "scmos";
    (void) sprintf(CellLibPath, MAGIC_LIB_PATH, TechDefault);
    TxPrintf("Using technology \"%s\".\n", TechDefault);
    
    if (MainGraphicsFile == NULL) MainGraphicsFile = "/dev/null";
    if (MainMouseFile == NULL) MainMouseFile = MainGraphicsFile;

    /* catch signals, must come after mainDoArgs & before SigWatchFile */
    SigInit();

    /* set up graphics */
    if ( !GrSetDisplay(MainDisplayType, MainGraphicsFile, MainMouseFile) )
    {
	MainExit(1);
    }

    if (!MainWasThawed)
    {
	SectionID sec_tech, sec_planes, sec_types, sec_styles;
	SectionID sec_connect, sec_contact, sec_compose;
	SectionID sec_cifinput, sec_cifoutput;
	SectionID sec_drc, sec_extract, sec_wiring, sec_router;
	SectionID sec_plow, sec_plot, sec_mzrouter;
#ifdef	LLNL
#ifdef ART
	SectionID sec_art;
#endif ART
#endif	LLNL

	/* initialize technology */
	TechInit();
	TechAddClient("tech", DBTechInit, DBTechSetTech, nullProc,
			(SectionID) 0, &sec_tech);
	TechAddClient("planes",	DBTechInitPlane, DBTechAddPlane, nullProc,
			(SectionID) 0, &sec_planes);
	TechAddClient("types", DBTechInitType, DBTechAddType, DBTechFinalType,
			sec_planes, &sec_types);

	TechAddClient("styles", DBWTechInit, DBWTechAddStyle, nullProc,
			sec_types, &sec_styles);

	TechAddClient("contact", DBTechInitContact,
			DBTechAddContact, DBTechFinalContact,
			sec_types|sec_planes, &sec_contact);

	TechAddClient("compose", DBTechInitCompose,
			DBTechAddCompose, DBTechFinalCompose,
			sec_types|sec_planes|sec_contact, &sec_compose);

	TechAddClient("connect", DBTechInitConnect,
			DBTechAddConnect, DBTechFinalConnect,
			sec_types|sec_planes|sec_contact, &sec_connect);

#ifdef NO_CIF
	TechAddClient("cifoutput", nullProc,nullProc,nullProc,
			(SectionID) 0, &sec_cifoutput);
	
	TechAddClient("cifinput", nullProc,nullProc,nullProc,
		     (SectionID) 0, &sec_cifinput);
#else
	TechAddClient("cifoutput", CIFTechInit, CIFTechLine, CIFTechFinal,
			(SectionID) 0, &sec_cifoutput);
	
	TechAddClient("cifinput", CIFReadTechInit, CIFReadTechLine,
		    CIFReadTechFinal, (SectionID) 0, &sec_cifinput);
#endif
#ifdef NO_ROUTE
	TechAddClient("mzrouter", nullProc,nullProc,nullProc,
			sec_types|sec_planes, &sec_mzrouter);
#else
	TechAddClient("mzrouter", MZTechInit, MZTechLine, MZTechFinal,
			sec_types|sec_planes, &sec_mzrouter);
#endif
	TechAddClient("drc", DRCTechInit, DRCTechAddRule, DRCTechFinal,
			sec_types|sec_planes, &sec_drc);
#ifdef NO_PLOW
	TechAddClient("drc", nullProc,nullProc,nullProc,
			(SectionID) 0, (SectionID *) 0);
#else
	TechAddClient("drc", PlowDRCInit, PlowDRCLine, PlowDRCFinal,
			(SectionID) 0, (SectionID *) 0);
#endif
#ifdef NO_ROUTE
	TechAddClient("drc", nullProc,nullProc,nullProc,
			sec_mzrouter, (SectionID *) 0);
#else
	TechAddClient("drc", MZDRCInit, MZDRCLine, nullProc,
			sec_mzrouter, (SectionID *) 0);
#endif
#ifdef NO_EXT
	TechAddClient("extract", nullProc, nullProc,nullProc,
			sec_types|sec_connect, &sec_extract);
#else
	TechAddClient("extract", nullProc, ExtTechLine, ExtTechFinal,
			sec_types|sec_connect, &sec_extract);
#endif
	
	TechAddClient("wiring", WireTechInit, WireTechLine, WireTechFinal,
			sec_types, &sec_wiring);

#ifdef NO_ROUTE
	TechAddClient("router", nullProc,nullProc,nullProc,
			sec_types, &sec_router);
#else
	TechAddClient("router", RtrTechInit, RtrTechLine, RtrTechFinal,
			sec_types, &sec_router);
#endif
#ifdef NO_PLOW
	TechAddClient("plowing", nullProc,nullProc,nullProc,
			sec_types|sec_connect|sec_contact, &sec_plow);
#else
	TechAddClient("plowing", PlowTechInit, PlowTechLine, PlowTechFinal,
			sec_types|sec_connect|sec_contact, &sec_plow);
#endif	
#ifdef NO_PLOT
	TechAddClient("plot", nullProc,nullProc,nullProc,
			sec_types, &sec_plot);
#else
	TechAddClient("plot", PlotTechInit, PlotTechLine, PlotTechFinal,
			sec_types, &sec_plot);
#endif

#ifdef	LLNL
#ifdef ART
	TechAddClient("art", ArtTechInit, ArtTechLine, ArtTechFinal,
			sec_types|sec_planes, &sec_art);
#endif ART
#endif	LLNL
    }

    if (!MainWasThawed || strcmp(TechDefault, savedTechName) != 0)
    {
	if (!TechLoad(TechDefault))
	{
	    TxError("Cannot load technology \"%s\"\n", TechDefault);
	    MainExit(0);
	}
    }

    if (MainWasThawed && strcmp(savedDisplayType, MainDisplayType) == 0)
    {
	GrResetStyles();
    }
    else
    {
	if (GrLoadStyles(DBWStyleType, ".", SysLibPath) != 0)
		MainExit(1);
    }

    if (MainWasThawed && (strcmp(savedMonType, MainMonType) == 0)
	    && (strcmp(savedTechName, TechDefault) == 0)
	    && (strcmp(savedDisplayType, MainDisplayType) == 0))
    {
	GrResetCMap();
    }
    else
    {
	if (!GrReadCMap(DBWStyleType, (char *) NULL, MainMonType,
		".", SysLibPath))
	{
	    MainExit(1);
	}
    }
    if (!GrLoadCursors(".", SysLibPath)) MainExit(1);
    GrSetCursor(0);

    if (!MainWasThawed)
    {
	/* initialize the undo package */
	(void) UndoInit((char *) NULL, (char *) NULL);

	/* initialize windows */
	WindInit();

	/* initialize commands */
	CmdInit();

	/* Initalize the interface between windows and its clients */
	DBWinit();
	CMWinit();

	/* Initialize the circuit extractor */
#ifndef NO_EXT
	ExtInit();
#endif

	/* Initialize plowing */
#ifndef NO_PLOW
	PlowInit();
#endif
	/* Initialize selection */
	SelectInit();

	/* Initialize the wiring module */
	WireInit();

#ifndef NO_ROUTE
	/* Initialize the netlist menu */
	NMinit();
#endif

	/* Initialize the design-rule checker */
	DRCInit();

	/* Initialize the maze router */
#ifndef NO_ROUTE
	MZInit();

	/* Initialize the interactive router - 
	   NOTE the mzrouter must be initialized prior to the irouter
	   so that default parameters will be completely setup */
	IRInit();
#endif

	/* Initialize the Mocha Chip system */
#ifdef	MOCHA
	MochaInit();
#endif

	/* Initialize the Sim Module */
	SimInit();


	/* Read in system startup file, if it exists. */
	TxSetPoint(GR_CURSOR_X, GR_CURSOR_Y, WIND_UNKNOWN_WINDOW);
	f = PaOpen(MAGIC_SYS_DOT, "r", (char *) NULL, ".",
	    (char *) NULL, (char **) NULL);
	if (f != NULL) { 
	    TxDispatch(f); 
	    (void) fclose(f);
	}

	/* Freeze our load image if requested */
	if (MainDoFreeze)
	{
	    time_t saveDate;

	    time(&saveDate);
	    strcpy(MainSaveDate, ctime(&saveDate));

	    /*
	     * When the thawed image starts up, we want to
	     * see that it was thawed (hence MainWasThawed should
	     * be TRUE), and also don't want to save it again (hence
	     * MainDoFreeze should be FALSE).
	     */
	    MainDoFreeze = FALSE;
	    MainWasThawed = TRUE;

	    /*
	     * Also close any open files so they can be re-used when
	     * this image is thawed again.  This will cause the mouse
	     * to be disabled for this session (although it will be OK
	     * when Magic is re-thawed), but a :reset command will fix
	     * that.
	     */
	    GrFlush();
	    GrClose();

	    /* Save the image */
	    TxPrintf("Saving load image in file '%s' ...", MainSaveFile);
	    (void) fflush(stdout);
	    if (!SaveImage(MainOrigFile, MainSaveFile))
	    {
		TxError("\nCannot save load image.\n");
		MainExit(1);
	    }
	    TxPrintf("Done\n");

	    /* Restore the environment so we can continue normally */
	    MainWasThawed = FALSE;
	}
    }

    /*
     * Strive for a wee bit more parallelism; let the graphics
     * display run while we're reading in startup files & initial cell.
     */
    GrFlush();

    /* Read in user's startup files, if there are any. */
    home = getenv("HOME");
    if (home != NULL)
    {
	(void) sprintf(startupFileName, "%s/.magic", home);
	f = PaOpen(startupFileName, "r", (char *) NULL, ".",
	    (char *) NULL, (char **) NULL);
	if (f != NULL) {
	    TxDispatch(f); 
	    (void) fclose(f);
	}
    }
    f = PaOpen(".magic", "r", (char *) NULL, ".",
	(char *) NULL, (char **) NULL);
    if (f != NULL) {
	TxDispatch(f); 
	(void) fclose(f);
    }

    /*
     * Bring in a new cell to start up if one was given
     * on the command line
     */
    if (MainFileName)
	DBWreload(MainFileName);
    
    /* Create an initial box. */

    DBWSetBox(EditCellUse->cu_def, &EditCellUse->cu_def->cd_bbox);

    /* Set the initial fence for undo-ing:  don't want to be able to
     * undo past this point.
     */
    UndoFlush();
    TxClearPoint();
}

/*
 * ----------------------------------------------------------------------------
 * mainFinish:
 *
 *	Finish up things for Magic.  This routine is NOT called on an
 *	error exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various things, such as stopping measurement gathering.
 * ----------------------------------------------------------------------------
 */

mainFinished()
{
    /* Close up things */
#ifdef MALLOCTRACE
    mallocTraceDone();
#endif MALLOCTRACE
    MainExit(0);
}

/*---------------------------------------------------------------------------
 * magicMain:
 *
 *	Top-level procedure of the Magic Layout System.  There is purposely
 *	not much in here so that we have more flexibility.  Also, it is
 *	not called 'main' so that other programs that use Magic may do
 *	something else.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	None.
 *
 * Note:  Try not to add code to this procedure.  Add it instead to one of the
 *	procedures that it calls.
 *
 *----------------------------------------------------------------------------
 */

void
magicMain(argc, argv)
    int argc;
    char *argv[];
{

#ifdef macII
    set42sig();
#endif macII
    mainInitBeforeArgs(argc, argv);
    mainDoArgs(argc, argv);
    mainInitAfterArgs();
    TxDispatch( (FILE *) NULL);
    mainFinished();
}


/*
 * There is no 'moncontrol' call on many machines, but that is no big deal.
 * We just won't do performance monitoring.
 */

#ifdef NEED_MONCNTL
moncontrol(mode) {}
#endif NEED_MONCNTL

/*
 * ----------------------------------------------------------------------------
 *
 * mainInitProfile --
 *
 * Check for '-P' in the argument list.  If present, we override the
 * default action of Magic on startup, which is to turn off profiling
 * until explicitly enabled by the "*profile on" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May turn profiling on or off.
 *
 * ----------------------------------------------------------------------------
 */

mainInitProfile(argc, argv)
    register char **argv;
    register argc;
{
    register char *arg;

    moncontrol(0);
    for (argv++; (arg = *argv++) && --argc > 0; )
	if (arg[0] == '-' && arg[1] == 'P')
	{
	    moncontrol(1);
	    return;
	}
}
