SHELL=/bin/sh
#  rcsid="$Header: Makefile,v 6.0 90/08/29 13:14:42 mayo Exp $"

MODULE=magicusage
MAGICDIR=..
CADDIR = /udir/mayo/cadhome
H=      ${MAGICDIR}/include
LIB=    ${MAGICDIR}/lib
IFLAGS= -I. -I${H}
DFLAGS= `cat ../misc/DFLAGS`
CFLAGS= `cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

MAGICUSAGE	= magicusage.o ${LIB}/libmagicutils.a 

magicusage:	${MAGICUSAGE}
		rm -f magicusage
		${CC} ${MAGICUSAGE} -o magicusage

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

clean:	
	rm -f *.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

 
install: ${CADDIR}/bin/magicusage

${CADDIR}/bin/magicusage: magicusage
	-rm -f ${CADDIR}/bin/magicusage
	cp magicusage ${CADDIR}/bin/magicusage

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:14:39 PDT 1990.
 
# include file dependencies for "magicusage.c"
magicusage.o:	magicusage.c
magicusage.o:	${H}/magic.h
magicusage.o:	${H}/paths.h
magicusage.o:	${H}/hash.h
magicusage.o:	${H}/stack.h
magicusage.o:	${H}/utils.h
magicusage.o:	${H}/pathvisit.h
magicusage.o:	${H}/malloc.h
 
