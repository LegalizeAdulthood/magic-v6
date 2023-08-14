/* windCmdAM.c -
 *
 *	This file contains Magic command routines for those commands
 *	that are valid in all windows.
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
static char rcsid[]="$Header: windCmdAM.c,v 6.0 90/08/28 19:02:12 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <errno.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "windows.h"
#include "malloc.h"
#include "runstats.h"
#include "macros.h"
#include "signals.h"
#include "graphics.h"
#include "styles.h"
#include "txcommands.h"
#include "glyphs.h"
#include "windInt.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "dbwind.h"
#include "utils.h"


/*
 * ----------------------------------------------------------------------------
 *
 * windCenterCmd --
 *
 * Implement the "center" command.
 * Move a window's view to center the point underneath the cursor.
 *
 * Usage:
 *	center 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in the window underneath the cursor is changed
 *	to center the point underneath the cursor.  
 *
 * ----------------------------------------------------------------------------
 */

windCenterCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    Point rootPoint;
    Rect newArea, oldArea;

    if (w == NULL)
    {
	TxError("Point to a window first.\n");
	return;
    };

    if (cmd->tx_argc == 1)
    {
	if ((w->w_flags & WIND_SCROLLABLE) == 0) {
	    TxError("Sorry, can't scroll this window.\n");
	    return;
	};
	WindPointToSurface(w, &cmd->tx_p, &rootPoint, (Rect *) NULL);
    }
    else
    {
	TxError("Usage: center\n");
	return;
    }

    oldArea = w->w_surfaceArea;
    newArea.r_xbot = rootPoint.p_x - (oldArea.r_xtop - oldArea.r_xbot)/2;
    newArea.r_xtop = newArea.r_xbot - oldArea.r_xbot + oldArea.r_xtop;
    newArea.r_ybot = rootPoint.p_y - (oldArea.r_ytop - oldArea.r_ybot)/2;
    newArea.r_ytop = newArea.r_ybot - oldArea.r_ybot + oldArea.r_ytop;

    WindMove(w, &newArea);
}


/*
 * ----------------------------------------------------------------------------
 * windCloseCmd --
 *
 *	Close the window that is pointed at.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is closed, and the client is notified.  The client may
 *	refuse to have the window closed, in which case nothing happens.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windCloseCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (w == (Window *) NULL)
    {
	TxError("Point to a window first\n");
	return;
    }
    if (!WindDelete(w))
    {
	TxError("Unable to close that window\n");
	return;
    }
}


/*
 * ----------------------------------------------------------------------------
 * windCrashCmd --
 *
 *	Generate a core dump.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dumps core by calling niceabort().
 *
 * ----------------------------------------------------------------------------
 */

    /*ARGSUSED*/
windCrashCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1)
    {
	TxError("Usage:  *crash\n");
	return;
    }

    TxPrintf("OK -- crashing...\n");
    TxFlush();
    niceabort();
}

