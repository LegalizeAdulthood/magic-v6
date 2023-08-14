/*
 * This file is an attempt to shut lint up about certain
 * common complaints.
 *
 * rcsid="$Header: lintfix.c,v 6.0 90/08/28 18:47:39 mayo Exp $"
 */

#include <stdio.h>

/*VARARGS2*/
/*ARGSUSED*/
sscanf(s, fmt, a)
	char *s, *fmt;
{
    return(0);
}

/*VARARGS2*/
/*ARGSUSED*/
fprintf(f, fmt, a)
	FILE *f;
	char *fmt;
{
	return (0);
}

/*VARARGS1*/
/*ARGSUSED*/
printf(fmt, a)
	char *fmt;
{
	return (0);
}

/*VARARGS2*/
/*ARGSUSED*/
/* sprintf is 'char *' in <stdio.h>, but not in the man page! */
char *		
sprintf(s, fmt, a)
	char *s;
	char *fmt;
{
	return (0);
}
