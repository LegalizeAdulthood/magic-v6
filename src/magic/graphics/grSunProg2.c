/* grSunProg2.c
 *
 *	Reads button pushes and characters from windows on the color display 
 *	and passes them down a pipe to Magic.  It also sends a SIGIO to Magic
 *	when it does this.  In addition, it scans for interrupt characters and
 *	interrupts Magic when one is found.
 *
 *	Magic may send commands over another pipe and this program will process
 *	them.  The commands recognized are:
 *
 *		A %s	-- Add the device to our list of windows to watch.
 *		D %s	-- Delete the device in our list of windows to watch.
 *
 *	Magic ships input events to Magic in this format:
 *
 *		{K,U,D} key x y wid
 *
 *		Where the first field is the event type:
 *		    K = keyboard character
 *		    U = mouse button up
 *		    D = mouse button down
 *		The second filed is the ascii value of the key printed as an
 *		integer (but for buttons, the ascii value of 'L' is used for
 *		the left button, and similarly 'M' and 'R' for the middle and
 *		right buttons.
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
static char rcsid[]="$Header: grSunProg2.c,v 6.0 90/08/28 18:41:08 mayo Exp $";
#endif  not lint

#ifdef	sun

#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sunwindow/window_hs.h>

/* If not NULL, then log all actions taken by this program */
#define LOG_FILE "grSunProg2.log"
/* #define LOG_FILE	NULL */


FILE *logFile = NULL;		/* File for debugging log. */
FILE *ttyFile = NULL;		/* File for error messages. */
int notifyPID = 0;		/* Process for SIGIO and SIGINT. */
int interruptChar = 0;		/* Ascii value of interrupt character. */


#define MAX_WINDOWS	32

/*---------------------------------------------------------------------------
 * log:
 *
 *	Put a string in our debugging log.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Prints to the log, if there is one.
 *
 *----------------------------------------------------------------------------
 */

void
log(str)
    char *str;
{
    if (logFile != NULL)
    {
	fprintf(logFile, str);
	fprintf(logFile, "\n");
	(void) fflush(logFile);
    }
}

/*---------------------------------------------------------------------------
 * tty:
 *
 *	Print an error message to the user's terminal and to the log.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Prints a message.
 *
 *----------------------------------------------------------------------------
 */

void
tty(str)
    char *str;
{
    log(str);
    if (ttyFile != NULL)
    {
	fprintf(ttyFile, "grSunProg2: ");
	fprintf(ttyFile, str);
	fprintf(ttyFile, "\n");
	(void) fflush(ttyFile);
    }
}


/*---------------------------------------------------------------------------
 * notifyIO:
 *
 *	Send a SIGIO to Magic.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Just sends the signal.
 *
 *----------------------------------------------------------------------------
 */

void
notifyIO()
{
  if (notifyPID != 0) 
  {
      char msg[100];
      kill(notifyPID, SIGIO);
      sprintf(msg, "Sending SIGIO signal to PID %d", notifyPID);
      log(msg);
  }
}


/*---------------------------------------------------------------------------
 * notifyINT:
 *
 *	Sends a SIGINT to Magic.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Just sends the signal.
 *
 *----------------------------------------------------------------------------
 */

void
notifyINT()
{
  if (notifyPID != 0) 
  {
      char msg[100];
      kill(notifyPID, SIGINT);
      sprintf(msg, "Sending SIGINT signal to PID %d", notifyPID);
      log(msg);
  }
}

/*---------------------------------------------------------------------------
 * die:
 *
 *	Abort ourselves.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	This process terminates.
 *
 *----------------------------------------------------------------------------
 */


void
die(code)
    int code;
{
    log("grSunProg2 dieing");
    exit(code);
}

/*---------------------------------------------------------------------------
 * openWindow:
 *
 *	Opens a new window and sets its input characteristics.
 *
 * Results:	
 *	A 'FILE' handle for the window.
 *
 * Side Effects:
 *	Opens the window.  Window should already exist on the screen.
 *
 *----------------------------------------------------------------------------
 */