/*
 * ----------------------------------------------------------------------------
 * windDebugCmd --
 *
 *	Change to a new debugging mode.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windDebugCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (cmd->tx_argc != 1) goto usage;
    windPrintCommands = !windPrintCommands;
    TxError("Window command debugging set to %s\n", 
	(windPrintCommands ? "TRUE" : "FALSE"));
    return;

usage:
    TxError("Usage:  *winddebug\n");
}

/*
 * ----------------------------------------------------------------------------
 * windDumpCmd --
 *
 *	Dump out debugging info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windDumpCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    (void) windDump();
}


/*
 * ----------------------------------------------------------------------------
 *
 * windEchoCmd --
 *
 *	Echo the arguments
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Text may appear on the terminal
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windEchoCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    int i;
    bool newline = TRUE;

    for (i = 1; i < cmd->tx_argc; i++)
    {
	if (i != 1)
	    TxPrintf(" ");
	if ( (i == 1) && (strcmp(cmd->tx_argv[i], "-n") == 0) )
	   newline = FALSE;
	else
	    TxPrintf("%s", cmd->tx_argv[i]);
    }
    
    if (newline)
	TxPrintf("\n");
    TxFlush();
}


/*
 * ----------------------------------------------------------------------------
 *
 * windFilesCmd --
 *
 *	Find out what files are currently open.
 *
 * Usage:
 *	*files
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windFilesCmd()
{
#define NUM_FD	20	/* max number of open files per process */
    extern int errno, sys_nerr;
    extern char *sys_errlist[];
    int fd;
    struct stat buf;
    int unopen, open;

    open = unopen = 0;
    for (fd = 0; fd < NUM_FD; fd++) {
	if (fstat(fd, &buf) != 0) {
	    if (errno == EBADF) {
		unopen++;
	    } else {
		if (errno < sys_nerr)
		    TxError("file descriptor %d: %s\n", fd, sys_errlist[errno]);
		else
		    TxError("file descriptor %d: unknown error\n", fd);
	    }
	}
	else {
	    char *type;
	    switch (buf.st_mode & S_IFMT) {
		case S_IFDIR: {type = "directory"; break;}
		case S_IFCHR: {type = "character special"; break;}
		case S_IFBLK: {type = "block special"; break;}
		case S_IFREG: {type = "regular"; break;}
		case S_IFLNK: {type = "symbolic link"; break;}
		case S_IFSOCK: {type = "socket"; break;}
		default: {type = "unknown"; break;}
	    }
	    TxError("file descriptor %d: open  (type: '%s', inode number %ld)\n", 
		fd, type, buf.st_ino);
	    open++;
	}
    }
    TxError("%d open files, %d unopened file descriptors left\n", open, unopen);
}

