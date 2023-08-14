/*
 * Program to construct dependency lines for makefiles.
 *
 * Marvin Theimer and Tim Mann, 3/18/84.
 *
 * Synopsis
 *	makedep [options] [source files]
 *
 * Discussion
 *   Makedep constructs a makefile-style dependency list
 *   showing which header files the object files constructed
 *   from the given source files depend
 *   upon.  The dependency of the object file upon the source 
 *   file is not indicated in the output; this dependency can 
 *   usually be inferred by the make program.
 *
 *   Makedep handles nested includes properly, propagating
 *   dependencies of one header file upon another back to
 *   each object file whose source file includes the dependent
 *   header file.
 *
 * Options
 *   -o outfile
 *	Output file name.  The default is "dependencies".  The name "-" 
 *	indicates standard output.
 *
 *   -I dir
 *	Add "dir" to the include file search list.  Multiple
 *	-I options accumulate, building the search list from
 *	left to right, with the system include directories
 *	added at the end.  The space separating the directory
 *	name from the -I may be omitted.  Directory names
 *	may be given relative to the directory from which
 *	makedep is invoked.  
 *
 *   -U
 *	Use the standard Unix header directories as the system
 *	search list.  Equivalent to specifying -I/usr/include after
 *	all other -I options.
 *
 *   -V
 *	Use the standard V-System header directories as the
 *	system search list.  Equivalent to specifying the options
 *	-I/usr/sun/include -I/usr/local/include -I/usr/include after
 *	all other -I options.
 *
 *   -xV
 *	Use the experimental V-System header directories as the
 *	system search list.  Equivalent to specifying the options
 *	-I/usr/sun/xinclude -I/usr/sun/include -I/usr/local/include
 *	-I/usr/include after all other -I options.
 *
 *   -N
 *	Use no system search list.  Suppresses the warning message 
 *	ordinarily printed when a header file cannot be found.  This
 *	option is useful when you are not interested in dependencies
 *	on system include files.
 *
 *   -e ext
 *	Object files have extension ".ext".  Defaults to .b if -V or -xV
 *	is specified, .o otherwise.
 *
 *   -d
 *	Turn on debug output.  Useful only to the maintainers.
 *
 *   If the source files depend on any header files in standard system 
 *   include directories, one of the options -U, -V, -xV, or -N should 
 *   normally be specified.  These four options are mutually exclusive.
 *   If none of these options is given, only the directories specified 
 *   in -I options are included in the search list (as with the -N flag), 
 *   but warning messages are still printed for any header files that
 *   cannot be found.
 */

#include "makedep.h"

extern FILE *freopen();


main(argc, argv)
    int argc;
    char **argv;
  {

    Debug = 0;

    Initialize();
    ProcessCommandLine(argc, argv);
    /* Redirect stdout to the output file. */
    if (strcmp(OutputFileName, "-") != 0)
       {
        if (freopen(OutputFileName, "w", stdout) == NULL)
          {
	    fprintf(stderr, "%s: can't open %s for writing.\n", 
		MyName, OutputFileName);
	    perror(OutputFileName);
	    exit(errno);
	  }
      }
    GenIncludeFileDependencies();
    PrintDependencies();

    exit(0);
  }


/*
 * Initialize various program data structures and variables.
 */

Initialize()
  {

    /* Set up default option flag settings. */
    NFlag = FALSE;
    UFlag = FALSE;
    VFlag = FALSE;
    xVFlag = FALSE;
    eFlag = FALSE;

    /* Set up default source and object extensions. */
    strcpy(ObjExt, DefaultObjExt);

    /* Set up default output filename. */
    strcpy(OutputFileName, DefaultOutputFileName);

    /* Set up default search list for include files. */
    InclDirs = MakeList();
    UserInclDirs = MakeList();	/* Empty to begin with. */

    /* Set up default source directory list. */
    SrcFiles = MakeList();
  }


/*
 * ProcessCommandLine:
 * Process user's command line.
 */