FILE *
openWindow(name)
    char *name;		/* A string of the form "/dev/winXX" */
{
    struct inputmask imask;
    FILE *file;
    char msg[100];
    int key;

    log("open window...");
    file = fopen(name, "r");
    if (file == NULL)
    {
	sprintf(msg, "Could not open '%s' for read.", name);
	tty(msg);
	return NULL;
    }

    /* Receive keyboard and mouse events. */
    input_imnull(&imask);
    imask.im_flags |= (IM_ASCII | IM_NEGEVENT | IM_POSASCII);
    win_setinputcodebit(&imask, MS_LEFT);
    win_setinputcodebit(&imask, MS_MIDDLE);
    win_setinputcodebit(&imask, MS_RIGHT);
    for (key = ASCII_FIRST; key <= ASCII_LAST; key++)
        win_setinputcodebit(&imask, key);
    win_setinputmask(fileno(file), &imask, NULL, WIN_NULLLINK);

    log("done.");
    return file;
}

/*---------------------------------------------------------------------------
 * closeWindow:
 *
 *	Close down a window.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	The window is closed.
 *
 *----------------------------------------------------------------------------
 */


void
closeWindow(file)
    FILE *file;
{
    log("close window");
    fclose(file);
    log("done.");
}

/*---------------------------------------------------------------------------
 * main:
 *
 *	 Read input events from multiple windows and pass them off to Magic.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Sends events and SIGIO signals to Magic.  Also sends SIGINTs when
 *	interrupt characters are received.
 *
 *----------------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    int requestFD, eventFD;
    FILE *eventFile, *requestFile;
    char msg[200];
    char report[50];
    int windInBits, magicInBit;
    FILE *windFiles[MAX_WINDOWS];
    char *windNames[MAX_WINDOWS];
    int windWids[MAX_WINDOWS];
    int i;

    /* 
     * Initialize our table of active windows. 
     */
    windInBits = 0;
    for (i = 0; i < MAX_WINDOWS; i++)
    {
	windFiles[i] = (FILE *) NULL;
	windNames[i] = (char *) NULL;
	windWids[i] = -1;
    }

    /* 
     * Open our log files. 
     */
    if (LOG_FILE != NULL)
	logFile = fopen(LOG_FILE, "w");
    ttyFile = fopen("/dev/tty", "w");
    signal(SIGINT, SIG_IGN);
    log("Hello world, this is the grSunProg2.");

    /*
     * Process command line arguments.
     */
    if (argc != 5)
    {
	tty("Usage: notifyPID requestFD eventFD interruptChar");
	tty("Usage: (all arguments are integers)");
	tty("Usage: NOTE: This program is an internal process of Magic.");
	die(1);
    }

    if (sscanf(argv[1], "%d", &notifyPID) != 1) 
    {
	tty("Funny PID for notification");
	tty(argv[1]);
	die(1);
    }
    if (sscanf(argv[2], "%d", &requestFD) != 1) 
    {
	tty("Funny requestFD");
	tty(argv[2]);
	die(1);
    }

    if (sscanf(argv[3], "%d", &eventFD) != 1) 
    {
	tty("Funny eventFD");
	tty(argv[3]);
	die(1);
    }

    if (sscanf(argv[4], "%d", &interruptChar) != 1) 
    {
	tty("Funny interruptChar");
	tty(argv[4]);
	die(1);
    }

    /*
     * Open our input and output streams to Magic.
     */
    requestFile = fdopen(requestFD, "r");
    if (requestFile == NULL) 
    {
	tty("Could not open requestFD");
	die(1);
    }
    setbuf(requestFile, NULL);
    magicInBit |= (1 << fileno(requestFile));

    eventFile = fdopen(eventFD, "w");
    if (eventFile == NULL) 
    {
	tty("Could not open eventFile");
	die(1);
    }
    setbuf(eventFile, NULL);

    /*
     * Main loop:  wait for events and process them.
     */
    while (TRUE)
    {
	int inbits;

	nextinput: ;

	log("Going into select...");
	inbits = (windInBits | magicInBit);
	(void) select(20, &inbits, (int *) NULL, (int *) NULL, NULL);

	/*
	 * Do we have a request from Magic?
	 */
	if ((inbits & magicInBit) != 0)
	{
	    char ch;
	    char name[100];
	    char req[100];
	    int wid;

	    if (fgets(req, 95, requestFile) == NULL) die(1);
	    log("Got a request from Magic:");
	    log(req);

	    ch = req[0];
	    switch (ch)
	    {
		case 'A':
		    if (sscanf(req, "%c %s %d", &ch, name, &wid) != 3)
		    {
			tty("Funny 'A' request.\n");
		    }
		    else
		    {
			for(i = 0; i < MAX_WINDOWS; i++)
			{
			    if (windFiles[i] == NULL)
			    {
				windFiles[i] = openWindow(name);
				if (windFiles[i] != NULL)
				{
				    windNames[i] = 
					(char *) malloc(strlen(name) + 5);
				    strcpy(windNames[i], name);
				    windWids[i] = wid;
				    windInBits |= (1 << fileno(windFiles[i]));
				}
				break;
			    }
			}
			log("Done adding window.");
		    }
		    break;
		case 'D':
		    if (sscanf(req, "%c %s", &ch, name) != 2)
		    {
			tty("Funny 'D' request.\n");
		    }
		    else
		    {
			for(i = 0; i < MAX_WINDOWS; i++)
			{
			    if ((windNames[i] != NULL) &&
			      (strcmp(windNames[i], name) == 0))
			    {
				windInBits &= ~(1 << fileno(windFiles[i]));
				free(windNames[i]);
				windNames[i] = NULL;
				closeWindow(windFiles[i]);
				windFiles[i] = 0;
				windWids[i] = -1;
				break;
			    }
			}
		    }
		    log("Done deleting window.");
		    break;
		default:
		    tty("Unknown grSunProg2 request");
		    break;
	    }
	    log("Done with request");
	}

	/*
	 * Do we have something from a window?
	 */
	while ((inbits & windInBits) != 0)
	{
	    struct inputevent event;
	    bool noSend;
	    int fd;
	    int index;
	    char keytype;
	    int key;

	    log("Got something from a window");

	    /* Find first active file descriptor (fd). */
	    for (fd = 0; ((1 << fd) & inbits & windInBits) == 0; fd++) 
	    {
		/* noop */
	    }
	    inbits &= ~(1 << fd);

	    index = -1;
	    for (i = 0; i < MAX_WINDOWS; i++)
	    {
		if ((windFiles[i] != NULL) && (fileno(windFiles[i]) == fd)) 
		{
		    index = i;
		    break;
		}
	    }
	    if (index < 0 || index >= MAX_WINDOWS)
		tty("Bogus window table index.\n");

	    log("Getting event");
	    noSend = FALSE;
	    input_readevent(fd, &event);

	    if (logFile != NULL)
		fprintf(logFile, "Got event code = %d\n", event.ie_code);

	    keytype = '?';
	    key = 0;
	    if ((event.ie_code >= ASCII_FIRST) && (event.ie_code <= ASCII_LAST))
	    {
		/*
		 * Keyboard character.
		 */
		keytype = 'K';		/* Keyboard character. */
		if (event.ie_code == 13)
		    key = '\n';
		else
		    key = event.ie_code;
		if ((interruptChar != 0) && (key == interruptChar))
		{
		    /* Send a signal instead of the event. */
		    noSend = TRUE;
		    notifyINT();
		}
	    }
	    else if ((event.ie_code == MS_LEFT) || 
	      (event.ie_code == MS_MIDDLE) || (event.ie_code == MS_RIGHT))
	    {
		/*
		 * Mouse button.
		 */
		if (win_inputposevent(&event))
		    keytype = 'D';		/* Button down. */
		else
		    keytype = 'U';		/* Button up. */
		switch (event.ie_code)
		{
		    case MS_LEFT:
			key = 'L';
			break;
		    case MS_MIDDLE:
			key = 'M';
			break;
		    case MS_RIGHT:
			key = 'R';
			break;
		    default:
			tty("Funny mouse button pressed.");
			noSend = TRUE;
			break;
		}
	    }
	    else
	    {
		/*
		 * Some other event -- ignore it.
		 */
		noSend = TRUE;
	    }
	    log("Prepared event");
	    if (noSend) log("Not sending event");

	    /*
	     * Now send off the event to Magic.
	     */
	    if (!noSend)
	    {
		sprintf(report, "%c %d %d %d %d\n", keytype, key, 
		    event.ie_locx, event.ie_locy, windWids[index]);
		log("Got an event:");
		log(report);
		fprintf(eventFile, "%s", report);
		(void) fflush(eventFile);
		notifyIO();
	    }
	}
	log("Go around loop again");

    }  /* End of main loop. */

    /*NOTREACHED*/
}

#else

#include <stdio.h>
main()
{
    fprintf(stderr, "This program is only used on SUNs, but was compiled elsewhere!\n");
    exit(1);
}

#endif	sun
