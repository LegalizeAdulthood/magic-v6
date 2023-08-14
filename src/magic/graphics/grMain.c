/* grMain.c -
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
 * This file contains a few core variables and routines for
 * manipulating color graphics displays.  Its main function is
 * to provide a central dispatch point to various routines for
 * different display types.
 */

#ifndef lint
static char rcsid[]="$Header: grMain.c,v 6.2 90/09/13 12:09:26 mayo Exp $";
#endif  not lint

/*
 * The following display types are currently suported by Magic:
 *
 *	UCB512		An old AED512 with the Berkeley microcode roms,
 *			with attached bitpad (SummaGraphics Bitpad-One).
 *
 *	UCB512N		A new AED512 with the Berkeley microcode roms,
 *			with attached SummaGraphics Bitpad-one.
 *
 *    	AED767         	An AED767 with a SummaGraphics Bit Pad One.
 *                    	Because of a lack of features in this device,
 *                    	programable cursors and BitBlt do not work.
 *                    	Many thanks to Norm Jouppi and DECWRL for doing
 *                    	this port.
 *
 *	AED1024		An AED1024 with a SummaGraphics Mouse and rev. D roms.
 *			Because of a lack of features in this device,
 *			programable cursors do not work.  Many thanks to 
 *			Peng Ang and LSI Logic Corp for doing this port.
 *
 *	UCB1024		An AED1024 with SummaGraphics Mouse and the special
 *			AED ROMs that provide the same features as the
 *			UCB512 ROMs.
 *
 *	NULL		A null device for running Magic without using
 *			a graphics display.  This device does nothing
 *			when its routines are called.
 *
 *	SUN120		A Sun Microsystems workstation, model Sun2/120 with
 *			a separate colorboard (/dev/cgone0) and the
 *			Sun optical mouse.  Also works on some old Sun1s with
 *			the 'Sun2 brain transplant'.
 *
 *	SUNBW		A black & white Sun, such as a Sun2.
 *			Because this device only has one bit-plane, Magic will
 *			leave little white spots on the screen after erasing
 *			the box or highlight areas.
 *
 *	SUN160		A Sun with one screen -- in color.
 *
 *	SUN110		A Sun 3/110 with a single color screen, but with a
 *			brain-damaged pixrect library.  The driver for this
 *			is almost the same as the SUN160 driver except for
 *			a couple of critical performance hacks.
 *
 *	SUN60		A Sun 3/60 with a single color screen.  Exactly the same
 *			driver as the SUN100 driver.
 *
 *	X10a		A port to the X10 window system, done by Doug Pan and
 *			Prof. Mark Linton at Stanford.  Modifications of this
 *			were done at Lawrence Livermore National Labs.
 *
 *	X11		A port to the X11 window system, based on the Stanford
 *	XWIND		X10 driver, mods done at Brown University, an X11 port
 *			done at the University of Washington, and the X10a
 *			driver from Lawrence Livermore Labs.  This driver was
 *			developed by Don Stark (Stanford & decwrl).
 *
 * To port Magic to another type of display, you need to add its name to
 * the table 'grDisplayTypes' and then add a pointer to an initialization
 * routine to 'grInitProcs'.  The initialization routine will fill in all
 * of the graphics routine pointers so that they point to procedures that
 * can handle the new display type.  All calls to device-specific 
 * procedures are made by indirecting through these pointers.
 */

#include <stdio.h>
#include <sgtty.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>
#include "magic.h"
#include "textio.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"

#ifdef	sun
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#endif	sun

#ifndef	NO_VARARGS
#include <varargs.h>
#endif	NO_VARARGS

extern char *getenv();
extern int errno;

#define FAVORITE_DISPLAY	"NULL"	/* Default display type */

/* The following rectangle is describes the display area and is available
 * to the user of this module.
 */
global Rect GrScreenRect = {0, 0, 0, 0};

/* The first of the following tables defines the legal
 * display types and the second table defines an
 * initialization routine for each type.
 *
 * These entries MUST be all upper case, since what the user types will
 * be converted to upper case before comparison.
 */

static char *grDisplayTypes[] = {
#ifdef 	SUNVIEW
    "SUN60",
    "SUN110",
    "SUN160",
    "SUNBW",
#endif	SUNVIEW
#ifdef	SUN120
    "SUN120",
#endif	SUN120
#ifdef	AED
    "UCB512",
    "UCB512N",
    "AED767",
    "AED1024",
    "UCB1024",
#endif	AED
#ifdef	X10
    "XWIND",
    "X10",
#endif	X10
#ifdef	X11
    "XWIND",
    "X11", 	
#endif
    "NULL",
    NULL};