ProcessCommandLine(argc, argv)
    int argc;
    char **argv;
  {
    int nArg = 1;
    char *p;

    MyName = argv[0];

    /* Parse command line options. */
    while (nArg < argc)
      {
        if ( argv[nArg][0] == '-' )
	  {
	    switch (argv[nArg][1] )
	      {
                case 'o':
		    if (argv[nArg][2] == '\0')
		      {
			p = argv[nArg+1];
				/* Name is a separate input arg. */
			nArg++;	/* Incr. over the input arg. */
		      }
		    else
		      {
			p = &(argv[nArg][2]);
				/* Name is tacked onto the -o directly. */
		      }
		    strcpy(OutputFileName, p);
		    break;

	        case 'I':
		    if (argv[nArg][2] == '\0')
		      {
			AddList(argv[nArg+1], UserInclDirs);
				/* Name is a separate input arg. */
			nArg++;	/* Incr. over the input arg. */
		      }
		    else
		      {
			AddList(&(argv[nArg][2]), UserInclDirs);
				/* Name is tacked onto the -I directly. */
		      }
	      	    break;

	        case 'N':
		    NFlag = TRUE;
	      	    break;

	        case 'U':
		    UFlag = TRUE;
		    AddDefaultDirectoryLists(DefaultUnixInclDirs, InclDirs);
	      	    break;

		case 'V':
		    VFlag = TRUE;
		    AddDefaultDirectoryLists(DefaultVInclDirs, InclDirs);
		    if (!eFlag) strcpy(ObjExt, DefaultVObjExt);
		    break;

	        case 'x':  /* xV */
		    if (argv[nArg][2] != 'V') 
			goto badswitch;  /* sorry, Edsger */
		    xVFlag = TRUE;
		    AddDefaultDirectoryLists(DefaultXVInclDirs, InclDirs);
		    if (!eFlag) strcpy(ObjExt, DefaultVObjExt);
	      	    break;

	        case 'e':
		    eFlag = TRUE;
		    if (argv[nArg][2] == '\0')
		      {
			p = argv[nArg+1];
				/* Name is a separate input arg. */
			nArg++;	/* Incr. over the input arg. */
		      }
		    else
		      {
			p = &(argv[nArg][2]);
				/* Name is tacked onto the -e directly. */
		      }
		    strcpy(ObjExt, p);
	      	    break;

		case 'd':
		    Debug = TRUE;
		    break;

	 	default:
		badswitch:
		    fprintf(stderr, "%s: Unknown switch: %s\n", 
			MyName, argv[nArg]);
		    exit(1);
	      }
	  }
	else			/* No more options */
	  {
	    break;
	  }
	nArg++;
      }

    if (NFlag + UFlag + VFlag + xVFlag > 1)
      {
	fprintf(stderr, "%s: -N, -U, -V, and -xV are mutually exclusive.\n",
		MyName);
	exit(1);
      }

    /* Add source files for which dependencies are to be found. */
    while (nArg < argc)
      {
	AddList(argv[nArg], SrcFiles);
	nArg++;
      }

    /* Merge the list of user include directories with the list of "regular"
       include directories to get the final search path. */
    MergeLists(UserInclDirs, InclDirs);

    if (Debug)
      {
	printf("NFlag: %d, UFlag: %d, VFlag: %d, xVFlag",
		NFlag, UFlag, VFlag, xVFlag);
	printf("    ObjExt: %s\n", ObjExt);
	printf("output filename: %s\n", OutputFileName);
	PrintList(SrcFiles, "SrcFiles");
	PrintList(InclDirs, "InclDirs");
	PrintList(UserInclDirs, "UserInclDirs");
      }
  }


/*
 * AddDefaultDirectoryLists:
 * Adds the directory names in dirs to the list l.
 * The names in dirs are separated by blanks.
 */

AddDefaultDirectoryLists(dirs, l)
    char *dirs;
    StringList l;
  {
    char *p, *p1;

    p = dirs;
    while (*p != '\0')
      {
	for (p1 = p; ((*p1 != ' ') && (*p1 != '\0')); p1++)
	    ;
	while (*p1 == ' ')
	  {
	    *p1++ = '\0';
	  }
	AddList(p, l);
	p = p1;
      }
  }
