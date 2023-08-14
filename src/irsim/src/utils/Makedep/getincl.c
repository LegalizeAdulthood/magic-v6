/*
 * Module which finds all include file references in a specified file.
 */


#include "makedep.h"

#define MaxLine 200		/* Maximum line size allowed for a file. */

extern FILE *fopen();
extern char *rindex();

/*
 * GetIncludeFileList:
 * Scan the file corresponding to p and extract all include file references
 * from it.  
 */

StringList *GetIncludeFileList(p)
    StringList *p;
  {
   FILE *pf;
   char *linePtr;
   char line[MaxLine];
   char dirName[MaxLine];
   StringList *iList;
   char *p1;
 
    /* Open the file corresponding to p for reading. */
    pf = fopen(p->str, "r");
    if (pf == NULL)
      {
	perror(p->str);
	exit(errno);
      }

    /* Determine the correct path prefix for the 
     *   local directory. */
    strcpy(dirName, p->str);
    p1 = rindex(dirName, '/');
    if (p1 != NULL)
	*p1 = '\0';		/* chop off the last component */
    else
	*dirName = '\0';	/* no path prefix */

    /* Scan each line of the file in search of include file references. */
    iList = MakeList();
    linePtr = fgets(line, MaxLine, pf);
    while (linePtr != NULL)
      {
	ProcessLine(linePtr, iList, dirName);
	linePtr = fgets(line, MaxLine, pf);
      }
    /* Close p's file. */
    fclose(pf);

    return(iList);
  }


/*
 * ProcessLine:
 * Process a line from a file, looking for include file references.
 * Create a list of records, one per include file reference.
 * Each include file record contains the correctly expanded include file,
 * i.e. the include file name is prepended with the appropriate directory
 * prefix, as determined from InclDirs directory search list.
 */

ProcessLine(linePtr, iList, dirName)
    char *linePtr;
    StringList *iList;
    char *dirName;		/* Local directory name. */
  {
    char *p = linePtr;
    int relLocal;
    char name[MaxLine], dname[MaxLine];
    char *p1;
    StringList *dp;

    if (*p != '#')
      {				/* Couldn't find the # in col. 1. */
	return;
      }
    p++;
    /* Skip over any blanks or tabs between the # and the include. */
    while ((*p == ' ') || (*p == '\t'))
      {
	p++;
      }
    /* Parse the include string. */
    if (strncmp(p, "include", 7) != 0)
      {				/* Didn't find the include string. */
	return;
      }
    p += 7;
    /* Skip over any blanks or tabs between the include and the " or < . */
    while ((*p == ' ') || (*p == '\t'))
      {
	p++;
      }
    /* Determine whether the include is relative to the local dir or not. */
    if (*p == '"')
      {
	relLocal = TRUE;
      }
    else if (*p == '<')
      {
	relLocal = FALSE;
      }
    else
      {
	return;			/* It's neither - so this isn't an include file
				   refernence. */
      }
    p++;			/* p now points at the include file's name. */
    /* Extract the include file's name. */
    p1 = name;
    while ((*p != '"') && (*p != '>') && (*p != '\0'))
      {
	*p1++ = *p++;
      }
    if (*p == '\0')
      {
	return;			/* Abrupt end-of-line encountered.  This isn't
				   an include file ref. after all. */
      }
    *p1 = '\0';

    /* Now determine which directory the include file is actually in and
       prepend the file's name with the appropriate directory name. */
    if (*name == '/')
      {				/* Dealing with an absolute path name. */
	if (TryToAddIncl(name, iList))
	  {
	    return;
	  }
	else
	  {
	    if (!NFlag)
	      {
	        fprintf(stderr,"%s: Warning - include file '%s' not found.\n",
		    MyName, name);
	      }
	    return;
	  }
      }
    if (relLocal)
      {
	/* Look in the local directory first. */
	if (*dirName != '\0')
	  {
	    strcpy(dname, dirName);
	    strcat(dname, "/");
	    strcat(dname, name);
	  }
	else
	  {
	    strcpy(dname, name);
	  }
	if (TryToAddIncl(dname, iList))
	  {
	    return;
	  }
      }
    for (dp = InclDirs->next; dp != NULL; dp = dp->next)
      {
        strcpy(dname, dp->str);
	strcat(dname, "/");
	strcat(dname, name);
	if (TryToAddIncl(dname, iList))
	  {
	    return;
	  }
      }
    /* We couldn't find the include file */
    if (!NFlag)
      {
        fprintf(stderr, "%s: Warning - include file '%s' not found.\n",
		 MyName, name);
      }
  }


/*
 * TryToAddIncl:
 * Tries to open the file  name  and if successful adds  name  to iList.
 * Returns whether name was added to iList or not.
 */

int TryToAddIncl(name, iList)
    char *name;
    StringList *iList;
  {
    FILE *inclF;

    inclF = fopen(name, "r");
    if (inclF != NULL)
      {
	/* Add the include file to the iList */
	AddList(name, iList);
	fclose(inclF);
	return(TRUE);
      }
    return(FALSE);
  }
