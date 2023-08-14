/* grAed1.c -
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
 * This file contains primitive functions to manipulate an Aed512
 * color display.  Included here are initialization and closing
 * functions, and several utility routines used by the other Aed
 * modules.
 */

#include <stdio.h>
#include <sgtty.h>
#include "magic.h"
#include "geometry.h"
#include "graphics.h"
#include "windows.h"
#include "graphicsInt.h"
#include "grAedInt.h"
#include "textio.h"
#include "txcommands.h"
#include "signals.h"


#ifndef lint
static char rcsid[]="$Header: grAed1.c,v 6.0 90/08/28 18:40:22 mayo Exp $";
#endif  not lint


/* Imports from other Aed modules: */
extern bool aedGetCursorPos(), aedDrawGrid();
extern Void AedEnableTablet(), AedDisableTablet();
extern Void AedSetCMap(), aedPutText(), aedDefineCursor();
extern Void AedSetCursor(), AedTextSize(), AedDrawGlyph();
extern Void AedBitBlt(), aed1024BitBlt(), NullBitBlt();
extern int AedReadPixel();
extern Void aedDrawLine(), aedSetLineStyle(), aedSetCharSize();
extern Void aedSetWMandC(), aedFillRect();

/* This is kind of a long story, and very kludgy, but the following
 * things need to be defined as externals because of the way lint
 * libraries are made by taking this module and changing all procedures
 * names "Aedxxx" to "Grxxx".  The change is only done at the declaration
 * of the procedure, so we need these declarations to handle uses
 * of those names, which don't get modified.  Check out the Makefile
 * for details on this.
 */

extern Void AedClose(), AedFlush(), AedInit();

/* The following variables are used to hold state that we keep around
 * between procedure calls in order to reduce the amount of information
 * that must be shipped to the terminal.
 */

int aedCurx, aedCury;		/* Current access position */
extern aedCurCharSize;		/* Current character size */
int aedWMask;			/* Current write mask value */
int aedColor;			/* Current color value */
int aedStipple;			/* Current stipple number */

/* Variables to distinguish between different sorts of AED displays. 
 */

char *aedDSP = NULL;		/* String sent to define a stipple pattern */
int aedSTP = -1;		/* Data for STP command. -1 means ignore. */
int aedSGT = -1;		/* Data for SGT command. -1 means ignore. */
bool aedDTM = FALSE;		/* Issue a DTM (define tablet mapping)? */
int aedDTMxorigin;		/* X origin for DTM. */
int aedDTMyorigin;		/* Y origin for DTM. */
int aedDTMxscale;		/* X scale for DTM. */
int aedDTMyscale;		/* Y scale for DTM. */

int aedCursorRow;		/* First invisable scan line used for storing
				 * cursors, etc.
				 */
bool aedHasProgCursor;		/* Does this display have a programmable
				 * cursor?
				 */
Point aedOffPoint;		/* The location of a spare 14 X 24 chunk of
				 * memory used for behind-the-scenes text 
				 * clipping.
				 */
int aedButtonOrder;		/* Tells how buttons are encoded for this
				 * display.  See grAedInt.h.
				 */

/* files for the mouse and the graphics screen */
FILE *grAedOutput = NULL;
FILE *grAedInput = NULL;

/* The following variables hold the AED terminal state so we can restore
 * it when we are finished.
 */

static struct sgttyb aedsgttyb;	/* Used to save terminal control bits */
static int aedsgflags;		/* Used to save flags from Aedsgttyb */
static char aedsgispeed,	/* Used to save baud rates */
    aedsgospeed;
static int aedlocalmode;	/* Used to save terminal local mode word */


/*---------------------------------------------------------
 * aedSetWMandC:
 *	This is a local routine that resets the value of the current
 *	write mask and color, if necessary.
 *
 * Results:	None.
 *
 * Side Effects:
 *	If AedWMask is different from mask, then the new mask is output to
 *	the display and stored in AedWMask.  The same holds for AedColor
 *	and color.
 *
 * Errors:		None.
 *---------------------------------------------------------
 */

