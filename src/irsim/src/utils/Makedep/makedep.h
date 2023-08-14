/*
 * Primary include file for makedep
 */


#include <stdio.h>
#include <errno.h>

#define FALSE 0
#define TRUE 1

extern int errno;
int Debug;
char *MyName;	/* name by which makedep was invoked */


/*
 * List definitions
 */

/* StringList record states. */
#define HEADER 1
#define UNPROCESSED 2
#define PROCESSED 3
#define START_MARK_VALUE 4

typedef struct StringListType
  {
    char *str;
    int state;
    struct DepListType *dep;
    struct StringListType *next;
  }
    StringList;

typedef struct DepListType
  {
    struct StringListType *inclFile;
    struct DepListType *next;
  }
    DepList;

extern StringList *MakeList();


/*
 * Various string objects and their default defns.
 */

/* Extensions for object. */
#define DefaultObjExt "o"
#define DefaultVObjExt "b"
char ObjExt[16];

/* Source file list. */
StringList *SrcFiles;

/* Search lists for include files. */
#define DefaultVInclDirs "/usr/sun/include /usr/local/include /usr/include"
#define DefaultXVInclDirs "/usr/sun/xinclude /usr/sun/include /usr/local/include /usr/include"
#define DefaultUnixInclDirs "/usr/include"
StringList *InclDirs;
StringList *UserInclDirs;

/* Output file name. */
#define DefaultOutputFileName "dependencies"
char OutputFileName[128];

/* Command line option flags */
int NFlag, UFlag, VFlag, xVFlag, eFlag;

/* List of include files that have been encountered. */
StringList *IList;


#define Equal(a,b) (strcmp(a,b) == 0)
