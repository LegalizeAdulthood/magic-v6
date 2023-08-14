/*
 * sim2dlys.c --
 *
 * This program accepts as input three files: a .sim and .al file
 * produced as the result of running Magic's circuit extractor on
 * a routed gate-array layout followed by running ext2sim, and a
 * netlist file that is used to identify the electrical pins on each
 * net (for purposes of counting I/O loads).
 *
 * Usage:
 *	sim2dlys file
 *		[-a file] [-n file]
 *		[-d delay]
 *		[-D driver-file]
 *		[-c capacitance-scale]
 *		[-m min-multiplier max-multiplier]
 *		[-o outfile]
 *		[-s substrate-node]
 *		[-I load-per-input] [-O load-per-input]
 *	> DLYS
 *
 * Normally, reads file.sim, file.al, and file.net.  If a different .al
 * or .net file are desired, the -a or -n flags may be used to specify
 * the alternate files (without the suffix).
 *
 * The options are as follows:
 *	-a file			The .al file to read is file.al
 *	-n file			The .net file to read is file.net
 *	-c capacitance-scale	1 units in the .sim file is equal to
 *				capacitance-scale FEMTOfarads (the default
 *				is 1.0).  May be a real number.
 *	-d delay		Picoseconds per picofarad
 *	-D driver-file		If this option is present, driver-file should
 *				be a file containing two tokens per line:
 *				a hierrchical pin name, and a drive factor.
 *				The drive factor is in picoseconds per
 *				picofarad (encoding the effective on
 *				resistance of the driver).
 *	-m min max		To give min and max delays, multiply the
 *				computed delays by min and max respectively
 *				(both may be real numbers)
 *	-o outfile		Write the output to outfile instead of stdout
 *	-s node			The name of the substrate node in the .sim
 *				file (to determine which capacitances are
 *				to substrate).
 *	-I iload		Count each input on a net as iload femtofarads
 *				of extra load.  If only -I is present, both
 *				inputs and outputs are counted as having iload
 *				femtofarads of extra load.  If neither -I nor
 *				-O are present, inputs and outputs are
 *				considered to be free of any extra loading.
 *	-O oload		Count each output on a net as oload femtofarads
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
#include "malloc.h"

/* One of the following exists for each logical terminal (from the netlist) */
typedef struct term
{
    struct net	*term_net;	/* Points to the net we belong to */
    struct term	*term_next;	/* Next term in net */
    char	*term_name;	/* Points to name in hash table */
} Term;

/* One of the following exists for each net */
typedef struct net
{
    int		 net_cap;	/* Capacitance to substrate in femtofarads */
    int		 net_iloads;	/* Number of receivers on net */
    int		 net_oloads;	/* Number of drivers on net */
    struct term	*net_terms;	/* List of terminals on this net */
} Net;

/* Forward declarations */
Term *findTerm();

/* Common constants */
#define	LINESIZE	1024	/* Max size of all strings */
#define	INITHASHSIZE	32	/* Initial size of all hash tables */


float capScale = 1.0;
float minMult = 1.0, maxMult = 1.0;
float psPerPf = 100.0;
int inputLoad = 0, outputLoad = 0;

HashTable netHash, scaldHash, driveHash;