Void
aedSetWMandC(mask, color)
    int mask;			/* New value for write mask */
    int color;			/* New value for current color */
{
    aedWMask = mask;
    putc('L', grAedOutput);
    putc(mask&0377, grAedOutput);
    aedColor = color;
    putc('C', grAedOutput);
    putc(aedColor&0377, grAedOutput);
}



/*---------------------------------------------------------
 * aedSetLineStyle:
 *	This local routine sets the current line style.
 *
 * Results:	None.
 *
 * Side Effects:
 *	A new line style is output to the display.
 *---------------------------------------------------------
 */

Void
aedSetLineStyle(style)
    int style;			/* New stipple pattern for lines. */
{
    style &= 0377;
    putc('1', grAedOutput);
    putc(style&0377, grAedOutput);
    putc(0377, grAedOutput);
}


/*---------------------------------------------------------
 * aedSetSPattern:
 *	aedSetSPattern associates a stipple pattern with a given
 *	stipple number.  This is a local routine called from
 *	grStyle.c .
 *
 * Results:	None.
 *
 * Side Effects:
 *	The eight low-order bytes in pattern are set to be used
 *	whenever the given stipple number is set by the routine
 *	aedSetStipple below.
 *---------------------------------------------------------
 */

Void
aedSetSPattern(stipple, pattern)
    int stipple;			/* The stipple number, 1-15. */
    int pattern[8];			/* 8 8-bit patterns integers */
{
    int i;
    aedStipple = -1;
    fputs(aedDSP, grAedOutput);
    putc(stipple&017, grAedOutput);
    for (i=0;  i<8;  i++) putc(pattern[i] & 0377, grAedOutput);
}


/*---------------------------------------------------------
 * aedSetStipple:
 *	This routine sets the Aeds current stipple number.
 *
 * Results: None.
 *
 * Side Effects:
 *	The current stipple number in the Aed is set to stipple,
 *	if it wasn't that already.
 *---------------------------------------------------------
 */

Void
aedSetStipple(stipple)
    int stipple;			/* The stipple number to be used. */
{
    aedStipple = stipple;
    putc('"', grAedOutput);
    putc(stipple&017, grAedOutput);
}


/*---------------------------------------------------------
 * AedInit:
 *	AedInit initializes the graphics display and clears its screen.
 *	Files must have been previously opened with GrSetDisplay();
 *
 * Results:	TRUE (always works, as far as this routine is concerned).
 *
 * Side Effects:
 *	The display is re-initialized and its color map is reset.
 *---------------------------------------------------------
 */