/*
 * ----------------------------------------------------------------------------
 *
 * windGrowCmd --
 *
 *	Grow a window to full-screen size or back to previous size.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Text may appear on the terminal
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windGrowCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    if (w == NULL)
    {
	TxError("Point to a window first.\n");
	return;
    };

    WindFullScreen(w);
}


/*
 * ----------------------------------------------------------------------------
 * windGrstatsCmd --
 *
 *	Take statistics on the graphics code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windGrstatsCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    char *RunStats(), *rstatp;
    static struct tms tlast, tdelta;
    int i, style, count;
    int us;
    extern int GrNumClipBoxes;
    int usPerRect, rectsPerSec;

    if (cmd->tx_argc < 2 || cmd->tx_argc > 3)
    {
	TxError("Usage: grstats num [ style ]\n");
	return;
    }

    if (!StrIsInt(cmd->tx_argv[1]) || 
	(cmd->tx_argc == 3 && !StrIsInt(cmd->tx_argv[2])))
    {
	TxError("Count & style must be numeric\n");
	return;
    }
    if (w == (Window *) NULL)
    {
	TxError("Point to a window first.\n");
	return;
    }

    count = atoi(cmd->tx_argv[1]);
    if (cmd->tx_argc == 3)
	style = atoi(cmd->tx_argv[2]);
    else
	style = -1;

    WindUpdate();

    if (style >= 0)
	GrLock(w, TRUE);

    (void) RunStats(RS_TINCR, &tlast, &tdelta);
    GrNumClipBoxes = 0;
    for (i = 0; i < count; i++)
    {
	if (SigInterruptPending)
	    break;
	if (style < 0)
	{
	    WindAreaChanged(w, (Rect *) NULL);
	    WindUpdate();
	}
	else
	{
	    Rect r;
#define GRSIZE	15
#define GRSPACE 20
	    r.r_xbot = w->w_screenArea.r_xbot -  GRSIZE/2;
	    r.r_ybot = w->w_screenArea.r_ybot -  GRSIZE/2;
	    r.r_xtop = r.r_xbot + GRSIZE - 1;
	    r.r_ytop = r.r_ybot + GRSIZE - 1;
	    GrClipBox(&w->w_screenArea, STYLE_ERASEALL);
	    GrSetStuff(style);
	    while (r.r_xbot <= w->w_screenArea.r_xtop)
	    {
		while (r.r_ybot <= w->w_screenArea.r_ytop)
		{
		    GrFastBox(&r);
		    r.r_ybot += GRSPACE;
		    r.r_ytop += GRSPACE;
		}
		r.r_xbot += GRSPACE;
		r.r_xtop += GRSPACE;
		r.r_ybot = w->w_screenArea.r_ybot -  GRSIZE/2;
		r.r_ytop = r.r_ybot + GRSIZE - 1;
	    }
	}
    }
    rstatp = RunStats(RS_TINCR, &tlast, &tdelta);

    us = tdelta.tms_utime * (1000000 / 60);
    usPerRect = us / MAX(1, GrNumClipBoxes);
    rectsPerSec = 1000000 / MAX(1, usPerRect);
    TxPrintf("[%s]\n%d rectangles, %d uS, %d uS/rectangle, %d rects/sec\n", 
	rstatp, GrNumClipBoxes, us, usPerRect, rectsPerSec);

    if (style >= 0)
	GrUnlock(w);
}


/*
 * ----------------------------------------------------------------------------
 * windHelpCmd --
 *
 *	Just a dummy proc.  (Only for this particular, global, client)
 *	This is just here so that there is an entry in our help table!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windHelpCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    ASSERT(FALSE, windHelpCmd);
}


static char *logKeywords[] =
    {
	"update",
	0
    };

/*
 * ----------------------------------------------------------------------------
 * windLogCommandsCmd --
 *
 *	Log the commands and button pushes in a file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windLogCommandsCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    char *fileName;
    bool update;

    if ((cmd->tx_argc < 1) || (cmd->tx_argc > 3)) goto usage;

    update = FALSE;

    if (cmd->tx_argc == 1) 
	fileName = NULL;
    else
	fileName = cmd->tx_argv[1];

    if (cmd->tx_argc == 3) {
	int i;
	i = Lookup(cmd->tx_argv[cmd->tx_argc - 1], logKeywords);
	if (i != 0) goto usage;
	update = TRUE;
    }

    TxLogCommands(fileName, update);
    return;

usage:
    TxError("Usage: %s [filename [update]]\n", cmd->tx_argv[0]);
}


/*
 * ----------------------------------------------------------------------------
 *
 * windMacroCmd --
 *
 *	Define a new macro.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes the macro package to define a new macro.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windMacroCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{

    if (cmd->tx_argc == 1)
    {
	char *cp;
	char ch;
	bool any = FALSE;

	for (ch = 0; ch < 127; ch++)
	{
	    cp = MacroRetrieve(ch);
	    if (cp != NULL)
	    {
		char str[9];

		TxVisChar(str, ch);
		TxPrintf("Macro '%s' contains \"%s\"\n", str, cp);
		freeMagic(cp);
		any = TRUE;
	    }
	}

	if (!any)
	    TxPrintf("No macros are defined.\n");
    }
    else if ( (strlen(cmd->tx_argv[1]) != 1) || (cmd->tx_argc > 3) )
    {
       TxError("Usage: %s [char [string]]\n", cmd->tx_argv[0]);
       return;
    }
    else if (cmd->tx_argc == 2) 		/* print contents of a macro */
    {
	char *cp;
	char str[9];

	cp = MacroRetrieve(cmd->tx_argv[1][0]);
	TxVisChar(str, cmd->tx_argv[1][0]);
	if (cp != NULL)
	{
	    TxPrintf("Macro '%s' contains \"%s\"\n", str, cp);
	    freeMagic(cp);
	}
	else
	    TxPrintf("Macro '%s' is empty\n", str);
    }
    else if (cmd->tx_argc == 3)		/* delete or define a macro */
    {
	if (cmd->tx_argv[2][0] == '\0')
	    MacroDelete(cmd->tx_argv[1][0]);
	else
	    MacroDefine(cmd->tx_argv[1][0], cmd->tx_argv[2]);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * windMallocCmd --
 *
 * Interface for debugging the memory allocator mallocMagic/freeMagic.
 * Can enable/disable tracing, or cause the memory allocator to allocate
 * (and then drop on the floor) a chunk of size bytes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Can allocate memory irreversibly, or create a log file for
 *	tracing.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

windMallocCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    char *p, *addr;
    int n;
    typedef enum { ON, OFF, WATCH, UNWATCH, ONLY, ALL } cmdType;
    static struct
    {
	char *cmd_name;
	cmdType cmd_val;
    } cmds[] = {
	"all",		ALL,
	"off",		OFF,
	"on",		ON,
	"only",		ONLY,
	"watch",	WATCH,
	"unwatch",	UNWATCH,
	0
    };

    if (cmd->tx_argc < 2 || cmd->tx_argc > 3)
    {
	TxError("Usasge: *malloc cmd [arg]\n");
	return;
    }

    if (StrIsInt(cmd->tx_argv[1]))
    {
	n = atoi(cmd->tx_argv[1]);
	if (n < 0)
	{
	    TxError("Number of bytes must be >= 0\n");
	    return;
	}

	p = mallocMagic((unsigned) n);
	TxPrintf("Malloc(%d) returned 0x%x\n", n, p);
	if (!mallocSanity())
	{
	    TxError("ERROR:  mallocVerify() returned FALSE!\n");
	}
    }
    else
    {
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
	    case ON:
		mallocTraceInit(cmd->tx_argc > 2
				? cmd->tx_argv[2]
				: "malloc.out");
		break;
	    case OFF:
		mallocTraceDone();
		break;
	    case ALL:
		mallocTraceOnlyWatch(FALSE);
		break;
	    case ONLY:
		mallocTraceOnlyWatch(TRUE);
		break;
	    case WATCH:
		if (cmd->tx_argc < 3)
		{
		    TxError("Usage: *malloc watch address\n");
		    return;
		}
		(void) sscanf(cmd->tx_argv[2], "%x", &addr);
		(void) mallocTraceWatch(addr);
		break;
	    case UNWATCH:
		if (cmd->tx_argc < 3)
		{
		    TxError("Usage: *malloc unwatch address\n");
		    return;
		}
		(void) sscanf(cmd->tx_argv[2], "%x", &addr);
		mallocTraceUnWatch(addr);
		break;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * windMemstatsCmd --
 *
 * Show memory utilization statistics as gathered by mallocMagic.
 * If an argument is given, output is appended to the file of
 * that name.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

#define	NSIZES	BYTETOWORD(BIGOBJECT)

windMemstatsCmd(w, cmd)
    Window *w;
    TxCommand *cmd;
{
    unsigned nCum[NSIZES], nCur[NSIZES], sCum, sCur;
    unsigned sCumSlow, sCurSlow;
    unsigned wCum, wCur;
    FILE *f = stdout;
    int n;

    if (cmd->tx_argc > 1)
	if ((f = fopen(cmd->tx_argv[1], "a")) == (FILE *) NULL)
	{
	    perror(cmd->tx_argv[1]);
	    return;
	}

    mallocSizeStats(nCum, nCur, &sCum, &sCur);
    sCumSlow = sCum;
    sCurSlow = sCur;
    (void) fprintf(f, "Size Statistics\n");
    (void) fprintf(f, "#w\t# cum\t# now\t#w cum\t#w now\n");
    for (n = 1; n < NSIZES; n++)
    {
	(void) fprintf(f, "%d\t%d\t%d\t", n, nCum[n], nCur[n]);
	wCum = n * nCum[n];
	wCur = n * nCur[n];
	(void) fprintf(f, "%d\t%d\n", wCum, wCur);
	sCumSlow -= wCum;
	sCurSlow -= wCur;
    }
    (void) fprintf(f, "slow\t%d\t%d\t%d\t%d\n", nCum[0], nCur[0], 
	    sCumSlow, sCurSlow);
    (void) fprintf(f, "\nPage Statistics\n");
    mallocPageStats(f);
    (void) fprintf(f, "\n");

    if (f != stdout)
	(void) fclose(f);
}

