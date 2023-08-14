/* X10Helper.c -- helper process for magic on X windows which signals the
 *		parent (magic) whenever there is input available.
 *
 * Verion "a", for the Livermore X10a driver.
 */


#ifndef lint
static char rcsid[]="$Header: X10helper.c,v 6.0 90/08/28 18:40:17 mayo Exp $";
#endif  not lint

#include <X/Xlib.h>
#include <stdio.h>
#include <signal.h>

int readPipe,writePipe;	/* pipe file descriptor to magic process */
int parentID;		/* process id of parent */
int sizeOfEvent;	/* number of bytes in an X event */
Window window;		/* magic window identifier */

int MapWindow();

main (argc, argv)
    int argc;
    char **argv;
{
    char *writedata;
    char *dispName;
    int intdata;

    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, MapWindow);
#ifndef SYSV
    signal(SIGTSTP,SIG_IGN);
    signal(SIGCONT,SIG_IGN);
#endif
#ifdef macII
    signal(SIGTSTP,SIG_IGN);
    signal(SIGCONT,SIG_IGN);
#endif

    sscanf(argv[1], "%d %d", &readPipe,&writePipe);
    dispName = NULL;
    if (argc > 3) dispName = argv[3];
    XOpenDisplay(dispName);
    parentID = getppid();
    simulateInterrupts();
}

MapWindow()
{
    Window window;

    if ( read(readPipe, (char *)&window, sizeof(window)) == sizeof(window))
    {
	XMapWindow(window);
	XSelectInput(window,
	    KeyPressed | ButtonPressed | ButtonReleased | ExposeWindow | ExposeRegion);
	XFlush();
#ifdef SYSV
    /* Note:  HAS NOT BEEN TESTED.  Just a "good guess" based upon some mods
     * to a similar driver which was tested.	1/19/89, Livermore.
     */
    signal(SIGTERM, MapWindow);
#endif SYSV

    }
    else
	fprintf(stderr,"xhelper: read on pipe failed\n");
}

simulateInterrupts ()
{
    XEvent event;

    sizeOfEvent = sizeof(event);

    for(;;){
	XNextEvent(&event);
	if (isControlC(event)) {
	    kill(parentID, SIGINT);
	} else {
	    write(writePipe, &event, sizeOfEvent);
	    kill(parentID, SIGIO);
	};
    };
}

isControlC (event)
    XEvent event;
{
    if (event.type == KeyPressed) {
	char inChar;
	int nbytes;

	inChar = XLookupMapping(&event, &nbytes)[0];
	return (inChar == 3);
    } else {
	return 0;
    };
}