Void
GrInit()
{
    static int litout = LLITOUT|LDECCTQ;
    static int ldisc = NTTYDISC;

    /* First, grab up the display modes, then reset them to put it
     * into cooked mode.  If doing fast I/O
     * then set the LLITOUT bit.  Note:  setting the LLITOUT bit only
     * works if it happens before the stty.  
     * LDECCTQ is set in the hopes of preventing output to the AED
     * from being restarted by additional characters sent after it
     * has sent a ^S to stop output.
     */

    (void) ioctl(fileno(grAedOutput), TIOCSETD, (char *) &ldisc);
    (void) ioctl(fileno(grAedOutput), TIOCLGET, (char *) &aedlocalmode);
    (void) ioctl(fileno(grAedOutput), TIOCLBIS, (char *) &litout);
    (void) gtty(fileno(grAedOutput), &aedsgttyb);
    aedsgflags = aedsgttyb.sg_flags;
    aedsgispeed = aedsgttyb.sg_ispeed;
    aedsgospeed = aedsgttyb.sg_ospeed;
    aedsgttyb.sg_flags = (aedsgttyb.sg_flags &
	~(RAW | ECHO | LCASE)) | CBREAK | EVENP | ODDP | CRMOD;
    (void) stty(fileno(grAedOutput), &aedsgttyb);

    /* Output an initialization string to the display.  The
     * initialization string resets the terminal, sets formats,
     * clears the display, and initializes the read and write
     * masks.
     */

    /* ESC: escape to interpreter
     * SEN: set encoding scheme 18D8N
     *	    1: one character function codes
     *      8: eight-bit binary operands
     *	    D: returned operands variable length decimal
     *	    8: eight-bit binary coordinate encoding
     *	    N: one pixel per byte
     * SBC: set background color to 0
     */
    fputs("\33\33G18D8N[", grAedOutput);
    putc(0, grAedOutput);

    /* SWM: set write mask to 377
     * \14\33\33: clear screen (\14), reenter interpreter
     * SEC: set color to 377
     */
    fputs("L\377\14\33\33C", grAedOutput);
    putc(0377, grAedOutput);

    /* SRM: set read mask to 377, 377, 377, 377
     * SCC: set cursor colors to 177, 277, 42 frames
     * SCP: set cursor to crosshair, 0, 0
     */
    fputs("M\377\377\377\377c\177\277\42]+", grAedOutput);
    putc(0, grAedOutput);
    putc(0, grAedOutput);

    /* SLS: set line style to solid, width one */
    fputs("1\377\377", grAedOutput);

    /* SGT: Select Graphic Tablet Command. */
    if (aedSGT >= 0)
    {
	fputs("+9", grAedOutput);	
	putc(aedSGT, grAedOutput);	
    }

    /* STP: Set Tablet Parameters */
    if (aedSTP >= 0)
    {
	fputs("+(", grAedOutput);	
	putc(aedSTP, grAedOutput);	
    }

    /* DTM: Define Tablet Mapping */
    if (aedDTM)
    {
	putc('2', grAedOutput);			/* DTM */
	aedEncode16(aedDTMxorigin);		/* xorigin */	
	aedEncode16(aedDTMyorigin);		/* yorigin */	
	putc(aedDTMxscale, grAedOutput);	/* xscale */
	putc(aedDTMyscale, grAedOutput);	/* yscale */
    }

    /* ETC: enable tablet cursor -- turn off crosshair */
    aedSetPos(0,0);
    putc('3', grAedOutput);	
    putc(0, grAedOutput);

    (void) fflush(grAedOutput);

    aedCurx = -1;
    aedCury = -1;
    aedCurCharSize = -1;
    aedWMask = aedColor = 0377;
    aedStipple = 0;

    return TRUE;
}


/*---------------------------------------------------------
 * AedClose:
 *	AedClose does whatever is necessary to reset the characteristics
 *	of the Aed512 after the program is finished.  Also closes all
 *	files associated with graphics output and mouse input.
 *
 * Results:	None.
 *
 * Side Effects:
 *	The graphics display modes are reset.
 *---------------------------------------------------------
 */

Void
GrClose()
{
    if (grAedOutput != NULL)  /* This file might not be set up yet */
    {
	(void) fflush(grAedOutput);
	aedsgttyb.sg_flags = aedsgflags;
	aedsgttyb.sg_ispeed = aedsgispeed;
	aedsgttyb.sg_ospeed = aedsgospeed;
	(void) stty(fileno(grAedOutput), &aedsgttyb);
	(void) ioctl(fileno(grAedOutput), TIOCLSET, (char *) &aedlocalmode);
	(void) fclose(grAedOutput);
    }
    if (grAedInput != NULL)
    {
	TxDeleteInputDevice(1 << fileno(grAedInput));
	(void) fclose(grAedInput);
    }
}


/*---------------------------------------------------------
 * AedFlush:
 * 	Flush output to display.
 *
 * Results:	None.
 *
 * Side Effects:
 *	None.
 *---------------------------------------------------------
 */

