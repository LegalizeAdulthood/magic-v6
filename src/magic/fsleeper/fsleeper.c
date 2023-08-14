/*
 * fsleeper --
 *
 * Cause a terminal on the local machine to be logged in
 * as sleeper on a remote machine.
 *
 * Use:
 *	fsleeper [-t tty] [-l username] [machine]
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

#include <stdio.h>
#ifndef SYSV
# include <sgtty.h>
#else
# include <termio.h>
#endif

#define DEFAULTMACHINE	"blablabla"
#define DEFAULTUSER	"cad"

#ifndef lint
    static char rcsid[] = "$Header: fsleeper.c,v 6.0 90/08/28 19:03:52 mayo Exp $";
#endif lint
#ifdef LLITOUT
static int litout = LLITOUT;
#endif
#ifdef NTTYDISC
static int ldisc = NTTYDISC;
#endif

main(argc, argv)
    char *argv[];
{
    char *machine, *user;
    char *tty;
    char *progname;
    char cmd[200];
#ifdef SYSV
    struct termio tio;
#else
    struct sgttyb sgttyb;
#endif
    int localmode;
    int i;
    int fd;

    tty = (char *) NULL;
    machine = (char *) NULL;
    user = (char *) NULL;
    for (argc--, progname = *argv++; argc--; argv++)
    {
	if (strncmp(*argv, "-t", 2) == 0)
	{
	    if (argv[0][2])
		tty = &argv[0][2];
	    else
		tty = *++argv;
	    continue;
	}
	if (strncmp(*argv, "-l", 2) == 0)
	{
	    if (argv[0][2])
		user = &argv[0][2];
	    else
		user = *++argv;
	    continue;
	}
	machine = *argv;
    }

    if (tty == (char *) NULL)
    {
	char *pport, *ptablet, *pdistype, *pmontype;

	FindDisplay(ttyname(0), "displays", ".:~cad/lib/",
			&pport, &ptablet, &pdistype, &pmontype);
	if (pport == (char *) NULL)
	{
	    printf("No graphics display in 'displays' file for this tty\n");
	    printf("Use the '-t tty' switch\n");
	    exit (1);
	}
	tty = pport;
    }

    if (machine == (char *) NULL)
	machine = DEFAULTMACHINE;
    if (user == (char *) NULL)
	user = DEFAULTUSER;

    for (i = 0; i < 20; i++)
	(void) close(i);
#ifdef TIOCNOTTY
    (void) ioctl(0, TIOCNOTTY, (struct sgttyb *) 0);
#else
    setpgrp();
#endif

    fd = open(tty, 2);
    (void) dup(fd);
    (void) dup(fd);

#ifdef TIOCSETD
    (void) ioctl(fd, TIOCSETD, (char *) &ldisc);
#endif
#ifdef TIOCLGET
    (void) ioctl(fd, TIOCLGET, (char *) &localmode);
#endif
#ifdef TIOCLBIS
    (void) ioctl(fd, TIOCLBIS, (char *) &litout);
#endif
#ifndef SYSV
    (void) gtty(fd, &sgttyb);
    sgttyb.sg_flags = (sgttyb.sg_flags &
	~(RAW | CBREAK | ECHO | LCASE)) | EVENP | ODDP | CRMOD;
    sgttyb.sg_ispeed = B9600;
    sgttyb.sg_ospeed = B9600;
    (void) stty(fd, &sgttyb);
#else
    ioctl(fd, TCGETA, &tio);
    tio.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
    tio.c_oflag |=  (OPOST|ONLCR|TAB3);
    tio.c_oflag &= ~(OCRNL|ONOCR|ONLRET);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    ioctl(fd, TCSETA, &tio);
#endif


    /* Start up a rlogin */

    sprintf(cmd, "rlogin %s -l %s", machine, user);
    system(cmd);
}

