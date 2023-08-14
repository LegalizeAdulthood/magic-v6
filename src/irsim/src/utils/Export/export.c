/*
 * Create an exports file from the implementation files.
 *
 * Items declared as public are copied to the new file.
 * Initializations and array sizes are omitted (e.g. "int x = 3" will
 * output "int x;", "char a[ 100 ];" will output "char a[];"
 *
 * Normally a temporary definitions file is created and compared to
 * the given destination.  If they are different, the temporary file
 * is copied on top of the destination.  This is so that dependencies
 * when using "make" are not triggered.
 *
 */

#include <stdio.h>
#include <signal.h>

#define	procedure	void
#define	and		&&
#define	or		||
#define	not		!

typedef enum { FALSE, TRUE } Boolean;
typedef char *String;


String		tmpname;
char		*ModuleName();
procedure	abnorm();
FILE		*fin, *fout;

main( argc, argv )
  int argc;
  String argv[];
  {
    extern  String mktemp();
    String  outfname;
    char    syscmd[ 512 ];
    FILE    *tmp;
    int r;
    int i;

    if( argc < 3 )
	Crash( "usage: %s file.c [ file.c ] output\n", ModuleName( *argv ) );

    signal( SIGINT, abnorm );
    signal( SIGQUIT, abnorm );

    tmpname = mktemp( "/tmp/exportXXXXXX" );
    fout = fopen( tmpname, "w", stdout );
    if( fout == NULL )
	Crash( "can't write %s", tmpname );
    
    outfname = argv[ --argc ];
    for( i = 1; i < argc; i++ )
      {
	if( ( fin = fopen( argv[ i ], "r" ) ) == NULL )
	    Crash( "can't read %s\n", argv[ i ] );
	Process( ModuleName( argv[ i ] ) );
	fclose( fin );
      }
    fclose( fout );
    (void) sprintf( syscmd, "cmp -s %s %s", tmpname, outfname );
    r = system( syscmd );
    if (r != 0)
      {
	(void) sprintf( syscmd, "mv %s %s", tmpname, outfname );
	r = system( syscmd );
	if( r != 0 )
	    fprintf( stderr, "can't create %s\n", outfname );
      }
    else
	unlink( tmpname );
    exit(0);
  }



String ModuleName( s )
  String s;
  {
    String r, p;
    static char buf[ 256 ];

    for( r = s; *r != '\0'; r++ );
    while( r > s and *r != '/' ) r--;
    if( *r == '/' ) r++;
    p = buf;
    while( *r ) *p++ = *r++;
    *p = '\0';
    return( buf );
  }


char  buff[ 1025 ];
char  *line = &buff[1];

Process( modname )
  String  modname;
  {
    register	String	p;
    register	Boolean	isproc;
    register	Boolean	isarray;
    register	Boolean	firstIndex;
    Boolean		header;

    header = FALSE;
    while( (p = fgets( line, 1024, fin )) != NULL )
      {
	if( *p == 'p' and strncmp( p, "public", 6 ) == 0 )
	  {
	    isproc = FALSE;
	    isarray = FALSE;
	    firstIndex = TRUE;
	    if( not header )
	      {
		fprintf( fout, "\n\t/* EXPORTS FROM %s */\n\n", modname );
		header = TRUE;
	      }
	    if( p[ 6 ] == '\n' )
	      {
		CopyMacro();
		continue;
	      }
	    else if( not (p[6] <= ' ' ) )
		continue;
	    if( !strncmp( &p[ 7 ], "typedef", 7 ) )
	      {
		CopyDef( &p[ 7 ] );
		goto finish;
	      }
	    fputs( "extern ", fout );
	    for( p += 7; *p != ';'; p++ )
	      {
		switch( *p )
		  {
		    case '(':
			if( p[-1] > ' ' )
			  {
			    isproc = TRUE;
			    fputs( "( /* ", fout );
			  }
			else
			    fputc( *p, fout );
			break;
		    case ')' :
			if( isproc )
			  {
			    fputs( "*/ )", fout );
			    isproc = FALSE;
			    goto finish;
			  }
			else
			    putc( *p, fout );
			break;
		    case '[' :
			isarray = TRUE;
			if( firstIndex )
			    fputs( "[ /*", fout );
			else
			    fputs( "[ ", fout );
			break;
		    case ']' :
			if( isarray )
			  {
			    if( firstIndex )
				fputs( "*/ ]", fout );
			    else
				fputs( " ]", fout );
			    firstIndex = FALSE;
			    isarray = FALSE;
			  }
			else
			    putc( *p, fout );
			break;
		    case '=' :
			goto finish;
			break;
		    case '\n' :
			p = fgets( line, 1024, fin );
			if( p == NULL )
			    goto finish;
			else
			    p--;
			break;
		    default :
			putc( *p, fout );
		  }
	      }
	    finish :
	    fputs( ";\n", fout );
	  }
      }
  }


CopyDef( s )
  char  *s;
  {
    register	String	p;
    register	int	pCount = 0;

    p = s;
    do
      {
	putc( *p, fout );
	if( *p == '{' )
	    pCount++;
	else if( *p == '}' )
	    pCount--;
	else if( *p == '\n' )
	  {
	    p = fgets( line, 1024, fin );
	    if( p == NULL )
		return;
	    else
		p--;
	  }
	p++;
      }
    while( not (*p == ';' and pCount == 0) );
  }


CopyMacro()
  {
    register	String	p;
    
    while( (p = fgets( line, 1024, fin )) != NULL )
      {
	fputs( line, fout );
	while( *p != '\n' ) p++;
	p--;
	if( *(p) != '\\' )
	    return;
      }
  }



/*
 * Terminate program.
 */
procedure abnorm( signo )
int signo;
  {
    if( fout )
	fclose( fout );
    unlink( tmpname );
    exit( signo );
  }


/* VARARGS1 */
Crash( m1, m2, m3, m4 )
  String  m1;
  {
    fprintf( stderr, m1, m2, m3, m4 );
    if( fout != NULL )
      {
	fclose( fout );
	unlink( tmpname );
      }
    exit( -1 );
  }
