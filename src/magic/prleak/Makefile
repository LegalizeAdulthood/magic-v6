#
# Makefile for prleak program
# rcsid $Header: Makefile,v 4.9 89/09/08 10:22:08 mayo Exp $
#

CADDIR = /udir/mayo/cadhome
L=	../lib
H=	../include
IFLAGS= -I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS=	prleak.o ${L}/libmagicutils.a

prleak:	${OBJS}
	rm -f prleak
	${CC} ${CFLAGS} ${OBJS} -o prleak

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

install: ${CADDIR}/bin/prleak

${CADDIR}/bin/prleak: prleak
	-rm -rf ${CADDIR}/bin/prleak
	cp prleak ${CADDIR}/bin/prleak

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

 
# include file dependencies for "prleak.c"
prleak.o:	prleak.c
prleak.o:	${H}/magic.h
prleak.o:	${H}/malloc.h
prleak.o:	${H}/hash.h
