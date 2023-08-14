/*
 * test.c --
 *
 * Test out signals.
 *
 */

#ifndef lint
static char rcsid[]="$Header: test.c,v 6.0 90/08/28 18:57:12 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <fcntl.h>

main(argc, argv)
    char *argv[];
{
    FILE *file;
    int filenum;


    if (argc != 2) {
	printf("Usage:  test filename\n");
	exit(1);
    }
    file = fopen(argv[1], "rw");
    if (file == NULL) {
	printf("Could not open file '%s'\n", argv[1]);
	exit(1);
    };
    filenum = fileno(file);
    printf("File number %d\n", filenum);

    if (fcntl(filenum, F_SETOWN, getpid()) == -1) {
	perror("Could net set F_SETOWN"); 
	exit(1);
    }

    printf("OK.\n");
}