extern aedSetDisplay();
extern sunSetDisplay();
extern sunWSetDisplay();
extern x10aSetDisplay();
extern x11suSetDisplay();
extern nullSetDisplay();

static (*(grInitProcs[]))() = {
#ifdef 	SUNVIEW
    sunWSetDisplay,
    sunWSetDisplay,
    sunWSetDisplay,
    sunWSetDisplay,
#endif	SUNVIEW
#ifdef	SUN120
    sunSetDisplay,
#endif	SUN120
#ifdef	AED
    aedSetDisplay,	/* Handles all AEDs (UCB512s, AED767s, and AED1024s) */
    aedSetDisplay,
    aedSetDisplay,
    aedSetDisplay,
    aedSetDisplay,
#endif	AED
#ifdef	X10
    x10aSetDisplay,  
    x10aSetDisplay,  
#endif	X10
#ifdef	X11
    x11suSetDisplay,  
    x11suSetDisplay,  
#endif	X11
    nullSetDisplay,
    NULL};

/* The following variables are pointers to the various graphics
 * procedures.  The macros in graphics.h cause these pointers
 * to be indirected through when calls occur to graphics procedures.
 * This indirection allows for several display types to be supported
 * by a single version of Magic.  The pointers are initially NULL,
 * but are rewritten by the various graphics initializers.
 */

Void (*GrLockPtr)()		= NULL;
Void (*GrUnlockPtr)()		= NULL;
Void (*GrInitPtr)()		= NULL;
Void (*GrClosePtr)()		= NULL;
Void (*GrSetCMapPtr)()		= NULL;

Void (*GrEnableTabletPtr)()	= NULL;
Void (*GrDisableTabletPtr)()	= NULL;
Void (*GrSetCursorPtr)()	= NULL;
Void (*GrTextSizePtr)()		= NULL;
Void (*GrDrawGlyphPtr)()	= NULL;
Void (*GrBitBltPtr)()		= NULL;
int  (*GrReadPixelPtr)()	= NULL;
Void (*GrFlushPtr)()		= NULL;
Void (*GrCreateWindowPtr)()	= NULL;
Void (*GrDeleteWindowPtr)()	= NULL;
Void (*GrConfigureWindowPtr)()	= NULL;
Void (*GrOverWindowPtr)()	= NULL;
Void (*GrUnderWindowPtr)()	= NULL;
Void (*GrDamagedPtr)()		= NULL;
Void (*GrUpdateIconPtr)()   	= NULL;


/* variables similar to the above, except that they are only used
 * internal to the graphics package
 */
Void (*grPutTextPtr)()		= NULL;
int (*grGetCharSizePtr)()	= NULL;
Void (*grSetSPatternPtr)()	= NULL;
Void (*grDefineCursorPtr)()	= NULL;
bool (*grDrawGridPtr)()		= NULL;
Void (*grDrawLinePtr)()		= NULL;
Void (*grSetWMandCPtr)()	= NULL;
Void (*grFillRectPtr)()		= NULL;
Void (*grSetStipplePtr)()	= NULL;
Void (*grSetLineStylePtr)()	= NULL;
Void (*grSetCharSizePtr)()	= NULL;

/* The following variables are set by initialization routines for the
 * various displays.  They are strings that indicate what kind of
 * dstyle, cmap and cursor files should be used for this display.  Almost
 * all of the displays are happy with the default values given below.
 * Note:  a NULL grCMapType means that this display doesn't need a
 * color map (it's black-and-white).
 */

char *grDStyleType = "7bit";
char *grCMapType = "7bit";
char *grCursorType = "bw";

/* The following variable is a device-dependent limit on how many
 * stipples are permitted.  It defaults to something, but may be reset
 * by device initializers to a different number, depending
 * on what the devices can actually support.  Must not be greater
 * than GR_NUM_STIPPLES.
 */

int grMaxStipples = GR_NUM_STIPPLES;

/* Procedure to print text on stdout and stderr.  It is located here so that
 * graphics drivers and do something special with it if they want.
 * This routine should only be called by the textio module.
 */
#ifdef	NO_VARARGS
    extern Void fprintf();
    Void (*GrFprintfPtr)() = fprintf;
#else
    extern int vfprintf();
    Void (*GrVfprintfPtr)() = vfprintf;
#endif	NO_VARARGS

/* Procedure to change the default terminal state. It gets passed 2 pointers,
 * one to the terminal input mode structure, and one to the terminal literal
 * characters structure.  See textio module for details.
 */
