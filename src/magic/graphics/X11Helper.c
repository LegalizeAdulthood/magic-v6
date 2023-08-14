/* X11suHelper.c -- helper process for magic on X windows which signals the
 *		  parent (magic) whenever there is input available.
 */

#include <stdio.h>
#include <signal.h>
#include <X11/Xlib.h>

int readPipe,writePipe;	/* pipe file descriptor to magic process */
int parentID;		/* process id of parent */
Display *grXdpy;

int MapWindow();

main (argc, argv)
    int argc;
    char **argv;
{
    XEvent xevent;

#ifdef macII
    set42sig();
#endif macII

    sscanf(argv[1], "%d %d", &readPipe,&writePipe);
    grXdpy = XOpenDisplay(0);
    parentID = getppid();

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTERM,MapWindow); 
#ifdef SIGTSTP
    signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGCONT
    signal(SIGCONT,SIG_IGN);
#endif

    while (1)
    {
	XNextEvent(grXdpy, &xevent);
	if (isString(xevent) ==0) 
	{
	    write(writePipe, &xevent, sizeof(XEvent));
	    kill(parentID, SIGIO);
	}
    }
}

isString (event)
    XEvent event;
{
    if (event.type == KeyPress) 
    {
	XKeyPressedEvent *KeyPressedEvent = (XKeyPressedEvent *) &event;
	char inChar[10],c,*p;
	int nbytes;
	KeySym keysym;

	nbytes = XLookupString(KeyPressedEvent,inChar,sizeof(inChar),
			 &keysym,NULL);
	p = inChar;
	while (nbytes--)
	{
	     if ( (c = *p++) == 3)
	     {
	     	  kill(parentID, SIGINT);
	     } 
	     else
	     {
	          write(writePipe, &event, sizeof(XEvent));
	          write(writePipe, &c, sizeof(char));
	          kill(parentID, SIGIO);
	     }
	} 
	return 1;
    }
    else return 0;
}

MapWindow()
{
    Window window;

    if ( read(readPipe, (char *)&window, sizeof(Window)) == sizeof(Window))
    {
	XSelectInput(grXdpy, window,
		     KeyPressMask|ButtonPressMask|
		     ButtonReleaseMask|ExposureMask|
		     StructureNotifyMask|
		     OwnerGrabButtonMask);
	XSync(grXdpy,1);
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