main(argc, argv)
    int argc;
    char *argv[];
{
    char *simName = NULL, *alName = NULL, *netName = NULL, *outName = NULL;
    char *driveName = NULL;
    bool hasILoad = FALSE, hasOLoad = FALSE;
    char *scaldName = NULL;
    char *subsNode = "GND";
    char *arg;
    int nargs;

    argc--, argv++;
    while (--argc > 0)
    {
	arg = *argv++;
	if (arg[0] != '-')
	{
	    if (simName)
	    {
		(void) fprintf(stderr, "Only one .sim file can be specified\n");
		goto usage;
	    }
	    simName = arg;
	    continue;
	}

	/* Check for proper # of arguments */
	switch (arg[1])
	{
	    default:
		nargs = 0;
		break;
	    case 'a': case 'c': case 'd': case 'n':
	    case 'o': case 's': case 'I': case 'O':
	    case 'S': case 'D':
		nargs = 1;
		break;
	    case 'm':
		nargs = 2;
		break;
	}

	if (argc < nargs)
	{
	    (void) fprintf(stderr, "Option \"%s\" requires %d argument%s.\n",
		    arg, nargs, nargs != 1 ? "s" : "");
	    goto usage;
	}

	switch (arg[1])
	{
	    case 'a':
		alName = *argv++;
		break;
	    case 'n':
		netName = *argv++;
		break;
	    case 'c':
		capScale = atof(*argv++);
		break;
	    case 'd':
		psPerPf = atof(*argv++);
		break;
	    case 'm':
		minMult = atof(*argv++);
		maxMult = atof(*argv++);
		break;
	    case 'o':
		outName = *argv++;
		break;
	    case 's':
		scaldName = *argv++;
		break;
	    case 'D':
		driveName = *argv++;
		break;
	    case 'I':
		inputLoad = atoi(*argv++);
		hasILoad = TRUE;
		break;
	    case 'O':
		outputLoad = atoi(*argv++);
		hasOLoad = TRUE;
		(void) fprintf(stderr, "Warning: -O not implemented\n");
		break;
	    case 'S':
		subsNode = *argv++;
		break;
	    default:
		(void) fprintf(stderr, "Unrecognized flag: \"%s\"\n", arg);
		goto usage;
	}

	argc -= nargs;
    }

    if (simName == NULL)
    {
	(void) fprintf(stderr, "Missing .sim file name\n");
	goto usage;
    }

    /* Default initializations */
    if (hasILoad && !hasOLoad) outputLoad = inputLoad;
    if (alName == NULL) alName = simName;
    if (netName == NULL) netName = simName;

    (void) fprintf(stderr,
	"%f femtofarads per .sim capacitance unit (default)\n", capScale);
    (void) fprintf(stderr,
	"%f picoseconds per picofarad\n", psPerPf);
    (void) fprintf(stderr,
	"Multipliers: best case = *%.2f, worst case = *%.2f\n",
	minMult, maxMult);
    (void) fprintf(stderr,
	"%f femtofarads per input, %f femtofarads per output\n",
	inputLoad, outputLoad);


    /* Open the output file */
    if (outName && freopen(outName, "w", stdout) == NULL)
    {
	perror(outName);
	exit (1);
    }

    /* Hash table is keyed by name */
    HashInit(&netHash, INITHASHSIZE, 0);
    readal(alName, &netHash);
    readnet(netName, &netHash);
    readsim(simName, subsNode, &netHash);

    /* Read the SCALD mapping table if one is to be used */
    if (scaldName)
    {
	HashInit(&scaldHash, INITHASHSIZE, 0);
	readscald(scaldName, &scaldHash);
    }

    /* Read the table that will hold per-output drive factors */
    HashInit(&driveHash, INITHASHSIZE, 0);
    if (driveName)
	readdrive(driveName);

    outdelays(netName, &netHash, scaldName != NULL);
    printf(";\n");

    exit (0);

usage:
    (void) fprintf(stderr, "Usage: sim2dlys rootname [-a file] [-n file]\n\
                    [-d delay] [-c capacitance-scale] [-o outfile]\n\
		    [-m min-multiplier max-multiplier] [-s sfile]\n\
		    [-I load-per-input] [-O load-per-input]\n\
		    [-S substrate-node]\n");
    exit (1);
}

/*
 * outdelays --
 *
 * Output the delays file from the hash table 'netHash' and
 * the linked network of Net and Term structs pointed to by
 * its entries.  The order of output is determined by the
 * .net file 'netFileName'.
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
 *	Writes to the standard output.
 */