Void (*GrTermStatePtr)() = NULL;


/* Procedures called just before and after Magic is suspended (via ^Z). */
extern Void grNullProc();
Void (*GrStopPtr)() = grNullProc;
Void (*GrResumePtr)() = grNullProc;


/*---------------------------------------------------------
 * GrSetDisplay --
 *	This routine sets a display type, opens files,  and initializes the
 *	display.
 *
 * Results:
 *	TRUE is returned if the display was found and initialized
 *	successfully.  If the type didn't register, or the file is 
 *	NULL, then FALSE is returned.
 *
 * Side Effects:
 *	Tables are set up to control which display routines are
 *	used when communcating with the display.  The display
 *	is initialized and made ready for action.
 *---------------------------------------------------------
 */

bool
GrSetDisplay(type, outName, mouseName)
char *type;			/* Name of the display type. */
char *outName;			/* Filename used for communciation with 
				 * display. */
char *mouseName;		/* Filename used for communciation 
				 * with tablet. */

{
    char **ptr;
    char *cp;
    int i;
    bool res;

    if (outName == NULL) 
    {
	TxError("No graphics device specified.\n");
	return FALSE;
    }
    if (mouseName == NULL)
    {
	TxError("No mouse specified.\n");
	return FALSE;
    }

    /* Convert display type to upper case. */
    for (cp = type; *cp; cp++) { if (islower(*cp)) *cp = toupper(*cp); }

    /* See if the display type is in our table. */
    ptr = grDisplayTypes;
    for (i=0; *ptr; i++)
    {
	if (strncmp(*ptr, type,strlen(*ptr)) == 0) break;
	ptr++;
    }

    /* Did we find it? */
    if (*ptr == NULL)
    {
	TxError("Unknown display type:  %s\n", type);
 	TxError("These display types are available in this version of Magic:\n");
	ptr = grDisplayTypes;
	for (i = 0; *ptr; i++)
	{
	    TxError("        %s\n", *ptr);
	    ptr++;
	}
	TxError("Use '-d NULL' if you don't need graphics.\n");
	return FALSE;
    }

    /* Call the initialization procedure. */
    res = (*(grInitProcs[i]))(type, outName, mouseName);
    if (!res) 
    {
	TxError("The graphics display couldn't be correctly initialized.\n");
	TxError("Use '-d NULL' if you don't need graphics.\n");
    }
    return res;
}


/*
 * ----------------------------------------------------------------------------
 *
 * grSunFbDepth --
 *
 *	Find the depth of the Sun's frame buffer.
 *
 * Results:
 *	An integer describing the depth of the display.  Returns 0 if there
 *	were any problems (such as not being on a sun).
 *
 * Side Effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

grSunFbDepth(device)
    char *device;		/* The device (screen) to check on. */
{
#ifdef	SUNVIEW
    struct pixrect *pr;
    int depth;

    ASSERT(device != NULL, "grSunFbDepth");
    if (access(device, O_RDWR) < 0)
    {
	TxError("Error opening '%s', error #%d\n", device, errno);
	MainExit(2);
    }

    pr = pr_open(device);
    if (pr == NULL)
    {
	TxError("Can't create pixrect for device %s\n", device);
	MainExit(3);
    }

    depth = pr->pr_depth;
    pr_close(pr);
    return depth;
#else	SUNVIEW
    return 0;
#endif	SUNVIEW
}


/*
 * ----------------------------------------------------------------------------
 * GrGuessDisplayType --
 *
 *	Try to guess what sort of machine we are on, and set the display
 *	ports and type appropriately.  This info is overrided by
 *	~cad/lib/displays and by command line switches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the strings passed in.
 * ----------------------------------------------------------------------------
 */
