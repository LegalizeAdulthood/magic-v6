/*
 * ext2dlys.c --
 *
 * This program accepts as input two files: a .ext file produced as
 * the result of running Magic's circuit extractor on a routed layout
 * (along with all the rest of the .ext files in the hierarchical
 * tree), and a netlist file that is used to identify the electrical
 * pins on each net (for purposes of counting I/O loads).
 *
 * Usage:
 *	ext2dlys file
 *		[-d psPerPf]
 *		[-l psPerCentimicron]
 *		[-m min-multiplier max-multiplier]
 *		[-o outfile]
 *		[-D driver-file]
 *		[-I cap-per-input]
 *		[-O cap-per-input]
 *		[-L netlist]
 *		[-M scald-map-file]
 *		[-S capacitance-scale]
 *	> DLYS
 *
 * In addition, accepts the arguments documented in extflat(1).
 *
 * Normally, reads file.ext and file.net.  If a different .net file
 * is desired, the -n flag may be used to specify the alternate file
 * (without the suffix).
 *
 * The options are as follows:
 *	-d psPerPf		Picoseconds per picofarad
 *	-l psPerCentimicron	Lightspeed delay in picoseconds per centimicron
 *				of distance between a driver and receiver,
 *				used for delays along transmission lines.
 *	-m min max		To give min and max delays, multiply the
 *				computed delays by min and max respectively
 *				(both may be real numbers)
 *	-o outfile		Write the output to outfile instead of stdout
 *	-t capacitance-scale	1 unit of capacitance in the .ext file is
 *				equal to capacitance-scale ATTOfarads (the
 *				default is 1.0).  May be a real number.
 *	-I cap-per-input	Count each input on a net as iload attofarads
 *				of extra load.  If only -I is present, both
 *				inputs and outputs are counted as having iload
 *				attofarads of extra load.  If neither -I nor
 *				-O are present, inputs and outputs are
 *				considered to be free of any extra loading.
 *	-D driver-file		If this option is present, driver-file should
 *				be a file containing two tokens per line:
 *				a hierrchical pin name, and a drive factor.
 *				The drive factor is in picoseconds per
 *				picofarad (encoding the effective on
 *				resistance of the driver).
 *	-L file			The .net file to read is file.net
 *	-M scald-file		If present, scald-file contains a mapping
 *				between Magic names and scald names, with
 *				each line consisting of the Magic name,
 *				then white space, and then the Scald name
 *				(which can itself contain white space)
 *				up to the end of the line.
 *	-O cap-per-output	Count each output on a net as oload attofarads
 *				of extra load.
 *
 * The standard output is a wire-delays file with the following format:
 *
 *  <signal_name> = <location>[<mindelay>:<maxdelay>],...
 *		    <location>[<mindelay>:<maxdelay>];
 *  <signal_name> = <location>[<mindelay>:<maxdelay>],...
 *		    <location>[<mindelay>:<maxdelay>];
 *
 * Here, <signal_name> is the name of each signal appearing in a
 * "comment" line prior to the terminals in the netlist, and each
 * <location> is one of the terminals in the net (names are for
 * each physical instance of a part, so are made unique after
 * SIZE and TIMES expansion).  Currently, the delays to all
 * locations in the same net are the same.  The end of file
 * is indicated by two consecutive semicolons.
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "magic.h"
#include "hash.h"
#include "utils.h"
#include "geometry.h"
#include "malloc.h"
#include "extflat.h"

/* Forward declarations */
int mainArgs();
char *scaldOutName();

/* Common constants */
#define	LINESIZE	1024	/* Max size of all strings */
#define	INITHASHSIZE	32	/* Initial size of all hash tables */


float capScale = 1.0;
float minMult = 1.0, maxMult = 1.0;
float psPerPf = 100.0;		/* Assuming 100 ohm drivers (typical bipolar) */
float psPerCU = 3.33e-5;	/* Lightspeed = 3e16 CU/sec */
int inputLoad = 0, outputLoad = 0;

HashTable scaldHash, driveHash, signalHash, termHash;

bool hasILoad = FALSE, hasOLoad = FALSE;
char *extName = NULL;
char *netName = NULL;
char *outName = NULL;
char *driveName = NULL;
char *scaldName = NULL;