Void
outdelays(netFileName, netHash, hasScald)
    char *netFileName;
    HashTable *netHash;
    bool hasScald;
{
    char line[LINESIZE], signalName[LINESIZE], *termName, *index();
    HashEntry *he, *heScald, *heDrive;
    register char *cp;
    bool firstInNet;
    float load;
    FILE *file;
    Term *term;
    Net *net;

    file = PaOpen(netFileName, "r", ".net", ".", (char *) NULL, (char **) NULL);
    if (file == (FILE *) NULL)
    {
	(void) fprintf(stderr, "%s.net: ", netFileName);
	perror("");
	return;
    }

    firstInNet = TRUE;
    (void) strcpy(signalName, "(none)");
    while (fgets(line, sizeof line, file))
    {
	/* Nuke trailing newline */
	cp = index(line, '\n');
	if (cp) *cp = '\0';

	/* Lines beginning with white space delimit nets */
	if (line[0] == '\0' || isspace(line[0]))
	{
	    /*
	     * Only print trailing ";" immediately after the
	     * last term in a net.
	     */
	    if (!firstInNet)
		printf(";\n");
	    firstInNet = TRUE;

	    /* Skip to the first non-white character to find the SIGNAL name */
	    for (cp = line; *cp && isspace(*cp); cp++) /* Nothing */;
	    if (*cp)
		(void) strcpy(signalName, cp);
	    continue;
	}

	/* Print the signal name if beginning a new net */
	if (firstInNet) printf("%s =\n", signalName);
	else printf(",\n");
	firstInNet = FALSE;

	/* Find the net for this terminal */
	load = 0.0;
	he = HashLookOnly(netHash, line);
	if (he == NULL) printf("  ***missing***");
	else
	{
	    term = (Term *) HashGetValue(he);
	    if (term == (Term *) NULL) printf("  ***noterm***");
	    else
	    {
		net = term->term_net;
		load = net->net_cap * capScale;
		load += net->net_iloads * inputLoad;
		load += net->net_oloads * outputLoad;

		/* HACK: check 1 output, N inputs */
		if (net->net_oloads != 1)
		    (void) fprintf(stderr, "Net %s has %d outputs\n",
			signalName, net->net_oloads);

		/*
		 * Scaling: load is in femtofarads (1e-15).
		 * We want nanoseconds (1e-9).
		 *
		 *	ps = picofarads * psPerPf
		 *	ns = ps / 1000.0
		 * so,
		 *	ns = (femtofarads / 1000.0) * psPerPf / 1000.0
		 */
		heDrive = HashLookOnly(&driveHash, line);
		if (heDrive)
		    load *= (*((float *) HashGetValue(heDrive))) / 1.0e6;
		else
		    load *= psPerPf / 1.0e6;
	    }
	}

	termName = term->term_name;
	if (hasScald)
	{
	    /* Handle renaming to SCALD names if applicable */
	    heScald = HashLookOnly(&scaldHash, termName);
	    if (heScald && HashGetValue(heScald))
		termName = (char *) HashGetValue(heScald);
	}

	/* Delay is printed in nanoseconds */
	printf("  %s[%.1f:%.1f]", termName, minMult*load, maxMult*load);
    }

    if (!firstInNet)
	printf(";\n");

    (void) fclose(file);
}

/*
 * readdrive --
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
readdrive(driveName)
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
 * readscald --
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

readscald(fileName, scaldHash)
    char *fileName;
    HashTable *scaldHash;
{
    char line[LINESIZE], *index();
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

	he = HashFind(scaldHash, line);
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
 * readnet --
 *
 * Read the .net file 'fileName', which is used to count
 * the number of loads per net.  This procedure must be
 * called after readal().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets net_iloads, net_oloads in each Net struct.
 *	Also adds additional entries to the hash table netHash
 *	if any of the names of nodes in a net don't appear
 *	already in netHash.
 */

readnet(fileName, netHash)
    char *fileName;
    HashTable *netHash;
{
    FILE *file;
    char line[LINESIZE], prevname[LINESIZE], *cp, *index();
    Term *term, *prevterm;
    HashEntry *he;
    Net *net;

    file = PaOpen(fileName, "r", ".net", ".", (char *) NULL, (char **) NULL);
    if (file == (FILE *) NULL)
    {
	(void) fprintf(stderr, "%s.net: ", fileName);
	perror("");
	return;
    }

    prevname[0] = '\0';
    while (fgets(line, sizeof line, file))
    {
	if (isspace(line[0]))
	{
	    /* Marks the beginning of a new net */
	    prevname[0] = '\0';
	    continue;
	}
	cp = index(line, '\n');
	if (cp) *cp = '\0';
	he = HashFind(netHash, line);
	term = (Term *) HashGetValue(he);
	if (term == (Term *) NULL)
	{
	    /*
	     * Name wasn't seen before, so we'll have to add it
	     * to the hash table netHash.
	     * If this is the first name in the net, allocate
	     * a new term with a completely new net; otherwise,
	     * merge the term for this name with that for prevname.
	     */
	    term = findTerm(line, netHash);
	    if (prevname[0] == '\0')
	    {
		/* Create a new net struct consisting of the two terms */
		MALLOC(Net *, net, sizeof (Net));
		net->net_cap = 0;
		net->net_iloads = 0;
		net->net_oloads = 0;
		net->net_terms = term;
		term->term_net = net;
		term->term_next = (Term *) NULL;
	    }
	    else
	    {
		/* Merge with prevname's term */
		prevterm = findTerm(prevname, netHash);
		mergeTerm(term, prevterm);
	    }
	}
	(void) strcpy(prevname, line);
	net = term->term_net;

	/* HACK: assume one output per net; the rest are inputs */
	if (net->net_oloads == 0) net->net_oloads++;
	else net->net_iloads++;
    }

    (void) fclose(file);
}

/*
 * readal --
 *
 * Read the .al file 'fileName' for naming information.
 * This fills in the HashTable *netHash and creates the
 * linked structure of Terms and Nets it points to.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 */