void
GrGuessDisplayType(graphics, mouse, display, monitor)
    char **graphics;		/* default device for sending out graphics */
    char **mouse;		/* default device for reading mouse (tablet) */
    char **display;		/* default type of device (AED, etc...) */
    char **monitor;		/* default type of monitor (pale, std) */
{
    bool onSun;			/* Are we on a Sun? */
    bool have2sunScreens;	/* do we have a color Sun board? */
    bool haveX;			/* are we running under X? */
    bool haveSuntools;		/* is Suntools running? */

    *graphics = NULL;
    *mouse = NULL;
    *display = NULL;
    *monitor = "std";

    /* Check for signs of suntools. */
    onSun = (access("/dev/win0", 0) == 0);
    haveSuntools = (getenv("WINDOW_PARENT") != NULL);
    haveX = (getenv("DISPLAY") != NULL && getenv("WINDOWID") != NULL);
    have2sunScreens = FALSE;

#ifdef	sun
    {
	/* See if an auxiliary Sun color screen is alive and well. */
	int fd;
	fd = open("/dev/cgone0", O_RDWR);
	if (fd > -1)
	{
	    close(fd);
	    have2sunScreens = TRUE;
	}
    }
#endif	sun

    if (haveX)
    {
	*mouse = *graphics = NULL;
	*display = "XWIND";
    }
    else if (onSun && !haveSuntools) {
	TxError("You are on a Sun but not running suntools or X.\n");
	*mouse = *graphics = NULL;
	*display = "NULL";
    }
    else if (onSun && have2sunScreens) {
	/* GUESS:  probably a SUN120 with color board */
	*mouse = *graphics = "/dev/cgone0";
	*display = "SUN120";
    } 
    else if (onSun && (grSunFbDepth("/dev/fb") >= 8)) {
	/* GUESS:  either a SUN160 or a SUN110 (main screen is color) */

	*mouse = *graphics = "/dev/fb";
	*monitor = "std";
	*display = "SUN160";

#ifdef	sun	/* VAXes don't know about FBIOGATTR */

	{
	    struct fbgattr attr;
	    int fd;
	    /* The default is SUN160, but see if it's really a SUN110 */
	    if ((fd = open(*graphics, O_RDWR)) >= 0)
	    {
	        if (ioctl(fd, FBIOGATTR, &attr) != -1
		    && attr.real_type == FBTYPE_SUN4COLOR) *display = "SUN110";
	        (void) close(fd);
	    }
	}
#endif	sun
    } 
    else if (onSun) {
	/* CATCH ALL FOR SUNs: probably a black & white sun */
	*mouse = *graphics = "/dev/fb";
	*display = "SUNBW";
    } 
    else {
	/* GUESS:  who knows, maybe a VAX? */
	*mouse = *graphics = NULL;
	*display = FAVORITE_DISPLAY;
    }
}


/*
 * ----------------------------------------------------------------------------
 * grFgets --
 *
 *	Just like fgets, except that it times out after 20 seconds, and prints
 *	a warning message.  After one second a warning message is also 
 *	printed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */

char *
grFgets(str, n, stream, name)
    char *str;
    int n;
    FILE *stream;
    char *name;		/* The user name of the stream, for the error msg */
{
    int fn;
    char *newstr;
    struct timeval threeSec, twentySecs;
    extern int errno;

    threeSec.tv_sec = 3;	
    threeSec.tv_usec = 0;
    twentySecs.tv_sec = 20;	
    twentySecs.tv_usec = 0;

    fn = 1 << fileno(stream);
    newstr = str;
    n--;
    if (n < 0) return (char *) NULL;

    while (n > 0)
    {
	int f;
	char ch;
        int sel;

	f = fn;
	sel = select(20, &f, (int *) NULL, (int *) NULL, &threeSec);
	if  (sel == 0)
	{
	    TxError("The %s is responding slowly, or not at all.\n", name);
	    TxError("I'll wait for 20 seconds and then give up.\n");
	    TxError("On AEDs, typing return on the AED keyboard may help.\n");
	    f = fn;
	    sel = select(20, &f, (int *) NULL, (int *) NULL, &twentySecs);
	    if (sel == 0)
	    {
		TxError("The %s did not respond.\n", name);
		return (char *) NULL;
	    }
	    else if (sel < 0)
	    {
		if (errno == EINTR) {
		    TxError("Timeout aborted.\n");
		}
		else
		{
		    perror("magic");
		    TxError("Error in reading the %s\n", name);
		}
		return (char *) NULL;
	    }
	    else
		TxError("The %s finally responded.\n", name);
	}
	else if (sel < 0)
	{
	    if (errno != EINTR)
	    {
		perror("magic");
		TxError("Error in reading the %s\n", name);
		return (char *) NULL;
	    }
	    /* else try again, back to top of the loop */
	    continue;
	}

	ch = getc(stream);
	*newstr = ch;
	n--;
	newstr++;
	if (ch == '\n')
	    break;
    }

    *newstr = '\0';
    return str;
}


/*---------------------------------------------------------------------------
 * grNullProc --
 *
 *	A procedure of the type 'Void' that does absolutely nothing.
 *	Used when we need to point a procedure pointer to something, but
 *	don't want it to do anything.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */

Void
grNullProc()
{
}
