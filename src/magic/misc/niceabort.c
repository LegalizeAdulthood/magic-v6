/*
 * niceabort.c --
 *
 * Nice version of abort which dumps core without actually
 * killing the running job.
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
static char rcsid[]="$Header: niceabort.c,v 6.1 90/09/13 12:08:54 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include "magic.h"
#include "textio.h"
#include "utils.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "main.h"

#define	GCORE		"/usr/ucb/gcore"
#define	CRASHDIR	"~cad/crash/magic/"

/*
 * Command used to mail core dump messages.  The '%s' is filled in with the
 * name of a file to be mailed.
 */
#define MAIL_COMMAND	"/bin/mail magiccrash < %s"


/* The following array is used to hold information about the
 * assertion that failed.
 */

char AbortMessage[200];
bool AbortFatal = FALSE;

/* For lint */
long time();
char *ctime();

/*
 * ----------------------------------------------------------------------------
 *
 * niceabort --
 *
 * Nice version of abort which dumps core (using gcore) without actually
 * killing the running job.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The first time we are called, we dump a core image; on successive
 *	calls, we don't bother to dump core.
 * ----------------------------------------------------------------------------
 */

niceabort()
{
    static int timesCalled = 0;
    int parentPid = getpid();
    int cpid, gcpid, wpid;
    FILE *commentFile, *crashFile;
    time_t now;
    char pidString[20], line[150], command[200], tempName[200], *crashDir;

    timesCalled++;
    if (timesCalled > 10) 
    {
	TxPrintf("\nAbort called more than 10 times -- things are really hosed!\n");
	TxPrintf("Recommendation:\n");
	TxPrintf("  1) Copy all your files to another directory.\n");
	TxPrintf("  2) Send magic a SIGTERM signal and it will ATTEMPT to write out your files.\n");
	TxPrintf("     (It might trash them, though.)\n");
	TxPrintf("Magic going to sleep now for 10 hours -- please kill me.\n");
	sleep(3600);  
    }

    if (timesCalled > 1)
    {
	TxPrintf("Magic has encountered another internal inconsistency:\n\n");
	TxPrintf("     %s\n\n", AbortMessage);
	TxPrintf("but it won't dump core this time.\n");
	if (AbortFatal)
	    TxPrintf("Unfortunately,  Magic can't recover from this error.\n");
	else
	    TxPrintf("Please save your files as soon as possible and quit magic.\n");
	return;
    }

    TxPrintf("Magic has encountered a major internal inconsistency:\n\n");
    TxPrintf("     %s\n\n", AbortMessage);

#ifdef TITAN
    fflush(stdout);
    abort();
#endif

#ifdef SYSV
    if (1)
#else
    if (AbortFatal)
#endif
	TxPrintf("Unfortunately,  Magic can't recover from this error.\n");
    else
    {
	TxPrintf("It will try to recover, but you should save all your\n");
	TxPrintf("files as soon as possible and quit magic.\n");
    }
    TxPrintf("\n");

    /* If mainDebug is set, dbx is presumably being used, so don't generate
     * core dump or ask for comment,
     */
    if(mainDebug)
    {
	TxPrintf("-D is set, so magic won't dump core.\n");
	return;
    }

#ifndef SYSV
    TxPrintf("Please wait while magic generates a core image of itself....\n");
    sprintf(pidString, "%d", parentPid);
    cpid = fork();
    if (cpid < 0)
    {
	perror("fork");
	goto done;
    }

    /* Child */

    if (cpid == 0)
    {
	/*
	 * Suspend the parent to make it as easy as
	 * possible for gcore to make a core image.
	 */
	kill(SIGSTOP, parentPid);
	gcpid = vfork();
	if (gcpid < 0)
	    perror("vfork");
	else
	{
	    if (gcpid == 0)
	    {
		execl(GCORE, "gcore", pidString, 0);
		exit (1);
	    }
	    while ((wpid = wait(0)) != gcpid && wpid != -1)
		/* Nothing */;
	}
	/*
	 * Resume the parent
	 */
	kill(SIGCONT, parentPid);
	exit (0);
    }

    while ((wpid = wait(0)) != cpid && wpid != -1)
	/* Nothing */;

    /*
     * We use PaOpen merely to to tilde expansion on CRASHDIR,
     * to find the real name of the directory into which we want
     * to place the crash dump.
     */

    if (crashFile = PaOpen(CRASHDIR, "r", "", ".", "", &crashDir))
    {
	(void) fclose(crashFile);

	(void) sprintf(command, "mv core.%s %s", pidString, crashDir);
	system(command);
	TxPrintf(".... done\n");
	(void) sprintf(tempName, "%s/core.%s", crashDir, pidString);
	(void) chmod(tempName, 0644);

	TxPrintf("Please type a description of the problem, so the\n");
	TxPrintf("magic maintainers will know what went wrong.\n");
	TxPrintf("Terminate your description with a dot on a line\n");
	TxPrintf("by itself (\".\"):\n\n");

	(void) sprintf(tempName, "%s/comments.%s", crashDir, pidString);
	commentFile = fopen(tempName, "w");
	if (commentFile == (FILE *) NULL)
	    goto done;
	(void) chmod(tempName, 0644);

	time(&now);
	fprintf(commentFile, "~s -- Magic crashed %24.24s --\n", ctime(&now));
	fputs(MagicVersion, commentFile);
	fprintf(commentFile, "%s\n", AbortMessage);
	while (TxGetLine(line, sizeof line) != NULL)
	{
	    /*
	     * We would use EOF to end the message, but there is no EOF when
	     * magic is running unless it is accepting input from a
	     * command file.
	     */
	    if (strcmp(line, ".") == 0)
		break;
	    fprintf(commentFile, "%s\n", line);
	}
	fclose(commentFile);
	sprintf(command, MAIL_COMMAND, tempName);
	system(command);

	TxPrintf("Thank you.\n");
    }
#endif

done:
    if (!AbortFatal)
	TxPrintf("Please try to quit from magic as soon as possible!\n");
}
