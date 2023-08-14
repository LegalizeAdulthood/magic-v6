/*
 * String List Package
 */

#include "makedep.h"

extern char *malloc();


/*
 * GetMem:
 * malloc with error exit.
 */

char *GetMem(n)
    int n;
  {
    char *p;

    p = malloc(n);
    if (p == NULL)
      {
	fprintf(stderr, "%s: Ran out of memory!\n", MyName);
	exit(1);
      }
    return(p);
  }


/*
 * MakeList:
 * Create an empty list.
 */

StringList *MakeList()
  {
    StringList *ptr;

    ptr = (StringList *) GetMem(sizeof(StringList));
    ptr->str = NULL;
    ptr->state = HEADER;
    ptr->dep = NULL;
    ptr->next = NULL;
    return(ptr);
  }


/*
 * AddList:
 * Add a string to a list.
 */

AddList(s, l)
    char *s;
    StringList *l;
  {
    StringList *ptr;

    ptr = (StringList *) GetMem(sizeof(StringList));
    ptr->str = GetMem(strlen(s)+1);
    strcpy(ptr->str, s);
    ptr->state = UNPROCESSED;
    ptr->dep = NULL;
    ptr->next = l->next;
    l->next = ptr;
  }


/*
 * InsertList:
 * Insert a string to a list after the designated record.
 * The same implementation as for AddList can be used.
 */

InsertList(s, l)
    char *s;
    StringList *l;
  {
    AddList(s, l);
  }


/*
 * DeleteList:
 * Delete a string from a list.
 */

DeleteList(s, l)
    char *s;
    StringList *l;
  {
    StringList *ptr, *ptr1;

    if (l->next == NULL)
      {
	return;
      }
    for (ptr = l; ptr->next != NULL; ptr = ptr->next)
      {
	if (Equal(s, ptr->next->str))
	  {
	    ptr1 = ptr->next;
	    ptr->next = ptr->next->next;
	    free(ptr1->str);
	    free(ptr1);
	    return;
	  }
      }
  }


/*
 * MemberOfList:
 * Determines whether s is a member of l.
 */

int MemberOfList(s, l)
    char *s;
    StringList *l;
  {
    StringList *ptr;

    for (ptr = l->next; ptr != NULL; ptr = ptr->next)
      {
	if (Equal(s, ptr->str))
	  {
	    return(TRUE);
	  }
      }
    return(FALSE);
  }


/*
 * DestroyList:
 * Deallocates a list and all its members.
 */

DestroyList(l)
    StringList *l;
  {
    StringList *ptr;

    while (l->next != NULL)
      {
	ptr = l->next;
	l->next = ptr->next;
	free(ptr->str);
	free(ptr);
      }
    free(l);
  }


/*
 * MergeLists:
 * Merges list l1 onto the front of list l2.
 * l1 is NOT destroyed in the process.
 */

MergeLists(l1, l2)
    StringList *l1;
    StringList *l2;
  {
    StringList *p;

    p = l1->next;
    while (p != NULL)
      {
	AddList(p->str, l2);
	p = p->next;
      }
  }

/*
 * PrintList:
 * Prints a list preceded by a banner.
 * Intended for diagnostic purposes.
 */

PrintList(l, banner)
    StringList *l;
    char *banner;
  {
    printf("%s:\n", banner);
    for (l = l->next; l != NULL; l = l->next)
      {
	printf("  %s,  state: %d\n", l->str, l->state);
      }
  }


/*
 * AddDepList:
 * Add a dependency record for pi to the dependency list for p.
 */

AddDepList(pi, p)
    StringList *pi, *p;
  {
    DepList *ptr;

    ptr = (DepList *) GetMem(sizeof(DepList));
    ptr->inclFile = pi;
    ptr->next = p->dep;
    p->dep = ptr;
  }


/*
 * PrintDepList:
 * Print a dependency list preceded by a banner.
 * Intended for diagnostic purposes.
 */

PrintDepList(l, banner)
    StringList *l;
    char *banner;
  {
    DepList *p;

    printf("%s:\n", banner);
    for (p = l->dep; p != NULL; p = p->next)
      {
	printf("  %s\n", p->inclFile->str);
      }
  }


/*
 * CheckDepList:
 * Checks to ensure that all dependency records actually point at a record
 * which is on the IList.
 */

CheckDepList(p)
    StringList *p;
  {
    DepList *ptr;
    StringList *p1;
    int ok;

    for (ptr = p->dep; ptr != NULL; ptr = ptr->next)
      {
        ok = FALSE;
	for (p1 = IList->next; p1 != NULL; p1 = p1->next)
	  {
	    if (p1 == ptr->inclFile)
	      {
		ok = TRUE;
		break;
	      }
	  }
	if (!ok)
	  {
	    fprintf(stderr, "\n%s: dependency %s doesn't point into IList.\n",
	    		MyName, ptr->inclFile->str);
	  }
      }
  }