main(argc, argv)
    int argc;
    char *argv[];
{
    EFInit();
    extName = EFArgs(argc, argv, mainArgs, (ClientData) NULL);
    if (extName == NULL)
	exit (1);

    /* Default initializations */
    if (hasILoad && !hasOLoad) outputLoad = inputLoad;
    if (netName == NULL) netName = extName;

    (void) fprintf(stderr,
	"%f femtofarads per .ext capacitance unit (default)\n", capScale);
    (void) fprintf(stderr,
	"%f picoseconds per picofarad\n", psPerPf);
    (void) fprintf(stderr,
	"%f picoseconds per centimicron\n", psPerCU);
    (void) fprintf(stderr,
	"Multipliers: best case = *%.2f, worst case = *%.2f\n",
	minMult, maxMult);
    (void) fprintf(stderr,
	"%d attofarads per input, %d attofarads per output\n",
	inputLoad, outputLoad);

    /* Open the output file */
    if (outName && freopen(outName, "w", stdout) == NULL)
    {
	perror(outName);
	exit (1);
    }

    /* Read the .ext files into memory */
    EFReadFile(extName);

    /* Read the SCALD mapping table if one is to be used */
    HashInit(&scaldHash, INITHASHSIZE, HT_STRINGKEYS);
    if (scaldName)
	readScald(scaldName);

    /* Read the table giving all driving outputs and their drive factor */
    HashInit(&driveHash, INITHASHSIZE, HT_STRINGKEYS);
    if (driveName)
	readDrive(driveName);

    /* Flatten the .ext files */
    EFFlatBuild(extName, EF_FLATNODES|EF_FLATCAPS|EF_FLATDISTS);

    /* Read the .net file to get signal name to node name mappings */
    HashInit(&termHash, INITHASHSIZE, HT_STRINGKEYS);
    HashInit(&signalHash, INITHASHSIZE, HT_STRINGKEYS);
    readNet(netName);

    /* Compute and output delays */
    outDelays();
    printf(";\n");

#ifdef	free_all_mem
    /* Cleanup */
    EFFlatDone();
    EFDone();
#endif	free_all_mem

    exit (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * mainArgs --
 *
 * Process those arguments that are specific to ext2sim.
 * Assumes that *pargv[0][0] is '-', indicating a flag
 * argument.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	After processing an argument, updates *pargc and *pargv
 *	to point to after the argument.
 *
 *	May initialize various global variables based on the
 *	arguments given to us.
 *
 *	Exits in the event of an improper argument.
 *
 * ----------------------------------------------------------------------------
 */

Void
mainArgs(pargc, pargv)
    int *pargc;
    char ***pargv;
{
    char **argv = *pargv;
    int argc = *pargc;
    char *cp;

    switch (argv[0][1])
    {
	case 'd':
	    if ((cp = ArgStr(&argc, &argv, "delay")) == NULL)
		goto usage;
	    psPerPf = atof(cp);
	    break;
	case 'l':
	    if ((cp = ArgStr(&argc, &argv, "picoseconds")) == NULL)
		goto usage;
	    psPerCU = atof(cp);
	    break;
	case 'm':
	    if ((cp = ArgStr(&argc, &argv, "multiplier")) == NULL)
		goto usage;
	    minMult = atof(cp);
	    if (argc-- <= 0)
		goto usage;
	    maxMult = atof(*argv++);
	    break;
	case 'L':
	    if ((netName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 'o':
	    if ((outName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 't':
	    if ((cp = ArgStr(&argc, &argv, "capacitance")) == NULL)
		goto usage;
	    capScale = atof(cp);
	    break;
	case 'D':
	    if ((driveName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 'I':
	    if ((cp = ArgStr(&argc, &argv, "load")) == NULL)
		goto usage;
	    inputLoad = atoi(cp);
	    hasILoad = TRUE;
	    break;
	case 'M':
	    if ((scaldName = ArgStr(&argc, &argv, "filename")) == NULL)
		goto usage;
	    break;
	case 'O':
	    if ((cp = ArgStr(&argc, &argv, "load")) == NULL)
		goto usage;
	    outputLoad = atoi(cp);
	    hasOLoad = TRUE;
	    break;
	default:
	    (void) fprintf(stderr, "Unrecognized flag: \"%s\"\n", argv[0]);
	    goto usage;
    }

    *pargv = argv;
    *pargc = argc;
    return;

usage:
    (void) fprintf(stderr, "\
Usage: ext2dlys rootname [-d delay] [-m min-multiplier max-multiplier]\n\
	[-o outfile] [-t capacitance-scale] [-I load-per-input] [-L netlist]\n\
	[-M scaldmapfile] [-O load-per-input]\n\
       or else see options to extcheck(1)\n");
    exit (1);
}

/*
 * readDrive --
 *
 * Read the drivers file.  This file consists of lines containing
 * two tokens:
 *
 *	pinName driveFactor
 *
 * where pinName is a full hierarchical label name and driveFactor is
 * picoseconds per picofarad.  All nets driven by pinName will use
 * driveFactor ps/pF when converting from capacitance into delay.
 *
 * Creates an entry in the hash table driveHash for pinName with
 * a value pointing to a malloc'd float with value 'driveFactor'
 * (the malloc is to avoid machine dependencies).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Builds the hash table driveHash described above.
 */

Void
readDrive(driveName)
    char *driveName;
{
    char line[LINESIZE], pinName[LINESIZE];
    float drive, *pdrive;
    HashEntry *he;
    FILE *f;

    f = fopen(driveName, "r");
    if (f == NULL)
    {
	perror(driveName);
	return;
    }

    while (fgets(line, sizeof line, f))
    {
	if (sscanf(line, "%s %f", pinName, &drive) < 2)
	    continue;
	he = HashFind(&driveHash, pinName);
	MALLOC(float *, pdrive, sizeof (float));
	*pdrive = drive;
	HashSetValue(he, (ClientData) pdrive);
    }

    (void) fclose(f);
}

/*
 * readScald --
 *
 * Read the Magic-to-SCALD name mapping file 'fileName'.
 * This file gives an equivalent SCALD name for each Magic
 * terminal name.  Each line begins with a Magic name,
 * followed by white space, followed by the SCALD name
 * (which itself may contain embedded white space), and
 * terminated by the end of the line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Builds the hash table scaldHash; the value of the
 *	hash entry for a given Magic name is its associated
 *	SCALD name.
 */

readScald(fileName)
    char *fileName;
{
    char line[LINESIZE], *strchr();
    register char *cp, *ep;
    FILE *file;
    HashEntry *he;

    file = fopen(fileName, "r");
    if (file == (FILE *) NULL)
    {
	perror(fileName);
	return;
    }

    while (fgets(line, sizeof line, file))
    {
	for (cp = line; *cp && !isspace(*cp); cp++) /* Nothing */;
	if (*cp == '\0')
	    continue;

	/* Mark the end of the Magic name and skip to the SCALD name */
	*cp++ = '\0';
	while (*cp && isspace(*cp)) cp++;

	/* Zero the trailing newline */
	for (ep = cp; *ep && *ep != '\n'; ep++) /* Nothing */;
	*ep = '\0';

	he = HashFind(&scaldHash, line);
	if (HashGetValue(he))
	{
	    (void) fprintf(stderr,
		"Warning: multiple SCALD names for Magic name \"%s\"\n", line);
	    continue;
	}
	ep = StrDup((char **) NULL, cp);
	HashSetValue(he, ep);
    }

    (void) fclose(file);
}

/*
 * readNet --
 *
 * Read the .net file 'netFileName' to figure out the signal
 * names for each electrical net.  Stores each signal name
 * in the EFNode to which each terminal name in the net belongs.
 *
 * Convention:
 *	Each net in a .net file is preceded by a line beginning
 *	with white space, but followed by the SIGNAL name of that
 *	net (not necessarily one of the names of a terminal in
 *	that net).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Leaves entries in the signalHash table for each signal
 *	name pointing to the corresponding EFNode.  Adds entries
 *	to termHash for each terminal name in the netlist.  Uses
 *	the efnode_client fields to point back to their associated
 *	signal name (StrDup'd).
 */

Void
readNet(netFileName)
    char *netFileName;
{
    char line[LINESIZE], signalName[LINESIZE];
    register char *cp;
    HashEntry *he;
    EFNode *node;
    FILE *file;

    file = PaOpen(netFileName, "r", ".net", ".", (char *) NULL, (char **) NULL);
    if (file == (FILE *) NULL)
    {
	(void) fprintf(stderr, "%s.net: ", netFileName);
	perror("");
	return;
    }

    (void) strcpy(signalName, "(none)");
    while (fgets(line, sizeof line, file))
    {
	/* Nuke trailing newline */
	cp = strchr(line, '\n');
	if (cp) *cp = '\0';

	/* Lines beginning with white space delimit nets */
	if (line[0] == '\0' || isspace(line[0]))
	{
	    /* Skip to the first non-white character to find the SIGNAL name */
	    for (cp = line; *cp && isspace(*cp); cp++) /* Nothing */;
	    if (*cp)
		(void) strcpy(signalName, cp);
	    else
		signalName[0] = '\0';
	    continue;
	}

	/*
	 * If no signal name appeared before this terminal, then
	 * make the signal name be the same as this terminal.
	 */
	(void) strcpy(signalName, line);

	/* Find the node for which 'line' is a name */
	he = EFHNLook((HierName *) NULL, line, "Terminal");
	if (he == NULL)
	    continue;

	/* Add it to the table of all drivers/receivers */
	(void) HashFind(&termHash, line);

	/* Find the entry in signalHash for signalName */
	node = ((EFNodeName *) HashGetValue(he))->efnn_node;
	if (cp = (char *) node->efnode_client)
	{
	    if (strcmp(cp, signalName) != 0)
	    {
		(void) fprintf(stderr,
		    "Node %s belongs to two different signals:\n", line);
		(void) fprintf(stderr, "    %s and %s\n", cp, signalName);
		(void) fprintf(stderr, "    Ignoring %s\n", signalName);
	    }
	}
	else
	{
	    node->efnode_client = StrDup((char **) NULL, signalName);
	    he = HashFind(&signalHash, signalName);
	    if (HashGetValue(he))
	    {
		(void) fprintf(stderr,
		    "Strange: one signal (%s), two nodes\n", signalName);
	    }
	    else HashSetValue(he, (ClientData) node);
	}
    }

    (void) fclose(file);
}

/*
 * outDelays --
 *
 * Output the delays file to the standard output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to the standard output.
 */

Void
outDelays()
{
    float minCDelay, maxDelay, minWDelay, minDelay;
    float delay, drive;
    int numReceivers, numDrivers, minDist, maxDist;
    EFNodeName *nnR, *nnD;
    char *signalName;
    HashEntry *he;
    HashSearch hs;
    bool isFirst;
    EFNode *node;

    HashStartSearch(&hs);
    while (he = HashNext(&signalHash, &hs))
    {
	signalName = he->h_key.h_name;
	node = (EFNode *) HashGetValue(he);

	/* Count number of drivers, receivers in this net */
	numDrivers = numReceivers = 0;
	for (nnR = node->efnode_name; nnR; nnR = nnR->efnn_next)
	{
	    if (isReceiver(nnR)) numReceivers++;
	    if (isDriver(nnR)) numDrivers++;
	}

	/*
	 * If no explicit drivers, pick the first name in the node
	 * as a driver arbitrarily.
	 */
	if (numDrivers == 0)
	{
	    char *termName = EFHNToStr(node->efnode_name->efnn_hier);

	    numReceivers--;
	    numDrivers++;
	    (void) HashFind(&driveHash, termName);
	}

	isFirst = TRUE;
	printf("%s =\n", signalName);
	for (nnR = node->efnode_name; nnR; nnR = nnR->efnn_next)
	{
	    if (isReceiver(nnR))
	    {
		minCDelay = minWDelay = (float) INFINITE_THRESHOLD;
		maxDelay = 0.0;
		for (nnD = node->efnode_name; nnD; nnD = nnD->efnn_next)
		{
		    if (isDriver(nnD))
		    {
			if (EFLookDist(nnD->efnn_hier, nnR->efnn_hier,
				&minDist, &maxDist))
			{
			    delay = psPerCU * (float) minDist;
			    if (delay < minWDelay) minWDelay = delay;
			    delay = psPerCU * (float) maxDist;
			    if (delay > maxDelay) maxDelay = delay;
			}
			if (node->efnode_cap > 0)
			{
			    /*
			     * Capacitance is in attofarads (1e-18).
			     * We want picoseconds (1e-12)
			     *	ps = picofarads * psPerPf
			     *	   = (attofarads / 1e6) * psPerPf
			     */
			    delay = capScale * node->efnode_cap;
			    delay += inputLoad * numReceivers;
			    delay += outputLoad * numDrivers;
			    delay *= findDrive(nnD, &drive) ? drive : psPerPf;
			    delay /= 1.0e6;
			    if (delay < minCDelay) minCDelay = delay;
			    if (delay > maxDelay) maxDelay = delay;
			}
		    }
		}

		/*
		 * Speed of light delay is always an absolute lower bound
		 * on total delay, hence the following check.
		 */
		minDelay = (minWDelay < (float) INFINITE_THRESHOLD)
				? MAX(minWDelay, minCDelay)
				: minCDelay;

		/* Delays are in ps, but get printed in ns; scale by 1e-3 */
		if (!isFirst)
		    printf(",\n");
		printf("  %s[%.1f:%.1f]", scaldOutName(nnR),
			minMult * minDelay / 1000.0,
			maxMult * maxDelay / 1000.0);
		isFirst = FALSE;
	    }
	}
	if (!isFirst)
	    printf(";\n");
    }
}

/*
 * isReceiver --
 *
 * Determine whether a terminal is a receiver.
 * This will be true if it IS in the terminal hash table (termHash)
 * and NOT in the driver hash table (driveHash).
 *
 * Results:
 *	TRUE if a receiver, FALSE if not.
 *
 * Side effects:
 *	None.
 */

bool
isReceiver(nn)
    EFNodeName *nn;
{
    HashEntry *he;
    char *cp;

    cp = EFHNToStr(nn->efnn_hier);
    if (HashLookOnly(&termHash, cp) == NULL)
	return FALSE;
    if (HashLookOnly(&driveHash, cp) != NULL)
	return FALSE;

    return TRUE;
}

/*
 * isDriver --
 *
 * Determine whether a terminal is a driver.
 * This will be true if it is in the driver hash table (driveHash).
 * We don't bother looking in the terminal hash table, since that
 * was already checked when the driver hash table was built up.
 *
 * Results:
 *	TRUE if a driver, FALSE if not.
 *
 * Side effects:
 *	None.
 */

bool
isDriver(nn)
    EFNodeName *nn;
{
    char *cp;

    cp = EFHNToStr(nn->efnn_hier);
    return HashLookOnly(&driveHash, cp) != NULL;
}

/*
 * findDrive --
 *
 * Find the "drive factor" stored in the driveHash table
 * for the terminal identified by 'nn'.
 *
 * Results:
 *	TRUE if drive information exists for this terminal,
 *	FALSE if none does.
 *
 * Side effects:
 *	None.
 */

bool
findDrive(nn, pDrive)
    EFNodeName *nn;
    float *pDrive;
{
    HashEntry *he;
    char *cp;

    cp = EFHNToStr(nn->efnn_hier);
    he = HashLookOnly(&driveHash, cp);
    if (he == NULL || HashGetValue(he) == NULL)
	return FALSE;

    *pDrive = * ((float *) HashGetValue(he));
    return TRUE;
}

/*
 * scaldOutName --
 *
 * Return a pointer to the string to be used to output this
 * terminal name.  This will be either the SCALD name for
 * the Magic name, if there's an entry for nn's name in the
 * scaldHash table, or else just nn's name.
 *
 * Results:
 *	TRUE if drive information exists for this terminal,
 *	FALSE if none does.
 *
 * Side effects:
 *	The string returned may be statically allocated, so
 *	subsequent calls to EFHNToStr() or this procedure
 *	will trash it.
 */

char *
scaldOutName(nn)
    EFNodeName *nn;
{
    HashEntry *he;
    char *cp;

    cp = EFHNToStr(nn->efnn_hier);
    he = HashLookOnly(&scaldHash, cp);
    if (he)
	return (char *) HashGetValue(he);

    return cp;
}
