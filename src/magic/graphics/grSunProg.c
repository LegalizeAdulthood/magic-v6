
/* grSunProg
 *
 *	Reads button pushes from a window on the color display and
 *	passes them down a pipe to Magic.  Magic may send an 'A' over
 *	another pipe and this program will respond with the curosr position
 *	sent over a third pipe.
 *
 *	Things are done this way so that we can get SIGIO signals on the
 *	button stream -- windows can't send SIGIOs.  Also, this scheme allows
 *	us to keep track of the cursor position easily.
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
static char rcsid[]="$Header: grSunProg.c,v 6.0 90/08/28 18:41:06 mayo Exp $";
#endif  not lint

#ifdef	sun

#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sunwindow/window_hs.h>
#undef bool
#define Rect MagicRect  /* Avoid Sun's definition of Rect. */
#include "magic.h"
#include "geometry.h"
#include "graphics.h"

/* If not NULL, then log all actions taken by this program */
/* char *LOG_FILE = "./Magic-grSunProg.log"; */
char *LOG_FILE = NULL; 

int lastx = GR_CURSOR_X;
int lasty = GR_CURSOR_Y;

FILE *logFile = NULL;
FILE *ttyFile = NULL;
int notifyPID = 0;


log(str)
    char *str;
{
    if (logFile != NULL)
    {
	(void) fprintf(logFile, str);
	(void) fprintf(logFile, "\n");
	(void) fflush(logFile);
    }
}

tty(str)
    char *str;
{
    log(str);
    if (ttyFile != NULL)
    {
	(void) fprintf(ttyFile, "grSunProg: ");
	(void) fprintf(ttyFile, str);
	(void) fprintf(ttyFile, "\n");
	(void) fflush(ttyFile);
    }
}


notify()
{
  if (notifyPID != 0) 
  {
      char msg[100];
      kill(notifyPID, SIGIO);
      sprintf(msg, "Sending SIGIO signal to PID %d", notifyPID);
      log(msg);
  }
}

main(argc, argv)
    int argc;
    char *argv[];
{
    FILE *fileWindow, *fileReq, *fileButtons, *filePoint;
    struct inputmask imask;
    char msg[200];
    int pointFD, requestFD, buttonFD;

    if (LOG_FILE != NULL)
	logFile = fopen(LOG_FILE, "w");

    ttyFile = fopen("/dev/tty", "w");
    signal(SIGINT, SIG_IGN);

    log("Hello world, this is the new grSunProg.");

    if (argc != 7)
    {
	tty("Usage: colorWindowName textWindowName notifyPID requestFD pointFD buttonFD");
	goto die;
    }

    fileWindow = fopen(argv[1], "r");
    if (fileWindow == NULL)
    {
	sprintf(msg, "Could not open '%s' for read.", argv[1]);
	tty(msg);
	goto die;
    }

    {
	int textnum;

	textnum = win_nametonumber(argv[2]);

	input_imnull(&imask);
	imask.im_flags |= IM_NEGEVENT;
	win_setinputcodebit(&imask, MS_LEFT);
	win_setinputcodebit(&imask, MS_MIDDLE);
	win_setinputcodebit(&imask, MS_RIGHT);
	win_setinputcodebit(&imask, LOC_MOVE);
	win_setinputmask(fileno(fileWindow), &imask, NULL, textnum);
    }

    if (sscanf(argv[3], "%d", &notifyPID) != 1) 
    {
	tty("Funny PID for notification");
	tty(argv[3]);
	goto die;
    }
    if (sscanf(argv[4], "%d", &requestFD) != 1) 
    {
	tty("Funny requestFD");
	tty(argv[4]);
	goto die;
    }
    if (sscanf(argv[5], "%d", &pointFD) != 1) 
    {
	tty("Funny pointFD");
	tty(argv[5]);
	goto die;
    }
    if (sscanf(argv[6], "%d", &buttonFD) != 1) 
    {
	tty("Funny buttonFD");
	tty(argv[6]);
	goto die;
    }

    fileReq = fdopen(requestFD, "r");
    setbuf(fileReq, NULL);
    filePoint = fdopen(pointFD, "w");
    setbuf(filePoint, NULL);
    fileButtons = fdopen(buttonFD, "w");
    setbuf(fileButtons, NULL);

    while (TRUE)
    {
	int fds, ready;

	nextinput: ;

	fds = (1 << fileno(fileWindow)) | (1 << fileno(fileReq));
	(void) select(20, &fds, (int *) NULL, (int *) NULL, NULL);

	if ((fds & (1 << fileno(fileWindow))) != 0)
	{
	    struct inputevent event;

	    input_readevent(fileno(fileWindow), &event);
	    lastx = event.ie_locx;
	    lasty = event.ie_locy;

	    if (event.ie_code != LOC_MOVE)
	    {
		char but[50];

		sprintf(but, "xx %d %d", lastx, lasty);
		switch (event.ie_code)
		{
		    case MS_LEFT:
			but[0] = 'L';
			break;
		    case MS_MIDDLE:
			but[0] = 'M';
			break;
		    case MS_RIGHT:
			but[0] = 'R';
			break;
		    default:
			tty("Hey!  Funny button from the mouse.");
			goto nextinput;
			break;
		}
		if (win_inputposevent(&event))
		    but[1] = 'D';
		else
		    but[1] = 'U';
		(void) fprintf(fileButtons, "%s\n", but);
		(void) fflush(fileButtons);
		log("Got a button:");
		log(but);
		notify();
	    }
	}
	else if ((fds & (1 << fileno(fileReq))) != 0)
	{
	    int ch;
	    char poi[50];
	    ch = getc(fileReq);
	    if (ch == EOF) goto die;
	    sprintf(poi, "P %d %d", lastx, lasty);
	    (void) fprintf(filePoint, "%s\n", poi);
	    (void) fflush(filePoint);
	    log("Sending a point:");
	    log(poi);
	    notify();
	}
    }

die:
    log("grSunProg dieing");
    exit(1);
}

#else

#include <stdio.h>
main()
{
    (void) fprintf(stderr, "This program is only used on SUNs, but was compiled elsewhere!\n");
}

#endif	sun
