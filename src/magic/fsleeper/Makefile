SHELL=/bin/sh
# rcsid = "$Header: Makefile,v 6.0 90/08/29 13:14:17 mayo Exp $"
CADDIR  = /udir/mayo/cadhome
FSLEEPER	= fsleeper.o
LIBS 		= ../lib/libmagicutils.a

fsleeper: ${FSLEEPER}
	rm -f fsleeper
	cc `cat ../misc/CFLAGS` `cat ../misc/DFLAGS` ${FSLEEPER} ${LIBS} -o fsleeper

.c.o:
	rm -f $*.o
	${CC} `cat ../misc/CFLAGS` `cat ../misc/DFLAGS` -c $*.c

install: ${CADDIR}/bin/fsleeper

${CADDIR}/bin/fsleeper: fsleeper
	cp fsleeper ${CADDIR}/bin

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:14:14 PDT 1990.
 
# include file dependencies for "fsleeper.c"
fsleeper.o:	fsleeper.c
 
