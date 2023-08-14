/*
 * Module which generates the list of source files and which generates
 * the include file dependencies.
 */


#include "makedep.h"

/* External and forward fcn declarations */
extern StringList *GetIncludeFileList();
StringList *FindIListLocation();



/*
 * GenIncludeFileDependencies:
 * Generate all include file dependencies.
 * Starting with an empty list of include files, each source file 
 * on SrcFiles is examined
 * to extract all include file references it contains.  These file names
 * are placed on an include file list, IList.  Also, each source file gets
 * a linked list of (immediate) dependencies associated with it.
 * Each include file on IList
 * is examined to extract its include file dependencies; which are added to
 * the list.  Also, each include file gets a linked list of (immediate)
 * dependencies associated with it.
 * The IList is kept in sorted order in order to remove duplicates
 * and processed list entries are marked as such to prevent reprocessing.
 */

GenIncludeFileDependencies()
  {
    StringList *p;
    int done;

    if (Debug)
      {
	printf("\nGenIncludeFileDependencies:\n");
      }

    IList = MakeList();		/* Create an empty IList. */
    /* Run through the source files, generating each one's immediate
       dependency list. */
    for (p = SrcFiles->next; p != NULL; p = p->next)
      {
	GenDependencies(p);
      }
    /* Repeatedly iterate over IList until all elements have been processed. */
    done = FALSE;
    while (!done)
      {
        done = TRUE;		/* Assume we're done until proven otherwise. */
	for (p = IList->next; p != NULL; p = p->next)
	  {
	    if (p->state == UNPROCESSED)
	      {
	        done = FALSE;
		GenDependencies(p);
	      }
	  }
      }
  }


/*
 * GenDependencies:
 * Generate the immediate dependency list for p and mark it as processed.
 * Adds newly encountered include files to IList.  IList is kept in sorted
 * order so that duplicates can be removed.
 */

GenDependencies(p)
    StringList *p;
  {
    StringList *il, *pil, *pIList;
    DepList *dil;

    p->state = PROCESSED;
    /* Generate a list of include files referenced directly in p. */
    il = GetIncludeFileList(p);
    /* Create a list of corresponding dependency records for p. */
    for (pil = il->next; pil != NULL; pil = pil->next)
      {
	AddDepList(pil, p);
      }

    if (Debug)
      {
        printf("%s ", p->str);
	PrintDepList(p, "dependency list");
      }

    /* Merge the list of include files into IList, discarding duplicates.
       Make the pointers in p's dependency list point at the
       records in IList rather than the il list.  This is simpler to program
       than attempting to extract the non-duplicated records from il and
       selectively updating things. */
    for (dil = p->dep; dil != NULL; dil = dil->next)
      {
	pIList = FindIListLocation(dil->inclFile);
				/* pIList now points either to a record in
				   IList for this include file or to the
				   record in IList after which the include
				   file should be inserted. */
	if ((pIList == IList) || (!Equal(pIList->str, dil->inclFile->str)))
	  {
	    /* No duplicate - insert a new record into IList. */
	    InsertList(dil->inclFile->str, pIList);
	    pIList = pIList->next;
				/* Make pIList point at the new record. */
	  }
	dil->inclFile = pIList;	/* pIList at this point points at the correct
				   record in IList. */
      }
    /* Get rid of the il list of include files.  All references are now to
       the records in IList. */
    DestroyList(il);
  }


/*
 * FindIListLocation:
 * Search the (sorted) IList for a record corresponding to p.  If one is
 * found then return a pointer to it.  Otherwise return a pointer to the
 * record in IList after which such a record would appear.
 */

StringList *FindIListLocation(p)
    StringList *p;
  {
    StringList *ptr;
    int order;

    for (ptr = IList; ptr->next != NULL; ptr = ptr->next)
      {
        order = strcmp(p->str, ptr->next->str);
	if (order == 0)
	  {
	    /* Found a corresponding record.  Return a ptr to it. */
	    return(ptr->next);
	  }
	else if (order < 0)
	  {
	    /* ptr->next->str is lexically greater than p->str.  p should
	       go before it but after ptr. */
	    return(ptr);
	  }
      }
    /* p goes at the end of the list. */
    return(ptr);
  }
