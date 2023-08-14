/*
 * Module which prints out the dependencies of each source file to the
 * file OutputFileName.
 */


#include "makedep.h"

extern char *rindex();

int CurrentMarkValue;		/* This variable is used to keep track of
				   which marking cycle is currently going
				   on.  Each source file's dependency graph
				   is traversed using a new value of
				   this variable.  Dependency records are
				   marked to avoid infinite recursions
				   through a possibly cyclic dependency
				   graph. */


/*
 * PrintDependencies:
 * Prints out the dependencies of each file on the source file list SrcFiles.
 * This is done by recursively printing the dependency list of each file.
 * I.e. each source file's dependency list is traversed and the print
 * routine is called to print the dependency list of each file on the list.
 * Since there may be cycles in the graph of dependencies, a marking scheme
 * is employed.
 */

PrintDependencies()
  {
    StringList *p;

    if (Debug)
      {
	printf("\nPrintDependencies:\n");
	PrintList(IList, "IList");
	for (p = SrcFiles->next; p != NULL; p = p->next)
	  {
	    CheckDepList(p);
	  }
      }

    /* Mark all include file records' state with the same starting value. */
    for (p = IList->next; p != NULL; p = p->next)
      {
	p->state = START_MARK_VALUE;
      }
    CurrentMarkValue = START_MARK_VALUE;
    /* Traverse the list source files and print the dependencies of each. */
    for (p = SrcFiles->next; p != NULL; p = p->next)
      {
        if (p->dep != NULL)	/* Don't print anything if there are no
				   dependencies! */
	  {
	    CurrentMarkValue++;
	    PrintSrcObjFile(p);
	    PrintDeps(p);
	    printf("\n");
	  }
      }
  }


/*
 * PrintSrcObjFile:
 * Print the source file name with its source extension replaced by its
 * object extension.  Only the base name is printed.
 */

PrintSrcObjFile(p)
    StringList *p;
  {
    char name[80];
    char *ptr;

    /* Replace the source file's extension with its object extension. */
    strcpy(name, p->str);
    ptr = name + strlen(name);
    while ((ptr != name) && (*ptr != '.'))
      {
	ptr--;
      }
    if (ptr == name)
      {
	fprintf(stderr, "%s: malformed source file name: %s\n", 
	    MyName, p->str);
	exit(1);
      }
    ptr++;			/* Get over the '.' */
    strcpy(ptr, ObjExt);

    /* Find the start of the base name. */
    ptr = rindex(name, '/');
    if (ptr != NULL)
      {
        ptr++;
      }
    else
      {
        ptr = name;
      }

    /* Print out the first part of the appropriate makefile-style 
       dependency line. */
    printf("%s:", ptr);
  }


/*
 * PrintDeps:
 * Recursive routine which prints out the dependencies for p and invokes
 * itself for all dependencies of dependencies of p.  Uses a marking scheme
 * to avoid cycles in the dependency graph.
 */

PrintDeps(p)
    StringList *p;
  {
    DepList *ptr;
    StringList *p1;

    /* Traverse the list of immediate dependencies. */
    for (ptr = p->dep; ptr != NULL; ptr = ptr->next)
      {
        p1 = ptr->inclFile;
        if (p1->state < CurrentMarkValue)
	  {			/* We've haven't yet seen this dependency. */
	    printf(" \\\n\t%s", p1->str);
				/* Print out the immediate dependency. */
	    p1->state = CurrentMarkValue;
				/* Mark the include file as already seen. */
	    PrintDeps(p1);
				/* Print out the dependency's dependencies. */
	  }
      }
  }