Void
GrFlush()
{
    if (grAedOutput != NULL)  /* This file might not be set up yet */
    {
	(void) fflush(grAedOutput);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * aedStdin --
 *
 *      Handle the stdin device for the AED driver.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Adds events to the event queue.
 *
 * ----------------------------------------------------------------------------
 */
 /*ARGSUSED*/

Void
aedStdin(fd, cdata)
    int fd;
    ClientData cdata;
{
    int ch;
    TxInputEvent *event;

    /* The following variables, and grAedTabletOn, are used to avoid having
     * to ask the AED for the point location on every keystroke.  If the
     * tablet is off, or if this isn't a new command from the last time
     * we asked, then re-use the old point.  This is a bit of a hack.  The
     * check on grAedTabletOn is needed because we can't read the cursor
     * position otherwise.
     */

    static int lastCommandNum = -1;
    static Point lastCommandPoint;

    event = TxNewEvent();
    ch = getc(stdin);
    if (ch == EOF)
	event->txe_button = TX_EOF;
    else
	event->txe_button = TX_CHARACTER;
    event->txe_ch = ch;
    event->txe_wid = WIND_UNKNOWN_WINDOW;
    if ((lastCommandNum == TxCommandNumber) || !grAedTabletOn) {
	/* Use the previous point location. */
	event->txe_p = lastCommandPoint;
    } else {
	/* Actually read the point location. */
	if (grAedMouseIsDisabled) {
	    event->txe_p.p_x = GR_CURSOR_X;
	    event->txe_p.p_y = GR_CURSOR_Y;
	} else if (!aedGetCursorPos(&(event->txe_p))) {
	    TxError("%s, using (%d, %d) instead.\n",
		"Could not read cursor position from AED", 
		GR_CURSOR_X, GR_CURSOR_Y);
	    TxError("Tablet (mouse) is now disabled.\n");
	    grAedMouseIsDisabled = TRUE;
	    TxDeleteInputDevice(1 << fileno(grAedInput));
	    event->txe_p.p_x = GR_CURSOR_X;
	    event->txe_p.p_y = GR_CURSOR_Y;
	};
	lastCommandPoint = event->txe_p;
	lastCommandNum = TxCommandNumber;
    }
    TxAddEvent(event);
}



/*---------------------------------------------------------
 * aedSetDisplay:
 *	This routine sets the appropriate parameters so that
 *	Magic will work with the particular display type.
 *
 * Results:	None.
 *
 * Side Effects:
 *	Depends on the display type.
 *---------------------------------------------------------
 */
 /*ARGSUSED*/

bool
aedSetDisplay(dispType, outFileName, mouseFileName)
    char *dispType;			/* Some sort of AED display */
    char *outFileName;
    char *mouseFileName;
{
    extern Void grAedMouseDevice();
#define BUFFERSIZE 512
    static char buffer[BUFFERSIZE];
    int aedTabletType;

    TxAddInputDevice((1 << fileno(stdin)), aedStdin, (ClientData) NULL);
    if (TxStdinIsatty) SigWatchFile(fileno(stdin), "stdin");

    /* open devices */
    grAedOutput = fopen(outFileName, "w");
    if (grAedOutput == NULL) 
    {
	TxError("Unable to open graphics display for output to '%s'\n", 
		outFileName);
	return FALSE;
    }

    /* Use a smaller-than-normal buffer size in order to reduce the
     * delay before redisplay begins.
     */

    setbuffer(grAedOutput, buffer, BUFFERSIZE);
    if (strcmp(mouseFileName, "/dev/null") == 0)
    {
	grAedMouseIsDisabled = TRUE;
    }
    else
    {
	grAedInput = fopen(mouseFileName, "r");
	if (grAedInput == NULL)
	{
	    TxError("Unable to open tablet device '%s'\n", mouseFileName);
	    grAedMouseIsDisabled = TRUE;
	    return FALSE;
	}
	else
	{
	    /* Mouse input must be unbuffered if we are going to use CBREAK
	     * and select on a per-character basis.
	     */
	    setbuf(grAedInput, (char *) NULL);
	    grAedMouseIsDisabled = FALSE;
	    TxAddInputDevice(1 << fileno(grAedInput), grAedMouseDevice, 
		(ClientData) NULL);
	    SigWatchFile(fileno(grAedInput), mouseFileName);
	}
    }

    /* Set up the procedure values in the indirection table. */

    GrLockPtr = grSimpleLock;
    GrUnlockPtr = grSimpleUnlock;
    GrInitPtr = AedInit;
    GrClosePtr = AedClose;
    GrSetCMapPtr = AedSetCMap;

    GrEnableTabletPtr = AedEnableTablet;
    GrDisableTabletPtr = AedDisableTablet;
    GrSetCursorPtr = AedSetCursor;
    GrTextSizePtr = AedTextSize;
    GrDrawGlyphPtr = AedDrawGlyph;
    GrReadPixelPtr = AedReadPixel;
    GrFlushPtr = AedFlush;

    /* local indirections */
    grSetSPatternPtr = aedSetSPattern;
    grPutTextPtr = aedPutText;
    grDefineCursorPtr = aedDefineCursor;
    GrBitBltPtr = AedBitBlt;
    grDrawGridPtr = aedDrawGrid;
    grDrawLinePtr = aedDrawLine;
    grSetWMandCPtr = aedSetWMandC;
    grFillRectPtr = aedFillRect;
    grSetStipplePtr = aedSetStipple;
    grSetLineStylePtr = aedSetLineStyle;
    grSetCharSizePtr = aedSetCharSize;

    grDStyleType = "7bit";
    grCMapType = "7bit";
    grCursorType = "color";

    /* 
     * Device-specific stuff 
     */

    /* Here are some constants for naming the type of bitpad or mouse. 
     * Note that many of these combinations HAVE NOT BEEN TESTED.
     * Only rely on the ones that appear in the code below.
     */
#define	TAB_OLDBITPAD1	1	/* Ancient UCB512 with ancient UCB roms. */
#define	TAB_BITPAD1	2
#define	TAB_SUMMAMOUSE	3
#define TAB_KURTA	4	/* Use button order BUT124. */
#define	TAB_GTCO	5

    /* Constants for "DSP: define stipple pattern" command. */
#define	UCB_DSP	","
#define	AED_DSP	"+!"

    /*
     * UCB512
     */
    if (strcmp(dispType,"UCB512") == 0) {
	GrScreenRect.r_xtop = 511;
	GrScreenRect.r_ytop = 483;
	aedDSP = UCB_DSP;
	GrBitBltPtr = AedBitBlt;
	aedHasProgCursor = TRUE;
	aedButtonOrder = BUT248;
	aedTabletType = TAB_OLDBITPAD1;
    /*
     * UCB512N
     */
    } else if (strcmp(dispType,"UCB512N") == 0) {
	GrScreenRect.r_xtop = 511;
	GrScreenRect.r_ytop = 483;
	aedDSP = UCB_DSP;
	GrBitBltPtr = AedBitBlt;
	aedHasProgCursor = TRUE;
	aedButtonOrder = BUT248;
	aedTabletType = TAB_BITPAD1;
    /*
     * AED767
     */
    } else if (strcmp(dispType,"AED767") == 0) {
	GrScreenRect.r_xtop = 767;
	GrScreenRect.r_ytop = 574;
	aedDSP = AED_DSP;
	GrBitBltPtr = NullBitBlt;
	aedHasProgCursor = FALSE;
	aedButtonOrder = BUT248;
	aedTabletType = TAB_BITPAD1;
    /*
     * AED1024
     */
    } else if (strcmp(dispType,"AED1024") == 0) {
	WindScrollBarWidth = 11;
	GrScreenRect.r_xtop = 1023;
	GrScreenRect.r_ytop = 768;
	aedDSP = AED_DSP;
	GrBitBltPtr = aed1024BitBlt;	
	aedHasProgCursor = FALSE;
	aedButtonOrder = BUT421;
	aedTabletType = TAB_SUMMAMOUSE;
    /*
     * UCB1024
     */
    } else if (strcmp(dispType,"UCB1024") == 0) {
	WindScrollBarWidth = 11;
	GrScreenRect.r_xtop = 1023;
	GrScreenRect.r_ytop = 768;
	aedDSP = UCB_DSP;
	GrBitBltPtr = aed1024BitBlt;	
	aedHasProgCursor = TRUE;
	aedButtonOrder = BUT421;
	aedTabletType = TAB_SUMMAMOUSE;
    /*
     * Bogus display type -- code for grMain1.c doesn't match this code.
     */
    } else {
	ASSERT(FALSE, "aedSetDisplay: bad display type");
    }

    /* Set up off-screen memory useage. */
    aedCursorRow = GrScreenRect.r_ytop + 1;
    aedOffPoint.p_x = 512 - 14;
    aedOffPoint.p_y = aedCursorRow;

    /* Set up tablet type. */
    switch (aedTabletType)
    {
	case TAB_OLDBITPAD1:
	    /* An OLD UCB512 with UCB ROMs before the SGT and STP commands
	     * were implemented. 
	     */
	    aedSGT = -1;	/* SGT: don't do Select Graphics Tablet. */
	    aedSTP = -1;	/* STP: don't Set Tablet Parameters. */
	    aedDTM = TRUE;	/* DTM: Define Tablet Mapping. */
	    aedDTMxorigin = 400;	/* set DTM x origin. */
	    aedDTMyorigin = 496;	/* set DTM y origin. */
	    aedDTMxscale = 40;		/* set DTM x scale. */
	    aedDTMyscale = 40;		/* set DTM y scale. */
	    break;
	case TAB_BITPAD1:
	    aedSGT = 1;		/* SGT: Select Graphics Tablet. */
	    aedSTP = 1;		/* STP: Set Tablet Parameters. */
	    aedDTM = TRUE;	/* DTM: Define Tablet Mapping. */
	    aedDTMxorigin = 400;	/* set DTM x origin. */
	    aedDTMyorigin = 496;	/* set DTM y origin. */
	    aedDTMxscale = 40;		/* set DTM x scale. */
	    aedDTMyscale = 40;		/* set DTM y scale. */
	    break;
	case TAB_KURTA:
	    aedSGT = 0;		/* SGT: Select Graphics Tablet. */
	    aedSTP = 3;		/* STP: Set Tablet Parameters. */
	    aedDTM = TRUE;	/* DTM: Define Tablet Mapping. */
	    aedDTMxorigin = 400;	/* set DTM x origin. */
	    aedDTMyorigin = 496;	/* set DTM y origin. */
	    aedDTMxscale = 40;		/* set DTM x scale. */
	    aedDTMyscale = 40;		/* set DTM y scale. */
	    break;
	case TAB_GTCO:
	    aedSGT = 2;		/* SGT: Select Graphics Tablet. */
	    aedSTP = 3;		/* STP: Set Tablet Parameters. */
	    aedDTM = TRUE;	/* DTM: Define Tablet Mapping. */
	    aedDTMxorigin = 400;	/* set DTM x origin. */
	    aedDTMyorigin = 496;	/* set DTM y origin. */
	    aedDTMxscale = 40;		/* set DTM x scale. */
	    aedDTMyscale = 40;		/* set DTM y scale. */
	    break;
	case TAB_SUMMAMOUSE:
	    aedSGT = 3;		/* SGT: Select Graphics Tablet. */
	    aedSTP = 3;		/* STP: Set Tablet Parameters. */
	    aedDTM = TRUE;	/* DTM: Define Tablet Mapping. */
	    aedDTMxorigin = 0;		/* set DTM x origin. */
	    aedDTMyorigin = 0;		/* set DTM y origin. */
	    aedDTMxscale = 100;		/* set DTM x scale. */
	    aedDTMyscale = 100;		/* set DTM y scale. */
	    break;
	default:
	    ASSERT(FALSE, "aedSetDisplay: bad bitpad type");
    }

    (void) AedInit();
    return TRUE;
}