Void
readal(fileName, netHash)
    char *fileName;
    HashTable *netHash;
{
    char line[LINESIZE], name1[LINESIZE], name2[LINESIZE];
    Term *term1, *term2;
    FILE *file;

    file = PaOpen(fileName, "r", ".al", ".", (char *) NULL, (char **) NULL);
    if (file == (FILE *) NULL)
    {
	(void) fprintf(stderr, "%s.al: ", fileName);
	perror("");
	return;
    }

    while (fgets(line, sizeof line, file))
    {
	if (line[0] != '=') continue;
	if (sscanf(line, "= %s %s", name1, name2) != 2) continue;
	term1 = findTerm(name1, netHash);
	term2 = findTerm(name2, netHash);
	mergeTerm(term1, term2);
    }

    (void) fclose(file);
}

Term *
findTerm(name, netHash)
    char *name;
    HashTable *netHash;
{
    HashEntry *he;
    Term *term;

    he = HashFind(netHash, name);

    term = (Term *) HashGetValue(he);
    if (term)
	return (term);

    MALLOC(Term *, term, sizeof (Term));
    term->term_name = he->h_key.h_name;
    term->term_net = (Net *) NULL;
    term->term_next = (Term *) NULL;
    HashSetValue(he, (ClientData) term);
    return (term);
}

Void
mergeTerm(term1, term2)
    Term *term1, *term2;
{
    Net *net, *net2;
    Term *term;

    if (term1->term_net == (Net *) NULL)
    {
	if (net = term2->term_net)
	{
	    /* Cons term1 to the front of the list for this net */
	    term1->term_net = net;
	    term1->term_next = net->net_terms;
	    net->net_terms = term1;
	    return;
	}

	/* Create a new net struct consisting of the two terms */
	MALLOC(Net *, net, sizeof (Net));
	net->net_cap = 0;
	net->net_iloads = 0;
	net->net_oloads = 0;
	net->net_terms = term1;
	term1->term_net = net;
	term1->term_next = term2;
	term2->term_net = net;
	term2->term_next = (Term *) NULL;
	return;
    }

    /* Term1 has already been assigned */
    if (term2->term_net == (Net *) NULL)
    {
	/* Cons term2 to the front of the list for this net */
	net = term1->term_net;
	term2->term_net = net;
	term2->term_next = net->net_terms;
	net->net_terms = term2;
	return;
    }

    /*
     * Hard case: both terms have been assigned.
     * If to the same net, then we don't have to do anything more;
     * otherwise, reassign all the terms from term2->term_net to
     * term1->term_net and cons the list for term2->term_net onto
     * the front of that for term1->term_net.
     */
    if (term1->term_net == term2->term_net)
	return;

    net = term1->term_net;
    net2 = term2->term_net;
    if (net2->net_terms)
    {
	for (term = net2->net_terms; term; term = term->term_next)
	{
	    term->term_net = net;
	    if (term->term_next == (Term *) NULL)
		term->term_next = net->net_terms;
	}
	net->net_terms = net2->net_terms;
	net->net_iloads += net2->net_iloads;
	net->net_oloads += net2->net_oloads;

	/* HACK: assume one output per net */
	if (net->net_oloads > 1)
	{
	    net->net_iloads += net->net_oloads - 1;
	    net->net_oloads = 1;
	}
    }
    FREE((char *) net2);
}

/*
 * readsim --
 *
 * Read the .sim file 'fileName' for capacitance information,
 * summing the per-net capacitance in the net's net_cap
 * field.  We're only interested in capacitance to the
 * node *substrateNode.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies net_cap fields in Net structs.
 */

Void
readsim(fileName, substrateNode, netHash)
    char *fileName, *substrateNode;
    HashTable *netHash;
{
    char line[LINESIZE], name1[LINESIZE], name2[LINESIZE], *namep;
    HashEntry *he;
    Term *term;
    FILE *file;
    Net *net;
    int cap;

    file = PaOpen(fileName, "r", ".sim", ".", (char *) NULL, (char **) NULL);
    if (file == (FILE *) NULL)
    {
	(void) fprintf(stderr, "%s.sim: ", fileName);
	perror("");
	return;
    }

    while (fgets(line, sizeof line, file))
    {
	if (line[0] != 'C') continue;
	if (sscanf(line, "C %s %s %d", name1, name2, &cap) != 3) continue;
	if (strcmp(name2, substrateNode) == 0) namep = name1;
	else if (strcmp(name1, substrateNode) == 0) namep = name2;
	else continue;

	/* Ignore capacitance for nodes not in the netlist */
	he = HashLookOnly(netHash, namep);
	if (he == (HashEntry *) NULL)
	    continue;
	term = (Term *) HashGetValue(he);
	if (term == (Term *) NULL)
	    continue;

	net = term->term_net;
	net->net_cap += cap;
    }

    (void) fclose(file);
}
